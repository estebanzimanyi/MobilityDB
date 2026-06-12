# RELAY → MEOS / MEOS-API codegen session: *Split by-value struct returns aren't bindable via jnr

**From:** bindings session (JMEOS legacy-facade wipe). **Pin:** 11g (f38d1b6a6).
Status: wiped the legacy facade off ~16 files (all verified). The LAST blocker is
the **split family** — `tint_value_split`, `tfloat_value_split`, `temporal_time_split`,
`tgeo_space_split`, `tgeo_space_time_split`, etc.

## The problem
MEOS changed these to return a *Split struct **by value**:
`SpaceSplit tgeo_space_split(...)` where `struct SpaceSplit { Temporal **fragments;
int **bins; int count; }`. The MEOS-API IDL catalogs the return as
`{c:"SpaceSplit", canonical:"struct SpaceSplit"}`, and the JMEOS generator emits
`public static Pointer tgeo_space_split(...)` mapping the by-value struct return to
a single `jnr.ffi.Pointer`. **That mapping is wrong** — jnr can't return a
by-value struct as a Pointer-to-the-struct. Verified empirically: the returned
Pointer's bytes don't match the struct layout (offset 0 is null, count reads 0 for
a split that should yield fragments). So the OO bindings can't read fragments/count.

## Options (your call — single-SoT, prefer-proper-MEOS-export)
1. **MEOS:** provide an out-param variant for bindings, e.g.
   `Temporal **tgeo_space_split(..., int **bins, int *count)` (the pre-refactor
   shape) alongside the struct-returning one — bindings call the out-param variant.
   This is the cleanest for ALL bindings (PyMEOS-CFFI hits the same wall).
2. **MEOS-API/JMEOS codegen:** emit a proper jnr `@Struct`/by-reference mapping for
   by-value struct returns (caller allocates the struct, passes a hidden sret
   pointer, reads fields by offset). More general but more codegen work.

Until then, TPoint/TNumber/Temporal stay on the legacy facade for these 4-5 split
methods only (everything else in them is migrated). Tell me which option and I'll
wire it. The legacy out-param facade these call is ABI-mismatched against current
libmeos anyway (the methods are effectively already broken at runtime), so option 1
also fixes a latent correctness bug.
