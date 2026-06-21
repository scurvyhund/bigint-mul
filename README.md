# bigint-mul — Arbitrary-Precision Big Integer Multiplication in C

Two implementations of **arbitrary-precision big integer multiplication**
in C, each demonstrating a different approach: one with no compiled-in
limit on input size (pure string arithmetic), one computing a full
256-bit product from 128-bit inputs using GCC's `__uint128_t`.

---

## schoolbook — Arbitrary precision, string arithmetic

`schoolbook/big-mul.c` multiplies two arbitrarily large unsigned decimal
integers using the grade-school long multiplication algorithm. There is no
compiled-in upper limit on input size; the only ceiling is available stack
memory (inputs are held in VLAs).

**How it works:** each decimal digit of the multiplier is multiplied against
every digit of the multiplicand, partial products are accumulated column by
column with carry propagation, and the result is printed directly as decimal
digits — no native integer types involved beyond `int8_t` for digit storage.

### Build

```
cd schoolbook
make          # debug build  (-O0, debug symbols)
make release  # release build (-O1)
```

### Usage

```
./big-mul <integer1> <integer2>
```

### Examples

```
$> ./big-mul 12345 67890
   838102050

$> ./big-mul 56011910234567656654300034565 234877590876543211123455430567
   13155942536288445545076126710938271624364921654225057548355

$> ./big-mul 0 99999
   0
```

---

## u128-native — 256-bit result via __uint128_t

`u128-native/dev256.c` multiplies two unsigned integers each up to
(2^128)-1, producing a full 256-bit result. It uses GCC's `__uint128_t`
extension to work in 64-bit limbs, avoiding any string arithmetic in the
multiply itself.

**How it works:** each 128-bit input is split into two 64-bit halves using
type casts and bit shifts (`>> 64` to extract the high half, `(u64)` cast to
isolate the low half). Four 64×64→128 partial products are computed using
`__uint128_t` casts, and carries between limbs are propagated the same way —
bit shifts and casts isolate the high and low 64 bits at each stage. The
final result is assembled into a `u256` struct (lo: 64-bit, mid: 64-bit,
hi: 128-bit).

The decimal conversion works digit by digit: the 256-bit value is
repeatedly divided by 10, with the remainder cascading down through
`hi → mid → lo` on each iteration to extract one decimal digit at a time.
Digits are collected least-significant-first then reversed for printing.

**Input limit:** each factor must be ≤ (2^128)-1 =
`340282366920938463463374607431768211455`. The product can reach (2^256)-1.

### Build

```
cd u128-native
make          # debug build  (-O0, debug symbols)
make release  # release build (-O2)
```

Requires GCC with `__uint128_t` support (any modern x86-64 GCC).

### Usage

```
./dev256 <integer1> <integer2>
```

### Examples

```
$> ./dev256 123456789 987654321
Product: 121932631112635269

	 * Formula to build 256 bit result:
	 *

hihi: 0, hilo: 0, lohi: 0, lolo: 121932631112635269

	 *  -----  high 128 bits  ------      --- low 128 bits ---
	 * (((hihi << 64) + hilo) << 128)  +  (lohi << 64) + lolo
	 *

$> ./dev256 340282366920938463463374607431768211455 340282366920938463463374607431768211455
Product: 115792089237316195423570985008687907852589419931798687112530834793049593217025

	 * Formula to build 256 bit result:
	 *

hihi: 18446744073709551615, hilo: 18446744073709551614, lohi: 0, lolo: 1

	 *  -----  high 128 bits  ------      --- low 128 bits ---
	 * (((hihi << 64) + hilo) << 128)  +  (lohi << 64) + lolo
	 *
```

---

## Comparison

| | schoolbook | u128-native |
|---|---|---|
| Input limit | unlimited (memory) | (2^128)-1 per factor |
| Output limit | unlimited (memory) | (2^256)-1 |
| Algorithm | string long multiplication | 64-bit limb multiply |
| Dependencies | C99 standard library | GCC `__uint128_t` (x86-64) |
| Speed | O(n²) in digit count | O(1) fixed-width |

## Related projects

Big integer arithmetic like this underpins the BigFermat prime search:

- [bi-quad](https://github.com/scurvyhund/bi-quad) — exhaustive hunt for
  bi-quadratic emirps and prime palindromes on the curve 2n²+2n+1 (the
  sum of two consecutive squares); uses GMP and `__uint128_t` for
  arithmetic past the 64-bit wall.
- [bigdec2hex](https://github.com/scurvyhund/bigdec2hex) — arbitrary-
  precision decimal-to-hexadecimal converter; same string-arithmetic
  approach as the schoolbook multiplier here.

---

## Author

Jim Adams
