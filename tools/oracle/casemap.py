#!/usr/bin/env python3
"""Which oracle cases execute which functions.

    tools/oracle/casemap.py            build build-oracle/work/casemap.json
    tools/oracle/casemap.py --update [RE]  add new catalogue cases only (drop removed ones),
                                       and profile again the known cases matching RE
    tools/oracle/casemap.py FUNCTION   list the cases that execute FUNCTION

Runs every case once with the coverage build (tools/oracle/build.sh cov), keeps
a profile per case, and records the functions each case executed. The map is
what lets tools/oracle/mutate.py test a mutant only on the cases that can
reach it, and what answers "does the oracle reach this function at all?"
before a function is refactored.

Function names are as LLVM records them: `name` for external functions,
`file.c;name` (or `path/file.c:name`) for static ones.
"""

import concurrent.futures
import glob
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle  # noqa: E402

MAP = os.path.join(oracle.WORK, "casemap%s.json" % oracle.WIDE)
PROFDATA = oracle.llvm_tool("llvm-profdata")
RAW = os.path.join(oracle.WORK, "casemap-raw" + oracle.WIDE)


def catalogue_digest(cases):
    return hashlib.sha1(json.dumps([c["id"] for c in cases]).encode()).hexdigest()


def functions_of(case_id):
    d = os.path.join(RAW, case_id.replace("/", "_"))
    raws = sorted(glob.glob(os.path.join(d, "*.profraw")))
    if not raws:  # every case runs magick or imdriver: none left a profile (aborted, timed out)
        sys.stderr.write("casemap: no profile for %s: no .profraw written\n" % case_id)
        return None
    merged = os.path.join(d, "merged.profdata")
    r = subprocess.run([PROFDATA, "merge", "-sparse", "-o", merged] + raws,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:  # e.g. a process killed mid-write; the case maps to nothing
        sys.stderr.write("casemap: no profile for %s: %s\n" % (case_id, r.stderr.decode()[:200]))
        return None
    out = subprocess.run([PROFDATA, "show", "--covered", "--all-functions", merged],
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                         check=True).stdout.decode()
    shutil.rmtree(d, ignore_errors=True)
    # With --covered, show prints one executed function name per line.
    return [line.strip() for line in out.splitlines() if line.strip()]


def profile_case(cov_bin, manifest, case, timeout=None):
    """The functions one case executes in the coverage build, or None."""
    d = os.path.join(RAW, case["id"].replace("/", "_"))
    os.makedirs(d, exist_ok=True)
    oracle.run_case(cov_bin, "casemap", case, manifest,
                    {"LLVM_PROFILE_FILE": os.path.join(d, "%p.profraw")}, timeout)
    shutil.rmtree(oracle.case_dir("casemap", case), ignore_errors=True)
    return functions_of(case["id"])


def merged_functions(m, ids, per_case):
    """The map's function entries without cases the catalogue lost, plus the new cases'."""
    functions = {n: sorted(ids.intersection(v)) for n, v in m["functions"].items()}
    for cid, names in per_case.items():
        add_case(functions, cid, names or [])
    return {n: sorted(v) for n, v in functions.items() if v}


def add_case(functions, cid, names):
    for n in names:
        functions.setdefault(n, []).append(cid)


def merged_unmapped(m, ids, per_case):
    lost = [c for c, names in per_case.items() if names is None]
    return sorted(ids.intersection(m["unmapped"])) + sorted(lost)


def update_map(pattern=None):
    """Add the cases the catalogue gained since the map was built, and drop the ones it lost,
    without rerunning the rest (cases added for unreached code, owner 2026-11-20). The new
    cases are profiled with today's coverage build, whose names may include helpers the map's
    own build did not have; the old entries keep theirs. PATTERN (a regex on case id or label)
    profiles the matching known cases again too, their old entries replaced: a map built
    before a function was split shows its new helpers only for the cases profiled since."""
    cov_bin = oracle.build("cov")
    manifest, cases = oracle.load_cases(oracle.build("base", "origin/main"))
    with open(MAP) as f:
        m = json.load(f)
    ids = {c["id"] for c in cases}
    known = set(m["unmapped"]).union(*m["functions"].values())
    redo = {c["id"] for c in oracle.select(cases, pattern)} if pattern else set()
    new = [c for c in cases if c["id"] not in known or c["id"] in redo]
    per_case = oracle.parallel(lambda c: profile_case(cov_bin, manifest, c, 600), new,
                               min(4, oracle.default_jobs()), "casemap update")
    kept = ids - redo
    m.update(catalogue=catalogue_digest(cases), functions=merged_functions(m, kept, per_case),
             unmapped=merged_unmapped(m, kept, per_case),
             updated_with=m.get("updated_with", []) + [oracle.file_sha(cov_bin)])
    with open(MAP, "w") as f:
        json.dump(m, f)
    print("casemap: %d cases profiled (%d asked for again), %d dropped"
          % (len(new), len(redo), len(known - ids)))
    for c in new:
        print("  %s  %s: %d functions" % (c["id"], c["label"], len(per_case[c["id"]] or [])))


def build_map():
    cov_bin = oracle.build("cov")
    base_bin = oracle.build("base", "origin/main")
    manifest, cases = oracle.load_cases(base_bin)
    shutil.rmtree(RAW, ignore_errors=True)
    start = time.time()

    def one(case, timeout=None):
        return profile_case(cov_bin, manifest, case, timeout)

    # OpenCL cases apart, few at a time and with a long timeout: when the device profile does not
    # match, ImageMagick benchmarks the devices (about 150 s of CPU on the coverage build), and
    # among a full set of parallel cases they hit oracle.TIMEOUT, were killed before writing a
    # profile, and mapped to nothing (2026-10-09; the mismatch itself is fixed in
    # oracle.opencl_profile). The sweeps x11cl4 and x11cl5 took 520 accelerate.c mutants for
    # unreached.
    ocl = [c for c in cases if c.get("env", {}).get("MAGICK_OCL_DEVICE")]
    rest = [c for c in cases if not c.get("env", {}).get("MAGICK_OCL_DEVICE")]
    per_case = oracle.parallel(one, rest, oracle.default_jobs(), "casemap")
    per_case.update(oracle.parallel(lambda c: one(c, 600), ocl, min(4, oracle.default_jobs()),
                                    "casemap opencl"))
    by_function, unmapped = {}, sorted(c for c, n in per_case.items() if n is None)
    for cid, names in per_case.items():
        for n in names or []:
            by_function.setdefault(n, []).append(cid)
    for v in by_function.values():
        v.sort()
    with open(MAP, "w") as f:
        json.dump({"catalogue": catalogue_digest(cases), "cov_binary": oracle.file_sha(cov_bin),
                   "functions": by_function, "unmapped": unmapped}, f)
    print("casemap: %d cases, %d functions reached, %d cases without a profile, %.0fs"
          % (len(cases), len(by_function), len(unmapped), time.time() - start))


def load_map(cases=None):
    with open(MAP) as f:
        m = json.load(f)
    if cases is not None and m["catalogue"] != catalogue_digest(cases):
        sys.exit("casemap.json is stale: the catalogue changed; rerun tools/oracle/casemap.py")
    return m["functions"]


def cases_for(function, fmap):
    """Cases executing `function`, matching static-function spellings too."""
    hits = set(fmap.get(function, []))
    for name, ids in fmap.items():
        if name.endswith(";" + function) or name.endswith(":" + function):
            hits.update(ids)
    return sorted(hits)


if __name__ == "__main__":
    if len(sys.argv) == 1:
        build_map()
    elif sys.argv[1] == "--update":
        update_map(sys.argv[2] if len(sys.argv) > 2 else None)
    else:
        ids = cases_for(sys.argv[1], load_map())
        print("%d cases execute %s" % (len(ids), sys.argv[1]))
        for i in ids[:40]:
            print("  " + i)
