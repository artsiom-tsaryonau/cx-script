#!/usr/bin/env bash
# Regression tests that need only a C compiler: lock release on exec, error line numbers.
set -euo pipefail
CX=$(cd "$(dirname "$0")/.." && pwd)/cx
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
export CX_CACHE="$T/cache"
cd "$T"

# 1. Editing a script while a previous run is still alive must not report "locked".
cat >a.c <<'C'
#include <stdio.h>
#include <unistd.h>
int main(void){ puts("v1"); fflush(stdout); sleep(3); return 0; }
C
"$CX" a.c >/dev/null & pid=$!
sleep 1
sed -i 's/v1/v2/' a.c
out=$("$CX" a.c 2>&1 | head -1) && [[ "$out" == v2 ]] || { echo "FAIL lock: $out"; exit 1; }
wait "$pid"

# 2. Compiler errors must point at the original line (shebang line is blanked, not removed).
printf '#!/usr/bin/env cx\nint main(void){\n  broken_here;\n}\n' >b.c
err=$("$CX" b.c 2>&1 || true)
grep -q 'script.c:3' <<<"$err" || { echo "FAIL line numbers:"; echo "$err"; exit 1; }
echo "regress ok"
