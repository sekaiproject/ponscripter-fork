/* -*- C++ -*-
 * 
 *  AnimationInfo.cpp - General image storage class of ONScripter
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

#include "AnimationInfo.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(BPP16)
#define BPP 16
#define RMASK 0xf800
#define GMASK 0x07e0
#define BMASK 0x001f
#define AMASK 0
#else
#define BPP 32
// the mask is the same as the one used in TTF_RenderGlyph_Blended
#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#define AMASK 0xff000000
#endif
#define RGBMASK 0x00ffffff

AnimationInfo::AnimationInfo()
{
    image_name = NULL;
    image_surface = NULL;
    alpha_buf = NULL;

    duration_list = NULL;
    color_list = NULL;
    file_name = NULL;
    mask_file_name = NULL;

    trans_mode = TRANS_TOPLEFT;

    reset();
}

AnimationInfo::~AnimationInfo()
{
    reset();
}

void AnimationInfo::reset()
{
    remove();

    trans = 256;
    pos.x = pos.y = 0;
    visible = false;
    abs_flag = true;

    font_size_xy[0] = font_size_xy[1] = -1;
    font_pitch = -1;
}

void AnimationInfo::deleteImageName(){
    if ( image_name ) delete[] image_name;
    image_name = NULL;
}

void AnimationInfo::setImageName( const char *name ){
    deleteImageName();
    image_name = new char[ strlen(name) + 1 ];
    strcpy( image_name, name );
}

void AnimationInfo::deleteSurface(){
    if ( image_surface ) SDL_FreeSurface( image_surface );
    image_surface = NULL;
    if (alpha_buf) delete[] alpha_buf;
    alpha_buf = NULL;
}

void AnimationInfo::remove(){
    deleteImageName();
    deleteSurface();
    removeTag();
}

void AnimationInfo::removeTag(){
    if ( duration_list ){
        delete[] duration_list;
        duration_list = NULL;
    }
    if ( color_list ){
        delete[] color_list;
        color_list = NULL;
    }
    if ( file_name ){
        delete[] file_name;
        file_name = NULL;
    }
    if ( mask_file_name ){
        delete[] mask_file_name;
        mask_file_name = NULL;
    }
    current_cell = 0;
    num_of_cells = 0;
    remaining_time = 0;
    is_animatable = false;
    is_single_line = true;
    is_tight_region = true;
    direction = 1;

    color[0] = color[1] = color[2] = 0;
}

// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
bool AnimationInfo::proceedAnimation()
{
    bool is_changed = false;
    
    if ( loop_mode != 3 && num_of_cells > 1 ){
        current_cell += direction;
        is_changed = true;
    }

    if ( current_cell < 0 ){ // loop_mode must be 2
        current_cell = 1;
        direction = 1;
    }
    else if ( current_cell >= num_of_cells ){
        if ( loop_mode == 0 ){
            current_cell = 0;
        }
        else if ( loop_mode == 1 ){
            current_cell = num_of_cells - 1;
            is_changed = false;
        }
        else{
            current_cell = num_of_cells - 2;
            direction = -1;
        }
    }

    remaining_time = duration_list[ current_cell ];

    return is_changed;
}

void AnimationInfo::setCell(int cell)
{
    if (cell < 0) cell = 0;
    else if (cell >= num_of_cells) cell = num_of_cells - 1;

    current_cell = cell;
}

int AnimationInfo::doClipping( SDL_Rect *dst, SDL_Rect *clip, SDL_Rect *clipped )
{
    if ( clipped ) clipped->x = clipped->y = 0;

    if ( !dst ||
         dst->x >= clip->x + clip->w || dst->x + dst->w <= clip->x ||
         dst->y >= clip->y + clip->h || dst->y + dst->h <= clip->y )
        return -1;

    if ( dst->x < clip->x ){
        dst->w -= clip->x - dst->x;
        if ( clipped ) clipped->x = clip->x - dst->x;
        dst->x = clip->x;
    }
    if ( clip->x + clip->w < dst->x + dst->w ){
        dst->w = clip->x + clip->w - dst->x;
    }
    
    if ( dst->y < clip->y ){
        dst->h -= clip->y - dst->y;
        if ( clipped ) clipped->y = clip->y - dst->y;
        dst->y = clip->y;
    }
    if ( clip->y + clip->h < dst->y + dst->h ){
        dst->h = clip->y + clip->h - dst->y;
    }
    if ( clipped ){
        clipped->w = dst->w;
        clipped->h = dst->h;
    }

    return 0;
}

#if defined(BPP16)
#define BLEND_PIXEL(){\
    mask2 = (*alphap++ * alpha) >> 11;\
    Uint32 s1 = (*src_buffer | *src_buffer << 16) & 0x07e0f81f;\
    Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;\
    mask1 = (d1 + ((s1-d1) * mask2 >> 5)) & 0x07e0f81f;\
    *dst_buffer = mask1 | mask1 >> 16;\
}
#else
#define BLEND_PIXEL(){\
    mask2 = (*alphap * alpha) >> 8;\
    mask1 = mask2 ^ 0xff;\
    Uint32 mask_rb = (((*dst_buffer & 0xff00ff) * mask1 +\
                       (*src_buffer & 0xff00ff) * mask2) >> 8) & 0xff00ff;\
    Uint32 mask_g = (((*dst_buffer & 0x00ff00) * mask1 +\
                      (*src_buffer & 0x00ff00) * mask2) >> 8) & 0x00ff00;\
    *dst_buffer = mask_rb | mask_g;\
    alphap += 4;\
}
#endif

void AnimationInfo::blendOnSurface( SDL_Surface *dst_surface, int dst_x, int dst_y,
                                    SDL_Rect &clip, int alpha )
{
    if ( image_surface == NULL ) return;
    
    SDL_Rect dst_rect = {dst_x, dst_y, pos.w, pos.h}, src_rect;
    if ( doClipping( &dst_rect, &clip, &src_rect ) ) return;

    /* ---------------------------------------- */
    
    SDL_LockSurface( dst_surface );
    SDL_LockSurface( image_surface );
    
#if defined(BPP16)
    int total_width = image_surface->pitch / 2;
#else
    int total_width = image_surface->pitch / 4;
#endif    
    ONSBuf *src_buffer = (ONSBuf *)image_surface->pixels + total_width * src_rect.y + image_surface->w*current_cell/num_of_cells + src_rect.x;
    ONSBuf *dst_buffer = (ONSBuf *)dst_surface->pixels   + dst_surface->w * dst_rect.y + dst_rect.x;
#if defined(BPP16)    
    unsigned char *alphap = alpha_buf + image_surface->w * src_rect.y + image_surface->w*current_cell/num_of_cells + src_rect.x;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    unsigned char *alphap = (unsigned char *)src_buffer + 3;
#else
    unsigned char *alphap = (unsigned char *)src_buffer;
#endif
#endif

    Uint32 mask2, mask1;
    
    for (int i=0 ; i<dst_rect.h ; i++){
        for (int j=0 ; j<dst_rect.w ; j++, src_buffer++, dst_buffer++){
            BLEND_PIXEL();
        }
        src_buffer += total_width - dst_rect.w;
#if defined(BPP16)
        alphap += image_surface->w - dst_rect.w;
#else
        alphap += (image_surface->w - dst_rect.w)*4;
#endif        
        dst_buffer += dst_surface->w  - dst_rect.w;
    }

    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( dst_surface );
}

void AnimationInfo::blendOnSurface2( SDL_Surface *dst_surface, int dst_x, int dst_y,
                                     int alpha, int mat[2][2] )
{
    if ( image_surface == NULL ) return;
    
    int i, x, y;
    // calculate Inverse of mat
    int inv_mat[2][2], denom = mat[0][0]*mat[1][1]-mat[0][1]*mat[1][0];
    if (denom == 0) return;
    inv_mat[0][0] =  mat[1][1] * 1000000 / denom;
    inv_mat[0][1] = -mat[0][1] * 1000000 / denom;
    inv_mat[1][0] = -mat[1][0] * 1000000 / denom;
    inv_mat[1][1] =  mat[0][0] * 1000000 / denom;

    // project corner point and calculate bounding box
    int dst_corner_xy[4][2];
    int min_xy[2]={dst_surface->w-1, dst_surface->h-1}, max_xy[2]={0,0};
    for (i=0 ; i<4 ; i++){
        int c_x = (i<2)?(-pos.w/2):(pos.w/2);
        int c_y = ((i+1)&2)?(pos.h/2):(-pos.h/2);
        dst_corner_xy[i][0] = (mat[0][0] * c_x + mat[0][1] * c_y) / 1000 + dst_x;
        dst_corner_xy[i][1] = (mat[1][0] * c_x + mat[1][1] * c_y) / 1000 + dst_y;

        if (min_xy[0] > dst_corner_xy[i][0]) min_xy[0] = dst_corner_xy[i][0];
        if (max_xy[0] < dst_corner_xy[i][0]) max_xy[0] = dst_corner_xy[i][0];
        if (min_xy[1] > dst_corner_xy[i][1]) min_xy[1] = dst_corner_xy[i][1];
        if (max_xy[1] < dst_corner_xy[i][1]) max_xy[1] = dst_corner_xy[i][1];
    }

    // clip bounding box
    if (max_xy[0] < 0) return;
    if (max_xy[0] >= dst_surface->w) max_xy[0] = dst_surface->w - 1;
    if (min_xy[0] >= dst_surface->w) return;
    if (min_xy[0] < 0) min_xy[0] = 0;
    if (max_xy[1] < 0) return;
    if (max_xy[1] >= dst_surface->h) max_xy[1] = dst_surface->h - 1;
    if (min_xy[1] >= dst_surface->h) return;
    if (min_xy[1] < 0) min_xy[1] = 0;

    SDL_LockSurface( dst_surface );
    SDL_LockSurface( image_surface );
    
    Uint32 mask2, mask1;
    
#if defined(BPP16)
    int total_width = image_surface->pitch / 2;
#else
    int total_width = image_surface->pitch / 4;
#endif    
    // set pixel by inverse-projection with raster scan
    for (y=min_xy[1] ; y<= max_xy[1] ; y++){
        // calculate the start and end point for each raster scan
        int raster_min = min_xy[0], raster_max = max_xy[0];
        for (i=0 ; i<4 ; i++){
            if (dst_corner_xy[i][1] == dst_corner_xy[(i+1)%4][1]) continue;
            x = (dst_corner_xy[(i+1)%4][0] - dst_corner_xy[i][0])*(y-dst_corner_xy[i][1])/(dst_corner_xy[(i+1)%4][1] - dst_corner_xy[i][1]) + dst_corner_xy[i][0];
            if (dst_corner_xy[(i+1)%4][1] - dst_corner_xy[i][1] > 0){
                if (raster_min < x) raster_min = x;
            }
            else{
                if (raster_max > x) raster_max = x;
            }
        }

        ONSBuf *dst_buffer = (ONSBuf *)dst_surface->pixels + dst_surface->w * y + raster_min;

        // inverse-projection
        int x_offset = inv_mat[0][1] * (y-dst_y) / 1000 + pos.w/2;
        int y_offset = inv_mat[1][1] * (y-dst_y) / 1000 + pos.h/2;
        for (x=raster_min-dst_x ; x<=raster_max-dst_x ; x++, dst_buffer++){
            int x2 = inv_mat[0][0] * x / 1000 + x_offset;
            int y2 = inv_mat[1][0] * x / 1000 + y_offset;

            if (x2 < 0 || x2 >= pos.w ||
                y2 < 0 || y2 >= pos.h) continue;

            ONSBuf *src_buffer = (ONSBuf *)image_surface->pixels + total_width * y2 + x2 + pos.w*current_cell;
#if defined(BPP16)    
            unsigned char *alphap = alpha_buf + image_surface->w * y2 + x2 + pos.w*current_cell;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            unsigned char *alphap = (unsigned char *)src_buffer + 3;
#else
            unsigned char *alphap = (unsigned char *)src_buffer;
#endif
#endif
            BLEND_PIXEL();
        }
    }
    
    // unlock surface
    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( dst_surface );
}

// used to draw characters on text_surface
// Alpha = 1 - (1-Da)(1-Sa)
// Color = (DaSaSc + Da(1-Sa)Dc + Sa(1-Da)Sc)/A
void AnimationInfo::blendBySurface( SDL_Surface *surface, int dst_x, int dst_y, SDL_Color &color,
                                    SDL_Rect *clip, bool rotate_flag )
{
    if (image_surface == NULL || surface == NULL) return;
    
    SDL_Rect dst_rect = {dst_x, dst_y, surface->w, surface->h};
    if (rotate_flag){
        dst_rect.w = surface->h;
        dst_rect.h = surface->w;
    }
    SDL_Rect src_rect = {0, 0, 0, 0};
    SDL_Rect clipped_rect;

    /* ---------------------------------------- */
    /* 1st clipping */
    if ( clip ){
        if ( doClipping( &dst_rect, clip, &clipped_rect ) ) return;

        src_rect.x += clipped_rect.x;
        src_rect.y += clipped_rect.y;
    }
    
    /* ---------------------------------------- */
    /* 2nd clipping */
    SDL_Rect clip_rect = {0, 0, image_surface->w, image_surface->h};
    if ( doClipping( &dst_rect, &clip_rect, &clipped_rect ) ) return;
    
    src_rect.x += clipped_rect.x;
    src_rect.y += clipped_rect.y;

    /* ---------------------------------------- */
    
    SDL_LockSurface( surface );
    SDL_LockSurface( image_surface );
    
#if defined(BPP16)
    int total_width = image_surface->pitch / 2;
    Uint32 src_color = ((color.r & 0xf8) << 8 |
                        (color.g & 0xfc) << 3 |
                        (color.b & 0xf8) >> 3);
    src_color = (src_color | src_color << 16) & 0x07e0f81f;
#else
    int total_width = image_surface->pitch / 4;
    Uint32 src_color1 = color.r << 16 | color.b;
    Uint32 src_color2 = color.g << 8;
#endif    
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels + total_width * dst_rect.y + image_surface->w*current_cell/num_of_cells + dst_rect.x;
    unsigned char *src_buffer = NULL;
    if (rotate_flag)
        src_buffer = (unsigned char*)surface->pixels + surface->pitch*(surface->h - src_rect.x - 1) + src_rect.y;
    else
        src_buffer = (unsigned char*)surface->pixels + surface->pitch*src_rect.y + src_rect.x;
#if defined(BPP16)
    unsigned char *alphap = alpha_buf + image_surface->w * dst_rect.y + image_surface->w*current_cell/num_of_cells + dst_rect.x;
#endif

    Uint32 mask, mask1, mask2, mask_rb;
    
    for (int i=0 ; i<dst_rect.h ; i++){
        for (int j=0 ; j<dst_rect.w ; j++, dst_buffer++){

            mask2 = *src_buffer;

#if defined(BPP16)
            Uint32 an_1 = *alphap;
            *alphap = 0xff ^ ((0xff ^ an_1)*(0xff ^ mask2) >> 8);
            mask2 = (mask2 << 5) / *alphap;
            
            Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;

            mask_rb = (d1 + ((src_color - d1) * mask2 >> 5)) & 0x07e0f81f;
            *dst_buffer = mask_rb | mask_rb >> 16;
            alphap++;
#else
            Uint32 an_1 = *dst_buffer >> 24;
            Uint32 an = 0xff ^ ((0xff ^ an_1)*(0xff ^ mask2) >> 8);
            mask1 = ((0xff ^ mask2)*an_1)>>8;

            mask_rb =  ((*dst_buffer & 0xff00ff) * mask1 + 
                        src_color1 * mask2);
            mask_rb =  (((mask_rb / an) & 0x00ff0000) | 
                        (((mask_rb & 0xffff) / an) & 0xff));
            mask = (((*dst_buffer & 0x00ff00) * mask1 +
                     src_color2 * mask2) / an) & 0x00ff00;
            *dst_buffer = mask_rb | mask | (an << 24);
#endif            
            if (rotate_flag)
                src_buffer -= surface->pitch;
            else
                src_buffer++;
        }
        dst_buffer += total_width - dst_rect.w;
#if defined(BPP16)
        alphap += image_surface->w - dst_rect.w;
#endif        
        if (rotate_flag)
            src_buffer = (unsigned char*)surface->pixels + surface->pitch*(surface->h - src_rect.x - 1) + src_rect.y + i + 1;
        else
            src_buffer += surface->pitch  - dst_rect.w;
    }

    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( surface );
}

SDL_Surface *AnimationInfo::allocSurface( int w, int h )
{
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, BPP, RMASK, GMASK, BMASK, AMASK);
}

void AnimationInfo::allocImage( int w, int h )
{
    if (!image_surface ||
        image_surface->w != w ||
        image_surface->h != h){
        deleteSurface();

        image_surface = allocSurface( w, h );
#if defined(BPP16)    
        alpha_buf = new unsigned char[w*h];
#endif        
    }

    abs_flag = true;
    pos.w = w / num_of_cells;
    pos.h = h;
}

void AnimationInfo::copySurface( SDL_Surface *surface, SDL_Rect *rect )
{
    if (!image_surface || !surface) return;
    
    SDL_Rect src_rect = {0, 0, surface->w, surface->h};
    if (rect) src_rect = *rect;

    if (src_rect.x >= surface->w) return;
    if (src_rect.y >= surface->h) return;
    
    if (src_rect.x+src_rect.w >= surface->w)
        src_rect.w = surface->w - src_rect.x;
    if (src_rect.y+src_rect.h >= surface->h)
        src_rect.h = surface->h - src_rect.y;
        
    if (src_rect.w > image_surface->w)
        src_rect.w = image_surface->w;
    if (src_rect.h > image_surface->h)
        src_rect.h = image_surface->h;
        
    SDL_LockSurface( surface );
    SDL_LockSurface( image_surface );

    int i;
    for (i=0 ; i<src_rect.h ; i++)
        memcpy( (unsigned char*)image_surface->pixels + image_surface->pitch * i,
                (ONSBuf*)((unsigned char*)surface->pixels + (src_rect.y+i) * surface->pitch) + src_rect.x,
                src_rect.w*sizeof(ONSBuf) );
#if defined(BPP16)
    for (i=0 ; i<src_rect.h ; i++)
        memset( alpha_buf + image_surface->w * i, 0xff, src_rect.w );
#endif

    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( surface );
}

#if defined(BPP16)
#define SET_PIXEL(rgb, alpha) {\
    *buffer_dst++ = (((rgb)&0xf80000) >> 8) | (((rgb)&0xfc00) >> 5) | (((rgb)&0xf8) >> 3);\
    *alphap++ = (alpha);\
}
#else
#define SET_PIXEL(rgb, alpha) {\
    *buffer_dst++ = (rgb);\
    *alphap = (alpha);\
    alphap += 4;\
}
#endif

void AnimationInfo::fill( Uint8 r, Uint8 g, Uint8 b, Uint8 a )
{
    if (!image_surface) return;
    
    SDL_LockSurface( image_surface );
    ONSBuf *buffer_dst = (ONSBuf *)image_surface->pixels;

    Uint32 rgb = (r << 16)|(g << 8)|b;
    unsigned char *alphap = NULL;
#if defined(BPP16)    
    alphap = alpha_buf;
    int dst_margin = image_surface->w % 2;
#else    
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap = (unsigned char *)buffer_dst + 3;
#else
    alphap = (unsigned char *)buffer_dst;
#endif
    int dst_margin = 0;
#endif

    for (int i=0 ; i<image_surface->h ; i++){
        for (int j=0 ; j<image_surface->w ; j++)
            SET_PIXEL(rgb, a);
        buffer_dst += dst_margin;
    }
    SDL_UnlockSurface( image_surface );
}

void AnimationInfo::setupImage( SDL_Surface *surface, SDL_Surface *surface_m )
{
    if (surface == NULL) return;
    SDL_LockSurface( surface );
    Uint32 *buffer = (Uint32 *)surface->pixels;

    int w = surface->w;
    int h = surface->h;
    int w2 = w / num_of_cells;
    if (trans_mode == TRANS_ALPHA)
        w = (w2/2) * num_of_cells;

    allocImage(w, h);
    ONSBuf *buffer_dst = (ONSBuf *)image_surface->pixels;

    unsigned char *alphap = NULL;
#if defined(BPP16)    
    alphap = alpha_buf;
    int dst_margin = w % 2;
#else    
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap = (unsigned char *)buffer_dst + 3;
#else
    alphap = (unsigned char *)buffer_dst;
#endif
    int dst_margin = 0;
#endif

    SDL_PixelFormat *fmt = surface->format;
    Uint32 ref_color=0;
    if ( trans_mode == TRANS_TOPLEFT ){
        ref_color = *buffer;
    }
    else if ( trans_mode == TRANS_TOPRIGHT ){
        ref_color = *(buffer + surface->w - 1);
    }
    else if ( trans_mode == TRANS_DIRECT ) {
        ref_color = direct_color[0] << fmt->Rshift |
            direct_color[1] << fmt->Gshift |
            direct_color[2] << fmt->Bshift;
    }
    ref_color &= RGBMASK;

    int i, j, c;
    if ( trans_mode == TRANS_ALPHA ){
        for (i=0 ; i<h ; i++){
            for (c=0 ; c<num_of_cells ; c++){
                for (j=0 ; j<w2/2 ; j++, buffer++){
                    SET_PIXEL(*buffer, (*(buffer+(w2/2)) & 0xff) ^ 0xff);
                }
                buffer += (w2 - w2/2);
            }
            buffer += surface->w - w2*num_of_cells;
            buffer_dst += dst_margin;
        }
    }
    else if ( trans_mode == TRANS_MASK ){
        if (surface_m){
            SDL_LockSurface( surface_m );
            int mw = surface->w;
            int mh = surface->h;

            for (i=0 ; i<h ; i++){
                Uint32 *buffer_m = (Uint32 *)surface_m->pixels + mw*(i%mh);
                for (c=0 ; c<num_of_cells ; c++){
                    for (j=0 ; j<w2 ; j++, buffer++){
                        if (surface_m){
                            SET_PIXEL(*buffer, (*(buffer_m + j%mw) & 0xff) ^ 0xff);
                        }
                        else{
                            SET_PIXEL(*buffer, 0xff);
                        }
                    }
                }
                buffer_dst += dst_margin;
            }
            SDL_UnlockSurface( surface_m );
        }
    }
    else if ( trans_mode == TRANS_TOPLEFT ||
              trans_mode == TRANS_TOPRIGHT ||
              trans_mode == TRANS_DIRECT ){
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++){
                if ( (*buffer & RGBMASK) == ref_color ){
                    SET_PIXEL(*buffer, 0x00);
                }
                else{
                    SET_PIXEL(*buffer, 0xff);
                }
            }
            buffer_dst += dst_margin;
        }
    }
    else if ( trans_mode == TRANS_STRING ){
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++)
                SET_PIXEL(*buffer, *buffer >> 24);
            buffer_dst += dst_margin;
        }
    }
    else { // TRANS_COPY
        for (i=0 ; i<h ; i++){
            for (j=0 ; j<w ; j++, buffer++)
                SET_PIXEL(*buffer, 0xff);
            buffer_dst += dst_margin;
        }
    }
    
    SDL_UnlockSurface( surface );
}
