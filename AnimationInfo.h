/* -*- C++ -*-
 *
 *  AnimationInfo.h - General image storage class of Ponscripter
 *
 *  Copyright (c) 2001-2007 Ogapee (original ONScripter, of which this
 *  is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

#ifndef __ANIMATION_INFO_H__
#define __ANIMATION_INFO_H__

#include <SDL.h>
#include <string.h>
#include "defs.h"

class AnimationInfo {
public:
#ifdef BPP16
    typedef Uint16 ONSBuf;
#else
    typedef Uint32 ONSBuf;
#endif
    enum { TRANS_ALPHA    = 1,
           TRANS_TOPLEFT  = 2,
           TRANS_COPY     = 3,
           TRANS_STRING   = 4,
           TRANS_DIRECT   = 5,
           TRANS_PALETTE  = 6,
           TRANS_TOPRIGHT = 7,
           TRANS_MASK     = 8 };

    /* Variables from TaggedInfo */
    int      trans_mode;
    rgb_t    direct_color;
    int      palette_number;
    rgb_t    color;
    SDL_Rect pos; // pos and size of the current cell

    int    num_of_cells;
    int    current_cell;
    int    direction;
    int*   duration_list;
    rgb_t* color_list;
    int    loop_mode;
    bool   is_animatable;
    bool   is_single_line;
    bool   is_tight_region; // valid under TRANS_STRING
    bool   is_centered_text;

    string file_name;
    string mask_file_name;

    /* Variables from AnimationInfo */
    bool   visible;
    bool   abs_flag;
    int    trans;
    string image_name;
    SDL_Surface*   image_surface;
    unsigned char* alpha_buf;

    int scale_x, scale_y, rot; // for lsp2
    int blending_mode; // 0 = normal, 1 = additive
    int cos_i, sin_i;
    
    int font_size_x, font_size_y; // used by prnum and lsp string
    int font_pitch; // used by lsp string
    int remaining_time;

    int param; // used by prnum and bar
    int max_param; // used by bar
    int max_width; // used by bar

    AnimationInfo();
    ~AnimationInfo();
    void reset();

    void setImageName(const char* name) { image_name = name; }
    void setImageName(const string& name) { image_name = name; }    
    void deleteSurface();
    void remove();
    void removeTag();

    bool proceedAnimation();

    void setCell(int cell);
    static int doClipping(SDL_Rect* dst, SDL_Rect* clip,
			  SDL_Rect* clipped = NULL);
    void blendOnSurface(SDL_Surface* dst_surface, int dst_x, int dst_y,
			SDL_Rect &clip, int alpha = 256);
    void blendOnSurface2(SDL_Surface* dst_surface, int dst_x, int dst_y,
			 int alpha, int mat[2][2]);
    void blendBySurface(SDL_Surface* surface, int dst_x, int dst_y,
			SDL_Color &color, SDL_Rect* clip);

    static SDL_Surface* allocSurface(int w, int h);
    void allocImage(int w, int h);
    void copySurface(SDL_Surface* surface, SDL_Rect* rect);
    void fill(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void fill(rgb_t rgb, Uint8 a) { fill(rgb.r, rgb.g, rgb.b, a); }
    void setupImage(SDL_Surface* surface, SDL_Surface* surface_m,
		    bool has_alpha);
};

#endif // __ANIMATION_INFO_H__
