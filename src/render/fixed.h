/* Mibitype - A small library to load fonts.
 * by Mibi88
 *
 * This software is licensed under the BSD-3-Clause license:
 *
 * Copyright 2024 Mibi88
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FIXED_H
#define FIXED_H

#include <stdint.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define FAST 1

#if FAST
#define FIXED_MAX INT32_MAX
#define FIXED_MIN INT32_MIN
#define UFIXED_MAX UINT32_MAX
#define UFIXED_MIN UINT32_MIN
/* The fixed point type (to make the code easier to read). */
typedef int32_t fixed_t;
typedef uint32_t ufixed_t;

/* The precision of the fixed point numbers. */
#define PRECISION 7
#define UPRECISION 16

/* The number of iterations when calculating the square root */
#define SQRT_PRECISION 5
#else
#define FIXED_MAX INT64_MAX
#define FIXED_MIN INT64_MIN
#define UFIXED_MAX UINT64_MAX
#define UFIXED_MIN UINT64_MIN
/* The fixed point type (to make the code easier to read). */
typedef int64_t fixed_t;
typedef uint64_t ufixed_t;

/* The precision of the fixed point numbers. */
#define PRECISION 15
#define UPRECISION 32

/* The number of iterations when calculating the square root */
#define SQRT_PRECISION 10
#endif

/* SQRT lookup table settings. */
#define SQRT_LUT_MAX 2

#define SQRT_LUT_BIG_MAX 200

#define DIV_LUT_MAX 300

extern fixed_t _lut_div[DIV_LUT_MAX];
extern ufixed_t _lut_udiv[DIV_LUT_MAX];

/* Convert a float to a fixed point number. */
#define TO_FIXED(num) (fixed_t)((num)*(fixed_t)(1<<PRECISION))
/* Convert a fixed point number to an integer. */
#define TO_INT(num) ((num)>>PRECISION)
/* Multiply two fixed point numbers together. */
#define MUL(a, b) (((a)*(b))>>PRECISION)

/* Divide the fixed point number a by the fixed point number b. */
#define DIV(a, b) (((a)<<PRECISION)/(b))

/* Fixed point floor function. */
#define FLOOR(num) (num&~((1<<PRECISION)-1))

/* Division using the lookup table. */
#define FDIV(a, b) ((ABS(b) < TO_FIXED(DIV_LUT_MAX-1) && \
                     ABS(b) >= TO_FIXED(1)) ? \
                    /* Use the lookup table */ \
                    MUL(MUL(_lut_div[TO_INT(ABS(b))+1], \
                            ABS(b)-FLOOR(ABS(b)))+ \
                        MUL(_lut_div[TO_INT(ABS(b))], \
                            TO_FIXED(1)-(ABS(b)-FLOOR(ABS(b)))), a)* \
                    (b < 0 ? -1 : 1) : \
                    /* b is too big, calculate the division. */ \
                    DIV(a, b))

/* Unsigned math */
/* Convert a float to a fixed point number. */
#define UTO_FIXED(num) (ufixed_t)((num)*(ufixed_t)((ufixed_t)1<<UPRECISION))
/* Convert a fixed point number to an integer. */
#define UTO_INT(num) ((num)>>UPRECISION)
/* Multiply two fixed point numbers together. */
#define UMUL(a, b) (((a)*(b))>>UPRECISION)

/* Divide the fixed point number a by the fixed point number b. */
#define UDIV(a, b) (((a)<<UPRECISION)/(b))

/* Fixed point floor function. */
#define UFLOOR(num) (num&~((1<<UPRECISION)-1))

/* Division using the lookup table. */
#define UFDIV(a, b) (((b) < UTO_FIXED(DIV_LUT_MAX-1) && (b) >= UTO_FIXED(1)) ? \
                     /* Use the lookup table */ \
                     UMUL(UMUL(_lut_udiv[UTO_INT(b)+1], (b)-UFLOOR(b))+ \
                          UMUL(_lut_udiv[UTO_INT(b)], \
                               UTO_FIXED(1)-((b)-UFLOOR(b))), a) : \
                     /* b is too big, calculate the division. */ \
                     UMUL(a, UTO_FIXED(1)/UTO_INT(b)))
/*****************/

/* See
 * https://en.wikipedia.org/wiki/Bh%C4%81skara_I%27s_sine_approximation_formula.
 */
fixed_t dsin(fixed_t org_d);

fixed_t dcos(fixed_t d);

fixed_t dtan(fixed_t d);

/* Using Heron's method with a binary initial estimate. See
 * https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
 */
fixed_t fsqrt(fixed_t n);

/* Math functions using lookup tables. linit initalizes the lookup tables. */
void linit(void);

fixed_t lsqrt(fixed_t x);

fixed_t ldcos(fixed_t x);

fixed_t ldsin(fixed_t x);

fixed_t ldtan(fixed_t x);

/* The default sqrt function. */
#define SQRT lsqrt
/* The default sine function for degrees. */
#define DSIN ldsin
/* The default cosine function for degrees. */
#define DCOS ldcos
/* The default tangent function for degrees. */
#define DTAN ldtan

#endif
