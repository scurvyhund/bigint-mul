#!/usr/bin/env bash
#
# difftest.sh -- differential test of the two implementations.
#
# Multiplies the same random 128-bit pairs with u128-native/dev256 and
# schoolbook/big-mul and compares. They share no code, so agreement is
# real evidence; a mismatch means one of them is wrong.
#
# Usage:  ./difftest.sh [iterations]      (default 10000)
#
# Build both first:  make -C u128-native release && make -C schoolbook release

set -uo pipefail

N=${1:-10000}
DEV=u128-native/dev256
BIG=schoolbook/big-mul

for t in "$DEV" "$BIG"; do
   [ -x "$t" ] || { echo "missing $t -- build it first" >&2; exit 1; }
done

# Generate all operands in ONE python call.  The previous version spawned
# python3 per iteration; at ~26 ms of interpreter startup each that was
# over four minutes of pure process launch for a 10000-pair run.
mapfile -t PAIRS < <(python3 -c "
import random
random.seed()
M = (1 << 128) - 1
for _ in range($N):
    print(random.randint(0, M), random.randint(0, M))
")

pass=0; fail=0
for line in "${PAIRS[@]}"; do
   read -r a b <<< "$line"
   d=$("$DEV" "$a" "$b" 2>/dev/null | awk '/^Product:/{print $2}')
   m=$("$BIG" "$a" "$b" 2>/dev/null | grep -E '^[0-9]+$' | head -1)
   if [ "$d" = "$m" ] && [ -n "$d" ]; then
      pass=$((pass + 1))
   else
      fail=$((fail + 1))
      echo "FAIL: $a x $b"
      echo "  dev256 : $d"
      echo "  big-mul: $m"
   fi
done

echo
echo "=== $N pairs: $pass passed, $fail failed ==="
[ "$fail" -eq 0 ]
