# M125: campaign B, morning of 2026-11-13, property.c and blob.c

Branch `campaign/M125`, stacked on M124 (PR #102). `tools/ch.py` (cs-mcp 1.5.16).

| File | Before | After | Commits | Left |
| --- | ---: | ---: | ---: | --- |
| `property.c` | 2.47 | 2.53 | 1 | five big functions hold the score together (`InterpretImageProperties` cc 124, `GetMagickProperty` 123, `GetEXIFProperty` 99, `SetImageProperty` 94, `GetMagickPropertyLetter` 77, whose `WarnNoImageReturn` macro hides a `return`); 4,000 lines |
| `blob.c` | 2.71 | 2.71 | 0 | `WriteBlob`/`OpenBlob` cc 42, `ReadBlob` 33, `CloseBlob` 31, `SetBlobExtent` 29, `FileToBlob` 27, `SeekBlob` 25; past the function-count threshold; throws and trace calls stay |

`property.c`: `GetICCProperty`'s four copies of the length-query, fetch and `SetImageProperty` block
became a table of (`cmsInfoType`, key) and one 4-argument helper `SetICCInfoProperty` (Recipe
D; each call acquires and destroys its own `StringInfo`, where one was shared before, an
allocation-only difference). The oracle build defines `MAGICKCORE_LCMS_DELEGATE`, so the
changed branch ran: 14,982 cases reach `GetICCProperty`, 0 diverged.

`blob.c`: three probes (peek at the magic header, the write-open chain of `OpenBlob`, the
temporary-file branch of `ImageToBlob`) were flat or lower; the last needed a 6-argument helper.
An open lead: the write-open chain plus the fd, popen and fifo opens of `OpenBlob` together,
with the throws left in place, might take `OpenBlob` below Large Method.

Whole catalogue at the tip: 15,961 cases, 3 diverged (`info/d333f9b818`, `gaps/2857e5295a`,
`gaps/c5651ffad8`). `ci_warnings.sh` OK.
