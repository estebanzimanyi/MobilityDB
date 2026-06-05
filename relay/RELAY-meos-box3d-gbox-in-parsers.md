# RELAY → MEOS session (other machine): add `box3d_in` / `gbox_in` text parsers

**From:** MobilityNebula streaming-parity session (this box). **Date:** 2026-06-05.
**Severity:** parity blocker (small, well-scoped). **Type:** missing public MEOS API.
**Why you (MEOS):** per the *expose-MEOS-symbols-when-needed* directive — close the
gap in MEOS C so it propagates to the whole ecosystem, not a per-binding hack.

## What's needed

Two text→box input parsers, the inverses of the existing `box3d_out` / `gbox_out`:

```c
/* meos_geo.h, MEOS section (#if MEOS), next to the existing box3d_out/gbox_out */
extern BOX3D *box3d_in(const char *str);
extern GBOX  *gbox_in(const char *str);
```

## Why (consumer context)

MobilityNebula needs the **reverse** STBox conversions `box3d_to_stbox` and
`gbox_to_stbox` wired. Both are already in the 1,939 streamable surface
(`feeds/streamable.txt` lines 231 `box3d_to_stbox`, 586 `gbox_to_stbox`) but are
BLOCKED: there is no public MEOS parser to read a `BOX3D(...)` / `GBOX ...` text
literal into a `BOX3D*` / `GBOX*` operand. The *forward* `stbox_to_box3d` /
`stbox_to_gbox` are wired in MobilityNebula (PR #44); only the reverse pair
remains, gated on these two parsers.

## Implementation sketch (`meos/src/geo/postgis_funcs.c`, beside `box3d_out`/`gbox_out`)

- **`gbox_in`** — wrap liblwgeom's `GBOX *gbox_from_string(const char *str)`
  (declared in the bundled `liblwgeom.h:2056`). Validate non-null `str`; on a
  NULL return raise the standard MEOS parse error (as the other `*_in` do).
  It is the exact inverse of `gbox_out` (`"GBOX X((..))"` / `"GBOX Z((..))"`).

- **`box3d_in`** — liblwgeom has **no** BOX3D string parser, so parse
  `box3d_out`'s own grammar: an optional `"SRID=%d;"` prefix, then
  `"BOX3D((xmin,ymin,zmin),(xmax,ymax,zmax))"`, and call
  `box3d_make(xmin, xmax, ymin, ymax, zmin, zmax, srid)`. `box3d_out` emits
  `"%sBOX3D((%s,%s,%s),(%s,%s,%s))"` (postgis_funcs.c:223) — mirror it exactly.
  Use the project float parse and the standard MEOS parse-error on malformed
  input.

## Acceptance gate

- Round-trip: `box3d_in(box3d_out(b)) == b` and `gbox_in(gbox_out(b)) == b` for a
  2D box and a Z (3D) box.
- Exported (`nm -D` shows both), added to `meos.h` if the codegen catalog needs it.
- **Build BOTH targets** (libmeos + the PG extension) per the
  *meos-c-build-both-targets* rule — `*_in` inline ctors are libmeos-only.

## After it lands

Push a fresh `ecosystem-pin-*` tag (the usual bus). This session then repins
Nebula's vcpkg MEOS, regenerates `box3d_to_stbox` / `gbox_to_stbox` as a
follow-up wave, and the box-conversion family closes (+2 → 1,836 / 1,939).
Nothing else is blocked on this — it is the only missing piece for the reverse
box conversions.
