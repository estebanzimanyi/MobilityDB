# KEYVALUE skiplist — continuation plan

## 1. Background and current state

PR #685 ("Enable skiplists of key-value pairs", merged 2025-08-16 as commit `1c539aad4`, +1209/-321 across 12 files) extended the temporal-aggregation skiplist machinery so that an element is no longer a bare temporal value but a `(key, value)` pair, with caller-supplied `comp_fn` and `merge_fn` callbacks driving comparison and aggregation. The intent — readable from the example file `meos/examples/ais_expand_skiplist.c` — is to let MEOS clients build a per-key accumulator in one pass over a streaming source (one AIS record at a time, indexed by MMSI), rather than first grouping into `Temporal *` values and then folding through the existing temporal aggregate API. The PR was merged as a structural enabler; the work paused before any end-to-end validation.

Critically, every production aggregate in `meos/src/temporal/temporal_aggfuncs.c`, `meos/src/geo/tgeo_aggfuncs.c`, and `meos/src/npoint/tnpoint_aggfuncs.c` constructs its state via `temporal_skiplist_make()` (`meos/src/temporal/skiplist.c:146`) and folds via `temporal_skiplist_splice()` (`meos/src/temporal/temporal_aggfuncs.c:363`), which hard-wires `sktype = TEMPORAL` and `keys = NULL` (line 366). The MobilityDB serialize/deserialize plumbing in `mobilitydb/src/temporal/skiplist.c:115-116` is likewise temporal-only. The single in-tree client of the `KEYVALUE` branch is `meos/examples/ais_expand_skiplist.c:310-311`, which calls `skiplist_splice(list, NULL, &newrec_p, 1, NULL, false, KEYVALUE)`. Because `keys == NULL` there, `keyval_skiplist_merge` (the function carrying most of the bugs) routes its key/value selection through the `else` branch at lines 507-511 — and because the example only ever inserts one MMSI at a time and merges via `trip_merge_fn` separately, the outer merge path inside `skiplist_splice` is never exercised either. So the buggy code is reachable by API but unreachable in practice.

A separate fix landed concurrent with this assessment (commit `c59391834`, "normalise skiplist merge return convention to fix newvalues leak", +29/-1) but it only adjusts shell-pointer ownership in `skiplist_splice`; it does not touch the seven correctness bugs called out below. The bugs remain in the working tree as shipped.

## 2. Design intent reconstruction

The PR commit body says only "Enable skiplists of key-value pairs" + "Make skiplist an opaque data type" — there is no design write-up in the PR body (`gh pr view 685` returns an empty body), no DocBook chapter, and no Doxygen group for KEYVALUE. So intent has to be inferred from the example and the function signatures. From those, the model appears to be:

- An opaque `SkipList` (forward-declared in `meos/include/meos.h:268`) with two parameters at `skiplist_make` time: `key_size` and `value_size`, plus `comp_fn(void*, void*)` and `merge_fn(void*, void*) -> void*` callbacks (`meos/include/meos_internal.h:1348-1349`).
- Two operating modes selected per `skiplist_splice` call by the `SkipListType sktype` argument (`meos/include/meos_internal.h:741-743`):
  - `TEMPORAL`: legacy mode, value-only, comparison is on `Temporal` time spans, aggregation produced by `temporal_skiplist_merge`.
  - `KEYVALUE`: new mode, paired storage, comparison is on the user-supplied key (or, when `keys == NULL`, on the value treated as its own key), aggregation produced by `keyval_skiplist_merge` calling the user's `merge_fn`.
- The example demonstrates a third sub-mode within `KEYVALUE`: caller-managed values where the splice path is used purely to allocate a slot, the value is then mutated in place outside the skiplist, and `merge_fn` is supplied to `skiplist_make` but never actually invoked (because the example calls `skiplist_search` first and only splices on miss).

So three intended use cases coexist in the same surface: (a) AIS-style "one record at a time, key-indexed insert-or-fetch" — what the example does, (b) "batch fold of `(key, value)` arrays" — what `keyval_skiplist_merge` is written for, (c) the legacy temporal mode. The boundary between (a) and (b) is the source of the design ambiguity that the bugs hide behind: the example deliberately stays clear of the merge path, so the merge path was never wired into anything that runs.

It cannot be reconstructed from evidence alone whether a `Temporal`-with-discriminator-key aggregation (e.g. "sum of `tfloat` per ship MMSI" with the skiplist itself doing the per-MMSI grouping) was an intended next-stage use case or merely an enabling step. The user should confirm.

## 3. Code inventory

Touchpoints, classified:

- **Public API (header)** — `meos/include/meos_internal.h:741-743` (`SkipListType` enum), `1347-1355` (`temporal_skiplist_make`, `skiplist_make`, `skiplist_search`, `skiplist_free`, `skiplist_splice`, `temporal_skiplist_splice`, `skiplist_values`, `skiplist_keys_values`). `meos/include/meos.h:268` (forward-decl) and `1799-1827` (existing `*_transfn` aggregates, all temporal). `meos/include/temporal/skiplist.h:54-57` (`ensure_same_skiplist_subtype`, `skiplist_set_extra`, `skiplist_headval`).
- **Struct definition** — `meos/include/meos_internal.h:706-733` (`SkipListElem` with `void *key`, `void *value`; `struct SkipList` with `key_size`, `value_size`, `comp_fn`, `merge_fn`).
- **Core implementation (KEYVALUE-relevant)** — `meos/src/temporal/skiplist.c:113-140` (`skiplist_make`), `186-238` (`skiplist_elempos`, `skiplist_search`, both `#if MEOS`-guarded), `429-475` (`keyval_skiplist_common`), `488-551` (`keyval_skiplist_merge`), `573-736` (`skiplist_splice` with `sktype` dispatch), `767-781` (`skiplist_keys_values`, also `#if MEOS`-guarded). The `skiplist_free` routine at `322-349` does walk and `pfree(elem->key)` if non-NULL, so the free side is in principle key-aware.
- **Production aggregate dispatch** — `meos/src/temporal/temporal_aggfuncs.c:363-367` (`temporal_skiplist_splice` thin wrapper that hardcodes `keys=NULL, sktype=TEMPORAL`). All sites listed by the inventory grep (`732, 751, 772, 792, 850, 1006, 1210, 1241, 1272, 1304, 1338` in `temporal_aggfuncs.c`; `meos/src/geo/tgeo_aggfuncs.c:228, 236`; `meos/src/npoint/tnpoint_aggfuncs.c:76, 84`) call the temporal-only wrapper.
- **MobilityDB layer** — `mobilitydb/src/temporal/skiplist.c:115-116` uses `temporal_skiplist_make` + `temporal_skiplist_splice`. No `KEYVALUE` plumbing in PG-facing code.
- **In-tree client of KEYVALUE** — `meos/examples/ais_expand_skiplist.c` (477 lines), specifically the `skiplist_make(0, sizeof(trip_record), &trip_comp_fn, &trip_merge_fn)` at line 186 and the `skiplist_splice(..., KEYVALUE)` at line 310. The example is not wired into any `CMakeLists.txt` and is built ad-hoc per the comment at line 54.
- **Tests** — none. `grep -rn 'KEYVALUE\|keyval_skiplist' meos/test/ mobilitydb/test/` returns zero hits.
- **Documentation** — none. `grep -rn 'KEYVALUE\|keyval_skiplist\|skiplist_keys_values' doc/` returns zero hits.

## 4. Gap analysis

Confirmed correctness bugs, all introduced in `1c539aad4` and still present in the working tree:

1. **`skiplist.c:512` — degenerate compare.** `int cmp = list->comp_fn(key1, key1);` always returns 0; the `cmp < 0` and `cmp > 0` branches are dead. Net effect: the merge collapses both inputs as if every key matched, calling `merge_fn(val1, val2)` for every pair until one side runs out, then taking the asymmetric tail loop.
2. **`skiplist.c:539` — wrong array on tail.** `result[count] = keys2[j];` writes a *key* into the *value* result array; the next line then overwrites it with the value. Net effect: the key-tail is silently dropped.
3. **Missing key in tail loop.** The tail loop at lines 537-541 never writes to `newkeys1`, so for the `j < count2` remainder the new-keys array has uninitialised slots while `*newcount` claims they are valid.
4. **Missing symmetric `i < count1` tail loop.** The `assert(i == count1)` at line 535 will fire whenever state1 is longer than state2 with no overlap. Either the assertion is wrong (the function does not in fact guarantee state1 is consumed first) or a second tail loop is missing.
5. **`skiplist.c:707-709` — orphaned key alloc.** Inside `skiplist_splice`, on the new-element insertion path, `void *newkey = palloc(list->key_size); memcpy(newkey, keys[i], list->key_size);` is computed but `newelem->key` is never assigned. The local `newkey` leaks and `newelem->key` retains whatever `palloc0`-zeroed value `skiplist_alloc` left it with. A subsequent `skiplist_search` against this element will compare against a NULL `key_cur`, which `skiplist_elempos` does not guard against (it only checks the *query* `key`, not `key_cur`).
6. **Untestable `skiplist_search` interaction with KEYVALUE.** `skiplist_elempos` at line 206 says `key ? comp_fn(key, key_cur) : comp_fn(value, value_cur)`. With bug 5 leaving `key_cur == NULL`, even the example's `skiplist_search(list, NULL, &newrec)` (which routes through the value branch) is OK, but any caller that passes a non-NULL `key` to `skiplist_search` will dereference a NULL `key_cur` inside their `comp_fn`.
7. **`update` zero-out scope.** `skiplist.c:444` does `memset(update, 0, sizeof(&update));` — `sizeof(&update)` is the size of a *pointer*, not the array. The intent is `sizeof(int) * SKIPLIST_MAXLEVEL`. This bug is present in `keyval_skiplist_common` but is also present in the temporal common path (it pre-dates PR #685; worth confirming with `git blame` before claiming it as a KEYVALUE-introduced regression).

Unverified items the user needs to confirm:

- Whether the `tofree` parameter convention in `keyval_skiplist_merge` is intentional. Lines 517-518 only push merged-result pointers into `tofree1` if the caller passed a non-NULL `tofree` argument. The recent normalisation fix `c59391834` baked in that `*tofree` is always distinct from `result`, so this `if (tofree)` guard is stale and the comment at 490-492 contradicts the code.
- Whether `skiplist_keys_values` (line 767) is supposed to allocate a parallel `values` array or expects the caller to pre-allocate. The signature `void **skiplist_keys_values(SkipList *list, void **values)` reads the caller-passed `values` as an output buffer (line 776 writes through it) but does not document required length. No caller in the tree.
- Whether `ensure_same_skiplist_subtype` (declared at `meos/include/temporal/skiplist.h:54`) is implemented anywhere — the symbol does not appear in `skiplist.c`.

## 5. Validation criterion

A test that proves KEYVALUE end-to-end works should look like a single-shot per-key fold using *only* the public KEYVALUE API, with no `temporal_skiplist_*` hidden in the call chain.

Sketch: build a skiplist whose keys are `int32` ship IDs and whose values are running `int64` instant counters. Insert N records `(mmsi_i, 1)` with `mmsi_i` drawn from a small set so the same key recurs and the merge path *must* fire. The `comp_fn` is `int32_cmp`, the `merge_fn` is `(left, right) => left + right`. Drive it through `skiplist_splice(list, keys_arr, vals_arr, count, NULL, false, KEYVALUE)` with `count > 1` so we exercise the multi-element merge — not one-at-a-time as the example does. Then call `skiplist_keys_values(list, vals_out)` and assert that for each unique key, the recovered value equals the count of inputs with that key.

A second, harder test: feed two batches into the same skiplist with overlapping key ranges so the spliced-out segment is non-empty and `keyval_skiplist_merge` runs its main loop on real data. Assert the same per-key sum invariant.

A third test, mirroring the AIS pattern but as a unit test: insert one record at a time via the search-then-splice idiom from the example, build per-key accumulators of `TInstant *` arrays (not `TSequence`, to keep the test independent of the temporal-aggregate machinery), and verify final per-key counts match the input distribution.

The top-level call surface under test is exactly: `skiplist_make`, `skiplist_search`, `skiplist_splice` with `sktype=KEYVALUE`, `skiplist_keys_values`, `skiplist_free`. Whatever the final answer is, it should not silently round-trip through `temporal_skiplist_*`.

## 6. Continuation milestones

Sequenced, each independently mergeable:

**M1 — Fix the seven bugs and stand up a unit harness.** Repair bugs 1-5 and 7 in `skiplist.c`, implement (or remove) `ensure_same_skiplist_subtype`. Add a new `meos/test/temporal/tnumber_keyval_skiplist_test.c` driving the validation tests in section 5. Acceptance: harness compiles, all three tests pass under ASan with zero leaks, code coverage tool reports the previously-dead branches in `keyval_skiplist_merge` as covered.

**M2 — Decide and document the merge-result ownership convention.** Reconcile the `if (tofree)` guard at `skiplist.c:517` with the post-`c59391834` invariant. Either drop the guard (always populate `tofree`) or document why callers may pass NULL. Update the doc comment at lines 490-492 to match. Acceptance: comment and code agree, ASan still clean.

**M3 — Promote the AIS example to a runnable smoke test.** Wire `meos/examples/ais_expand_skiplist.c` into a CMake target and add a CTest invocation that runs it against a tiny synthetic CSV (e.g. ten ships, fifty records). Acceptance: `ctest -R ais_expand_skiplist` passes; output table matches a stored expected file.

**M4 — Make `skiplist_search` safe for non-NULL keys.** Once bug 5 is fixed and elements have well-formed keys, add a unit test that calls `skiplist_search(list, &probe_key, NULL)` and asserts both hit and miss. Acceptance: search returns the expected element index in both branches; passes with `comp_fn` exercised on `key_cur`.

**M5 — Decide the production wiring.** Either (a) keep KEYVALUE as a MEOS-only utility for streaming clients (status quo), and document it as such in `meos/include/meos_internal.h` and a new Doxygen group, or (b) lift one production aggregate to KEYVALUE — the obvious candidate is `tnumber_tsum_*` partitioned by an integer discriminator, but this requires user input on the SQL surface. Acceptance for (a): doc-only; for (b): one new SQL aggregate with regression tests in `mobilitydb/test/temporal/`.

**M6 — Coverage and Codacy hygiene.** Bring `skiplist.c` keyval branches above the project-wide threshold (do not paper over with `LCOV_EXCL` markers per project policy). If the `KEYVALUE`-specific branch in `skiplist_splice` ends up genuinely unreachable from any in-tree caller after M5, remove it rather than mask it. Acceptance: project Codacy gate is green on the PR.

**M7 — Drafts cleanup.** Remove or fold this draft document into the appropriate runbook once the work lands.

## 7. Effort estimate and biggest risks

Effort, eyeballing diffs:

- M1: ~150 LOC fixes + ~250 LOC test = half a day if the merge invariant is straightforward, a full day if the assert at line 535 turns out to require deeper rework of the algorithm.
- M2: <30 LOC + comment work = an hour.
- M3: small fixture CSV + CMake glue = half a day, mostly making the example deterministic (the timing print at line 462 has to be silenced or filtered).
- M4: pure test addition, ~80 LOC = an hour or two.
- M5: doc-only path is half a day; production-wiring path is open-ended (one SQL aggregate end-to-end is typically 2-3 days).
- M6: depends on what's left after M5; budget half a day.

Biggest risks, in order:

- **The merge algorithm may be more broken than just bugs 1-4.** With `cmp == comp_fn(key1, key1) == 0` always, the function has never run its real comparison — there's no evidence the surrounding control flow is correct even with the comparison fixed. The `assert(i == count1)` at line 535 in particular suggests the original author had a different invariant in mind than a generic ordered merge.
- **`keyval_skiplist_common` may have undiscovered issues.** The key/value min-max selection at lines 437-441 uses `keys[0]` and `keys[count-1]` without any guarantee the input is sorted. Validation tests must cover the unsorted-input case or document that the API requires sorted input.
- **The `update` array size bug 7 may pre-exist in the temporal path.** If so, fixing it could perturb existing aggregate tests; needs `git blame` before the M1 PR is opened.
- **`KEYVALUE` is reachable via a stable public header.** Any external MEOS user who started using the API after `1c539aad4` shipped is exposed to the bugs. Grep of bindings (PyMEOS, JMEOS, meos-rs) is out of scope here but should happen before M5.

## 8. Open design questions for the user

1. **Was KEYVALUE intended as a generic per-key aggregator, or specifically as a streaming insert-or-fetch table?** The example exclusively uses the latter pattern (search then splice-on-miss with `count=1`). If the former, M1's batch-merge tests are load-bearing; if the latter, the merge code can arguably be deleted and KEYVALUE simplified to a key-indexed slot allocator.
2. **Should `keys[]` be required to be sorted on entry to `skiplist_splice`?** The implementation appears to assume so (`keyval_skiplist_common` lines 437-441) but nothing enforces or documents it. Tradeoff: sorted-input contract is faster, unsorted-input contract is more ergonomic for streaming clients.
3. **Should `merge_fn` mutate the left operand in place (returning `left`, as the example does at `ais_expand_skiplist.c:117-121`) or return a fresh allocation?** The current `keyval_skiplist_merge` line 516 stores the return into `result` and optionally tracks it via `tofree`, which only works for fresh-alloc semantics. If mutate-in-place is the intended convention, the `tofree` machinery is wrong.
4. **Should production aggregates ever switch to KEYVALUE?** MEOS is the canonical layer; if KEYVALUE stays MEOS-only and is never wired into a SQL aggregate, the MobilityDB serialize/deserialize path stays simple. If it is wired, `mobilitydb/src/temporal/skiplist.c` needs a key-aware serialise format and that is a wire-format change.
5. **Naming.** `SkipListType { TEMPORAL, KEYVALUE }` collides visually with the much-used `TEMPORALTYPE` and adjacent enum constants in `meos/include/temporal/temporal.h:159`. Worth renaming to `SKIPLIST_TEMPORAL` / `SKIPLIST_KEYVALUE` while the surface is still effectively unused.
6. **Should the AIS example move to data-driven smoke testing (M3) or stay an external demo?** The existing CTest sweep precedent suggests there is appetite for the former, but examples have historically been hand-run.

## 9. Status update — 2026-05-03

A bug-audit pass landed three of the seven bugs originally listed in
section 4 onto the in-flight `bug-audit/all` branch. Re-classifying the
seven into "landed" vs "still open" so a future session can pick up
the remaining four:

**Landed on `bug-audit/all`:**

* **Bug 5** — `skiplist_splice` `newkey` not assigned to `newelem->key`.
  Fixed in commit `40718a71b` ("fix(temporal): assign palloc'd newkey
  to newelem->key in skiplist_splice"). The KEYVALUE-branch insertion
  path now stores the freshly-allocated key, eliminating the NULL
  `key_cur` deref on subsequent search.
* **Bug 6** — consequence of bug 5 (`skiplist_search` against NULL
  `key_cur`). Resolved transitively when bug 5 landed.
* **Bug 7** — `memset(update, 0, sizeof(&update))` truncated to
  pointer size. Fixed in `b72935f3d` ("fix(temporal): correct memset
  size on update array in skiplist_common"). The same bug existed in
  the temporal common path and is fixed in the same commit.

Section-4's prediction held: bug 7 was pre-PR-#685 in the temporal
path, so the fix was bundled across both call sites with explicit
`sizeof(int) * SKIPLIST_MAXLEVEL`.

The skiplist-merge return convention was also normalised in
`c59391834` ("fix(temporal): normalise skiplist merge return
convention to fix newvalues leak") — *tofree is now always distinct
from newvalues, the TSEQUENCE branch palloc+memcpy a fresh shell, and
`skiplist_splice` unconditionally `pfree(newvalues)` / `pfree(newkeys)`
after the insertion loop. The `if (tofree)` guard at line 517 of
`keyval_skiplist_merge` is therefore confirmed stale and section 5's
M2 milestone (decide-and-document the merge-result ownership
convention) is now load-bearing for the open-bug fixes.

**Still open (bugs 1-4):**

* **Bug 1** — `cmp = list->comp_fn(key1, key1)` always-zero compare at
  line 512.
* **Bug 2** — `result[count] = keys2[j]` writes a key into the value
  array on the tail loop at line 539.
* **Bug 3** — missing `newkeys1[count] = keys2[j]` write in the tail
  loop at lines 537-541.
* **Bug 4** — missing symmetric `i < count1` tail loop; the
  `assert(i == count1)` at line 535 fires whenever state1 is longer
  than state2 with no overlap.

These four are all in `keyval_skiplist_merge` (`meos/src/temporal/skiplist.c:489-551`)
and are the "design-ambiguous" cluster the audit memory parks. They
need design judgment from the user before the code can be repaired
(see section 8 questions 1, 2, 3).

## 10. Per-bug worked-out repair sketch (bugs 1-4)

Each entry below pins the bug to the current line in master, names the
intended invariant, lists the diff that would repair it under the
"sorted-input + fresh-alloc-merge" convention (the implicit contract
that section 8 questions 2 and 3 currently lean toward), and sketches
the unit test that would exercise it. **None of these diffs should be
applied without the user resolving the section-8 questions first** —
pick a different convention and the diffs change.

### Bug 1 — degenerate compare on the same key

```c
/* meos/src/temporal/skiplist.c:512  (current) */
int cmp = list->comp_fn(key1, key1);

/* Repaired diff (under sorted-input convention) */
int cmp = list->comp_fn(key1, key2);
```

**Intended invariant.** Caller guarantees `keys1` and `keys2` are each
sorted ascending under `comp_fn`. The merge produces a single sorted
output by walking both arrays in parallel and comparing the head of
each. When equal, `merge_fn(val1, val2)` produces the merged value;
when unequal, the smaller key is consumed and emitted as-is.

**Test.** Build a skiplist with `comp_fn = int32_cmp`, `merge_fn =
int64_sum`. Splice with `keys1 = [1, 3, 5]`, `vals1 = [10, 30, 50]`
into a list previously holding `keys2 = [2, 3, 4]`, `vals2 = [20, 30,
40]`. Expected output keys (after merge): `[1, 2, 3, 4, 5]`. Expected
values: `[10, 20, 60, 40, 50]`. The current code produces all-merged
nonsense because `cmp == 0` collapses every pair.

### Bug 2 — wrong array on tail loop

```c
/* meos/src/temporal/skiplist.c:537-541  (current) */
while (j < count2)
{
  result[count] = keys2[j];        /* writes a key into the value array */
  result[count++] = values2[j++];  /* immediately overwrites it */
}

/* Repaired diff (under sorted-input convention; bundles bug 3) */
while (j < count2)
{
  newkeys1[count] = keys2[j];
  result[count++] = values2[j++];
}
```

**Intended invariant.** When state1 is exhausted before state2, copy
the remaining `(key, value)` pairs from state2 into `(newkeys1, result)`
in order, advancing `j` and `count` together.

**Test.** Same harness as bug 1 but with strictly disjoint key ranges
on the long-tail side: `keys1 = [1]`, `vals1 = [10]` against `keys2 =
[2, 3, 4, 5]`, `vals2 = [20, 30, 40, 50]`. Expected output: keys
`[1, 2, 3, 4, 5]`, values `[10, 20, 30, 40, 50]`. The current code
silently drops the keys (overwrite at line 540) and the test would
expose it via mismatched `recovered_keys[i] != expected_keys[i]`.

### Bug 3 — missing key write in tail loop

Subsumed by bug 2's repaired diff above. Listed separately because the
audit memory tracked them as separate findings.

**Test.** The bug-2 test exercises this directly; no separate
fixture needed. Verifying `newkeys1[1..]` are populated is the same
assertion as verifying `recovered_keys[1..] == expected_keys[1..]`.

### Bug 4 — missing symmetric `i < count1` tail loop

```c
/* meos/src/temporal/skiplist.c:533-541  (current) */
while (i < count1 && j < count2) { ... }
/* We finished to aggregate state1 */
assert (i == count1);
/* Copy the values from state2 that are after the end of state1 */
while (j < count2) { ... }

/* Repaired diff */
while (i < count1 && j < count2) { ... }
/* Drain whichever side has remainder */
while (i < count1)
{
  newkeys1[count] = keys1 ? keys1[i] : values1[i];
  result[count++] = values1[i++];
}
while (j < count2)
{
  newkeys1[count] = keys2[j];
  result[count++] = values2[j++];
}
```

**Intended invariant.** A standard merge-of-two-sorted-arrays pattern
finishes by draining whichever input still has elements, not by
asserting one is empty. The current `assert(i == count1)` only holds
if state1 is guaranteed shorter-or-equal-than state2 in every input —
which is a contract the public signature does not document and the
example does not honour (the example does not call this path at all).

**Test.** Mirror of the bug-2 test with the imbalance on the other
side: `keys1 = [1, 2, 3, 4]`, `vals1 = [10, 20, 30, 40]` against
`keys2 = [5]`, `vals2 = [50]`. Expected output: keys `[1, 2, 3, 4, 5]`,
values `[10, 20, 30, 40, 50]`. Current code asserts and aborts.

### Test harness placement

All three bug-1-through-4 tests fit a single new file:
`meos/test/temporal/keyval_skiplist_test.c`. The harness should:

* Use `int32` keys + `int64` values (so the test does not pull in any
  temporal type and is independent of the temporal-aggregate
  machinery).
* Drive `skiplist_splice(list, keys_arr, vals_arr, count, NULL, false,
  KEYVALUE)` with `count > 1` so the multi-element merge path is
  actually entered (the existing AIS example uses `count = 1` and
  bypasses everything).
* Read back via `skiplist_keys_values(list, vals_out)` and assert
  per-key value equals the expected merged sum.
* Compile + run under ASan.

This harness already has section-5's "validation criterion" sketch as
a precedent — the diff above is the concrete realisation.

## 11. Conflict surface with `bug-audit/all`

Bugs 1-4 all live in `keyval_skiplist_merge` (`meos/src/temporal/skiplist.c:489-551`).
Three commits on `bug-audit/all` touch `skiplist.c`:

* `c59391834` modifies the return convention at the top + bottom of
  `keyval_skiplist_merge` (lines 491-495 and 542-549). The repair
  diffs above touch the loop body at lines 512, 533-541. Adjacent but
  non-overlapping; merge is mechanical.
* `40718a71b` modifies `skiplist_splice` (~lines 707-709 in the
  current shape). Different function — no conflict.
* `b72935f3d` modifies the `memset` at line 444 in
  `keyval_skiplist_common`. Different function — no conflict.

So the bug-1-through-4 fixes can land on a stand-alone branch off
master at any time, with the expectation of a trivial three-way merge
when `bug-audit/all` eventually reaches master. **The blocker is not
conflict; it is the user's call on questions 1, 2, 3 of section 8.**

## 12. Recommended next action

1. User answers section-8 questions 1, 2, 3 (intent, sorted-input,
   merge ownership).
2. Author runs M1 from section 6 with the answers in hand: applies
   the section-10 diffs (modulo whatever the answers change), adds
   the harness sketched above, verifies ASan-clean.
3. Branch + push; opening the PR is gated by user signal per
   `feedback_fork_push_review_gating`.

If the user picks "delete the merge code path" (section-8 question 1
favours streaming insert-or-fetch over batch fold), the work
collapses to: remove bugs 1-4 by removing the function, document
KEYVALUE as slot-allocator-only, retain bugs 5-7 fixes already on
`bug-audit/all`. That is the smaller deliverable.

## 13. Production cost-benefit assessment (added 2026-05-03)

This section assesses what it would actually cost to push KEYVALUE all
the way to production-quality, against what production-quality KEYVALUE
would actually return. Saves a future session from re-deriving the
analysis when the question comes up again.

### 13.1 Definition of "production"

* All four open correctness bugs (1-4) fixed *with intended-invariants
  documented* — not just plausible-looking patches.
* Public API in DocBook (new chapter or section in `doc/`) and Doxygen
  group, with a worked example.
* Unit tests covering both streaming and batch-merge paths, ASan-clean.
* Smoke test wired into CI (the AIS example or equivalent).
* Coverage above project threshold without `LCOV_EXCL` masking
  (per `feedback_no_lcov_excl_markers`).
* At least one in-tree consumer that actually exercises the API.
  Otherwise the API is still speculation.
* Bindings exposure decision (PyMEOS, JMEOS, meos-rs, MobilityDuck) —
  at minimum PyMEOS since it is the most-adopted.
* If any production SQL aggregate is to use it: a versioned,
  backward-compatible PG-side wire format in
  `mobilitydb/src/temporal/skiplist.c`. This is a forever-API decision.

### 13.2 Five-tier cost ladder

| Tier | What ships | Estimate |
|---|---|---|
| 0 | Per-bug repair sketches in this plan, no code | ~1 hour (already mostly done in section 10) |
| 1 | Doc comments in source marking `keyval_skiplist_merge` as unvalidated; `meos_internal.h` documents the narrowed contract | ~half day |
| 2 | M2/M3/M4 from section 6 + DocBook section + Doxygen group + PyMEOS wrapper for the streaming path | ~1 week |
| 3 | M1 from section 6 (bugs 1-4 fixed with tests) + M5 production wiring of one SQL aggregate (e.g. `tnumber_tsum_partitioned`) + full bindings exposure (4 bindings × ~1 day each) + PG wire-format design | **~3-5 weeks focused** |
| 4 | Tier 3 + real-workload validation, perf benchmarks, iterations from production findings | open-ended, months |

Tier 3 dominant costs:

1. **Defining intended invariants for batch-merge.** Bugs 1-4 are spec
   gaps as much as code gaps; fixing them requires writing the
   intended-behaviour spec first.
2. **PG wire-format decision.** Any production aggregate using KEYVALUE
   creates a forever-API on the persistence side
   (`mobilitydb/src/temporal/skiplist.c`).
3. **Bindings work.** PyMEOS exposure alone is 1-2 days; doing all
   four multiplies that; each binding may discover its own KEYVALUE
   ergonomic constraints.

### 13.3 Returns analysis

What KEYVALUE *enables* that cannot be done today, paired with the
demand signal observed in the tree:

| Capability | Concrete gain | Demand today |
|---|---|---|
| Per-key streaming aggregation in C without `Temporal *` materialisation | 2-3× throughput on AIS-scale bottleneck paths (estimated, not measured) | None in tree; no binding consumer; no benchmark |
| C-side substrate for streaming pipelines (Flink/Kafka/PG continuous aggregates) | Enables a pattern future projects could build on | MobilityFlink uses JVM keyed state; MobilityKafka is a stub README — no taker |
| Building block for `tnumber_tsum_partitioned`-style aggregates | Enables new SQL surface | Nobody has requested; PG `PARTITION BY` / `GROUP BY` already handles common cases |
| Demonstration of MEOS as streaming-friendly pivot library | Positioning for the H3-style MEOS-as-canonical-library posture | Real but soft; other MEOS surfaces serve this posture too |
| Validates `ais_expand_skiplist.c` as a credible reference example | Better example | Achievable in tier 2 without batch-merge |

What KEYVALUE does *not* enable:

* Anything currently in temporal aggregates (those work fine on the
  temporal path).
* Anything currently exposed to bindings (none use it).
* Anything currently shipped in the SQL surface.

### 13.4 The hard question

**Without a named consumer, even tier 2 is more investment than the
rest of the bug-audit queue would justify.** The bug-audit work has
measured ASan-clean leaks fixed in the audit branch with concrete
diffs and clear acceptance criteria. KEYVALUE production work has the
inverse profile: open-ended scope, speculative consumer, forever-API
decisions, no concrete acceptance criteria except "someone uses it."

Right framing: don't ask "what's the cost of finishing KEYVALUE?" —
ask "**what's the consumer that would justify the cost, and is
building that consumer in the roadmap?**"

### 13.5 Tier-by-tier recommendation

* **Default: tier 1.** Streaming-only as the documented contract,
  batch-merge code marked unvalidated. ~half day. Keeps the door open.
  Honest to readers.
* **If a consumer is named (binding feature, SQL aggregate, AIS-scale
  workload that needs per-key C-side state)**: tier 3 is justified.
  Define the consumer first; the spec for batch-merge falls out of the
  consumer's needs, which solves the "we don't know what bugs 1-4
  *should* do" problem.
* **Tier 2 is a trap.** A week of work for a streaming-only API that
  already works fine for the one in-tree consumer (the AIS example).
  Marginal gain "official"; cost "now we maintain it forever."

### 13.6 Decision criterion for a future session

When the user re-opens this question, the binary the session should
ask is: **is there a roadmap consumer for KEYVALUE batch-merge in the
next 6 months?**

* No → tier 0 or 1; do not invest further.
* Yes, name the consumer → tier 3; consumer's needs define the spec
  for the batch-merge fixes.
* "Maybe / interesting infrastructure to keep" → tier 1; do not let
  this drift into tier 2.

## 14. Decision log — section-8 questions answered 2026-05-03

User worked through all six section-8 design questions in
conversation. Recording the answers + the tier-conditional actions so
a future session does not re-litigate. **Default tier is 1** unless
the user names a roadmap consumer for batch-merge (per section 13.6).

* **Q1 (intent — generic per-key aggregator vs streaming
  insert-or-fetch) — streaming-only is the supported contract.**
  Batch-merge code is unvalidated and is kept marked-as-such, not
  deleted (per the 2026-05-03 standing direction "do not delete
  autonomously"). User explicitly authorised the deletion if a
  consumer is later not named within six months — until then, the
  code stays in tree but the documentation says "do not use."
* **Q2 (sorted-input contract) — sorted-input required, documented
  and asserted in debug builds.** Streaming-only consumers splice with
  `count=1` so the contract has zero cost for the validated path.
  Batch consumers typically have a natural sort key (AIS by GPS time,
  ticks by symbol+time); caller-side sort is rarely a problem in
  practice. Asymmetric cost-of-error: contract violation is on the
  caller, silent wrong results would be on the library forever.
  Tier 1 action: add doc comment to `skiplist_splice` and
  `keyval_skiplist_merge` declaring the contract. Tier 3 action:
  add `#ifdef DEBUG_BUILD` walk verifying sortedness.
* **Q3 (merge_fn ownership — fresh-alloc vs mutate-in-place) —
  mutate-in-place is the contract.** AIS example uses mutate-in-place
  (`trip_merge_fn` returns `left`), and that's the only validated
  consumer. Fresh-alloc semantics defeat the throughput case for
  going to C in the first place (one palloc per merge step kills
  AIS-scale workloads). Tier 1 action: document the contract in
  `meos_internal.h`; mark `tofree`/`nfree` parameters on
  `keyval_skiplist_merge` as unused-and-stale (the partial enum-decl
  comment in `2f132e9cf` already mentions the streaming pattern;
  needs the parameter-level comments too). Tier 3 action: drop the
  `tofree`/`nfree` parameters entirely; rewrite the function body
  for mutate-in-place; update bindings.
* **Q4 (production SQL aggregates switching to KEYVALUE) — firm
  no.** KEYVALUE stays MEOS-only. PG `PARTITION BY` / `GROUP BY`
  already handle the common batch cases; wiring KEYVALUE into a SQL
  aggregate creates a forever wire-format on the persistence side
  with no measured benefit. Cascade rule: if Q1 ever flips to "yes"
  in a future session because a consumer is named, this question
  re-opens *only* for that consumer's specific aggregate. Default
  remains "no for any aggregate not in the named-consumer's spec."
* **Q5 (naming — `SkipListType { TEMPORAL, KEYVALUE }`) — done.**
  Renamed to `SKIPLIST_TEMPORAL` / `SKIPLIST_KEYVALUE` on branch
  `cleanup/skiplisttype-enum-prefix` (`2f132e9cf`, off master). 5
  call sites + the enum decl. Both MEOS-only and PG-side builds
  verified clean. Pushed; awaiting user `gh pr create`.
* **Q6 (AIS example as ctest smoke test — M3) — gated on tier.**
  Stay external at tier 1 (default); promote at tier 2 or higher.
  Reasoning: a CI smoke test creates "now we maintain it forever"
  cost (CI flakiness on unrelated PRs touching `skiplist.c`, fixture
  drift) that's only justified by ongoing development on the
  validated streaming path. At tier 1 there is no such development.
  Tier 1 action: add a one-line comment at the top of
  `ais_expand_skiplist.c` pointing at this plan and explaining the
  supported pattern. Tier 2+ action: wire as M3 sketches —
  CMake target, fixture CSV, expected file, deterministic output.

### 14.1 Tier-1 default action list (summary)

If the user does not name a roadmap consumer in the next 6 months,
the following one-shot tier-1 work closes KEYVALUE for the current
era:

1. Doc comments to `meos_internal.h` near `skiplist_make` declaring
   sorted-input + mutate-in-place + streaming-only as the supported
   contract. Cross-reference this doc.
2. Doc comments to `keyval_skiplist_merge` and `keyval_skiplist_common`
   in `skiplist.c` flagging the function as unvalidated, listing the
   four open bugs by line, pointing at sections 9-10 of this plan.
3. Marker on `tofree`/`nfree` parameters of `keyval_skiplist_merge`
   declaring them unused-and-stale.
4. One-line comment at the top of `meos/examples/ais_expand_skiplist.c`
   pointing readers at this plan and explaining the supported pattern.

Estimated effort: half a day. Branch name suggestion:
`docs/keyval-streaming-contract`. Stand-alone branch off master, same
pattern as the other doc-only branches landed this fork-session.

### 14.2 Tier-3 trigger conditions

If the user later names a roadmap consumer (binding feature, SQL
aggregate, AIS-scale workload that needs per-key C-side state):

1. Re-open Q3, Q4 — the consumer's specific needs may flip the
   merge-ownership convention or the SQL-wiring decision.
2. Define the intended invariants for batch-merge — these are
   driven by the consumer's algorithm, not by inferring them from
   the existing buggy implementation.
3. Apply section-10 repair sketches as a starting point, refining
   per the consumer-defined invariants.
4. Add the M3 smoke test (the AIS example or a new harness).
5. Expose the API to PyMEOS first, then other bindings as needed.
6. Decide PG wire-format if any production aggregate is in scope.

## 15. Historical context — why KEYVALUE was built (added 2026-05-03)

User confirmed in conversation that the KEYVALUE work was driven by
a profiling pass on the AIS pipeline. The supporting evidence in the
tree, walked through end-to-end so a future session can see the
through-line without re-discovering it:

### 15.1 The C-side perf trail (PR #678 → PR #685)

* `meos/examples/ais_expand_full.c` is the production-grade AIS
  ingest: `MAX_NUM_SHIPS = 6500`, processes up to
  `MAX_NUM_RECS = 20000000` records. Per-record per-MMSI lookup is a
  **linear scan over the trips array**:
  ```c
  for (i = 0; i < num_ships; i++)
    if (trips[i].MMSI == rec.MMSI) { j = i; break; }
  ```
  Cost: O(N · M) ≈ 1.3 × 10¹¹ comparisons on a typical day's feed.
  This is the hot path a profiler would flag immediately.
* `meos/examples/ais_expand_skiplist.c` (added in PR #685 alongside
  the KEYVALUE infrastructure itself) replaces the linear scan with
  `skiplist_search` + `skiplist_splice` keyed by MMSI:
  O(N · log M) ≈ 3 × 10⁸ comparisons. Roughly 400× faster on the
  same workload.
* PR sequence: **#678 (Improve skiplist infrastructure) → #685
  (Enable skiplists of key-value pairs) → #696 (Add avgValue
  function)** — all by the user, all in summer 2025. The first two
  PRs have empty bodies; the perf-driven motivation lives in the
  AIS example diff, not in PR text.

### 15.2 The intended consumer family — streaming, not batch

User confirmed in conversation: KEYVALUE's intended consumers are
**streaming systems — Kafka and Flink on the MEOS side, PG
continuous aggregates on the SQL side**. The batch / interactive
binding family (PyMEOS, JMEOS, meos-rs, MobilityDuck) is the wrong
list to look at; KEYVALUE is for the streaming family that is
currently a separate (and underbuilt) ecosystem branch.

| Binding / surface | Family | KEYVALUE relevance | Status today |
|---|---|---|---|
| PyMEOS, JMEOS, meos-rs, MobilityDuck | Batch / interactive / OLAP | None | Active, no KEYVALUE need |
| MobilityKafka | **Streaming** | **Intended consumer** | One-commit README stub |
| MobilityFlink | **Streaming** | **Intended consumer** (currently JVM-side keyed state via JMEOS) | Active, but does not yet route to MEOS C |
| PG continuous aggregates | **Streaming SQL** | **Intended consumer** (`keyedAppendInstant`-like surface) | No SQL aggregate exists in tree |

The previous "no in-tree binding consumer" reading was correct on
the binding-snapshot facts but wrong on the architectural
interpretation. The consumers ARE named at the architectural
level; they are just not yet built.

### 15.3 The hypothetical SQL aggregate

Per user clarification, the SQL-side intent is something shaped
like:

```sql
-- Single-pass per-key fold; presort/GROUP BY would be expensive
-- or impossible in a streaming source. Returns a Map<MMSI, Trip>.
SELECT keyedAppendInstant(MMSI, tgeompoint(geom, t))
FROM ais_records;
```

The aggregate maintains an internal KEYVALUE skiplist keyed by
MMSI, processes records in arrival order, outputs a key-indexed
map at the end. **No such aggregate exists today** in
`mobilitydb/sql/`. There is no draft, no RFC, no branch — KEYVALUE
infrastructure was landed as a stepping-stone, the consuming
aggregate was never written. PR #685's empty body is the absence
of evidence here.

### 15.4 What this means for the open queue

Section 13.5's recommendation — "tier 1 default; tier 3 only with
named consumer" — was structurally correct but mis-interpreted
"named consumer". The right reading is:

* **The consumer family IS named (streaming systems: Kafka, Flink,
  PG continuous aggregates).** The open question is whether and
  when those consumers will be built.
* **If MobilityKafka or MobilityFlink begins active development**,
  KEYVALUE production work (M1: bugs 1-4 fixed with consumer-
  defined invariants) becomes a prerequisite. Tier 3 is the right
  level.
* **If the streaming bindings stay frozen / stub-state**, KEYVALUE
  stays at tier 1 with the architectural intent recorded.
* **If a PG continuous-aggregate consumer materialises**, the SQL
  aggregate design and the KEYVALUE batch-merge invariant
  definition happen together.

### 15.5 ~~Lower-hanging adjacent optimisation~~ — RETRACTED 2026-05-03

The original section 15.5 claimed that `appendInstant` /
`appendSequence` SQL aggregates would benefit from switching
`STYPE = <temporal-type>` to `STYPE = internal`. **That claim was
wrong.** Verified by code inspection in the new-task investigation:

* `Temporal_app_tinst_transfn` (`mobilitydb/src/temporal/temporal_aggfuncs.c:712`)
  calls `set_aggregation_context` before any state allocation, so
  the state lives in the aggregate's persistent memory context (not
  the per-call context).
* `temporal_app_tinst_transfn` (`meos/src/temporal/temporal_aggfuncs.c:1511`)
  calls `temporal_append_tinstant(state, inst, interp, maxdist,
  maxt, true)` — the `expand=true` flag is already set.
* `temporal_append_tinstant(... expand=true)` per its docstring at
  `meos/src/temporal/temporal_modif.c:2099-2107`: "When the temporal
  value passed as first argument has space for adding the instant,
  the function returns the temporal value." Already does in-place
  append when capacity allows.

So the existing aggregate is already O(1) amortized per row.
Switching to `STYPE = internal` would not be a perf win and would
likely add overhead (extra indirection through a SkipList wrapper).
**Do not propose this branch.**

If a real perf concern with `appendInstant` ever surfaces, the
investigation should start by benchmarking the current
implementation to see whether the expandable buffer is actually
being reused across transitions — not by assuming a pessimistic
state-copy story.

### 15.6 Updated decision criterion (supersedes section 13.6)

When the user re-opens the question, ask:

* **Is MobilityKafka / MobilityFlink / a PG continuous-aggregate
  consumer in active development in the next 6-12 months?**
  - Yes → tier 3 work begins; consumer's algorithm defines the
    batch-merge invariants. Section-10 repair sketches are the
    starting point but get refined per the consumer's needs.
  - No, but the streaming intent is preserved → stay at tier 1
    (tier-1 docs already landed on `docs/keyval-streaming-contract`,
    `c97cd82e2`). Do not let it drift to tier 2.
  - "Maybe" → tier 1; revisit when there is a concrete consumer.
