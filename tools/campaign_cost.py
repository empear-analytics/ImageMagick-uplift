#!/usr/bin/env python3
"""Estimate what the refactoring campaign costs in tokens, priced at public API list rates.

Ported from the earlier campaign's tool. It reads the log stores an agent harness leaves
on the machine it runs on:

  ~/.claude*/projects/<project>/*.jsonl  Claude Code transcripts from every config directory
  ($CLAUDE_CONFIG_DIR/projects too)      (~/.claude, ~/.claude-work, ...). Every assistant
                                         message carries usage (input, output, cache writes
                                         split by 5m / 1h TTL, cache reads), the model, and
                                         the cwd and git branch of the session.
  ~/.codex/sessions/YYYY/MM/DD/*.jsonl   Codex rollouts (cumulative usage per session).

    python tools/campaign_cost.py                      # this week's usage, by week and model
    python tools/campaign_cost.py --by day --by branch --since 2026-10-24
    python tools/campaign_cost.py --by session

Usage is counted per API response and dated by its own timestamp, in Stockholm time, so a
session that runs past midnight or across a weekly reset is split correctly. Weeks start
at the plan's reset (--reset-weekday, --reset-hour; default Wednesday 00:00 local; set the
hour your plan actually resets).

Run it on every machine that takes part and add the tables up; Claude Code on the web
leaves no local log. Subscription plans are not billed per token: the dollar figures are
what the same tokens would cost on the API at list price, a proxy for how fast a plan's
limit is used, not what was paid.
"""
from __future__ import annotations

import argparse
import collections
import datetime as dt
import glob
import json
import os

# $ per million tokens: (input, cache write 5m, cache write 1h, cache read, output).
# 5.5 rates: third-party price lists of October 2026 (the official page was not checked);
# Sonnet 5.5 cache reads are $0.10 since 2026-10-07 in most of them. Haiku 5.5 is priced at
# its rate for prompts up to 100K tokens, with cache multipliers assumed (1.25x, 2x, 0.1x).
# Earlier models: the September 2026 list used by the earlier campaign.
PRICES = {
    "claude-opus-5-5": (4.0, 5.0, 8.0, 0.20, 20.0),
    "claude-sonnet-5-5": (2.0, 2.50, 4.0, 0.10, 10.0),
    "claude-haiku-5-5": (0.10, 0.125, 0.20, 0.01, 0.50),  # cache rates assumed
    "claude-fable-5-1": (10.0, 12.5, 20.0, 0.25, 50.0),
    "claude-opus-5": (5.0, 6.25, 10.0, 0.50, 25.0),
    "claude-sonnet-5": (2.0, 2.50, 4.0, 0.20, 10.0),
    "claude-haiku-4-5": (1.0, 1.25, 2.0, 0.10, 5.0),
    # OpenAI: (input, -, -, cached input, output); Codex logs do not split cache writes
    "gpt-5.6-sol": (4.0, None, None, 0.40, 20.0),
    "gpt-5.6-terra": (2.0, None, None, 0.20, 12.0),
}
FIELDS = ("uncached", "w5m", "w1h", "read", "out")
WEEKDAYS = ["mon", "tue", "wed", "thu", "fri", "sat", "sun"]


def base_model(model):
    """Strip a date suffix or a context tag such as '[1m]' from a model id."""
    m = model.split("[")[0]
    for k in sorted(PRICES, key=len, reverse=True):
        if m == k or m.startswith(k + "-2") or m.startswith(k + "@"):
            return k
    return m


def cost_parts(model, c):
    """Dollar cost per component, or None when the model has no price."""
    p = PRICES.get(base_model(model))
    if p is None:
        return None
    base, p5, p1, pr, po = p
    return {"input": c["uncached"] * base / 1e6, "cache write": (c["w5m"] * (p5 or 0) + c["w1h"] * (p1 or 0)) / 1e6,
            "cache read": c["read"] * pr / 1e6, "output": c["out"] * po / 1e6}


def _last_sunday(year, month):
    d = dt.date(year, month + 1, 1) - dt.timedelta(days=1) if month < 12 else dt.date(year, 12, 31)
    return d - dt.timedelta(days=(d.weekday() + 1) % 7)


def stockholm(ts):
    """UTC ISO timestamp -> naive Stockholm local time (EU summer time rule; no tz database
    needed, so it also works on Windows without the tzdata package)."""
    t = dt.datetime.fromisoformat(ts.replace("Z", "+00:00")).astimezone(dt.timezone.utc).replace(tzinfo=None)
    start = dt.datetime.combine(_last_sunday(t.year, 3), dt.time(1))
    end = dt.datetime.combine(_last_sunday(t.year, 10), dt.time(1))
    return t + dt.timedelta(hours=2 if start <= t < end else 1)


def week_start(local, weekday, hour):
    """Start of the plan week that contains `local`."""
    reset = dt.datetime.combine(local.date(), dt.time(hour)) - dt.timedelta(days=(local.weekday() - weekday) % 7)
    if reset > local:
        reset -= dt.timedelta(days=7)
    return reset


def claude_records(roots, cwd_filter):
    """One record per API response: (harness, session, local time, model, branch, counts)."""
    seen = set()
    for root in roots:
        for f in sorted(glob.glob(os.path.join(root, "*", "*.jsonl"))):
            proj = os.path.basename(os.path.dirname(f))
            session = os.path.basename(f)[:8]
            cwd = branch = None
            with open(f, encoding="utf-8", errors="replace") as fh:
                for line in fh:
                    try:
                        o = json.loads(line)
                    except ValueError:
                        continue
                    cwd = o.get("cwd") or cwd
                    branch = o.get("gitBranch") or branch
                    m = o.get("message") if isinstance(o.get("message"), dict) else None
                    if not (m and m.get("role") == "assistant" and m.get("usage") and o.get("timestamp")):
                        continue
                    where = cwd or proj
                    if cwd_filter and cwd_filter not in where and cwd_filter.replace(" ", "-") not in proj:
                        continue
                    model = m.get("model") or "?"
                    if model.startswith("<"):
                        continue
                    key = (m.get("id"), o.get("requestId"))
                    if key[0] and key in seen:
                        continue
                    seen.add(key)
                    u = m["usage"]
                    cc = u.get("cache_creation") or {}
                    w5, w1 = cc.get("ephemeral_5m_input_tokens"), cc.get("ephemeral_1h_input_tokens")
                    if w5 is None and w1 is None:
                        w5 = u.get("cache_creation_input_tokens", 0) or 0  # TTL unknown: priced as 5m
                    c = collections.Counter(uncached=u.get("input_tokens", 0) or 0, out=u.get("output_tokens", 0) or 0,
                                            read=u.get("cache_read_input_tokens", 0) or 0, w5m=w5 or 0, w1h=w1 or 0, turns=1)
                    yield ("claude", session, stockholm(o["timestamp"]), model, branch or "?", c)


def codex_records(root, cwd_filter):
    """Codex logs only carry cumulative usage, so a session is one record at its first time."""
    for f in sorted(glob.glob(os.path.join(root, "*", "*", "*", "*.jsonl"))):
        cwd, first, total, models = None, None, None, collections.Counter()
        with open(f, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                try:
                    o = json.loads(line)
                except ValueError:
                    continue
                first = first or o.get("timestamp")
                p = o.get("payload") or {}
                if not isinstance(p, dict):
                    continue
                cwd = p.get("cwd") or cwd
                if p.get("model"):
                    models[p["model"]] += 1
                if p.get("type") == "token_count" and (p.get("info") or {}).get("total_token_usage"):
                    total = p["info"]["total_token_usage"]
        if not (total and models and first) or (cwd_filter and cwd_filter not in (cwd or "")):
            continue
        c = collections.Counter(uncached=total.get("input_tokens", 0) - total.get("cached_input_tokens", 0),
                                read=total.get("cached_input_tokens", 0), out=total.get("output_tokens", 0), turns=0)
        yield ("codex", os.path.basename(f)[8:27], stockholm(first), models.most_common(1)[0][0], "?", c)


def main():
    home = os.path.expanduser("~")
    roots = sorted(glob.glob(os.path.join(home, ".claude*", "projects")))
    if os.environ.get("CLAUDE_CONFIG_DIR"):
        roots.append(os.path.join(os.environ["CLAUDE_CONFIG_DIR"], "projects"))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--since", default="2026-10-24", help="first local day (YYYY-MM-DD)")
    ap.add_argument("--until", default="2100-01-01", help="last local day (YYYY-MM-DD)")
    ap.add_argument("--cwd-filter", default="ImageMagick", help="substring of the session's working directory")
    ap.add_argument("--claude-root", action="append", help="Claude Code projects directory (repeatable)")
    ap.add_argument("--codex-root", default=os.path.join(home, ".codex", "sessions"))
    ap.add_argument("--by", action="append", choices=["week", "day", "model", "branch", "session"],
                    help="grouping, repeatable; default: week and model")
    ap.add_argument("--reset-weekday", default="wed", choices=WEEKDAYS)
    ap.add_argument("--reset-hour", type=int, default=0, help="local hour of the weekly reset")
    a = ap.parse_args()
    by = a.by or ["week", "model"]
    wd = WEEKDAYS.index(a.reset_weekday)

    recs = list(claude_records(sorted(set(a.claude_root or roots)), a.cwd_filter)) + \
        list(codex_records(a.codex_root, a.cwd_filter))
    recs = [r for r in recs if a.since <= r[2].strftime("%Y-%m-%d") <= a.until]

    def key(r):
        h, session, t, model, branch, _ = r
        parts = {"week": "week from " + week_start(t, wd, a.reset_hour).strftime("%Y-%m-%d %a %H:%M"),
                 "day": t.strftime("%Y-%m-%d %a"), "model": base_model(model) if h == "claude" else model,
                 "branch": branch, "session": h + " " + session}
        return tuple(parts[b] for b in by)

    groups = collections.defaultdict(lambda: [collections.Counter(), collections.Counter(), set()])
    for r in recs:
        g = groups[key(r)]
        g[0].update(r[5])
        parts = cost_parts(r[3], r[5])
        if parts is None:
            g[2].add(r[3])
        else:
            g[1].update(parts)
    width = max([len(" | ".join(k)) for k in groups] + [10])
    print("%-*s %7s %11s %11s %13s %10s %10s" % (width, " | ".join(by), "turns", "uncached in", "cache write", "cache read", "output", "API $"))
    total, total_parts = 0.0, collections.Counter()
    for k in sorted(groups):
        c, parts, unpriced = groups[k]
        cost = sum(parts.values())
        total += cost
        total_parts.update(parts)
        print("%-*s %7d %11d %11d %13d %10d %10s" % (width, " | ".join(k), c["turns"], c["uncached"], c["w5m"] + c["w1h"],
                                                    c["read"], c["out"], "%.2f" % cost + (" +?" if unpriced else "")))
    print("%-*s %7s %11s %11s %13s %10s %10.2f" % (width, "total", "", "", "", "", "", total))
    if total:
        print("share of cost: " + ", ".join("%s %.0f%%" % (p, 100 * v / total) for p, v in total_parts.most_common()))
    unpriced = sorted({m for g in groups.values() for m in g[2]})
    if unpriced:
        print("no price for: " + ", ".join(unpriced) + " (marked +?; add them to PRICES)")
    print("Local time is Stockholm. Subscription plans are not billed per token; this is API-equivalent list price.")


if __name__ == "__main__":
    main()
