# RFC: MEOS-API — a versioned JSON catalog for MEOS's C-library public API

> **Issue [#836](https://github.com/MobilityDB/MobilityDB/issues/836)** — community discussion and sign-off

## The Problem

MEOS exposes several hundred C functions plus a structured catalog of types (structs, enums, opaque pointers). Every language binding consumes this same surface:

| Binding | Sync strategy today |
|---|---|
| PyMEOS | Hand-rolled CFFI annotations, manually updated against `meos.h` |
| JMEOS | Hand-rolled JNR-FFI bindings, manually updated |
| meos-rs | bindgen-driven, but MEOS-specific tweaks live in patches |
| GoMEOS | Hand-rolled CGO declarations, manually updated |
| MEOS.NET | Hand-rolled P/Invoke declarations, manually updated |
| MEOS.js | Hand-rolled emscripten/embind glue, manually updated |

This means **N bindings × M MEOS releases** of cross-cutting maintenance work every time a function is added, a parameter signature changes, or a struct gains a field — mostly mechanical work that could be automated if there were a single machine-readable description of MEOS's public API.

A reference implementation already exists: [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API), a Python tool (~6 files) that parses MEOS's `*.h` headers with libclang and emits `meos-api.json` — a structured catalog of every function, struct, and enum, with type signatures, ownership rules, and documentation strings. This RFC asks the community to ratify the catalog's JSON schema as a stable, versioned, citable specification.

## Why Now

Three converging forces:

1. **JMEOS 1.3** (PR [MobilityDB/JMEOS#9](https://github.com/MobilityDB/JMEOS/pull/9)) adopted `meos-api.json` as its source-of-truth for code generation. That is the first binding running on the catalog; every binding that follows ratchets the value of having the schema standardised as a contract that bindings declare compatibility against.
2. **The binding ecosystem** (PyMEOS / JMEOS / meos-rs / GoMEOS / MEOS.NET / MEOS.js) is now large enough that without a shared catalog, every new MEOS function multiplies into N hand-edits across N bindings. With a shared catalog, the same function propagates automatically when each binding's code-generator regenerates.
3. **Format-RFC precedent** — RFC #830 (TemporalParquet) and the MEOS-WKB byte-format spec (PR #833) established the pattern of canonising MEOS's external formats as standalone, versioned, citable specifications. `meos-api.json` is the logical next thing to canonise: the format that describes MEOS itself.

## Proposal

Define **MEOS-API**: a versioned JSON schema for the machine-readable description of the MEOS C library's public API, backed by the existing generator at [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API).

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

Annotations (ownership, nullability, array sizing, callback scope) live **in the C header as part of the Doxygen block**, not in a sidecar file. The generator parses the headers + annotations and emits `meos-api.json` as a derived artefact.

```c
/**
 * tpointseq_make: (transfer full) (nullable)
 * @instants: (array length=count): array of instants — caller retains ownership
 * @count: number of instants; must be ≥ 1
 */
TSequence *tpointseq_make(TInstant **instants, int count);
```

The annotation vocabulary follows [GObject Introspection (GIR)](https://gi.readthedocs.io/en/latest/annotations/giannotations.html) verbatim: `(transfer full|none|container)`, `(nullable)`, `(array length=…)`, `(out)`, `(scope async|notified|call)`, etc. Adopting the GIR vocabulary means inheriting 15 years of corner cases instead of rediscovering them.

**Why header-resident, not sidecar:** sidecar metadata files drift the moment a header changes without a corresponding sidecar update. Header-resident annotations are reviewed alongside the signature change. GIR's project history documents the sidecar→header migration explicitly; we adopt the lesson without paying the cost.

### Schema versioning

The catalog schema follows `MAJOR.MINOR.PATCH`, independent of the MEOS C-library version (carried separately as `meos_version`).

**Initial version: `0.1.0-draft`.** The schema is explicitly draft until `1.0.0` is ratified. Ratification requires three conditions:

1. **Real-binding validation** — at least one binding has regenerated end-to-end against the schema and shipped. [PyMEOS-CFFI](https://github.com/MobilityDB/PyMEOS-CFFI) is the natural first candidate.
2. **Completeness gaps resolved** — opaque pointer types, `va_list` callbacks, function-pointer typedefs, conditional compilation guards (`#ifdef CBUFFER` / `#ifdef NPOINT` / `#ifdef POSE`), the `meos.h` vs `meos_internal.h` split.
3. **GIR annotation coverage** — either the GIR vocabulary covers all MEOS constructs, or any gaps are documented with an explicit alternative.

### What ships

1. **The schema specification** — `doc/specs/meos-api-0.1-draft.md` (same template as `doc/specs/meos-wkb-0.9.md`).
2. **JSON Schema document** — `schemas/meos-api-0.1-draft.json` — a formal validator that consumers can use on any received `meos-api.json`.
3. **Reference generator** — already at [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API). Tracks the current draft and MEOS as it evolves.
4. **Round-trip test** — generate from a MEOS snapshot, validate against the JSON Schema, feed into PyMEOS-CFFI's generator, assert the binding builds.
5. **Annotation-coverage CI** — fails if any public symbol in `meos.h` is missing required Doxygen annotations.

## Alternatives Considered

1. **Hand-rolled per-binding parsers** — what the ecosystem does today. Multiplicative cost (N bindings × M releases). Not future-proof.
2. **Adopt an existing IDL** — WebIDL (wrong semantic shape for a C library), OpenAPI (wrong layer — HTTP/REST), Microsoft/CORBA IDL (wrong era — 1990s RPC). GIR and WIT are studied as prior art for vocabulary, not adopted wholesale.
3. **Publish the Clang AST directly** — Clang can dump its AST as JSON, but it contains far more than bindings need, the schema is not versioned for downstream consumption, and it changes between Clang releases. `meos-api.json` is the post-distillation form: a stable, version-pinned subset actually consumable by binding-generators.
4. **YAML or TOML** — same shape, different format. JSON has first-class parsers in the standard library of every binding language (Python, Java, Rust, Go, .NET, JavaScript) without third-party deps.
5. **Generate bindings directly** — i.e., have `MobilityDB/MEOS-API` emit Python / Java / Rust code directly. Centralises codegen but couples the tool to every target language. The intermediate-form approach (one emit, N consumers) scales better with N bindings.

## Open Questions

- **Directional sign-off** on canonising `meos-api.json` as the machine-readable form of MEOS's API, and on the architecture (header-resident GIR annotations, draft `0.x` schema, JSON serialisation). Not asking to lock `1.0.0` — that follows real-binding validation and completeness-gap closure.
- **Naming**: is `MEOS-API` the right name? Alternatives considered: `MEOS-IDL` (too jargon-y), `MEOS-Catalog`, `MEOS-Definitions`.
- **Schema completeness**: does the proposed schema cover what PyMEOS / JMEOS / meos-rs / GoMEOS / MEOS.NET / MEOS.js need? Anything missing from a function / struct / enum entry that would force binding-side workarounds?
- **Completeness gaps**: opaque pointer types, `va_list` callbacks, function-pointer typedefs, conditional compilation guards, `meos_internal.h` helpers. Each is a real MEOS construct that binding maintainers who've hand-rolled their wrappers can speak to.

## Related

- [Issue #836](https://github.com/MobilityDB/MobilityDB/issues/836) — community discussion and sign-off thread (includes revision history)
- [`MobilityDB/MEOS-API`](https://github.com/MobilityDB/MEOS-API) — the reference generator tool
- [PR #833](https://github.com/MobilityDB/MobilityDB/pull/833) — MEOS-WKB byte-format spec (the encoding catalog this one describes)
- [RFC #830 / doc/temporal-parquet/](../temporal-parquet/README.md) — TemporalParquet (a consumer of the API catalog)
- [GObject Introspection](https://gi.readthedocs.io/en/latest/) — prior art: annotation vocabulary adopted verbatim
- [WIT (WebAssembly Component Model)](https://component-model.bytecodealliance.org/design/wit.html) — prior art: ownership types and resource types studied
