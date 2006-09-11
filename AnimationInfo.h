/* -*- C++ -*-
 * 
 *  AnimationInfo.h - General image storage class of ONScripter
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ANIMATION_INFO_H__
#define __ANIMATION_INFO_H__

#include <SDL.h>
#include <string.h>

typedef unsigned char uchar3[3];

class AnimationInfo{
public:
#if defined(BPP16)
    typedef Uint16 ONSBuf;
#else
    typedef Uint32 ONSBuf;
#endif    
    enum { TRANS_ALPHA          = 1,
           TRANS_TOPLEFT        = 2,
           TRANS_COPY           = 3,
           TRANS_STRING         = 4,
           TRANS_DIRECT         = 5,
           TRANS_PALLET         = 6,
           TRANS_TOPRIGHT       = 7,
           TRANS_MASK           = 8
    };

    /* Variables from TaggedInfo */
    int trans_mode;
    uchar3 direct_color;
    int pallet_number;
    uchar3 color;
    SDL_Rect pos; // pose and size of the current cell

    int num_of_cells;
    int current_cell;
    int direction;
    int *duration_list;
    uchar3 *color_list;
    int loop_mode;
    bool is_animatable;
    bool is_single_line;
    bool is_tight_region; // valid under TRANS_STRING
        
    char *file_name;
    char *mask_file_name;

    /* Variables from AnimationInfo */
    bool visible;
    bool abs_flag;
    int trans;
    char *image_name;
    SDL_Surface *image_surface;
    unsigned char *alpha_buf;

    int font_size_xy[2]; // used by prnum and lsp string
    int font_pitch; // used by lsp string
    int remaining_time;

    int param; // used by prnum and bar
    int max_param; // used by bar
    int max_width; // used by bar
    
    AnimationInfo();
    ~AnimationInfo();
    void reset();
    
    void deleteImageName();
    void setImageName( const char *name );
    void deleteSurface();
    void remove();
    void removeTag();

    bool proceedAnimation();

    void setCell(int cell);
    static int doClipping( SDL_Rect *dst, SDL_Rect *clip, SDL_Rect *clipped=NULL );
    void blendOnSurface( SDL_Surface *dst_surface, int dst_x, int dst_y,
                         SDL_Rect &clip, int alpha=256 );
    void blendOnSurface2( SDL_Surface *dst_surface, int dst_x, int dst_y,
                          int alpha, int mat[2][2] );
    void blendBySurface( SDL_Surface *surface, int dst_x, int dst_y, SDL_Color &color,
                         SDL_Rect *clip, bool rotate_flag );
    
    static SDL_Surface *allocSurface( int w, int h );
    void allocImage( int w, int h );
    void copySurface( SDL_Surface *surface, SDL_Rect *rect );
    void fill( Uint8 r, Uint8 g, Uint8 b, Uint8 a );
    void setupImage( SDL_Surface *surface, SDL_Surface *surface_m );
};

#endif // __ANIMATION_INFO_H__
