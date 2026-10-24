#!/usr/bin/env python3
"""Check that refactor commits carry 'Recipe: <code>' and 'Verified: oracle|guard'.

  check_trailers.py --message-file .git/COMMIT_EDITMSG   (commit-msg hook)
  check_trailers.py --range main..HEAD                   (before a merge)

The codes are read from the playbook in the working tree, so a commit that uses a
recipe added on the same branch passes once the reviewer's change is in.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLAYBOOK = ROOT / "docs/refactoring/PLAYBOOK.md"


def codes():
    return set(re.findall(r"^## Recipe (\S+) - ", PLAYBOOK.read_text(), re.M))


def problems(msg, known):
    lines = msg.strip().splitlines()
    if not lines or not lines[0].startswith("refactor("):
        return []
    out = []
    rec = re.findall(r"^Recipe: *(\S+) *$", msg, re.M)
    ver = re.findall(r"^Verified: *(\S+) *$", msg, re.M)
    if len(rec) != 1:
        out.append(f"{len(rec)} 'Recipe:' trailers (need exactly one)")
    elif rec[0] not in known:
        out.append(f"recipe '{rec[0]}' is not in the playbook ({' '.join(sorted(known))})")
    if len(ver) != 1 or ver[0] not in ("oracle", "guard"):
        out.append("need one 'Verified: oracle' or 'Verified: guard' trailer")
    return out


def main():
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--message-file")
    g.add_argument("--range")
    a = ap.parse_args()
    known = codes()
    if a.message_file:
        msg = Path(a.message_file).read_text()
        msg = "\n".join(l for l in msg.splitlines() if not l.startswith("#"))
        p = problems(msg, known)
        for x in p:
            print("check_trailers:", x)
        return 1 if p else 0
    log = subprocess.run(["git", "log", "--no-merges", "--format=%h%x01%B%x00", a.range],
                         cwd=ROOT, capture_output=True, text=True, check=True).stdout
    bad = checked = 0
    for entry in filter(str.strip, log.split("\x00")):
        sha, msg = entry.strip().split("\x01", 1)
        if not msg.startswith("refactor("):
            continue
        checked += 1
        p = problems(msg, known)
        if p:
            bad += 1
            print(f"check_trailers: {sha} {msg.splitlines()[0][:70]}: {'; '.join(p)}")
    print(f"check_trailers: {checked} refactor commits, {bad} without valid trailers")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
