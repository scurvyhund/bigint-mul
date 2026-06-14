# bigint-mul

Two implementations of large integer multiplication in C, each demonstrating
a different approach to the same problem.

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
$ ./big-mul 12345 67890
   838102050

$ ./big-mul 56011910234567656654300034565 234877590876543211123455430567
   13155942536288445545076126710938271624364921654225057548355

$ ./big-mul 0 99999
   0
```

---

## u128-native — 256-bit result via __uint128_t

`u128-native/dev256.c` multiplies two unsigned integers each up to
(2^128)-1, producing a full 256-bit result. It uses GCC's `__uint128_t`
extension to work in 64-bit limbs, avoiding any string arithmetic in the
multiply itself.

**How it works:** each 128-bit input is split into two 64-bit halves. Four
64×64→128 products are computed and their partial results are combined with
explicit carry propagation into a `u256` struct (lo: 64-bit, mid: 64-bit,
hi: 128-bit). The result is then converted to a decimal string for output.

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
$ ./dev256 123456789 987654321
Product: 121932631112635269

$ ./dev256 340282366920938463463374607431768211455 340282366920938463463374607431768211455
Product: 115792089237316195423570985008687907852589419931798687112530834793049593217025
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

## Author

Jim Adams
