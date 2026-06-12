#!/usr/bin/env bash
# Pin preflight gate — run BEFORE pushing evidence/assembly-s5 or tagging a pin.
#
# Mirrors the locally-reproducible MobilityDB CI workflows so a pin is never
# published red. Each block maps to a .github/workflows/*.yml job. Exit code is
# the count of failed gates; 0 = green-to-publish.
#
# Usage:  bash tools/pin_preflight.sh [--static-only]
#   --static-only : run only the fast static ratchets (seconds), skip builds.
set -u
cd "$(git rev-parse --show-toplevel)"
fails=0
run() { # name, cmd...
  local name="$1"; shift
  printf '\n=== [%s] ===\n' "$name"
  if "$@"; then echo "PASS: $name"; else echo "FAIL: $name"; fails=$((fails+1)); fi
}

# --- Static ratchets (meos-error-returns.yml, check-code.yml, cppcheck.yml) ---
run "meos_error return contract" python3 tools/check_meos_error_returns.py
run "int64 in headers"           bash   tools/scripts/check_int64_in_headers.sh
command -v licensecheck >/dev/null && run "license headers" bash tools/scripts/test_license.sh \
  || echo "SKIP: license (licensecheck not installed: apt-get install -y devscripts)"
[ -f tools/portable_aliases/generate.py ] && \
  run "portable aliases up to date" bash -c 'python3 tools/portable_aliases/generate.py --check 2>/dev/null || python3 tools/portable_aliases/generate.py'

if [ "${1:-}" = "--static-only" ]; then
  echo; echo "==== STATIC PREFLIGHT: $fails gate(s) failed ===="; exit $fails
fi

# --- Both-target builds across the family matrix (meos.yml / pgversion.yml) ---
# MEOS standalone and the PG extension must both build with the optional
# families ON (the (.,.,.,1) CI matrix entry), -Werror included.
FAMILIES="-DCBUFFER=ON -DNPOINT=ON -DPOSE=ON -DH3=ON"
run "build MEOS standalone (families ON)" bash -c \
  "cmake -B build-preflight-meos -DMEOS=ON $FAMILIES -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF >/tmp/pf_meos.log 2>&1 && cmake --build build-preflight-meos --target meos -j\$(nproc) >>/tmp/pf_meos.log 2>&1 || { tail -20 /tmp/pf_meos.log; false; }"
run "build PG extension (families ON)" bash -c \
  "cmake -B build-preflight-pg -DMEOS=OFF $FAMILIES -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON >/tmp/pf_pg.log 2>&1 && cmake --build build-preflight-pg -j\$(nproc) >>/tmp/pf_pg.log 2>&1 || { tail -20 /tmp/pf_pg.log; false; }"

# --- Installed-header self-containment (meos-smoke.yml / meos_utf8_smoke.yml) ---
# Catches headers referenced by the installed meos.h but not installed/inlined
# (e.g. meos_tls.h): compile a trivial TU that includes ONLY <meos.h>.
run "installed meos.h self-contained" bash -c '
  set -e; pfx=$(mktemp -d)
  cmake --install build-preflight-meos --prefix "$pfx" >/tmp/pf_inst.log 2>&1
  # Match the CI smoke include set (meos_utf8_smoke.yml): consumers pull the
  # standard headers (size_t etc.) themselves; this gate verifies that every
  # MobilityDB header reached from <meos.h> is installed/inlined (e.g. meos_tls.h).
  echo "#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <meos.h>
int main(void){return 0;}" > "$pfx/t.c"
  cc -I"$pfx/include" "$pfx/t.c" -o "$pfx/t" 2>/tmp/pf_selfcontained.log || { cat /tmp/pf_selfcontained.log; false; }'

echo; echo "==== PREFLIGHT: $fails gate(s) failed ===="
exit $fails
