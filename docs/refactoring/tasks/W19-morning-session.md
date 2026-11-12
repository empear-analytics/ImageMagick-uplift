# W19 morning session (2026-11-12)

Batch cut from origin/main 7aebe4c2b after #94 (W18) merged, for the two hours to 9 am the owner asked for. Sonnet 5.5 lanes
in `~/im-w14a..d` on `campaign/W19-a..d`; each finished lane merged into `campaign/W19` and gated on the whole catalogue
(and `tools/ci_linux_warnings.sh --campaign-a`, six configurations) before every push. Start: campaign A 84 files, mean 8.86,
36 at 10.0. Housekeeping first: CI on merged main fails only on campaign B files (type.c, utility.c, quantum-import.c,
pixel-import.c, pixel-export.c, annotate.c, blob.c); whole wide catalogue on merged main 14,200 cases, 0 diverged; the stale
`casemap.json` rebuilt (`tools/oracle/build.sh cov`, then `casemap.py`: 15,955 cases, 8,180 functions reached, about 5 minutes,
in the foreground: a detached background job is killed when the `wsl` call returns).

## Pattern mining (no commit)

**Lane d, narrow files:** scanners for HT, UX, SG, LC/CR, PL/TB/KT, LS, RT and FB/FI over every below-10 narrow file found
no legal chain that raises a score (HT 0, UX 0, LC/CR 0, PL/TB/KT 0 legal matches, LS 0; one SG candidate: `PlasmaImageProxy`,
rejected: SG alone 8.50 -> 7.98, SG plus CN 7.63, Bumpy Road stays at 3 because each block still holds its missing-pixel
`return` test and a Complex Conditional appears; composite.c's stub ceiling 3.69 -> 3.42 with the five statics stubbed, 3.74
with eight, so it cannot move without the owner-held chains; `MorphologyApply`'s stage-limit switch would give a cc-9
helper and the caller stays Complex and Large; `poly_basis_dx` cases return expressions, not constants, so TB does not
apply). The only new idea: a Recipe S split of `montage.c` to free its 1,000-line budget for `MontageImageList`.

## Lane a: opencl.c 5.80 -> 7.36, accelerate.c 3.19 -> 3.27 (wide build, merged)

6 commits; guard and `--calls` PASS; `ci_linux_warnings` OK in all six trees; whole wide catalogue 14,200 cases, 0 diverged
(at the opencl tip and again at HEAD); merged gate 15,955 cases, 0 diverged. 783febde5 opencl.c `GetOpenCLEnabled`, Recipe V
step 6: a new helper `HasCurrentOpenCLEnv(&clEnv)`; 5.80 -> 6.33; the Code Duplication category disappears (`GetOpenCLEnabled`
and `SetOpenCLEnabled` were a clone pair, one of three; six duplicated functions fall to four). 919b6f0db playbook: **new
Recipe GC, Guard Returns from a Conjunction** (`if (A1&&..&&An) return(T); return(F);` becomes one `if (Ai') return(F);` per
operand, then `return(T);`); e84534496 `IsBenchmarkedOpenCLDevice` GC 6.83; 507ac087a `IsSameOpenCLDevice` GC 7.36 (Complex
Conditional leaves the file; the two twins do not clone-flag). 349376f4d playbook: **new Recipe GD, Guard Returns from a
Disjunction** (`if (A1||..||An) return(F);` becomes n guards); fa5ac1e23 accelerate.c `AccelerateGrayscaleImage` GD 3.27
(Complex Conditional leaves; obj_modulo IDENTICAL). Both recipes Decided by `owner` under the 2026-11-10 OK to evolve the
recipes; they split a short-circuit chain into sequential guards, preserving the operands' evaluation order and
short-circuit (the How text is in the playbook commits). Mining (`mine.py` over functions with findings): HT 0, SG-a 0,
UX 1 (display.c, its inner `if` has no exit, so it does not fit), SG-b 36 hits (mostly xwindow, 25; one-function stubs flat),
PL 1 (widget.c `XScreenEvent`: macro labels, not enums or decimal literals), LC/CR 19 blocks and FILL/RT 286 runs (mostly used by
earlier lanes). **The Code Duplication threshold is a cliff:** six duplicated functions flag; with four the category is gone
(+0.53); adding a new twin pair brings it back (an E helper `SetScoreOnSameOpenCLDevices`, twin of
`EnableSameOpenCLDevices`, scored 6.77 against 7.36). Rejected: accelerate.c Recipe P predicate `HasUndefinedRGBTraits` (flat
3.19: the new function adds a clone); `BenchmarkOpenCLDevices` inner loop E (-0.59, the clone cliff); the doctype scan of
`LoadOpenCLDeviceBenchmark` as a helper (7.36 -> 7.43, but the helper is itself Bumpy and the branch is unreached: owner
question). Remaining open for the owner: opencl.c `GetOpenCLCacheDirectory` (stub 7.65, pinned by its `#if` code);
accelerate.c's ten wrapper clones (the preamble question); animate.c `XMagickCommand` cc 9 (stub 4.49) needs throws moved into
helpers (the `GetMagickModule()` question), `XAnimateBackgroundImage` and `XAnimateImages` stubs 5.30 and 6.73 but 20+ steps;
`GetOpenCLPlatformString` and `GetOpenCLDeviceString` are the last twin pair (function pointers). Tooling: backgrounded `wsl`
jobs freeze (the clock stops): run the oracle in the foreground; with `WIDE=1` the `casemap.json` does not match the wide
catalogue (`--function` fails): run the whole wide catalogue; `verify_wt.sh` prints VERIFY FAILED for wide files.
