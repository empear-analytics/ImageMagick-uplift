# M114 - log.c file handler, ReadImage's seekable stream

Not a generated task file. Branch `campaign/M114`, stacked on `campaign/M113` (#77). Campaign B
on the Mac, night of 2026-10-29/30. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-30, Mac, night)

**log.c 4.20 → 4.29, constitute.c 4.65 → 4.76.**

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `LogMagickEventList`: the body of the file handler's size-limit test (close the log, rotate the generations) to `RotateLogFiles(log_info)`; the test stays | E | oracle, 22 cases | log 4.20 → 4.23 |
| `LogMagickEventList`: the file handler, test included, to `WriteLogFile(log_info,text)`; its `return(MagickFalse)` stays inside, and the caller returns `MagickFalse` when the function fails | W | oracle, 22 cases | log 4.23 → 4.29 |
| `ReadImage`: the seekable-stream block for decoders that need one to `OpenSeekableReadStream(image_info,read_info,magick_info,exception)`; both `return((Image *) NULL)` become `return(MagickFalse)` | W | oracle, 14,969 cases | constitute 4.65 → 4.76 |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact).
Line coverage at the tip: `RotateLogFiles` 38 calls, `WriteLogFile` 238, `OpenSeekableReadStream`
20,276. The `RotateLogFiles` commit says the moved body ran 11 times; that figure came from a
misread of line numbers in the coverage file from before the change, and the right count is 38.

**What made the log.c step work.** Moving the file handler alone scored 4.20 → 4.18, because the
new function took on the rotation's nesting. Taking the innermost block out first (E) and the
handler after it (W) gives 4.29. The same order may rescue other blocks this campaign reverted
for inherited nesting, such as `GetPathComponent`'s subimage block and `IdentifyImage`'s
transparent-pixel search.

**Tried and left.**
- `WriteImage`'s seekable-stream block writes `status` and `temporary`, which are both read
  later. Recipe O would give it seven parameters.
- Multi-root splits in `delegate.c`, `annotate.c`, `profile.c` and `utility.c`:
  `split_source.py` refuses them (`extern "C"` blocks, a function-local macro, a FreeType-only
  typedef, or statics shared with functions outside the family).
- `AcquireStreamRegion`: small, and its two bumps are each a few lines.
