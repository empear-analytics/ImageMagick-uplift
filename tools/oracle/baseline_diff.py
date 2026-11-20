#!/usr/bin/env python3
"""Compare two baseline builds' results on a set of cases, filling both caches.

    tools/oracle/baseline_diff.py --base 46b61ab8d --other origin/main --filter RE
    WIDE=1 tools/oracle/baseline_diff.py ...            the wide builds

For cases added to the catalogue after the campaign's oracle baseline (owner 2026-11-20:
cases may be added for code no case reaches): their baseline results must come from the
campaign's baseline commit (46b61ab8d), not from a later main that already holds refactoring
rounds, or the oracle could not see a change those rounds made in the code the cases reach.
The script computes the missing results of the cases matching RE on each baseline binary, as
`oracle.py run --base REF` would, stores them in that binary's cache, and prints, per case,
SAME or DIFFERENT (with oracle.explain's reasons). Identical results are evidence that the
rounds between the two commits kept the behaviour those cases see; a difference is a finding
to report. Exits 1 if any case differs.
"""
import argparse
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle  # noqa: E402


def baseline_results(ref, pattern, jobs):
    """(cases, {case_key: result}) of the baseline at REF, missing results computed and cached."""
    base_bin = oracle.build("base", ref)
    manifest, all_cases = oracle.load_cases(base_bin)
    todo = oracle.select(all_cases, pattern)
    cpath = oracle.cache_path(base_bin, manifest)
    cache = oracle.read_cache(cpath, False)
    missing = [c for c in todo if oracle.case_key(c) not in cache]
    if missing:
        cache.update(oracle.run_baseline(base_bin, missing, manifest, jobs))
        oracle.write_cache(cpath, cache)
        shutil.rmtree(os.path.join(oracle.WORK, "runs", "base"), ignore_errors=True)
    print("%s: %d cases, %d computed now, binary %s" % (ref, len(todo), len(missing),
                                                       oracle.file_sha(base_bin)[:10]))
    return todo, cache


def difference(a, b):
    """None when both results exist and agree, else why not."""
    if a is None or b is None:
        return "no result on one side"
    if oracle.same(a, b):
        return None
    return "; ".join(oracle.explain(a, b)[:5])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--base", required=True, help="the campaign's baseline ref")
    ap.add_argument("--other", required=True, help="the ref to compare it with")
    ap.add_argument("--filter", required=True, help="regex on case id or label")
    ap.add_argument("-j", "--jobs", type=int, default=oracle.default_jobs())
    args = ap.parse_args()
    cases, base = baseline_results(args.base, args.filter, args.jobs)
    _, other = baseline_results(args.other, args.filter, args.jobs)
    differ = 0
    for c in cases:
        why = difference(base.get(oracle.case_key(c)), other.get(oracle.case_key(c)))
        differ += why is not None
        print("%s %s  %s%s" % ("SAME     " if why is None else "DIFFERENT", c["id"], c["label"],
                               "" if why is None else ": " + why))
    print("baseline_diff: %d cases, %d differ" % (len(cases), differ))
    return 1 if differ else 0


if __name__ == "__main__":
    sys.exit(main())
