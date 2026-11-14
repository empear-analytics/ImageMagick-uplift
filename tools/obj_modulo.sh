#!/bin/sh
# obj_modulo.sh FILE CALLER HELPER [REV]
#
# The object-code check for Recipes B and N on a function no oracle case reaches
# (playbook, *Two levels of evidence*, *Unreached functions: B and N*).
#
# Compiles FILE as committed at REV (default HEAD) and as it is in the working tree,
# with the oracle build's own flags (build-oracle/cand) at -g0, __LINE__ and __FILE__
# pinned, as tools/obj_identical.sh does. Then:
#   - byte-identical object files: prints IDENTICAL and exits 0;
#   - otherwise disassembles both per function, with addresses and call/jump offsets
#     removed, and requires
#       (1) every function present on both sides, other than CALLER (the function the
#           step changed) and HELPER (the function it added, or for Recipe B the
#           function whose parameters it bundled), to be identical instruction for
#           instruction;
#       (2) the functions present after but not before to be HELPER alone;
#       (3) the functions present before but not after to be functions that HELPER's
#           body calls, or that CALLER's body calls and whose definition is the same
#           text on both sides (a different inlining decision, not a change of code);
#     and prints MODULO-IDENTICAL and exits 0, or DIFFERENT and exits 1.
# NOINLINE=F1,F2 in the environment keeps those existing functions out of line on
# both sides (see below).
# CALLER and HELPER themselves are not compared here: their text is checked by
# refactor_guard.py (literals and calls) and by tools/struct_fill_check.py.
set -e
file=$1
caller=$2
helper=$3
rev=${4:-HEAD}
[ -n "$file" ] && [ -n "$caller" ] && [ -n "$helper" ] || {
  echo "usage: obj_modulo.sh FILE CALLER HELPER [REV]"; exit 2; }
root=$(git rev-parse --show-toplevel)
build=$root/build-oracle/cand
# WIDE=1: the wide candidate (X11 and OpenCL), the only build that compiles
# accelerate.c, animate.c, display.c, opencl.c, widget.c and xwindow.c.
[ -n "${WIDE:-}" ] && build=$root/build-oracle/cand-wide
# BUILD=<dir>: any other configured tree under build-oracle/, for code the oracle's HDRI build leaves
# out (ci-clang-min, ci-clang-dlg: CI's non-HDRI configurations, which compile `#if !defined(
# MAGICKCORE_HDRI_SUPPORT)` branches); EXTRA_CFLAGS="-O2 -w" adds optimisation to those trees' -O0 and drops their -Werror diagnostics.
[ -n "${BUILD:-}" ] && build=$root/build-oracle/$BUILD
cd "$build"
cflags=$(sed -n 's/^CFLAGS = //p' Makefile)
cppflags=$(sed -n 's/^CPPFLAGS = //p' Makefile)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=$(basename "$file")
mkdir "$tmp/before" "$tmp/after"
git -C "$root" show "$rev:$file" > "$tmp/before/$base"
cp "$root/$file" "$tmp/after/$base"
# NOINLINE=F1,F2: keep these existing static functions out of line on both sides
# (`__attribute__((noinline))` added to each one's definition line in both copies).
# For a reuse that gives an existing helper a second caller: the compiler stops
# inlining it into its first caller, which then differs only by that decision;
# with the helper out of line on both sides, the first caller must compare equal.
if [ -n "${NOINLINE:-}" ]
then
  for side in before after
  do
    python3 - "$tmp/$side/$base" "$NOINLINE" <<'PY'
import re, sys
p, names = sys.argv[1], sys.argv[2].split(",")
s = open(p).read()
for n in names:
    s, k = re.subn(r"^static ([^;\n]*\b" + re.escape(n) + r"\()",
                   r"static __attribute__((noinline)) \1", s, flags=re.M)
    if k != 1:
        sys.exit("NOINLINE: %d definition lines of %s" % (k, n))
open(p, "w").write(s)
PY
  done
  echo "kept out of line on both sides: $NOINLINE"
fi
for side in before after
do
  ( cd "$tmp/$side" &&
    eval clang -c -Wno-builtin-macro-redefined -D__LINE__=0 \
      "-D__FILE__=\\\"pp\\\"" -DHAVE_CONFIG_H -I"$build" -I"$root" \
      -I"$build/config" -I"$root/$(dirname "$file")" $cppflags $cflags ${EXTRA_CFLAGS:-} -g0 \
      -o "$tmp/$side.o" "$base" ) || { echo "BUILD FAILED ($side)"; exit 1; }
done
# An object file with no function in it proves nothing: the build does not
# compile the file (all of it inside an #if this configuration leaves out).
objdump -d "$tmp/after.o" | grep -q '^[0-9a-f]* <[^>]*>:$' ||
  { echo "NOTHING COMPILED $file in $build (try WIDE=1)"; exit 2; }
if cmp -s "$tmp/before.o" "$tmp/after.o"
then
  echo "IDENTICAL $file ($(wc -c < "$tmp/after.o" | tr -d ' ') bytes of object code)"
  exit 0
fi
for side in before after
do
  mkdir "$tmp/$side.fn"
  # The function at an address: the object's own symbol table, so that a
  # function Mach-O's disassembly labels with a section marker (`ltmp0`, the
  # first function of __text) keeps its name. Mach-O prefixes C names with `_`;
  # it is dropped, so that names compare with CALLER and HELPER as written.
  macho=$(objdump -h "$tmp/$side.o" | grep -c 'file format mach-o' || true)
  nm -n "$tmp/$side.o" | awk -v macho="$macho" '$2 ~ /^[tT]$/ && $3 !~ /^(ltmp|L|l_)/ {
      n=$3; if (macho) sub(/^_/,"",n); print $1, n }' > "$tmp/$side.names"
  # Instruction lines are `   1c:\tmov ...` (x86-64 ELF) or `   1c:      \tmov ...`
  # (arm64 Mach-O): the address, a colon, then any blank run. Relocation lines
  # (-r) name the symbol an instruction refers to (the callee of a `bl`/`call`,
  # whose encoded target is a placeholder until linking): their offset is
  # dropped and a local label (a string literal's `l_.str.12`, a constant
  # pool's `lCPI4_0`, an ELF section such as `.rodata`) is compared as `<anon>`, since its
  # number is only its order.
  objdump -d -r --no-show-raw-insn "$tmp/$side.o" | awk -v dir="$tmp/$side.fn" \
      -v names="$tmp/$side.names" -v macho="$macho" '
    BEGIN { while ((getline l < names) > 0) { split(l, a, " "); nm[a[1]]=a[2] } }
    /^[0-9a-f]+ <[^>]+>:$/ {
      name=$2; gsub(/[<>:]/,"",name)
      if ($1 in nm) name=nm[$1]
      else if (macho) sub(/^_/,"",name)
      out=dir "/" name; printf "" > out; next }
    out != "" && /^[ \t]*[0-9a-f]+:[ \t]+(R_|ARM64_RELOC_|X86_64_RELOC_)/ {
      sub(/^[ \t]*[0-9a-f]+:[ \t]+/,"")
      gsub(/[ \t]+/," ")
      n=split($0, r, " "); sym=r[n]
      sub(/[-+]0x[0-9a-f]+$/,"",sym)
      # Mach-O: C names start with `_`; `l...`/`L...` (l_.str.3, lCPI4_0,
      # ltmp1) are assembler-local labels. ELF: section-relative (.rodata+0x..).
      if (macho ? sym ~ /^[lL]/ : sym ~ /^\./) sym="<anon>"
      print "  reloc " r[1] " " sym > out; next }
    out != "" && /^ *[0-9a-f]+:[ \t]+/ {
      sub(/^ *[0-9a-f]+:[ \t]+/,"");
      gsub(/[0-9a-f]+ <[^>+]+\+0x[0-9a-f]+>/,"<local>");
      gsub(/[0-9a-f]+ <([^>]+)>/,"<&>");
      gsub(/<[0-9a-f]+ /,"<");
      gsub(/# [0-9a-f]+ <[^>]*>/,"");
      # A %rip-relative address of a whole function of the same section (`lea
      # -0x9120(%rip),%rdx # <<XScreenEvent>>`, a callback passed by address) is
      # resolved by the assembler, so its displacement moves with the code between;
      # the annotation names the target, which is what is compared.
      if ($0 ~ /# <<[^>+]+>>$/) gsub(/-?0x[0-9a-f]+\(%rip\)/,"(%rip)");
      # arm64 `adrp xN, <page>`: unlinked, the page is the instruction'"'"'s own
      # (objdump names whatever function it falls in); the relocation line
      # after it names the real target.
      if ($0 ~ /^adrp[ \t]/) sub(/,.*$/,", <page>");
      print > out }'
  # A disassembly this script cannot read is not a pass: every function must
  # have instructions, and there must be functions.
  nfn=$(ls "$tmp/$side.fn" | wc -l | tr -d ' ')
  [ "$nfn" -gt 0 ] ||
    { echo "ERROR: no functions extracted from the $side object"; exit 2; }
  for fnf in "$tmp/$side.fn"/*
  do
    [ -s "$fnf" ] ||
      { echo "ERROR: no instructions extracted for $(basename "$fnf") ($side)"; exit 2; }
  done
  ls "$tmp/$side.fn" | sort > "$tmp/$side.syms"
  # Alignment padding after a function's last instruction (the nop family and
  # int3 the assembler emits so that the next function starts on its boundary)
  # is not code of the function: no path reaches it. A new out-of-line helper
  # moves the next function's boundary and with it the padding of the function
  # before it, so the trailing run of padding is dropped before comparing.
  # Padding inside a function (a loop head's alignment) stays compared.
  for fnf in "$tmp/$side.fn"/*
  do
    awk '{ l[NR]=$0 } END {
      n=NR
      while (n > 0 && l[n] ~ /^(data16 )*(cs )*(nop[wl]?|xchg +%ax,%ax|int3)( |$)/) n--
      for (i=1; i<=n; i++) print l[i] }' "$fnf" > "$fnf.t" && mv "$fnf.t" "$fnf"
  done
done
rc=0
only_after=$(comm -13 "$tmp/before.syms" "$tmp/after.syms" | tr '\n' ' ')
only_before=$(comm -23 "$tmp/before.syms" "$tmp/after.syms" | tr '\n' ' ')
echo "functions only after: ${only_after:-none}"
echo "functions only before: ${only_before:-none}"
for f in $only_after
do
  [ "$f" = "$helper" ] || { echo "  unexpected new function $f"; rc=1; }
done
fn_text() {  # fn_text NAME FILE: the definition of NAME in FILE
  awk -v h="$1" '
    $0 ~ "^(static|MagickExport|MagickPrivate|ModuleExport) .*[ *]" h "\\(" { on=1 }
    on { print }
    on && /^}/ { exit }' "$2"
}
body=$(fn_text "$helper" "$root/$file")
caller_body=$(fn_text "$caller" "$root/$file")
for f in $only_before
do
  echo "$body" | grep -q "[^A-Za-z0-9_]$f(" && continue
  # A function the shortened caller now inlines: allowed when the caller calls it
  # and its definition is the same text on both sides (an inlining decision, not a
  # change of code).
  if echo "$caller_body" | grep -q "[^A-Za-z0-9_]$f(" &&
     [ -n "$(fn_text "$f" "$tmp/before/$base")" ] &&
     [ "$(fn_text "$f" "$tmp/before/$base")" = "$(fn_text "$f" "$tmp/after/$base")" ]
  then
    echo "  $f now inlined into $caller (its text unchanged)"
    continue
  fi
  echo "  $f vanished but neither $helper calls it nor $caller with its text unchanged"
  rc=1
done
compared=0
for f in $(comm -12 "$tmp/before.syms" "$tmp/after.syms")
do
  [ "$f" = "$caller" ] && continue
  [ "$f" = "$helper" ] && continue
  compared=$((compared+1))
  cmp -s "$tmp/before.fn/$f" "$tmp/after.fn/$f" ||
    { echo "  $f differs"; diff "$tmp/before.fn/$f" "$tmp/after.fn/$f" | head -6; rc=1; }
done
echo "functions compared instruction for instruction: $compared"
# The objects differ (else IDENTICAL above): with no function compared, nothing
# was shown equal, and that is never a pass.
[ "$compared" -gt 0 ] ||
  { echo "ERROR: objects differ and no function was compared"; exit 2; }
if [ $rc -eq 0 ]
then
  echo "MODULO-IDENTICAL $file (all but $caller and $helper)"
else
  echo "DIFFERENT $file"
fi
exit $rc
