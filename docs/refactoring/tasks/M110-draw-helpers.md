# M110 - draw.c: gradient stops, fill alpha, stroke slope, dash index

Not a generated task file. Branch `campaign/M110`, stacked on `campaign/M109` (#71). Campaign B
on the Mac, day of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, afternoon)

**draw.c 1.90 → 2.00.**

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `DrawGradientImage`: the PadSpread colour-stop lookup to `GetPadGradientColor(gradient,offset,composite)`, returning `composite` (passed in: `CompositePixelInfoBlend` writes only alpha and colour, so the other fields carry over); inside the OpenMP loop, it writes only loop-body locals | E | oracle, 73 cases | flat 1.90 (cc 46 → 40) |
| the same for ReflectSpread, `GetReflectGradientColor` (its tests differ, so not D) | E | oracle, 73 cases | flat 1.90 (cc 40 → 36) |
| `GetFillAlpha`: point-to-edge distance to `GetEdgeDistance(p,i,x,y)`, returning `distance` | E | oracle, 354 cases | 1.90 → 1.94 |
| `GetFillAlpha`: the winding-number loop to `GetWindingNumber(polygon_info,x,y)` | E | oracle, 354 cases | flat 1.94 (cc 45 → 33) |
| `TraceStrokePolygon`: the slope computation for the first and each later segment (`.p` and `.q` members) to `GetStrokeSlope(dx,dy,&slope,&inverse_slope)` | D | oracle, 47 cases | 1.94 → 1.98 |
| `DrawDashPolygon`: the two dash-index advances to `GetNextDashIndex(draw_info,n)` | D | oracle, 6 cases | 1.98 → 2.00 |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact). Line coverage at the tip: every new function runs (`GetPadGradientColor` 6.4
million calls, `GetReflectGradientColor` 3,200, `GetEdgeDistance` 7.2 million,
`GetWindingNumber` 2.1 million, `GetStrokeSlope` 1,601, `GetNextDashIndex` 299; both of
`DrawDashPolygon`'s copies ran before the step, once and 298 times).

**Tried and reverted.**
- `TraceStrokePolygon`: the miter distance expression (six identical copies) to a helper,
  flat with only 6 lines saved.
- `ConvertPathToPolygon`: the edge-store block (three identical copies) to a helper of seven
  arguments, flat with a new Excess Arguments finding.

**What blocks the rest of draw.c.**
- `TracePath`: almost every block uses `ThrowPointExpectedException`, a macro that returns.
- `TraceStrokePolygon`'s join switches: `CheckPathExtent`, a macro that reallocates and can
  return.
- `DrawGradientImage`'s RepeatSpread lookup and `DrawPrimitive`'s method cases read or write
  six or more locals (`status` among them), so a helper takes too many arguments.
- `RenderMVGContent` (cc 461) has not been attempted in this session.

**Question for the owner.** `GetStrokeSlope` is Recipe D over two copies that write two
members each, so the shared helper writes both through pointers, as in #70's 12-bit export
helper. Same question as there: is that within D?
