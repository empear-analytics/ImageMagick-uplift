#!/usr/bin/env python3
"""Check a Recipe SD step on a function no oracle case executes.

  switch_split_check.py FILE CALLER HELPER [HELPER ...] [--rev REV]

FILE is compared as committed at REV (default HEAD) and as it is in the working tree.
CALLER is the function whose `switch` the step split; the HELPERs are the functions the
step added, in the order the calls chain: the one CALLER's `default:` group calls first,
then the one that helper's `default:` group calls, and so on (Recipe SD steps 1 to 6, and
the `Targets` form of step 8). The script reads the groups of every `switch` in the chain
(a group is a run of labels and the body after them) and prints OK when all of these hold:

  - the added functions are exactly the HELPERs, and nothing else of the file changed:
    with the HELPER definitions cut out and each `switch (E) { ... }` of CALLER replaced
    by a marker, the two files are the same token stream;
  - every `switch` in the chain, the caller's and each helper's, controls on the same
    expression text `E`, and a helper's body is its `switch` and nothing but bindings
    `T *const v = p->v;` (a member bound under its own name, a member of the `const`
    targets typedef `T` that the caller's `const T x = { &m1, &m2, ... };` initialiser
    fills by the addresses of the variables of those names, in order);
  - a `default:` group that is not the innermost holds only the call of the next helper,
    `Next(a1,a2,...);` and `break;`, each argument the parameter of its position by name
    (`&p` for a pointer parameter `p` of the same name, or `p` passed on), so no argument is
    transposed;
  - the label sets partition the original's: flattening the chain (each link replaced by the
    groups of the helper it calls) gives, in the working tree, exactly the groups the
    original's chain had, label for label (every label in exactly one group of exactly one
    `switch`, `default:` in the innermost one, no label lost, added or doubled);
  - every group's body is the original's, token for token, once each bound member `(*v)` is
    read as `v` and a `&p` of a pointer parameter as `p` (the rewrite Recipe O step 5 and
    Recipe SD step 8 give); a group the caller keeps is the original's as written;
  - no moved group holds a `return`, `continue` or `goto`, and within each `switch` the
    groups keep their original order.

It cannot check types, the precedence of `*v` against `.`, `->` and `[]` (a wrong one does
not compile for the members Recipe SD carries), or what the groups read from the caller
beyond the arguments of the link calls: the build, `tools/obj_modulo.sh` and reading do.
"""
import re
import subprocess
import sys
from collections import namedtuple
from dataclasses import dataclass

TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\d[\w.]*|->|<<=|>>=|\+\+|--|'
    r'<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|\S')


def literal_at(text, i):
    """The string or character literal opening at text[i], and the index after it."""
    quote = text[i]
    j = i + 1
    while j < len(text) and text[j] != quote:
        j += 2 if text[j] == "\\" else 1
    return text[i:j + 1], j + 1


def block_comment_at(text, i):
    """A space for the /* */ comment opening at text[i], and the index after it."""
    return " ", text.index("*/", i + 2) + 2


def line_comment_at(text, i):
    """Nothing for the // comment opening at text[i], and the index of its newline."""
    j = text.find("\n", i)
    return "", len(text) if j < 0 else j


def scan(text, i):
    """What text[i:] contributes once comments are dropped, and the index after it."""
    if text[i] in "\"'":
        return literal_at(text, i)
    if text.startswith("/*", i):
        return block_comment_at(text, i)
    if text.startswith("//", i):
        return line_comment_at(text, i)
    return text[i], i + 1


def strip_comments(text):
    out = []
    i = 0
    while i < len(text):
        piece, i = scan(text, i)
        out.append(piece)
    return "".join(out)


def tokens(text):
    return TOKEN.findall(text)


def match_close(toks, i, open_, close):
    """Index of the token closing the bracket opened at toks[i]."""
    depth = 0
    for j in range(i, len(toks)):
        depth += (toks[j] == open_) - (toks[j] == close)
        if depth == 0 and toks[j] == close:
            return j
    raise SystemExit("FAIL: unbalanced %s at token %d" % (open_, i))


Span = namedtuple("Span", "start params_start params_end body_open body_close")


def definition_start(toks, name_index):
    """Index of the first token of the declaration whose name is toks[name_index]."""
    start = name_index
    while start > 0 and toks[start - 1] not in (";", "}", "{"):
        start -= 1
    return start


def definition_at(toks, i):
    """(name, Span) of the function whose parameter list opens at toks[i], else None."""
    if i == 0 or not re.match(r"[A-Za-z_]\w*$", toks[i - 1]):
        return None
    close = match_close(toks, i, "(", ")")
    if close + 1 >= len(toks) or toks[close + 1] != "{":
        return None
    body_close = match_close(toks, close + 1, "{", "}")
    return toks[i - 1], Span(definition_start(toks, i - 1), i, close, close + 1, body_close)


def functions(toks):
    """name -> Span, top-level definitions."""
    found = {}
    depth = 0
    i = 0
    while i < len(toks):
        depth += (toks[i] == "{") - (toks[i] == "}")
        definition = definition_at(toks, i) if toks[i] == "(" and depth == 0 else None
        if definition is None:
            i += 1
            continue
        name, span = definition
        found[name] = span
        i = span.body_close + 1
    return found


def switch_at(toks, i):
    """(switch, close paren, open brace, close brace) of the `switch (E) { ... }` at toks[i]."""
    if toks[i] != "switch" or toks[i + 1] != "(":
        return None
    pc = match_close(toks, i + 1, "(", ")")
    return i, pc, pc + 1, match_close(toks, pc + 1, "{", "}")


def switches(toks, lo, hi):
    """Top-level (not nested in another switch) `switch (E) { ... }` in toks[lo:hi]."""
    found = []
    i = lo
    while i < hi:
        found_here = switch_at(toks, i)
        if found_here:
            found.append(found_here)
            i = found_here[3]
        i += 1
    return found


def label_at(toks, i):
    """The label `case X :` or `default :` at toks[i] as text, and the index after its colon."""
    j = i + 1
    while toks[j] != ":":
        j += 1
    return " ".join(toks[i:j + 1]), j + 1


def close_group(out, labels, body):
    """Move the group read so far, when it has a body, from labels and body to out."""
    if body:
        out.append((tuple(labels), tuple(body)))
        labels.clear()
        body.clear()


def groups(toks, lo, hi):
    """The groups of a switch body toks[lo:hi]: (labels, body tokens)."""
    out = []
    labels = []
    body = []
    i = lo
    depth = 0
    while i < hi:
        t = toks[i]
        if depth == 0 and t in ("case", "default"):
            close_group(out, labels, body)
            label, i = label_at(toks, i)
            labels.append(label)
        else:
            depth += (t == "{") - (t == "}")
            body.append(t)
            i += 1
    if labels:
        out.append((tuple(labels), tuple(body)))
    return out


def bracket_depth_change(t):
    """+1 for an opening bracket, -1 for a closing one, else 0."""
    return (t in "([{") - (t in ")]}")


def split_args(toks):
    args, cur, depth = [], [], 0
    for t in toks:
        depth += bracket_depth_change(t)
        if t == "," and depth == 0:
            args.append(cur)
            cur = []
        else:
            cur.append(t)
    if cur:
        args.append(cur)
    return args


def parameters(toks, fn):
    """The parameter declarations of the function fn, each a list of tokens."""
    return split_args(toks[fn.params_start + 1:fn.params_end])


def param_names(toks, fn):
    return [a[-1] for a in parameters(toks, fn)]


def bound_member_at(body, i, bound):
    """(member, 4) when body[i:] opens with `( * member )` for a bound member, else None."""
    window = list(body[i:i + 4])
    if len(window) < 4 or window[2] not in bound:
        return None
    if window[:2] + window[3:] != ["(", "*", ")"]:
        return None
    return window[2], 4


def address_of_at(body, i, ptr_params):
    """(parameter, 2) when body[i:] opens with `& parameter` for a pointer parameter, else None."""
    if body[i] != "&" or i + 1 >= len(body):
        return None
    return (body[i + 1], 2) if body[i + 1] in ptr_params else None


def normalise(body, bound, ptr_params):
    """(*v) -> v for bound members, &p -> p for pointer parameters."""
    out = []
    i = 0
    while i < len(body):
        rewrite = bound_member_at(body, i, bound) or address_of_at(body, i, ptr_params)
        token, width = rewrite or (body[i], 1)
        out.append(token)
        i += width
    return tuple(out)


@dataclass
class Source:
    """One side of the comparison: its comment-free text, its tokens and its functions."""

    text: str
    toks: list
    funcs: dict


def parse(text):
    clean = strip_comments(text)
    toks = tokens(clean)
    return Source(clean, toks, functions(toks))


BINDING = re.compile(r"^(?:[\w]+ )+\* const (\w+) = (\w+) -> (\w+) ;$")


def pointer_parameters(params):
    """The names of the pointer parameters among the declarations params."""
    return {a[-1] for a in params if "*" in a}


def statements(toks):
    """toks cut after each `;`: the complete statements, and the unfinished tail."""
    complete, cur = [], []
    for t in toks:
        cur.append(t)
        if t == ";":
            complete.append(cur)
            cur = []
    return complete, cur


class Chain:
    """The `switch` of a function: its controlling expression, bindings and groups.

    Only a function with a switch (ok) has the attributes expr, params, ptr_params, bound and
    groups. A strict chain also reads the body outside the switch: bindings and nothing else.
    """

    def __init__(self, src, name, problems, strict):
        self.name = name
        fn = src.funcs[name]
        found = switches(src.toks, fn.body_open + 1, fn.body_close)
        self.ok = bool(found)
        if not found:
            problems.append("%s has no switch" % name)
            return
        i, pc, bopen, bclose = found[0]
        params = parameters(src.toks, fn)
        self.expr = tuple(src.toks[i + 2:pc])
        self.params = [a[-1] for a in params]
        self.ptr_params = pointer_parameters(params)
        self.bound = {}
        if strict:
            outside = src.toks[fn.body_open + 1:i] + src.toks[bclose + 1:fn.body_close]
            self.read_bindings(outside, problems)
        self.groups = groups(src.toks, bopen + 1, bclose)

    def read_bindings(self, outside, problems):
        decls, unfinished = statements(outside)
        if unfinished:
            problems.append("%s: statements outside its switch: %s" %
                            (self.name, " ".join(unfinished)))
        for decl in decls:
            self.bind(decl, problems)

    def bind(self, decl, problems):
        """Record the binding `T *const v = p->v;` of a member v of the parameter p."""
        text = " ".join(decl)
        m = BINDING.match(text)
        if not m:
            problems.append("%s: not a binding: %s" % (self.name, text))
            return
        v, p, member = m.groups()
        if v != member:
            problems.append("%s: %s bound from member %s" % (self.name, v, member))
        if p not in self.params:
            problems.append("%s: binding from %s, not a parameter" % (self.name, p))
        self.bound[v] = (p, member)


CALL_TAIL = [")", ";", "break", ";"]


def link_target(norm_body, names):
    """Next helper name when the group is `{ Next(...); break; }`, else None."""
    b = list(norm_body)
    if b[:1] == ["{"] and b[-1:] == ["}"]:
        b = b[1:-1]
    if len(b) < 4 or b[-4:] != CALL_TAIL:
        return None
    if b[0] not in names or b[1] != "(":
        return None
    return b[0], b[2:-4]


Group = namedtuple("Group", "function index labels body")


def passes_parameter(arg, param, pointer_params):
    """Whether the argument tokens arg hand on the parameter param, or its address."""
    if arg == [param]:
        return True
    return arg == ["&", param] and param in pointer_params


@dataclass
class Walk:
    """Flattens the chain of switches of one side, collecting problems as it goes."""

    src: Source
    problems: list
    names: set
    ptr_all: set
    tag: str

    def flatten(self, name, expr, top=True):
        """[Group] with links expanded, and the chains read."""
        ch = Chain(self.src, name, self.problems, not top)
        if not ch.ok:
            return [], []
        self.check_expression(ch, expr)
        out, seen = [], [ch]
        for k, (labels, body) in enumerate(ch.groups):
            expanded, chains = self.expand(ch, k, labels, body)
            out.extend(expanded)
            seen.extend(chains)
        return out, seen

    def check_expression(self, ch, expr):
        if expr is not None and ch.expr != expr:
            self.problems.append("%s %s: controls on `%s`, not `%s`" %
                                 (self.tag, ch.name, " ".join(ch.expr), " ".join(expr)))

    def expand(self, ch, k, labels, body):
        """The group, or the link and the groups of the helper it calls, with their chains."""
        nb = normalise(body, set(ch.bound), self.ptr_all)
        link = self.link_of(ch.name, labels, nb)
        if link is None:
            return [Group(ch.name, k, labels, nb)], []
        nxt, args = link
        sub, subch = self.flatten(nxt, ch.expr, False)
        self.check_link_arguments(ch.name, nxt, args)
        return [Group(ch.name, k, None, ("LINK", nxt))] + sub, subch

    def link_of(self, name, labels, body):
        """(next helper, argument tokens) when the group is a `default:` that only calls one."""
        link = link_target(body, self.names)
        if link is None or labels != ("default :",):
            return None
        return link if link[0] != name and link[0] in self.src.funcs else None

    def check_link_arguments(self, name, nxt, args):
        """Arguments by position: the next helper's parameter of the same name."""
        nparams = param_names(self.src.toks, self.src.funcs[nxt])
        given = split_args(list(args))
        if len(given) != len(nparams):
            self.problems.append("%s: call of %s passes %d arguments for %d parameters" %
                                 (name, nxt, len(given), len(nparams)))
            return
        nptr = Chain(self.src, nxt, [], False).ptr_params
        for a, p in zip(given, nparams):
            if not passes_parameter(a, p, nptr):
                self.problems.append("%s: call of %s passes `%s` where the parameter is %s" %
                                     (name, nxt, " ".join(a), p))


DEFAULT = ("default :",)
EXITS = ("return", "continue", "goto")


def inventory_problems(old, new, caller, helpers):
    """Problems with the set of functions: the caller missing, the added ones not the helpers."""
    if caller not in old.funcs or caller not in new.funcs:
        return ["%s not found on both sides" % caller]
    problems = []
    added = sorted(set(new.funcs) - set(old.funcs))
    if added != sorted(helpers):
        problems.append("functions added: %s, expected %s" % (added, sorted(helpers)))
    gone = sorted(set(old.funcs) - set(new.funcs))
    if gone:
        problems.append("functions removed: %s" % gone)
    return problems


def leaf_groups(flat):
    """The groups of a flattened chain that are not links."""
    return [g for g in flat if g.labels is not None]


def doubled_labels(tag, leaves):
    """A problem for each label that is in a second group of the flattened chain."""
    seen = {}
    out = []
    for label, function in ((lab, g.function) for g in leaves for lab in g.labels):
        if label in seen:
            out.append("%s: label `%s` in two groups (%s and %s)" %
                       (tag, label, seen[label], function))
        seen[label] = function
    return out


def exit_problems(group):
    """A problem for each way out of the switch, other than break, that a moved group holds."""
    return ["%s: group %s holds `%s`" % (group.function, " ".join(group.labels), bad)
            for bad in EXITS if bad in group.body]


def kept_group_edited(labels, body, original):
    """Whether a group the caller keeps differs from the original's group of those labels."""
    if labels == DEFAULT or labels not in original:
        return False
    return original[labels] != body


def type_name(declaration):
    """The type of a parameter declaration: the token before its name, or before its `*`."""
    return declaration[-2] if declaration[-2] != "*" else declaration[-3]


def parameter_type(src, function, param):
    """The type name of the parameter param of function, None when it has none by that name."""
    fn = src.funcs[function]
    types = [type_name(a) for a in parameters(src.toks, fn) if a[-1] == param]
    return types[-1] if types else None


def typedef_members(text, ptype):
    """The members of `typedef struct _X { ... } ptype;` in text, in order."""
    td = re.search(r"typedef struct _\w+\s*\{([^{}]*)\}\s*" + re.escape(ptype) + r"\s*;", text)
    if td is None:
        return []
    parts = (part for decl in td.group(1).split(";") for part in decl.split(","))
    words = (re.findall(r"\w+", part) for part in parts)
    return [w[-1] for w in words if w]


def initialiser_problems(text, ptype, members):
    """Problems with the `const ptype x = { &m1, &m2, ... };` that fills the members."""
    init = re.findall(r"const\s+" + re.escape(ptype) + r"\s+\w+\s*=\s*\{([^}]*)\}", text)
    if not init:
        return ["no const %s initialiser in the file" % ptype]
    if [x.strip() for x in init[0].split(",")] != ["&" + m for m in members]:
        return ["the %s initialiser does not name its members by address in order" % ptype]
    return []


def cut_spans(src, caller, drop):
    """The spans to cut: the functions dropped (marker None) and the caller's switches."""
    spans = [(src.funcs[name].start, src.funcs[name].body_close, None)
             for name in drop if name in src.funcs]
    fn = src.funcs[caller]
    spans += [(i, bclose, "SWITCH")
              for (i, pc, bopen, bclose) in switches(src.toks, fn.body_open + 1, fn.body_close)]
    return sorted(spans)


def residue(src, caller, drop):
    """The tokens left once the dropped functions and the caller's switches are cut out."""
    out = []
    pos = 0
    for start, end, marker in cut_spans(src, caller, drop):
        out.extend(src.toks[pos:start])
        if marker:
            out.append(marker)
        pos = end + 1
    out.extend(src.toks[pos:])
    return out


def residue_difference(ores, nres):
    """Where two different residues first differ, as a problem."""
    for k, (a, b) in enumerate(zip(ores, nres)):
        if a != b:
            return ("the file differs outside the helpers and the switch near `%s` / `%s`" %
                    (" ".join(ores[max(0, k - 4):k + 4]), " ".join(nres[max(0, k - 4):k + 4])))
    return ("the file differs outside the helpers and the switch "
            "(length %d / %d)" % (len(ores), len(nres)))


class StepCheck:
    """An original source against the same source after a Recipe SD step: the checks of the
    module docstring, in order, collecting their problems."""

    def __init__(self, old, new, caller, helpers):
        self.old = old
        self.new = new
        self.caller = caller
        self.helpers = helpers
        self.problems = []

    def run(self):
        """(problems, summary); the summary is empty when the caller has no switch."""
        self.caller_chain = Chain(self.new, self.caller, self.problems, False)
        if not self.caller_chain.ok:
            return self.problems, ""
        self.flatten_both()
        for step in (self.check_reached, self.check_doubled_labels, self.check_labels,
                     self.check_bodies, self.check_kept_groups, self.check_exits,
                     self.check_order, self.check_bindings, self.check_residue):
            step()
        return self.problems, self.summary()

    def flatten_side(self, src, tag):
        walk = Walk(src, self.problems, self.names, self.ptr_all, tag)
        return walk.flatten(self.caller, self.caller_chain.expr)

    def flatten_both(self):
        """Flatten the chain of each side, and read its groups and labels."""
        self.names = set(self.helpers) | set(self.old.funcs)
        self.ptr_all = set().union(*[Chain(self.new, h, [], False).ptr_params
                                     for h in self.helpers])
        new_flat, self.new_chains = self.flatten_side(self.new, "new")
        old_flat, self.old_chains = self.flatten_side(self.old, "old")
        self.old_leaves, self.new_leaves = leaf_groups(old_flat), leaf_groups(new_flat)
        self.old_map = {g.labels: g.body for g in self.old_leaves}
        self.new_map = {g.labels: g.body for g in self.new_leaves}
        self.old_labels = {lab for labels in self.old_map for lab in labels}
        self.new_labels = {lab for labels in self.new_map for lab in labels}

    def check_reached(self):
        """Every helper must be reached by the chain."""
        reached = {c.name for c in self.new_chains}
        self.problems.extend("%s is not in the chain of %s" % (h, self.caller)
                             for h in self.helpers if h not in reached)

    def check_doubled_labels(self):
        """The label partition: every label in exactly one group, before and after."""
        for tag, leaves in (("original", self.old_leaves), ("result", self.new_leaves)):
            self.problems.extend(doubled_labels(tag, leaves))

    def check_labels(self):
        self.problems.extend("label `%s` lost" % lab
                             for lab in sorted(self.old_labels - self.new_labels))
        self.problems.extend("label `%s` added" % lab
                             for lab in sorted(self.new_labels - self.old_labels))

    def check_bodies(self):
        found = (self.group_problem(labels) for labels in self.old_map)
        self.problems.extend(p for p in found if p)

    def group_problem(self, labels):
        """The problem with the original's group of these labels, None when it is intact."""
        if labels in self.new_map:
            return self.body_difference(labels)
        if set(labels) <= self.new_labels:
            return "group %s: its labels are split across groups" % " ".join(labels)
        return None

    def body_difference(self, labels):
        was, now = self.old_map[labels], self.new_map[labels]
        if was == now:
            return None
        return "group %s: body differs\n    was: %s\n    now: %s" % (
            " ".join(labels), " ".join(was), " ".join(now))

    def check_kept_groups(self):
        """A group the caller keeps is unchanged as written (moved ones are normalised)."""
        original = {lab: body for (lab, body) in Chain(self.old, self.caller, [], False).groups}
        self.problems.extend("kept group %s was edited" % " ".join(lab)
                             for lab, body in self.caller_chain.groups
                             if kept_group_edited(lab, body, original))

    def check_exits(self):
        """No moved group may leave the switch other than by break."""
        for group in self.new_leaves:
            if group.function != self.caller:
                self.problems.extend(exit_problems(group))

    def check_order(self):
        """Within each switch the groups keep their original order."""
        order = {g.labels: n for n, g in enumerate(self.old_leaves)}
        by_function = {}
        for g in self.new_leaves:
            by_function.setdefault(g.function, []).append(order.get(g.labels, -1))
        self.problems.extend("%s: groups are not in their original order" % function
                             for function, index in by_function.items()
                             if index != sorted(index))

    def check_bindings(self):
        """The members of the targets typedef, filled by the caller's initialiser."""
        bindings = [(c.name, param, member)
                    for c in self.new_chains for param, member in c.bound.values()]
        for binding in bindings:
            self.check_binding(*binding)

    def check_binding(self, function, param, member):
        ptype = parameter_type(self.new, function, param)
        if ptype is None:
            self.problems.append("%s: parameter %s has no type" % (function, param))
            return
        members = typedef_members(self.new.text, ptype)
        if member not in members:
            self.problems.append("%s: %s is not a member of %s" % (function, member, ptype))
            return
        self.problems.extend(initialiser_problems(self.new.text, ptype, members))

    def check_residue(self):
        """Nothing else changed: cut the helpers and the caller's switches, compare the rest."""
        ores = residue(self.old, self.caller, [])
        nres = residue(self.new, self.caller, self.helpers)
        if ores != nres:
            self.problems.append(residue_difference(ores, nres))

    def summary(self):
        return ("%d labels in %d groups before, %d labels in %d groups across %d switches "
                "after" % (len(self.old_labels), len(self.old_leaves), len(self.new_labels),
                           len(self.new_leaves), len(self.new_chains)))


def check(old_text, new_text, caller, helpers):
    old, new = parse(old_text), parse(new_text)
    problems = inventory_problems(old, new, caller, helpers)
    if problems:
        return problems, ""
    return StepCheck(old, new, caller, helpers).run()


def parse_arguments(argv):
    """(revision, positional arguments) of the command line."""
    rev = "HEAD"
    args = []
    i = 1
    while i < len(argv):
        if argv[i] == "--rev":
            rev = argv[i + 1]
            i += 2
        else:
            args.append(argv[i])
            i += 1
    return rev, args


def committed_text(rev, path):
    """The file as committed at rev."""
    return subprocess.run(["git", "show", "%s:%s" % (rev, path)], capture_output=True,
                          text=True, check=True).stdout


def report(problems, summary, caller):
    """Print the verdict and return the exit status."""
    if not problems:
        print("OK %s: %s" % (caller, summary))
        return 0
    for p in dict.fromkeys(problems):
        print("FAIL: " + p)
    return 1


def main(argv):
    rev, args = parse_arguments(argv)
    if len(args) < 3:
        print(__doc__)
        return 2
    path, caller, helpers = args[0], args[1], args[2:]
    old = committed_text(rev, path)
    new = open(path).read()
    problems, summary = check(old, new, caller, helpers)
    return report(problems, summary, caller)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
