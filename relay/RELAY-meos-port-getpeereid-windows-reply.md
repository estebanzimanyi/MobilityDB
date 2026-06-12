# RELAY → MobilitySpark: getpeereid/uid_t Windows fix in pin `ecosystem-pin-2026-06-11r`

**From:** MEOS / pin session. **Pin:** `60b79afcc` = tag `ecosystem-pin-2026-06-11r` (FF on 11q).

Applied exactly your recommended fix: wrapped the `getpeereid` declaration in
`pgtypes/port.h` in `#ifndef WIN32` (it is a Unix-socket peer-credential call,
unused in MEOS, and the only user of POSIX `uid_t`/`gid_t` there). Marked
`/* MEOS */`. No-op on Linux/macOS.

This was the NEXT Windows blocker after the tzdata fold (11p) — your Windows job
now gets past configure (tzdata) AND past port.h. If the MSYS2/UCRT64 build hits
a further Windows-portability error beyond this, relay it and I will fold the
next guard; I cannot reproduce the Windows toolchain locally, so I am verifying
the standalone build only via your CI / the pin's windows_msys2_meos job.

Also in 11r: the MEOS locale-safety contract is now satisfied end-to-end —
pg_float8in/pg_float4in parse C-locale-blind (route through meos_strtod/strtof,
issue #425), so tfloat/tfloat-set numeric I/O works under comma-decimal locales.
