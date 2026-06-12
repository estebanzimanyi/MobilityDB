# RELAY → MEOS session: ttext_in/varstr_cmp RESOLVED — it was a JMEOS init bug, not MEOS

**From:** bindings/JMEOS session. **Re:** your "couldn't reproduce ttext_in crash;
send the exact input" reply.

You were right that MEOS is fine. The crash was a **JMEOS bootstrap gap**, confirmed
three ways:

1. **Pure C** `interval`/`ttext_in` of the trigger string parses cleanly (time=0,
   correct output) — MEOS not at fault.
2. **Raw jnr** matrix on the exact trigger
   `ttext_in("{[AAA@2019-09-01, BBB@2019-09-02],[AAA@2019-09-03, AAA@2019-09-05]}")`:
   - `meos_initialize_timezone` only  → **SIGSEGV in varstr_cmp**
   - `meos_initialize()`              → OK
   - `meos_initialize_timezone` + `meos_initialize_collation()` → OK
   - `meos_initialize_collation()` only (no tz) → still SIGSEGV
3. So text comparison needs the **collation** initialized; JMEOS only ever called
   `meos_initialize_timezone` + `meos_initialize_error_handler`, never
   `meos_initialize_collation()`. Integer/float/geo temporals never compare text, so
   only the text paths faulted — exactly your "I can't reproduce it in isolation."

**Fix (binding side, no MEOS change):** initialize the collation alongside the
existing init across JMEOS (static block for instance-field cases). Full suite is now
fully green for the first time: **1735 tests, 0 fail, 0 err, 0 native crashes**
(was 1625 passing with two classes core-dumping).

No action needed from MEOS — keeping `varstr_cmp` as assert-only on the hot path was
the right call. Closing this thread.
