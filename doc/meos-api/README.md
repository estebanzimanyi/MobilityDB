# RFC: MEOS-API — a versioned JSON catalog for MEOS's C-library public API

> Discussion: [Issue #836](https://github.com/MobilityDB/MobilityDB/issues/836)

## The Problem

MEOS exposes several hundred C functions plus a structured catalog of types (structs, enums, opaque pointers). Every language binding consumes this same surface:

| Binding | Sync strategy |
|---|---|
| PyMEOS | Hand-rolled CFFI annotations, manually updated against `meos.h` |
| JMEOS | Hand-rolled JNR-FFI bindings, manually updated |
| meos-rs | bindgen-driven, with MEOS-specific tweaks in patches |
| GoMEOS | Hand-rolled CGO declarations, manually updated |
| MEOS.NET | Hand-rolled P/Invoke declarations, manually updated |
| MEOS.js | Hand-rolled emscripten/embind glue, manually updated |

N bindings times M MEOS releases means cross-cutting maintenance work every time a function is added, a parameter signature changes, or a struct gains a field. Mostly mechanical work that automates if there is a single machine-readable description of MEOS's public API.

The reference implementation [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API) is a Python tool that parses MEOS's `*.h` headers with libclang and emits `meos-api.json`, a structured catalog of every function, struct, and enum with type signatures, ownership rules, and documentation strings.

## Proposal

MEOS-API is a versioned JSON schema for the machine-readable description of the MEOS C library's public API, backed by the generator at [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API).

### Catalog structure

`meos-api.json` is a JSON document with three top-level arrays:

```jsonc
{
  "version": "0.1.0-draft",
  "meos_version": "1.2.0",
  "generated_at": "2026-05-01T12:00:00Z",
  "functions": [ /* function entries */ ],
  "structs":   [ /* struct entries   */ ],
  "enums":     [ /* enum entries     */ ]
}
```

A typical function entry:

```jsonc
{
  "name": "tpointseq_make",
  "file": "meos.h",
  "returnType": { "c": "TSequence *", "canonical": "TSequence *" },
  "params": [
    { "name": "instants", "cType": "TInstant **" },
    { "name": "count",    "cType": "int" }
  ],
  "ownership": "caller",
  "nullable": true,
  "doc": "Creates a temporal point sequence from an array of instants."
}
```

### Annotation source-of-truth

Annotations (ownership, nullability, array sizing, callback scope) live in the C header as part of the Doxygen block, not in a sidecar file. The generator parses the headers plus annotations and emits `meos-api.json` as a derived artefact.

```c
/**
 * tpointseq_make: (transfer full) (nullable)
 * @instants: (array length=count): array of instants - caller retains ownership
 * @count: number of instants; must be >= 1
 */
TSequence *tpointseq_make(TInstant **instants, int count);
```

The annotation vocabulary follows [GObject Introspection (GIR)](https://gi.readthedocs.io/en/latest/annotations/giannotations.html) verbatim: `(transfer full|none|container)`, `(nullable)`, `(array length=...)`, `(out)`, `(scope async|notified|call)`, etc.

### Schema versioning

The catalog schema follows `MAJOR.MINOR.PATCH`, independent of the MEOS C-library version (carried separately as `meos_version`).

### Deliverables

1. The schema specification at `doc/specs/meos-api-0.1-draft.md`.
2. The JSON Schema validator at `schemas/meos-api-0.1-draft.json`.
3. The reference generator at [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API).
4. A round-trip test: generate from a MEOS snapshot, validate against the JSON Schema, feed into PyMEOS-CFFI's generator, assert the binding builds.
5. Annotation-coverage CI that fails if any public symbol in `meos.h` is missing required Doxygen annotations.

## Related

- [Issue #836](https://github.com/MobilityDB/MobilityDB/issues/836) — discussion thread
- [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API) — reference generator
- [PR #833](https://github.com/MobilityDB/MobilityDB/pull/833) — MEOS-WKB byte-format spec
- [`doc/temporal-parquet/`](../temporal-parquet/README.md) — TemporalParquet (a consumer of the API catalog)
- [GObject Introspection](https://gi.readthedocs.io/en/latest/) — annotation vocabulary adopted verbatim
