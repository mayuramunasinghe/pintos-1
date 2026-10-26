#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

/* 17.14 fixed-point format.
   17 bits for integer part and 14 bits for fractional part. */

#define FP_SHIFT 14
#define FP_F     (1 << FP_SHIFT)

/* Integer -> fixed point. */
#define FP_FROM_INT(n) ((n) * FP_F)

/* Fixed point -> integer, rounding toward zero. */
#define FP_TO_INT_ZERO(x) ((x) / FP_F)

/* Fixed point -> integer, nearest. */
#define FP_TO_INT_NEAREST(x) \
        ((x) >= 0 ? ((x) + FP_F / 2) / FP_F \
                  : ((x) - FP_F / 2) / FP_F)

/* Fixed-point addition. */
#define FP_ADD(x, y) ((x) + (y))

/* Fixed-point subtraction. */
#define FP_SUB(x, y) ((x) - (y))

/* Fixed-point + integer. */
#define FP_ADD_INT(x, n) ((x) + (n) * FP_F)

/* Fixed-point - integer. */
#define FP_SUB_INT(x, n) ((x) - (n) * FP_F)

/* Fixed-point * fixed-point. */
#define FP_MUL(x, y) (((int64_t) (x)) * (y) / FP_F)

/* Fixed-point / fixed-point. */
#define FP_DIV(x, y) (((int64_t) (x)) * FP_F / (y))

/* Fixed-point * integer. */
#define FP_MUL_INT(x, n) ((x) * (n))

/* Fixed-point / integer. */
#define FP_DIV_INT(x, n) ((x) / (n))

#endif /* threads/fixed-point.h */