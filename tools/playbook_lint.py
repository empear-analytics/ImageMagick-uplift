#!/usr/bin/env python3
"""Check docs/refactoring/PLAYBOOK.md against the rules in *Growing the catalogue*.

Every recipe has the required fields and at most eight numbered steps in *How*. Codes
are unique. Every recipe that is not a founding one has a NEW row in *Catalogue
changes*, and every row has a valid type and decider. Whether an AMEND really keeps
the same transformation is left to the reviewer: the script cannot judge it.
"""
import re
import sys
from pathlib import Path

PLAYBOOK = Path(__file__).resolve().parent.parent / "docs/refactoring/PLAYBOOK.md"
MAX_STEPS = 8
FIELDS = ["Use when", "How", "Evidence level", "Provenance"]
NEW_FIELDS = ["Not covered by", "Before", "After", "Careful"]
TYPES = {"NEW", "AMEND", "WORDING"}
DECIDER = re.compile(r"^(owner|mandate \(\d{4}-\d{2}-\d{2}\)|after the fact)$")


def recipes(text):
    """Return {code: (name, body)} for every '## Recipe <code> - <name>' section."""
    out = {}
    for m in re.finditer(r"^## Recipe (\S+) - (.+?)\n(.*?)(?=^## |\Z)", text, re.M | re.S):
        code, name, body = m.group(1), m.group(2).strip(), m.group(3)
        if code in out:
            raise SystemExit(f"duplicate recipe code {code}")
        out[code] = (name, body)
    return out


def how_steps(body):
    """Numbered top-level steps between **How:** and the next bold field."""
    m = re.search(r"^\*\*How:\*\*(.*?)(?=^\*\*[A-Z][^*]*:\*\*|\Z)", body, re.M | re.S)
    return len(re.findall(r"^\d+\. ", m.group(1), re.M)) if m else 0


def change_rows(text):
    i = text.find("## Catalogue changes")
    if i < 0:
        return None
    rows = []
    for line in text[i:].splitlines():
        if re.match(r"\| \d{4}-\d{2}-\d{2} ", line):
            rows.append([c.strip() for c in line.strip().strip("|").split("|")])
    return rows


def main():
    text = PLAYBOOK.read_text()
    errors = []
    recs = recipes(text)
    rows = change_rows(text)
    if rows is None:
        errors.append("no '## Catalogue changes' section")
        rows = []
    new_codes = set()
    for r in rows:
        if len(r) != 6:
            errors.append(f"change row has {len(r)} columns, not 6: {r[:2]}")
            continue
        date, typ, code, _change, _evidence, decider = r
        if typ not in TYPES:
            errors.append(f"{date}: type '{typ}' is not NEW, AMEND or WORDING")
        if not DECIDER.match(decider):
            errors.append(f"{date}: 'Decided by' is '{decider}'")
        if typ == "NEW":
            new_codes.add(code)
    for code, (name, body) in recs.items():
        founding = "founding recipe" in body
        for f in FIELDS + ([] if founding else NEW_FIELDS):
            if not re.search(r"^\*\*%s:\*\*" % re.escape(f), body, re.M):
                errors.append(f"Recipe {code}: missing **{f}:**")
        n = how_steps(body)
        if n > MAX_STEPS:
            errors.append(f"Recipe {code}: {n} steps in How (at most {MAX_STEPS}); "
                          "a change that needs more is a new recipe")
        if re.search(r"\.(c|h)\b", name):
            errors.append(f"Recipe {code}: name '{name}' names a file, not a transformation")
        if not founding and code not in new_codes:
            errors.append(f"Recipe {code}: no NEW row in Catalogue changes")
    for e in errors:
        print("playbook_lint:", e)
    print(f"playbook_lint: {len(recs)} recipes, {len(rows)} change rows, "
          f"{len(errors)} problem(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
