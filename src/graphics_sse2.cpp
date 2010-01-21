/* -*- C++ -*-
 * 
 *  graphics_sse2.cpp - graphics routines using X86 SSE2 cpu functionality
 *
 *  Copyright (c) 2009 "Uncle" Mion Sonozaki
 *
 *  UncleMion@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>
 *  or write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Based upon routines provided by Roto

#ifdef USE_X86_GFX

#include <SDL.h>
#include <emmintrin.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "graphics_common.h"


void imageFilterMean_SSE2(unsigned char *src1, unsigned char *src2, unsigned char *dst, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        MEAN_PIXEL();
        --n; ++dst; ++src1; ++src2;
    }

    // Do bulk of processing using SSE2 (find the mean of 16 8-bit unsigned integers, with saturation)
    __m128i mask = _mm_set1_epi8(0x7F);
    while(n >= 16) {
        __m128i s1 = _mm_loadu_si128((__m128i*)src1);
        s1 = _mm_srli_epi16(s1, 1); // shift right 1
        s1 = _mm_and_si128(s1, mask); // apply byte-mask
        __m128i s2 = _mm_loadu_si128((__m128i*)src2);
        s2 = _mm_srli_epi16(s2, 1); // shift right 1
        s2 = _mm_and_si128(s2, mask); // apply byte-mask
        __m128i r = _mm_adds_epu8(s1, s2);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src1 += 16; src2 += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    ++n;
    BASIC_MEAN();
}


void imageFilterAddTo_SSE2(unsigned char *dst, unsigned char *src, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        ADDTO_PIXEL();
        --n; ++dst; ++src;
    }

    // Do bulk of processing using SSE2 (add 16 8-bit unsigned integers, with saturation)
    while(n >= 16) {
        __m128i s = _mm_loadu_si128((__m128i*)src);
        __m128i d = _mm_load_si128((__m128i*)dst);
        __m128i r = _mm_adds_epu8(s, d);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    ++n;
    BASIC_ADDTO();
}


void imageFilterSubFrom_SSE2(unsigned char *dst, unsigned char *src, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        SUBFROM_PIXEL();
        --n; ++dst; ++src;
    }

    // Do bulk of processing using SSE2 (sub 16 8-bit unsigned integers, with saturation)
    while(n >= 16) {
        __m128i s = _mm_loadu_si128((__m128i*)src);
        __m128i d = _mm_load_si128((__m128i*)dst);
        __m128i r = _mm_subs_epu8(d, s);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    ++n;
    BASIC_SUBFROM();
}

void imageFilterBlend_SSE2(Uint32 *dst_buffer, Uint32 *src_buffer, Uint8 *alphap, int alpha, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst_buffer
    while( (((long)dst_buffer & 0xF) > 0) && (n > 0) ) {
        BLEND_PIXEL();
        --n; ++dst_buffer; ++src_buffer;
    }

    // Do bulk of processing using SSE2 (process 4 32bit (BGRA) pixels)
    // create basic bitmasks 0x00FF00FF, 0x000000FF
    __m128i bmask2 = _mm_set1_epi32(0x00FF00FF);
    __m128i bmask = _mm_srli_epi32(bmask2, 16);
    while(n >= 4) {
        // alpha1 = ((src_argb >> 24) * alpha) >> 8
        __m128i a = _mm_set1_epi32(alpha);
        __m128i buf = _mm_loadu_si128((__m128i*)src_buffer);
        __m128i tmp = _mm_srli_epi32(buf, 24);
        a = _mm_mullo_epi16(a, tmp);
        a = _mm_srli_epi32(a, 8);
        // double-up alpha1 (0x000000vv -> 0x00vv00vv)
        tmp = _mm_slli_epi32(a, 16);
        a = _mm_or_si128(a, tmp);
        // rb = (src_argb & bmask2) * alpha1
        tmp = _mm_and_si128(buf, bmask2);
        __m128i rb = _mm_mullo_epi16(a, tmp);
        // g = ((src_argb >> 8) & bmask) * alpha1
        buf = _mm_srli_epi32(buf, 8);
        tmp = _mm_and_si128(buf, bmask);
        __m128i g = _mm_mullo_epi16(a, tmp);
        // alpha2 = alpha1 ^ bmask2
        a = _mm_xor_si128(a, bmask2);
        buf = _mm_load_si128((__m128i*)dst_buffer);
        // rb += (dst_argb & bmask2) * alpha2
        tmp = _mm_and_si128(buf, bmask2);
        tmp = _mm_mullo_epi16(a, tmp);
        rb = _mm_add_epi32(rb, tmp);
        // rb = (rb >> 8) & bmask2
        tmp = _mm_srli_epi32(rb, 8);
        rb = _mm_and_si128(tmp, bmask2);
        // g += ((dst_argb >> 8) & bmask) * alpha2
        buf = _mm_srli_epi32(buf, 8);
        tmp = _mm_and_si128(buf, bmask);
        tmp = _mm_mullo_epi16(a, tmp);
        g = _mm_add_epi32(g, tmp);
        // g = g & (bmask << 8)
        tmp =_mm_slli_epi32(bmask, 8);
        g = _mm_and_si128(g, tmp);
        // dst_argb = rb | g
        tmp = _mm_or_si128(rb, g);
        _mm_store_si128((__m128i*)dst_buffer, tmp);

        n -= 4; src_buffer += 4; dst_buffer += 4; alphap += 16;
    }

    // If any pixels are left over, deal with them individually
    ++n;
    BASIC_BLEND();
}

#endif
