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

#include <fixed.h>
#include <stdio.h>

/* See
 * https://en.wikipedia.org/wiki/Bh%C4%81skara_I%27s_sine_approximation_formula.
 */
fixed_t dsin(fixed_t org_d) {
    fixed_t v;
    fixed_t d;
    int flip = 1;
    while(org_d < TO_FIXED(0)) org_d += TO_FIXED(360);
    while(org_d > TO_FIXED(360)) org_d -= TO_FIXED(360);
    d = org_d;
    if(d > TO_FIXED(180)){
        d -= TO_FIXED(180);
        flip = -1;
    }
    v = DIV(MUL(4*d, (TO_FIXED(180)-d)),
            (TO_FIXED(40500)-MUL(d, (TO_FIXED(180)-d))));
    return v*flip;
}

fixed_t dcos(fixed_t d) {
    return dsin(d+TO_FIXED(90));
}

fixed_t dtan(fixed_t d) {
    return DIV(dsin(d), dcos(d));
}

#define HALF (1<<PRECISION>>1)

fixed_t fsqrt(fixed_t x) {
    int i = 0;
    fixed_t n = 1;
    fixed_t a = x;
    fixed_t rx;
    fixed_t lx;
    while(n < x && i < 64){
        n <<= 1;
        a >>= 1;
        i++;
    }
    lx = MUL((HALF+MUL(HALF, a)), n);
    if(!lx){
        lx = 1;
    }
    for(i=0;i<SQRT_PRECISION;i++){
        rx = MUL(HALF, (lx+DIV(x, lx)));
        lx = rx;
        if(!lx) lx = 1;
    }
    return lx;
}

fixed_t _lut_sqrt[TO_FIXED(SQRT_LUT_MAX)];
fixed_t _lut_sqrt_big[SQRT_LUT_BIG_MAX];
fixed_t _lut_sin[360];
fixed_t _lut_div[DIV_LUT_MAX];
ufixed_t _lut_udiv[DIV_LUT_MAX];

void linit(void) {
    fixed_t i;
    /* Initialize the sqrt LUT (for small numbers) */
    for(i=0;i<TO_FIXED(SQRT_LUT_MAX);i++){
        _lut_sqrt[i] = fsqrt(i);
    }
    /* Initialize the sqrt LUT (for big numbers) */
    for(i=0;i<SQRT_LUT_BIG_MAX;i++){
        _lut_sqrt_big[i] = fsqrt(TO_FIXED(i));
    }
    /* Initialize the sine LUT */
    for(i=0;i<360;i++){
        _lut_sin[i] = dsin(TO_FIXED(i));
    }
    /* Initialize the division LUTs. */
    _lut_div[0] = FIXED_MAX;
    for(i=1;i<DIV_LUT_MAX;i++){
        _lut_div[i] = TO_FIXED(1)/i;
    }
    _lut_udiv[0] = UFIXED_MAX;
    for(i=1;i<DIV_LUT_MAX;i++){
        _lut_udiv[i] = UTO_FIXED(1)/i;
    }
}

fixed_t lsqrt(fixed_t x) {
    int intx;
    if(x < 0) return 0;
    if(x < TO_FIXED(SQRT_LUT_MAX)){
        return _lut_sqrt[x];
    }else if(x < TO_FIXED(SQRT_LUT_BIG_MAX-1)){
        intx = TO_INT(x);
        return MUL(_lut_sqrt_big[intx+1], x-FLOOR(x))+
               MUL(_lut_sqrt_big[intx], TO_FIXED(1)-(x-FLOOR(x)));
    }
    return fsqrt(x);
}

fixed_t ldcos(fixed_t x) {
    return ldsin(x+TO_FIXED(90));
}

fixed_t ldsin(fixed_t x) {
    int x1 = TO_INT(x);
    int x2;
    fixed_t v1;
    fixed_t v2;
    while(x1 >= 360) x1 -= 360;
    while(x1 < 0) x1 += 360;
    x2 = x1+1;
    if(x2 >= 360) x2 = 0;
    v1 = _lut_sin[x1];
    v2 = _lut_sin[x2];
    return MUL(v2, x-FLOOR(x))+MUL(v1, TO_FIXED(1)-(x-FLOOR(x)));
}

fixed_t ldtan(fixed_t x) {
    return DIV(ldsin(x), ldcos(x));
}
