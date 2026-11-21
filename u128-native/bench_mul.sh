#!/bin/bash
#
# bench_mul.sh -- compare ~/bin/bigmul against ./dev256 on identical
#                 128-bit factor pairs: speed AND agreement.
#
# Usage:  ./bench_mul.sh [pairs]        default 100
#
# WHAT THIS CAN AND CANNOT MEASURE
#
# Each multiply is a separate process. fork + exec + dynamic linking
# costs on the order of a millisecond; a 128x128 multiply costs
# nanoseconds. So the raw wall time is almost entirely PROCESS
# STARTUP, and a difference between the two programs is far more
# likely to be startup and output volume than arithmetic.
#
# To keep that honest the script measures a baseline -- the same
# number of invocations of /bin/true -- and reports the time with it
# subtracted. Treat even the adjusted figure as an upper bound on
# the arithmetic, not a measurement of it. A real per-multiply
# number needs a loop INSIDE one process.
#
# Correctness is the part this measures well: both products are
# checked against python3's arbitrary-precision integers, which
# share no code with either program.
#
# AND THE MACHINE MUST BE IDLE. The script refuses to print speed
# figures when load exceeds the core count, because a saturated box
# measures contention, not code. (Observed 2026-09-06: hunt at 786%
# of 8 cores; per-run times came out ~6x the idle figure and the
# two programs were indistinguishable inside the noise.)

set -uo pipefail

PAIRS=${1:-100}
BIGMUL=$HOME/bin/bigmul
DEV256=$(dirname "$0")/dev256

for p in "$BIGMUL" "$DEV256"; do
   [ -x "$p" ] || { echo "not executable: $p" >&2; exit 1; }
done

TMP=$(mktemp -d) || exit 1
trap 'rm -rf "$TMP"' EXIT

# ---- generate the factor pairs -------------------------------------
# Fixed seed: the same pairs every run, so timings are comparable and
# a failure is reproducible. Edge cases first (degenerate inputs are
# where multiply bugs live), then random over the full 0..2^128-1.
python3 - "$PAIRS" > "$TMP/pairs.txt" <<'PY'
import random, sys
n = int(sys.argv[1])
MAX = (1 << 128) - 1
edge = [
    (0, 0), (0, MAX), (1, 1), (1, MAX), (MAX, MAX),
    (2, MAX // 2), (1 << 64, 1 << 64), ((1 << 64) - 1, (1 << 64) - 1),
    (1 << 127, 2), (MAX, 1), (0, 1), ((1 << 64), (1 << 63)),
]
random.seed(20260906)
rows = edge[:n]
while len(rows) < n:
    rows.append((random.randint(0, MAX), random.randint(0, MAX)))
for a, b in rows[:n]:
    print(a, b)
PY

echo "pairs: $PAIRS  (first 12 are edge cases, rest random, seed fixed)"

# ---- is the box quiet enough to time anything? ---------------------
CORES=$(nproc)
LOAD=$(awk '{printf "%d", $1}' /proc/loadavg)
BUSY=0
if [ "$LOAD" -ge "$CORES" ]; then
   BUSY=1
   echo
   echo "!!! LOAD $(awk '{print $1}' /proc/loadavg) on $CORES cores --"
   echo "!!! the box is saturated. Correctness below is still valid;"
   echo "!!! SPEED FIGURES ARE MEANINGLESS and are suppressed."
   echo "!!! Re-run when nothing heavy is going (check: pgrep -x hunt)."
fi
echo

# ---- timing helper --------------------------------------------------
# $1 = label, $2... = command template; %A %B substituted per pair
run_timed() {
   local label=$1 out=$2; shift 2
   local t0 t1
   t0=$(date +%s%N)
   while read -r a b; do
      "$@" "$a" "$b"
   done < "$TMP/pairs.txt" > "$out" 2>/dev/null
   t1=$(date +%s%N)
   echo $(( (t1 - t0) / 1000 ))          # microseconds
}

echo "running (3 passes each, best taken)..."
best_bm=""; best_dv=""; best_base=""
for pass in 1 2 3; do
   b=$(run_timed bigmul "$TMP/bm.$pass.out"  "$BIGMUL")
   d=$(run_timed dev256 "$TMP/dv.$pass.out"  "$DEV256")
   n=$(run_timed base   "$TMP/base.out"      /bin/true)
   [ -z "$best_bm"   ] || [ "$b" -lt "$best_bm"   ] && best_bm=$b
   [ -z "$best_dv"   ] || [ "$d" -lt "$best_dv"   ] && best_dv=$d
   [ -z "$best_base" ] || [ "$n" -lt "$best_base" ] && best_base=$n
done

# ---- extract the products ------------------------------------------
# bigmul prints the product as its only non-blank line.
# dev256 prints "Product: <n>" plus explanatory text.
awk 'NF' "$TMP/bm.1.out"                    > "$TMP/bm.vals"
sed -n 's/^Product: *//p' "$TMP/dv.1.out"   > "$TMP/dv.vals"

# ---- correctness against an independent reference -------------------
python3 - "$TMP/pairs.txt" "$TMP/bm.vals" "$TMP/dv.vals" <<'PY'
import sys
pairs = [tuple(map(int, l.split())) for l in open(sys.argv[1])]
bm = [l.strip() for l in open(sys.argv[2]) if l.strip()]
dv = [l.strip() for l in open(sys.argv[3]) if l.strip()]

print()
print("=== correctness (reference: python3 arbitrary precision) ===")
for name, got in (("bigmul", bm), ("dev256", dv)):
    if len(got) != len(pairs):
        print(f"  {name}: WRONG COUNT -- {len(got)} outputs for "
              f"{len(pairs)} pairs")
        continue
    bad = [(i, a, b, g) for i, ((a, b), g) in enumerate(zip(pairs, got))
           if int(g) != a * b]
    if bad:
        print(f"  {name}: {len(bad)} WRONG of {len(pairs)}")
        for i, a, b, g in bad[:3]:
            print(f"    pair {i}: {a} * {b}")
            print(f"      got  {g}")
            print(f"      want {a*b}")
    else:
        print(f"  {name}: all {len(pairs)} correct")

if len(bm) == len(dv) == len(pairs):
    dis = sum(1 for x, y in zip(bm, dv) if x != y)
    print(f"  agreement: {'identical' if dis == 0 else str(dis)+' differ'}")
PY

# ---- speed ----------------------------------------------------------
if [ "$BUSY" -eq 1 ]; then
   echo
   echo "=== speed: SUPPRESSED (box was saturated -- see warning) ==="
   exit 0
fi
python3 - "$best_bm" "$best_dv" "$best_base" "$PAIRS" <<'PY'
import sys
bm, dv, base, n = (int(x) for x in sys.argv[1:5])
print()
print("=== speed (best of 3 passes) ===")
print(f"{'':10} {'total us':>12} {'us/pair':>10} {'minus exec':>12}")
for name, t in (("bigmul", bm), ("dev256", dv), ("/bin/true", base)):
    adj = "" if name == "/bin/true" else f"{(t-base)/n:>12.1f}"
    print(f"{name:10} {t:>12} {t/n:>10.1f} {adj}")
print()
ov = base / n
print(f"process startup is ~{ov:.0f} us/invocation -- "
      f"{100*base/max(bm,dv,1):.0f}% of the slower program's time.")
print("A 128x128 multiply is ~1e-3 us. Neither figure above measures")
print("arithmetic; they measure startup plus output volume.")
PY
