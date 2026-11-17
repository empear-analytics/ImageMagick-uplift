#!/usr/bin/env python3
"""Tests of tools/split_branch_check.py's naming of numbered static locals, run from the repository root:

    python3 tools/test_split_branch_check.py

The original object and the object of the file split off it number a function's `static` locals differently
(clang's `XMagickCommand.radius.203` is `.196` once other numbered names no longer come before it), so a
reference to the variable must compare equal across the two objects and a reference to another variable must
not. canonical_reference_names() names each such reference by its rank among the numbers of its stem that the functions use; the planted
defects below are the references that must still differ. Further tests cover .data.rel.ro.local tables, a
leading `endbr64` in one object only, and a report that lists every differing function.
"""
import contextlib, importlib.util, io, os, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("split_branch_check", os.path.join(ROOT, "tools", "split_branch_check.py"))
sbc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sbc)


def ref(sec, name, size=4096):
    return "R_X86_64_PC32\t%s[%s:%d+0]" % (sec, name, size)


def renamed(*refs):
    """The canonical lines of one function that makes the references REFS, in order."""
    return sbc.canonical_reference_names({"f": list(refs)})["f"]


class CanonicalReferenceNames(unittest.TestCase):
    OLD = [ref(".data", "F.radius.203"), ref(".data", "F.radius.212"), ref(".data", "F.factor.223"), ref(".bss", "plain", 8)]
    NEW = [ref(".data", "F.radius.196"), ref(".data", "F.radius.205"), ref(".data", "F.factor.216"), ref(".bss", "plain", 8)]

    def test_renumbered_statics_get_the_same_names(self):
        self.assertEqual(renamed(*self.OLD), renamed(*self.NEW))
        self.assertEqual(renamed(*self.OLD)[0], ref(".data", "F.radius.#0"))
        self.assertEqual(renamed(*self.OLD)[1], ref(".data", "F.radius.#1"))

    def test_swapped_order_differs(self):
        # planted defect: the two radius variables are referenced the other way round
        swapped = [self.NEW[1], self.NEW[0]] + self.NEW[2:]
        self.assertNotEqual(renamed(*self.OLD), renamed(*swapped))

    def test_a_reference_to_another_stem_differs(self):
        # planted defect: the variable the old object calls F.factor.* is called F.radius.* in the new one
        other = self.NEW[:2] + [ref(".data", "F.radius.216")] + self.NEW[3:]
        self.assertNotEqual(renamed(*self.OLD), renamed(*other))

    def test_a_missing_variable_differs(self):
        # planted defect: the second radius variable is never referenced (it is gone from the new object)
        fewer = [ref(".data", "F.radius.196"), ref(".data", "F.radius.196"), self.NEW[2], self.NEW[3]]
        self.assertNotEqual(renamed(*self.OLD), renamed(*fewer))

    def test_ranks_count_only_the_variables_the_functions_reference(self):
        # gcc's `display.N` has no function in its name: the kept file's one variable is rank 0 in both objects even
        # when the original object holds another `display.M` that only the moved functions use (not referenced here)
        self.assertEqual(renamed(ref(".bss", "display.4915", 8)), renamed(ref(".bss", "display.4863", 8)))

    def test_plain_names_other_sections_and_other_references_are_untouched(self):
        self.assertEqual(renamed(ref(".bss", "counter", 4), "mov %eax,%ebx", ref(".data.rel.local", "t.7", 8)),
                         [ref(".bss", "counter", 4), "mov %eax,%ebx", ref(".data.rel.local", "t.#0", 8)])


class ReadOnlyLocalTables(unittest.TestCase):
    """gcc's PIC code puts a file-scope `static const` table of string pointers (fx.c's `Operators`) in
    .data.rel.ro.local; a split that moves other tables shifts its offset, which must not count, while a reference
    to another table must."""
    SEC = ".data.rel.ro.local"
    OLD = {SEC: [(0x000, 0x1e0, "Constants"), (0x1e0, 0x268, "Operators")]}
    NEW = {SEC: [(0x000, 0x268, "Operators"), (0x268, 0x1e0, "Constants")]}

    def test_shifted_table_resolves_to_the_same_symbol(self):
        self.assertEqual(sbc.symbol_at(self.OLD, self.SEC, 0x1e0 + 0x10), sbc.symbol_at(self.NEW, self.SEC, 0x10))

    def test_another_table_differs(self):
        # planted defect: the new code reads Constants where the old read Operators
        self.assertNotEqual(sbc.symbol_at(self.OLD, self.SEC, 0x1e0 + 0x10),
                            sbc.symbol_at(self.NEW, self.SEC, 0x268 + 0x10))

    def test_numbered_names_are_ranked_in_this_section_too(self):
        self.assertEqual(renamed(ref(self.SEC, "tab.12", 64)), [ref(self.SEC, "tab.#0", 64)])


class LeadingEndbr64(unittest.TestCase):
    BODY = ["mov %rdi,%rax", "ret"]

    def test_a_leading_endbr64_in_one_object_only_is_counted_not_reported(self):
        kinds, bad = sbc.compare_by_name({"f": ["<f>", "endbr64"] + self.BODY}, {"f": ["<f>"] + self.BODY})
        self.assertEqual((kinds, bad), ({"leading endbr64": 1}, []))

    def test_endbr64_in_both_objects_is_no_difference(self):
        both = {"f": ["<f>", "endbr64"] + self.BODY}
        self.assertEqual(sbc.compare_by_name(both, dict(both)), ({}, []))

    def test_an_endbr64_elsewhere_still_differs(self):
        # planted defect: an endbr64 that is not the function's first instruction
        _, bad = sbc.compare_by_name({"f": ["<f>", "mov %rdi,%rax", "endbr64", "ret"]}, {"f": ["<f>"] + self.BODY})
        self.assertNotEqual(bad, [])

    def test_a_different_instruction_after_the_endbr64_still_differs(self):
        # planted defect: the leading endbr64 hides nothing that follows it
        _, bad = sbc.compare_by_name({"f": ["<f>", "endbr64", "mov %rsi,%rax", "ret"]}, {"f": ["<f>"] + self.BODY})
        self.assertEqual(bad, [("f", "mov %rsi,%rax", "mov %rdi,%rax")])


class ReportListsEveryFunction(unittest.TestCase):
    def test_more_than_ten_differing_functions_are_all_listed(self):
        names = ["f%02d" % i for i in range(12)]
        kinds, bad = sbc.compare_by_name({n: ["<%s>" % n, "mov $0x1,%eax"] for n in names},
                                         {n: ["<%s>" % n, "mov $0x2,%ebx"] for n in names})
        out = io.StringIO()
        with contextlib.redirect_stdout(out), self.assertRaises(SystemExit):
            sbc.report(kinds, bad)
        self.assertIn("DIFFERENT: 12 function(s): " + " ".join(names), out.getvalue())
        self.assertEqual(out.getvalue().count("  <f"), 12)


if __name__ == "__main__":
    unittest.main()
