# RELAY → bindings session: split family — fix in CODEGEN (sret), NOT in MEOS

**From:** MEOS / pin session. **Pin:** 11h (`840d4341`, tag `ecosystem-pin-2026-06-11h`,
FF on 11g — your 11g base is intact, nothing rebased).

## Decision: **Option 2** (codegen by-value-struct-return mapping). Do NOT add out-param variants to MEOS.

### Why option 1 is rejected
The by-value `*Split` return is **not** a regression — it is the established public
shape since `735f27a59` (the temporal-geo surface-completion), and it is **uniform
across the whole split family**. There are **seven** public ones:

| return type      | function |
|------------------|----------|
| `TimeSplit`      | `temporal_time_split` |
| `FloatSplit`     | `tfloat_value_split` |
| `FloatTimeSplit` | `tfloat_value_time_split` |
| `IntSplit`       | `tint_value_split` |
| `IntTimeSplit`   | `tint_value_time_split` |
| `SpaceSplit`     | `tgeo_space_split` |
| `SpaceTimeSplit` | `tgeo_space_time_split` |

Adding an out-param variant alongside each means **+7 MEOS public functions**, and by
`minimize-meos-api-prefer-parameter` every one of those explodes into an instantiation
in *every* binding (PyMEOS, JMEOS, GoMEOS, MEOS.NET, MEOS.js, Spark, Duck, the stream
connectors). One ABI quirk does not justify a 7×N surface multiplier. It also forks the
SQL/MEOS call sites and re-opens a C-API shape that is otherwise clean and consistent.

### Why option 2 is correct (NORTH STAR: single SoT, zero hand special-cases)
The defect is a **generator capability gap**, not a MEOS-API gap: the IDL faithfully
records `{c:"SpaceSplit", canonical:"struct SpaceSplit"}`; the JMEOS emitter wrongly
collapses a by-value struct return to a bare `Pointer`. Teach the emitter the **sret
calling convention** once and **all seven** (plus any future `*Split`) bind correctly,
in every binding, with **zero** MEOS changes.

### Concrete sret mechanics
On the SysV-AMD64 / AArch64 ABIs a struct this size is returned **via memory (sret)**:
the caller allocates the struct and passes a hidden pointer as an **implicit first
argument**; the callee fills it and returns that pointer.

- **jnr-ffi:** model the return as a `jnr.ffi.Struct` subclass and bind the function so
  the struct is returned by value — jnr inserts the sret pointer for you when the mapped
  return type is a `Struct` (not `Pointer`). Layout for `SpaceSplit` on LP64:
  `{ Pointer fragments; Pointer bins; int count; }` → offsets 0, 8, 16 (size 24 w/ pad).
  `fragments` is `Temporal**` (length `count`), `bins` is `int**` (length `count`).
- **PyMEOS-CFFI:** same fix at the CFFI layer — declare the real `struct SpaceSplit`
  in the cdef and let the return be the struct (cffi handles sret), then read
  `.fragments[i]` / `.count`. The current "returns a pointer" cdef is the same bug.

### On the "legacy out-param facade is ABI-mismatched / already broken"
Correct — that stale facade points at a pre-`735f...` symbol that no longer exists with
that signature. The fix is to bind the **current** struct-returning symbol via sret
(above), **not** to resurrect the out-param facade. So option 2 also closes that latent
runtime-correctness bug, with no MEOS export.

### Status / single known gap (logged, not silently capped)
Until the codegen sret mapping lands, those 4–5 split methods are the **only** remaining
items on the legacy facade — everything else in TPoint/TNumber/Temporal is migrated.
That is a bindings/MEOS-API-codegen task; **no pin change is required or forthcoming for
it**. If, while implementing sret, you find the IDL itself misrecords any `*Split` struct
layout (field order/types), relay that back — that *would* be a MEOS/IDL fix.

— pin stays at 11h; no new tag for this item.
