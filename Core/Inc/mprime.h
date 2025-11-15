#ifndef MPRIME_H
#define MPRIME_H

#include <stdint.h>
#include <stddef.h>

/*============================================================================
 * 128-BIT & 256-BIT TYPES
 *============================================================================*/

/* 128-bit integer using two 64-bit limbs */
typedef struct {
    uint64_t lo;
    uint64_t hi;
} u128_t;

/* 256-bit integer using four 64-bit limbs */
typedef struct {
    uint64_t l0; // bits 0-63
    uint64_t l1; // bits 64-127
    uint64_t l2; // bits 128-191
    uint64_t l3; // bits 192-255
} u256_t;


/*============================================================================
 * PUBLIC LUCAS-LEHMER TEST
 *============================================================================*/

/* * Public LL test for M_127; goes in .ovl_prime 
 * This function is defined in mprime.c
 */
int ll_test_M127(void);


#endif /* MPRIME_H */