/* -*- C++ -*-
 * 
 *  graphics_mmx.h - graphics macros common to graphics_* & AnimationInfo
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

// Incorporates code originally from AnimationInfo.cpp,
// copyright (c) 2001-2008 Ogapee
#ifdef BPP16
#define BPP 16
#define BLENDMASK  0x07e0f81f
#define BLENDSHIFT 5
#define RMASK 0xf800
#define GMASK 0x07e0
#define BMASK 0x001f
#define AMASK 0
#define RSHIFT 11
#define GSHIFT 5
#define BSHIFT 0
#define ASHIFT 0
#define RLOSS  3
#define GLOSS  2
#define BLOSS  3
#define ALOSS  0

#else

#define BPP 32
// the mask is the same as the one used in TTF_RenderGlyph_Blended
#define RBMASK 0x00ff00ff
#define GBMASK 0x0000ffff
#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#define AMASK 0xff000000
#define RSHIFT 16
#define GSHIFT 8
#define BSHIFT 0
#define ASHIFT 24
#define RLOSS  0
#define GLOSS  0
#define BLOSS  0
#define ALOSS  0

#endif

#define RGBMASK 0x00ffffff
#define RBMASK  0x00ff00ff
#define MEDGRAY 0x88888888

#define SET_PIXEL32(rgb, alpha) {\
    *dst_buffer = (rgb);\
    *alphap = (alpha);\
    alphap += 4;\
}

#ifdef BPP16

/* Used in AnimationInfo */

#define SET_PIXEL(rgb, alpha) {\
    *dst_buffer = (rgb);\
    *alphap++ = (alpha);\
}

#define SET_PIXEL32TO16(rgb, alpha) {\
    *img_buffer = (((rgb)&0xf80000) >> 8) | (((rgb)&0xfc00) >> 5) | (((rgb)&0xf8) >> 3);\
    *alphap++ = (alpha);\
}

#define BLEND_PIXEL(){\
    if ((*alphap == 255) && (alpha == 256)) {\
        *dst_buffer = *src_buffer;\
    } else if (*alphap != 0) {\
        Uint32 mask2 = (*alphap * alpha) >> 11;\
        Uint32 s1 = (*src_buffer | *src_buffer << 16) & BLENDMASK;\
        Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & BLENDMASK;\
        Uint32 mask1 = (d1 + ((s1-d1) * mask2 >> BLENDSHIFT)) & BLENDMASK;\
        *dst_buffer = mask1 | mask1 >> 16;\
    }\
    alphap++;\
}

#define BLEND_PIXEL8_ALPHA(){\
    Uint32 mask2 = *src_buffer; \
    if (mask2 != 0){ \
        *alphap = 0xff ^ ((0xff ^ *alphap)*(0xff ^ *src_buffer) >> 8); \
        mask2 = (mask2 << 5) / *alphap; \
        Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & BLENDMASK; \
        Uint32 mask = (d1 + ((src_color-d1) * mask2 >> BLENDSHIFT)) & BLENDMASK; \
        *dst_buffer = mask | mask >> 16; \
    } \
    alphap++; \
}

/* Used in ONScripterLabel_image */

#define BLEND_PIXEL8(){\
    Uint32 mask2 = *src_buffer >> 3; \
    if (mask2 != 0){ \
        Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & BLENDMASK; \
        Uint32 mask = (d1 + ((src_color-d1) * mask2 >> BLENDSHIFT)) & BLENDMASK; \
        *dst_buffer = mask | mask >> 16; \
    } \
}

#define BLEND_MASK_PIXEL(){\
    Uint32 s1 = (*src1_buffer | *src1_buffer << 16) & BLENDMASK; \
    Uint32 s2 = (*src2_buffer | *src2_buffer << 16) & BLENDMASK; \
    Uint32 mask = (s1 + ((s2-s1) * mask2 >> BLENDSHIFT)) & BLENDMASK; \
    *dst_buffer = mask | mask >> 16; \
}

// monocro 85/86/85 uses 85=(4+1)*(16+1)
#define MONOCRO_PIXEL(){\
    const ONSBuf tmp = ((*buffer & GMASK) >> GSHIFT) << GLOSS; \
    ONSBuf c = (((*buffer & RMASK) >> RSHIFT) << RLOSS) + \
                ((*buffer & BMASK) << BLOSS) + tmp; \
    c += c<<2; \
    c += (c<<4) + tmp; \
    c >>= 8; \
    *buffer = ((monocro_color_lut[c].r >> RLOSS) << RSHIFT) | \
              ((monocro_color_lut[c].g >> GLOSS) << GSHIFT) | \
              (monocro_color_lut[c].b >> BLOSS); \
}

#else //ndef BPP16

/* Used in AnimationInfo */

#define SET_PIXEL(rgb, alpha) {\
    *dst_buffer = (rgb);\
    *alphap = (alpha);\
    alphap += 4;\
}

#define BLEND_PIXEL(){\
    if ((*alphap == 255) && (alpha == 256)) {\
        *dst_buffer = *src_buffer;\
    } else if (*alphap != 0) {\
        Uint32 mask2 = (*alphap * alpha) >> 8;\
        Uint32 mask1 = mask2 ^ 0xff;\
        Uint32 mask_rb = (((*dst_buffer & RBMASK) * mask1 +\
                           (*src_buffer & RBMASK) * mask2) >> 8) & RBMASK;\
        Uint32 mask_g = (((*dst_buffer & GMASK) * mask1 +\
                          (*src_buffer & GMASK) * mask2) >> 8) & GMASK;\
        *dst_buffer = mask_rb | mask_g;\
    }\
    alphap += 4;\
}

#define BLEND_PIXEL8_ALPHA(){\
    Uint32 mask2 = *src_buffer; \
    if (mask2 != 0){ \
        Uint32 alpha = *dst_buffer >> ASHIFT; \
        Uint32 mask1 = ((0xff ^ mask2) * alpha) >> 8; \
        alpha = 0xff ^ ((0xff ^ alpha)*(0xff ^ mask2) >> 8); \
        Uint32 mask_rb = (*dst_buffer & RBMASK) * mask1 + \
                         src_color1 * mask2; \
        mask_rb = ((mask_rb / alpha) & RMASK) | \
                  (((mask_rb & GBMASK) / alpha) & BMASK); \
        Uint32 mask_g = (((*dst_buffer & GMASK) * mask1 + \
                          src_color2 * mask2) / alpha) & GMASK; \
        *dst_buffer = mask_rb | mask_g | (alpha << ASHIFT); \
    } \
}

/* Used in ONScripterLabel_image */

#define BLEND_PIXEL8(){\
    Uint32 mask2 = *src_buffer; \
    if (mask2 != 0){ \
        Uint32 mask1 = mask2 ^ 0xff; \
        Uint32 mask_rb = (((*dst_buffer & RBMASK) * mask1 + \
                            src_color1 * mask2) >> 8) & RBMASK; \
        Uint32 mask_g  = (((*dst_buffer & GMASK) * mask1 + \
                            src_color2 * mask2) >> 8) & GMASK; \
        *dst_buffer = mask_rb | mask_g; \
    } \
}

#define BLEND_MASK_PIXEL(){\
    Uint32 mask_rb = (((*src1_buffer & RBMASK) * mask1 + \
                       (*src2_buffer & RBMASK) * mask2) >> 8) & RBMASK;\
    Uint32 mask_g  = (((*src1_buffer & GMASK) * mask1 +\
                       (*src2_buffer & GMASK) * mask2) >> 8) & GMASK;\
    *dst_buffer = mask_rb | mask_g;\
}

#define ADDBLEND_PIXEL(){\
    Uint32 mask2 = (*alphap * alpha) >> 8;\
    Uint32 mask_rb = (*dst_buffer & RBMASK) +\
                     ((((*src_buffer & RBMASK) * mask2) >> 8) & RBMASK);\
    mask_rb |= ((mask_rb & AMASK) ? RMASK : 0) |\
               ((mask_rb & GMASK) ? BMASK : 0);\
    Uint32 mask_g = (*dst_buffer & GMASK) +\
                    ((((*src_buffer & GMASK) * mask2) >> 8) & GMASK);\
    mask_g |= ((mask_g & RMASK) ? GMASK : 0);\
    *dst_buffer = (mask_rb & RBMASK) | (mask_g & GMASK);\
    alphap += 4;\
}

#define SUBBLEND_PIXEL(){\
    Uint32 mask2 = (*alphap * alpha) >> 8;\
    Uint32 mask_r = (*dst_buffer & RMASK) -\
                    ((((*src_buffer & RMASK) * mask2) >> 8) & RMASK);\
    mask_r &= ((mask_r & AMASK) ? 0 : RMASK);\
    Uint32 mask_g = (*dst_buffer & GMASK) -\
                    ((((*src_buffer & GMASK) * mask2) >> 8) & GMASK);\
    mask_g &= ((mask_g & ~(GMASK | BMASK)) ? 0 : GMASK);\
    Uint32 mask_b = (*dst_buffer & BMASK) -\
                    ((((*src_buffer & BMASK) * mask2) >> 8) & BMASK);\
    mask_b &= ((mask_b & ~BMASK) ? 0 : BMASK);\
    *dst_buffer = (mask_r & RMASK) | (mask_g & GMASK) | (mask_b & BMASK);\
    alphap += 4;\
}

// monocro 85/86/85 uses 85=(4+1)*(16+1)
#define MONOCRO_PIXEL(){\
    const ONSBuf tmp = ((*buffer & GMASK) >> GSHIFT); \
    ONSBuf c = ((*buffer & RMASK) >> RSHIFT) + (*buffer & BMASK) + tmp; \
    c += c<<2; \
    c += (c<<4) + tmp; \
    c >>= 8; \
    *buffer = (monocro_color_lut[c].r << RSHIFT) | \
              (monocro_color_lut[c].g << GSHIFT) | \
              monocro_color_lut[c].b; \
}

#endif //ndef BPP16


#define BASIC_BLEND(){\
    while(--n > 0) {  \
        BLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}

#define BASIC_ADDBLEND(){\
    while(--n > 0) {  \
        ADDBLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}

#define BASIC_SUBBLEND(){\
    while(--n > 0) {  \
        SUBBLEND_PIXEL();  \
        ++dst_buffer, ++src_buffer;  \
    } \
}


#define MEAN_PIXEL(){\
    int result = ((int)(*src1) + (int)(*src2)) / 2;  \
    (*dst) = result; \
}

#define BASIC_MEAN(){\
    while (--n > 0) {  \
        MEAN_PIXEL();  \
        ++dst; ++src1; ++src2;  \
    }  \
}

#define ADDTO_PIXEL(){\
    int result = (*dst) + (*src);  \
    (*dst) = (result < 255) ? result : 255; \
}

#define BASIC_ADDTO(){\
    while (--n > 0) {  \
        ADDTO_PIXEL();  \
        ++dst, ++src;  \
    }  \
}

#define SUBFROM_PIXEL(){\
    int result = (*dst) - (*src);  \
    (*dst) = (result > 0) ? result : 0;  \
}

#define BASIC_SUBFROM(){\
    while(--n > 0) {  \
        SUBFROM_PIXEL();  \
        ++dst, ++src;  \
    } \
}

