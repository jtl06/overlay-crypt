#include "mprime.h"

// Define the overlay attribute for this file
#define OVL_PRIME __attribute__((section(".ovl_prime")))

/*============================================================================
 * 128-BIT HELPER FUNCTIONS (STATIC)
 *============================================================================*/

static OVL_PRIME u128_t u128_add(u128_t a, u128_t b)
{
    u128_t r;
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi;
    if (r.lo < a.lo) { // carry
        r.hi++;
    }
    return r;
}

static OVL_PRIME u128_t u128_sub(u128_t a, u128_t b)
{
    u128_t r;
    r.lo = a.lo - b.lo;
    r.hi = a.hi - b.hi;
    if (r.lo > a.lo) { // borrow
        r.hi--;
    }
    return r;
}

static OVL_PRIME int u128_cmp(u128_t a, u128_t b)
{
    if (a.hi > b.hi) return 1;
    if (a.hi < b.hi) return -1;
    if (a.lo > b.lo) return 1;
    if (a.lo < b.lo) return -1;
    return 0;
}

static OVL_PRIME u128_t u128_mul64(uint64_t a, uint64_t b)
{
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;

    uint64_t p0 = a_lo * b_lo;
    uint64_t p1 = a_lo * b_hi;
    uint64_t p2 = a_hi * b_lo;
    uint64_t p3 = a_hi * b_hi;

    uint64_t lo = p0;
    uint64_t hi = p3;

    uint64_t t = (p1 << 32);
    lo += t;
    hi += (p1 >> 32);
    if (lo < t) hi++;

    t = (p2 << 32);
    lo += t;
    hi += (p2 >> 32);
    if (lo < t) hi++;

    u128_t r;
    r.lo = lo;
    r.hi = hi;
    return r;
}

/*============================================================================
 * 256-BIT HELPER FUNCTIONS (STATIC)
 *============================================================================*/

static OVL_PRIME u256_t u128_sqr(u128_t s)
{
    uint64_t lo = s.lo;
    uint64_t hi = s.hi;

    u128_t p0 = u128_mul64(lo, lo);
    u128_t p1 = u128_mul64(lo, hi);
    u128_t p3 = u128_mul64(hi, hi);

    u128_t middle = u128_add(p1, p1);

    u256_t r;
    uint64_t c1, c2, c3;

    r.l0 = p0.lo;

    uint64_t t_l1 = p0.hi + middle.lo;
    c1 = (t_l1 < p0.hi);
    r.l1 = t_l1;

    uint64_t t_l2 = p3.lo + middle.hi;
    c2 = (t_l2 < p3.lo);
    r.l2 = t_l2 + c1;
    c3 = (r.l2 < t_l2);

    r.l3 = p3.hi + c2 + c3;

    return r;
}

static OVL_PRIME u256_t u256_sub_u64(u256_t x, uint64_t k)
{
    u256_t r = x;
    r.l0 -= k;
    if (r.l0 > x.l0) { // borrow
        r.l1--;
        if (r.l1 == 0xFFFFFFFFFFFFFFFFULL) {
            r.l2--;
            if (r.l2 == 0xFFFFFFFFFFFFFFFFULL) {
                r.l3--;
            }
        }
    }
    return r;
}

/*============================================================================
 * LUCAS–LEHMER FOR M_127 = 2^127 - 1
 *============================================================================*/

#define MPRIME_P    127u
#define MPRIME_M_LO 0xFFFFFFFFFFFFFFFFULL
#define MPRIME_M_HI 0x7FFFFFFFFFFFFFFFULL
#define P_REM       (MPRIME_P - 64u)  // 63

static OVL_PRIME u128_t ll_step(u128_t s)
{
    const uint64_t M_LO = MPRIME_M_LO;
    const uint64_t M_HI = MPRIME_M_HI;
    const u128_t M = { M_LO, M_HI };

    u256_t x = u128_sqr(s);
    x = u256_sub_u64(x, 2u);

    // x0 = low 127 bits
    u128_t x0;
    x0.lo = x.l0;
    x0.hi = x.l1 & M_HI;

    // x1 = x >> 127
    u128_t x1;
    x1.lo = (x.l1 >> P_REM) | (x.l2 << (64u - P_REM));
    x1.hi = (x.l2 >> P_REM) | (x.l3 << (64u - P_REM));

    u128_t r = u128_add(x0, x1);

    // second fold if r >= 2^127
    // (This is an inlined u128_add(r, {1, 0}) )
    if (r.hi & (1ULL << P_REM)) { // P_REM is 63
        r.hi &= M_HI;
        
        uint64_t old_lo = r.lo;
        r.lo += 1;
        if (r.lo < old_lo) { // carry
            r.hi++;
        }
    }

    if (u128_cmp(r, M) >= 0) {
        r = u128_sub(r, M);
    }

    return r;
}

/* Public LL test for M_127; goes in .ovl_prime */
OVL_PRIME int ll_test_M127(void)
{
    u128_t s = { 4ULL, 0ULL };

    for (unsigned i = 0; i < MPRIME_P - 2u; i++) {
        s = ll_step(s);
    }

    return (s.lo == 0u && s.hi == 0u);
}