#!/usr/bin/env python3
"""The oracle's line coverage in a form CodeScene accepts.

    tools/oracle/lcov4codescene.py [in.lcov [out.lcov]]

Reads build-oracle/oracle.lcov (linecov.py) and writes
build-oracle/oracle-codescene.lcov, for `cs-coverage upload --format lcov`:

- only the records of files in the repository, with repo-relative SF: paths;
  system headers and the generated files under build-oracle/ are dropped;
- every hit count capped at 2^31-1: llvm-cov emits counts up to 2^64-1, and
  CodeScene's parser rejects anything above 32 bits.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle  # noqa: E402

CAP = 2**31 - 1


def cap(count):
    return str(min(int(count), CAP)) if count.isdigit() else count


def capped(line):
    """The record line with its hit count capped, if it carries one."""
    tag, sep, rest = line.partition(":")
    if not sep:
        return line
    fields = rest.split(",")
    if tag == "DA":  # DA:<line>,<count>[,<checksum>]
        fields[1] = cap(fields[1])
    elif tag == "FNDA":  # FNDA:<count>,<name>
        fields[0] = cap(fields[0])
    elif tag == "BRDA":  # BRDA:<line>,<block>,<branch>,<count or ->
        fields[3] = cap(fields[3])
    else:
        return line
    return "%s:%s" % (tag, ",".join(fields))


def records(path):
    """Each record of the file as a list of lines, end_of_record included."""
    record = []
    with open(path) as f:
        for line in f:
            record.append(line.rstrip("\n"))
            if line.startswith("end_of_record"):
                yield record
                record = []


def convert(record):
    """The record for CodeScene, or None for a file outside the repository."""
    source = record[0][len("SF:"):]
    rel = os.path.relpath(os.path.realpath(source), os.path.realpath(oracle.ROOT))
    if rel.startswith("..") or rel.split(os.sep)[0] == "build-oracle":
        return None
    return ["SF:" + rel] + [capped(line) for line in record[1:]]


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(oracle.OUT, "oracle.lcov")
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(oracle.OUT, "oracle-codescene.lcov")
    kept = dropped = 0
    with open(dst + ".tmp", "w") as out:
        for record in records(src):
            converted = convert(record)
            if converted is None:
                dropped += 1
                continue
            out.write("\n".join(converted) + "\n")
            kept += 1
    os.replace(dst + ".tmp", dst)
    print("lcov4codescene: %d files kept, %d dropped, %s" % (kept, dropped, os.path.relpath(dst, oracle.ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
