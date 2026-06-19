#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate MEOS smoke-test C files from any meos_<type>.h public header.

Each generated test walks every `extern <ret> name(<args>);` declaration
in its header and emits one call site per function, drawing arguments
from a shared common-inputs block. Pointer results are checked for NULL
and freed; a non-fatal MEOS error handler keeps the suite running past
validation failures so a single VALIDATE_* hit doesn't mask the rest of
the surface. The whole binary is intended to run under valgrind:

    valgrind --leak-check=full --error-exitcode=1 ./<type>_smoketest

A successful run exits 0 with no leaks reported. Surfaces declared but
not implemented in libmeos.so end up in each config's SKIP_REASON map
so the suite stays linkable while the gap remains documented.

The generator is configuration-driven: each entry in CONFIGS bundles
the header path, the common-inputs C block, the per-arg-type → variable
map, and the SKIP_REASON map. Add a new type by appending a config; no
generator change needed.
"""

import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))

# Self-contained, family-local smoke configs live here: a family ships
# meos/test/smoke/<family>.json and is DISCOVERED by glob (below) — the
# generator never enumerates families. This mirrors the established
# append_portable_aliases() file(GLOB ...) model on the SQL side, so a new
# family adds zero edits to any central registry.
SMOKE_DIR = os.path.join(ROOT, "smoke")

# Generate against the *installed* MEOS headers — the contract the
# resulting test will link against. Override with $MEOS_INCLUDE_DIR if
# the library is installed somewhere other than the default prefix.
HEADERS = os.environ.get("MEOS_INCLUDE_DIR", "/usr/local/include")

EXTERN_RE = re.compile(r"^extern\s+(.*?);\s*$", re.MULTILINE | re.DOTALL)
SIG_RE    = re.compile(r"^(?P<ret>.*[\s\*])(?P<name>\w+)\s*\((?P<args>.*)\)\s*$",
                       re.DOTALL)


def cleanup_type(s: str) -> str:
    s = re.sub(r"\bconst\b", "", s).strip()
    stars = s.count("*")
    s = s.replace("*", "")
    s = re.sub(r"\s+", " ", s).strip()
    if stars:
        s = s + " " + ("*" * stars)
    if stars > 1:
        s = s.replace("**", "* *")
    return s


def parse_args(arg_block: str):
    if not arg_block.strip() or arg_block.strip() == "void":
        return []
    parts, depth, cur = [], 0, []
    for ch in arg_block:
        if ch == "," and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
        else:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            cur.append(ch)
    if cur:
        parts.append("".join(cur).strip())
    out = []
    for p in parts:
        m = re.match(r"^(.*?)([\*\s])(\w+)$", p.strip())
        if not m:
            out.append((cleanup_type(p), ""))
            continue
        ty = (m.group(1) + m.group(2)).strip()
        out.append((cleanup_type(ty), m.group(3)))
    return out


# Global skip patterns that apply to every config. Aggregate transfns /
# combinefns own their first argument (state) — they pfree it internally
# and return a new state. The smoke-test pattern (call function, then
# free the result) leaves the original input dangling and reuses it on
# the next call, producing use-after-free errors under valgrind. These
# functions need PG's aggregate framework (or a manual state-handoff
# pattern) to exercise correctly; the smoke test is the wrong harness.
GLOBAL_SKIP = {
    "re:_(transfn|combinefn|finalfn)$":
        "aggregate state-handoff pattern incompatible with smoke-test ownership model",
}


def emit_call(fname, ret, args, arg_map, skip_map, override_args,
              value_returns=()):
    # Direct-name skip
    if fname in skip_map:
        return f"  /* SKIP {fname}: {skip_map[fname]} */\n"
    # Regex-pattern skip (key starts with 're:')
    for k, v in skip_map.items():
        if k.startswith("re:") and re.search(k[3:], fname):
            return f"  /* SKIP {fname}: {v} */\n"
    # Global regex-pattern skip (applied after per-config so a config can
    # explicitly override by listing the function in its own skip map).
    for k, v in GLOBAL_SKIP.items():
        if k.startswith("re:") and re.search(k[3:], fname):
            return f"  /* SKIP {fname}: {v} */\n"
    call_args = []
    overrides = override_args.get(fname, {})
    for i, (ty, _name) in enumerate(args):
        if i in overrides:
            call_args.append(overrides[i])
            continue
        v = arg_map.get(ty)
        if v is None:
            return f"  /* SKIP {fname}: unmapped arg type '{ty}' */\n"
        call_args.append(v)
    call = f"{fname}({', '.join(call_args)})"
    ret = cleanup_type(ret)
    if ret == "char *":
        return (f"  {{ char *r = {call};\n"
                f"    printf(\"{fname}: %s\\n\", r ? r : \"NULL\");\n"
                f"    if (r) free(r); }}\n")
    if ret == "bool":
        return (f"  {{ bool r = {call};\n"
                f"    printf(\"{fname}: %d\\n\", (int) r); }}\n")
    if ret == "int":
        return (f"  {{ int r = {call};\n"
                f"    printf(\"{fname}: %d\\n\", r); }}\n")
    if ret == "double":
        return (f"  {{ double r = {call};\n"
                f"    printf(\"{fname}: %.6f\\n\", r); }}\n")
    # By-value scalar returns the CONFIG declares (e.g. a cell index like
    # Quadbin = uint64, or a uint32_t resolution): call and discard — a value
    # return owns no storage, so there is nothing to free. Opt-in per config
    # (default empty) so the existing suites' output is unchanged.
    if ret in value_returns:
        return (f"  {{ {ret} r = {call}; (void) r;\n"
                f"    printf(\"{fname}: ok\\n\"); }}\n")
    # Double-pointer returns (T **) need element-by-element free using
    # the n_out count populated by the function's int* arg. The generator
    # only uses this shape when the call signature contains an `int *`
    # argument that we mapped to &n_out — that's our witness for "array".
    has_count_out = any(t == "int *" for (t, _n) in args)
    if ret.count("*") == 2 and has_count_out:
        elt = ret.replace(" *", "*")[:-1].strip()  # 'TInstant ** ' → 'TInstant *'
        return (f"  {{ {ret} r = {call};\n"
                f"    printf(\"{fname}: %s n=%d\\n\", r ? \"OK\" : \"NULL\", n_out);\n"
                f"    if (r) {{\n"
                f"      for (int _i = 0; _i < n_out; _i++) if (r[_i]) free(r[_i]);\n"
                f"      free(r);\n"
                f"    }} }}\n")
    if "*" in ret:
        return (f"  {{ {ret} r = {call};\n"
                f"    printf(\"{fname}: %s\\n\", r ? \"OK\" : \"NULL\");\n"
                f"    if (r) free(r); }}\n")
    return f"  /* SKIP {fname}: unmapped return type '{ret}' */\n"


HEADER_TEMPLATE = """\
/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2026, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2026, PostGIS contributors
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice and
 * this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *****************************************************************************/

/**
 * @file
 * @brief MEOS smoke test for the {type_label} public API.
 *
 * Auto-generated by meos/test/gen_smoketest.py — do not edit by hand.
 *
 * Each public symbol exported by {header_relpath} gets one smoke-test
 * call site here. Arguments come from a shared common-inputs block.
 * Pointer results are freed; a non-fatal MEOS error handler keeps the
 * run going past VALIDATE_* failures so a single bad input doesn't
 * mask the rest of the surface.
 *
 * Run under valgrind to catch leaks/OOB reads:
 *
 *     valgrind --leak-check=full --error-exitcode=1 ./{out_basename}
 *
 * Build:
 *   gcc -Wall -g -I/usr/local/include -o {out_basename} \\
 *       {out_basename}.c -L/usr/local/lib -lmeos -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <meos.h>
#include <meos_geo.h>
{extra_includes}

#ifndef DatumDefined
typedef uintptr_t Datum;
#define DatumDefined 1
#endif

static void
test_error_handler(int level, int code, const char *msg)
{{
  (void) level; (void) code;
  fprintf(stderr, "[meos warn] %s\\n", msg);
}}

int
main(void)
{{
  meos_initialize();
  meos_initialize_timezone("UTC");
  meos_initialize_error_handler(test_error_handler);

{common_inputs}

  printf("****************************************************************\\n");
  printf("* {type_label} MEOS smoke test%*s*\\n", {pad}, "");
  printf("****************************************************************\\n");

"""

FOOTER_TEMPLATE = """
{cleanup}

  meos_finalize();
  return 0;
}}
"""


# -------------------------------------------------------------------------
# Per-type configurations.
# -------------------------------------------------------------------------

# meos_geo.h — temporal geometry / temporal point. The largest header
# (417 externs); this first pass covers the basic subset (constructors,
# predicates, simple accessors) and skips functions whose argument types
# need bespoke setup (AFFINE, GBOX, SkipList, bitmatrix, etc.).
TGEOMETRY_CONFIG = dict(
    type_label="tgeometry   ",
    header="meos_geo.h",
    out="tgeometry_smoketest.c",
    extra_includes='',
    arg_map={
        "Temporal *":          "tgeo1",
        "TInstant *":          "tgeo_inst1",
        "TSequence *":         "tgeo_tseq1",
        "TSequenceSet *":      "tgeo_tseqset1",
        "GSERIALIZED *":       "geom1",
        "GSERIALIZED **":      "&geom_out_param",
        "STBox *":             "stbox1",
        "Set *":               "geomset1",
        "Span *":              "tstzspan1",
        "SpanSet *":           "tstzspanset1",
        "Interval *":          "interv1",
        "TimestampTz":         "tstz1",
        "TimestampTz *":       "&tstz1",
        "bool":                "true",
        "bool *":              "&bool_out",
        "double":              "1.0",
        "int":                 "1",
        "int *":               "&n_out",
        "size_t":              "0",
        "size_t *":            "&size_out",
        "uint8 *":             "NULL",
        "uint8_t":             "1",
        "uint32":              "0",
        "uint32_t":            "0",
        "int32_t":             "0",
        "int32":               "0",
        "int64":               "1",
        "interpType":          "LINEAR",
        "Datum":               "geom1_datum",
    },
    override_args={},
    skip={
        # Functions whose argument types need bespoke setup the
        # default canned-inputs don't supply. First-pass skip list;
        # refine as needed.
        "re:AFFINE":     "needs an AFFINE matrix",
        "re:GBOX":       "needs a GBOX",
        "re:SkipList":   "needs a SkipList state",
        "re:bitmatrix":  "needs a bitmatrix",
        # Geographic / SRID-dependent paths need a populated spatial_ref_sys
        # CSV; skip rather than crash on the standalone test setup.
        "re:^geog":      "needs spatial_ref_sys CSV",
        "re:_geog$":     "needs spatial_ref_sys CSV",
        "re:_geography_": "needs spatial_ref_sys CSV",
        "re:_to_geography$": "needs spatial_ref_sys CSV",
        # Out-params with non-uniform shape (e.g. GSERIALIZED ***).
        "re:^geo_array_": "out-param triple-pointer not in canned set",
        # Returns a static string literal — free()ing it crashes.
        "geo_typename":   "returns static string; free() is invalid",
        # Constructors that crash on LINEAR interp default for a span/spanset
        # input — the canned tstzspanset has gaps which the kernel rejects.
        # Refining requires per-function interp arg overrides.
        "re:^tgeoseqset_from_base":  "needs STEP interp on multi-span input",
        "re:^tgeoseq_from_base":     "needs STEP interp on multi-span input",
        "re:^tpoint_from_base":      "needs hand-constructed Temporal input",
    },
    common_inputs="""\
  TimestampTz tstz1 = pg_timestamptz_in("2001-01-02", -1);
  Span *tstzspan1 = tstzspan_in("[2001-01-01, 2001-01-04]");
  SpanSet *tstzspanset1 = tstzspanset_in("{[2001-01-01, 2001-01-02], [2001-01-03, 2001-01-04]}");
  Interval *interv1 = pg_interval_in("1 day", -1);
  GSERIALIZED *geom1 = geom_in("Polygon((0 0,1 0,1 1,0 1,0 0))", -1);
  GSERIALIZED *geom_out_param = NULL;
  Set *geomset1 = geomset_in("{\\"Point(0 0)\\", \\"Point(1 1)\\"}");
  STBox *stbox1 = stbox_in("STBOX X((0, 0), (10, 10))");
  Datum geom1_datum = (Datum) geom1;
  size_t size_out = 0;
  bool bool_out = false;

  Temporal *tgeo1 = tgeometry_in(
    "[Polygon((0 0,1 0,1 1,0 1,0 0))@2001-01-02, Polygon((0 0,1 0,1 1,0 1,0 0))@2001-01-03]");
  TInstant *tgeo_inst1 = (TInstant *) temporal_start_instant(tgeo1);
  TSequence    *tgeo_tseq1    = (TSequence *) tgeo1;
  TSequenceSet *tgeo_tseqset1 = NULL;
  int n_out = 0;
""",
    cleanup="""\
  if (tgeo_inst1) free(tgeo_inst1);
  if (tgeo1) free(tgeo1);
  free(stbox1);
  free(geomset1);
  free(geom1);
  free(interv1);
  free(tstzspanset1);
  free(tstzspan1);""",
)


# Only the core, always-built geometry surface is configured in-tree. Optional
# families (rgeo/pose/cbuffer/npoint/quadbin/…) ship their own data-only
# meos/test/smoke/<family>.json sidecar in the PR that adds the family, and are
# discovered by glob in main() — so this registry never names a family.
CONFIGS = {
    "tgeometry":  TGEOMETRY_CONFIG,
}


def write_test(name, cfg):
    header_path = os.path.join(HEADERS, cfg["header"])
    out_path = os.path.join(ROOT, cfg["out"])
    out_basename = cfg["out"][:-2]      # strip .c
    label = cfg["type_label"]
    pad = max(0, 60 - len(label) - len(" MEOS smoke test"))

    # A family's header is only installed when its feature was compiled in
    # (e.g. meos_quadbin.h needs -DQUADBIN=on). Skip the config when absent so
    # a partial build regenerates cleanly instead of crashing.
    if not os.path.exists(header_path):
        print(f"Skipping {name}: {header_path} not installed "
              "(feature not compiled in)")
        return

    with open(header_path) as f:
        src = f.read()

    decls = []
    for m in EXTERN_RE.finditer(src):
        sig = m.group(1)
        sigm = SIG_RE.match(sig)
        if not sigm:
            continue
        ret = sigm.group("ret").strip()
        fname = sigm.group("name").strip()
        args = parse_args(sigm.group("args"))
        decls.append((fname, ret, args))

    body = "".join(emit_call(fname, ret, args,
                             cfg["arg_map"], cfg["skip"],
                             cfg["override_args"],
                             cfg.get("value_returns", ()))
                   for fname, ret, args in decls)
    # An input declared in common_inputs is only consumed by a call that takes
    # its argument type; when every such function is skipped (or the header
    # exports none), the variable is unused. Reference each unconsumed simple
    # placeholder with (void) — the same idiom the VALIDATE_* macros use — so
    # -Wall -Wunused-variable stays clean.
    arg_vars = sorted({
        v.lstrip("&").strip() for v in cfg["arg_map"].values()
        if re.fullmatch(r"&?\s*\w+", v)
    })
    guards = "".join(
        f"  (void) {v};\n" for v in arg_vars
        if re.search(r"\b" + re.escape(v) + r"\b", body) is None
    )
    body += guards
    head = HEADER_TEMPLATE.format(
        type_label=label, header_relpath=cfg["header"],
        out_basename=out_basename,
        common_inputs=cfg["common_inputs"],
        extra_includes=cfg["extra_includes"],
        pad=pad)
    foot = FOOTER_TEMPLATE.format(cleanup=cfg["cleanup"])
    with open(out_path, "w") as f:
        f.write(head + body + foot)
    print(f"Wrote {out_path}: {len(decls)} declarations parsed.")


def load_sidecar(path):
    """Load a self-contained, family-local smoke config (data-only JSON) that a
    family ships in meos/test/smoke/<family>.json. The schema mirrors the legacy
    in-file CONFIG dicts, with two ergonomic differences for JSON:
      - common_inputs / cleanup are arrays of lines (no C-newline escaping);
      - override_args integer indices arrive as strings and are restored to int.
    Everything else (header/out/arg_map/skip/value_returns/extra_includes) is
    passed straight through to write_test()."""
    with open(path) as f:
        raw = json.load(f)
    cfg = dict(raw)
    cfg["common_inputs"] = "".join(line + "\n" for line in raw.get("common_inputs", []))
    cfg["cleanup"] = "\n".join(raw.get("cleanup", []))
    cfg.setdefault("extra_includes", "")
    cfg.setdefault("arg_map", {})
    cfg.setdefault("skip", {})
    cfg.setdefault("value_returns", [])
    cfg["override_args"] = {
        fn: {int(k): v for k, v in ov.items()}
        for fn, ov in raw.get("override_args", {}).items()
    }
    return cfg


def main():
    target = sys.argv[1] if len(sys.argv) > 1 else None
    for name, cfg in CONFIGS.items():
        if target and name != target:
            continue
        write_test(name, cfg)
    # Discovered, self-contained family sidecars: a new family drops
    # meos/test/smoke/<family>.json and is generated here without the generator
    # ever naming it (the append_portable_aliases file(GLOB ...) model). The
    # legacy in-file CONFIGS above stay as-is and migrate to sidecars later.
    for path in sorted(glob.glob(os.path.join(SMOKE_DIR, "*.json"))):
        name = os.path.splitext(os.path.basename(path))[0]
        if target and name != target:
            continue
        write_test(name, load_sidecar(path))


if __name__ == "__main__":
    main()
