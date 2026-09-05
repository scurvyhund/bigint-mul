/* test_dev256.c - unit tests for mul256b and u256_to_string
 *
 * build with: gcc -std=c99 -Wall -Wextra -m64 -O2 -DTESTING \
 *                 -o test_dev256 test_dev256.c dev256.c
 *
 * usage: ./test_dev256          (no arguments; prints one line per case)
 *
 * -DTESTING is required: it compiles out main() in dev256.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* Declarations from your dev256.c */
typedef unsigned long long u64;
typedef __uint128_t u128;

struct u256 {
   unsigned long long lo;
   unsigned long long mid;
   __uint128_t hi;
};

typedef struct u256 u256;
char* u256_to_string(u256* tmp);
void mul256b(u256 *x, u256 *y, u256 *tmp_struc_ptr);

/* Helper: parse decimal string into u128 (same as your string_to_u128) */
static u128 str_to_u128(const char *s) {
    u128 v = 0;
    for (size_t i = 0; s[i]; ++i) v = v*10 + (s[i]-'0');
    return v;
}

/* Helper: reference product of two u128 as a decimal string.
 *
 * Schoolbook 2x2 limb multiply in base B = 2^64.  With
 * a = a1*B + a0 and b = b1*B + b0:
 *
 *   a*b = a1b1*B^2 + (a0b1 + a1b0)*B + a0b0
 *
 * The three column sums below are t0 / (t1,t2) / t3.  A 128x128
 * product is exactly 256 bits, so four limbs r0..r3 always suffice --
 * there is no fifth limb.
 */
static char *expected_product_from_u128(u128 a, u128 b) {
    /* split into 64-bit limbs */
    u64 a0 = (u64)a;
    u64 a1 = (u64)(a >> 64);
    u64 b0 = (u64)b;
    u64 b1 = (u64)(b >> 64);

    u128 t0 = (u128)a0 * b0;                 // low column
    u128 t1 = (u128)a0 * b1;                 // middle column
    u128 t2 = (u128)a1 * b0;                 // middle column
    u128 t3 = (u128)a1 * b1;                 // high column

    /* limb 0 is just the low half of t0 */
    u64 r0 = (u64)t0;

    /* middle column: high half of t0 plus the low halves of t1 and t2.
     * Bounded by 3*(2^64 - 1), so the carry out is at most 2 and the
     * sum cannot overflow a u128. */
    u128 mid = (t0 >> 64) + (u128)(u64)t1 + (u128)(u64)t2;
    u64 r1 = (u64)mid;

    /* high column: t3 plus the high halves of t1 and t2 plus the carry.
     * Peaks at exactly 2^128 - 1, so this does not overflow either. */
    u128 high = t3 + (t1 >> 64) + (t2 >> 64) + (mid >> 64);
    u64 r2 = (u64)high;
    u64 r3 = (u64)(high >> 64);

    /* limbs little-endian, then decimal by repeated divmod 10 */
    u64 limbs[4] = { r0, r1, r2, r3 };

    char *buf = malloc(80);   /* 2^256 - 1 is 78 digits, plus NUL */
    if (!buf) return NULL;

    int allzero = 1;
    for (int i = 0; i < 4; i++) if (limbs[i]) { allzero = 0; break; }
    if (allzero) { strcpy(buf, "0"); return buf; }

    size_t pos = 0;
    while (1) {
        u128 acc = 0;
        for (int i = 3; i >= 0; --i) {
            acc = (acc << 64) | limbs[i];
            limbs[i] = (u64)(acc / 10);
            acc = acc % 10;
        }
        buf[pos++] = (char)('0' + (int)acc);

        int done = 1;
        for (int i = 0; i < 4; i++) if (limbs[i]) { done = 0; break; }
        if (done) break;
    }
    buf[pos] = '\0';

    /* reverse */
    for (size_t i = 0; i < pos / 2; i++) {
        char t = buf[i]; buf[i] = buf[pos-1-i]; buf[pos-1-i] = t;
    }
    return buf;
}

/* small runner that builds u256 inputs like your main does and calls mul256b */
static char *run_mul_and_get(u128 a, u128 b) {
    u64 a_lo = (u64)a; u64 a_mid = (u64)(a >> 64);
    u64 b_lo = (u64)b; u64 b_mid = (u64)(b >> 64);
    u256 A = { .lo = a_lo, .mid = a_mid, .hi = 0 };
    u256 B = { .lo = b_lo, .mid = b_mid, .hi = 0 };
    u256 out = {0};
    mul256b(&A, &B, &out);
    char *s = u256_to_string(&out);
    return s;
}

/* Test cases */
int main(void) {
    struct { const char *a; const char *b; } tests[] = {
        {"0","0"},
        {"0","1"},
        {"1","1"},
        {"18446744073709551615","18446744073709551615"}, /* 2^64-1 squared */
        {"340282366920938463463374607431768211455","1"}, /* 2^128-1 times 1 */
        {"340282366920938463463374607431768211455", "340282366920938463463374607431768211455"}, /* (2^128-1)^2 */
        {"123456789012345678901234567890","98765432109876543210987654321"},
    };
    int n = sizeof(tests)/sizeof(tests[0]);
    for (int i=0;i<n;i++) {
        u128 a = str_to_u128(tests[i].a);
        u128 b = str_to_u128(tests[i].b);
        char *got = run_mul_and_get(a,b);
        char *want = expected_product_from_u128(a,b);
        if (!got || !want) { printf("malloc failed\n"); return 2; }
        int ok = strcmp(got,want) == 0;
        printf("Test %d: %s * %s => %s : %s\n", i+1, tests[i].a, tests[i].b, got, ok ? "OK":"MISMATCH");
        if (!ok) {
            printf(" Expected: %s\n", want);
        }
        free(got);
        free(want);
    }
    return 0;
}
