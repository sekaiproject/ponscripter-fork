/* -*- C++ -*-
 *
 *  AnimationInfo.cpp - General image storage class of Ponscripter
 *
 *  Copyright (c) 2001-2008 Ogapee (original ONScripter, of which this
 *  is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  Copyright (c) 2009 "Uncle" Mion Sonozaki
 *
 *  UncleMion@gmail.com
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>
 *  or write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "AnimationInfo.h"
#include "BaseReader.h"

#include "graphics_common.h"

#if defined(USE_X86_GFX)
#include "graphics_mmx.h"
#include "graphics_sse2.h"
#endif

#if defined(USE_PPC_GFX)
#include "graphics_altivec.h"
#endif

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//Mion: for special graphics routine handling
static unsigned int cpufuncs;


AnimationInfo::AnimationInfo()
{
    is_copy = false;

    image_surface = NULL;
#ifdef BPP16
    alpha_buf     = NULL;
#endif
    trans_mode    = TRANS_TOPLEFT;
    affine_flag   = false;
    locked        = 0;
    reset();
}

AnimationInfo::AnimationInfo(const AnimationInfo &anim)
{
    //copy constructor
    //deepcopy(anim);
    memcpy(this, &anim, sizeof(AnimationInfo));
    is_copy = true;
    printf("animinfo '%s': made a copy (constr)\n", (const char*)anim.image_name);
    fflush(stdout);
}


AnimationInfo::~AnimationInfo()
{
    reset();
}


AnimationInfo& AnimationInfo::operator =(const AnimationInfo &anim)
{
    if (this != &anim){
        memcpy(this, &anim, sizeof(AnimationInfo));
        is_copy = true;
    }
    return *this;
}


void AnimationInfo::deepcopy(const AnimationInfo &anim)
{
    if (this != &anim){
        reset();
        //copy the whole object
        memcpy(this, &anim, sizeof(AnimationInfo));
        if (anim.is_copy){
            return;
        }
        //unset the image_surface due to danger of accidental deletion
        image_surface = NULL;
#ifdef BPP16
        alpha_buf = NULL;
#endif
        //now set dynamic variables
        //duration_list = anim.duration_list;
        //color_list = anim.color_list;
        image_name = anim.image_name;
        file_name = anim.file_name;
        mask_file_name = anim.mask_file_name;

        if (anim.image_surface) {
            int w = anim.image_surface->w, h = anim.image_surface->h;
            allocImage( w, h );
            copySurface(anim.image_surface, NULL);
#ifdef BPP16
            memcpy(alpha_buf, anim.alpha_buf, w*h);
#endif
        }
    }
    locked = 0;
}


#ifdef _WIN32
#include <windows.h>
#define msleep Sleep
#else
#define msleep(x) usleep(x * 1000)
#endif

void AnimationInfo::reset()
{
    // This is stupid and ugly, but it seems to work well enough.
    // See lame excuses in header.
    int prevent_deadlock = 0;
    if (locked)
        fprintf(stderr, "Resetting an AnimationInfo that's still in use! Don't worry, I noticed in time.\n");
    while (locked) {
        msleep(1);
        if (++prevent_deadlock > 2000) {
            fprintf(stderr, "AnimationInfo is deadlocked, expect trouble\n");
            locked = 0;
            break;
        }
    }
    
    remove();

    trans = 256;
    orig_pos.x = orig_pos.y = 0;
    pos.x = pos.y = 0;
    abs_flag = true;
    showing_ = false;
    visible_ = false;
    enabled_ = true;
    enablemode = 0;

    scale_x = scale_y = rot = 0;
    blending_mode = BLEND_NORMAL;

    font_size_x = font_size_y = -1;
    font_pitch  = -1;

    mat[0][0] = 1000;
    mat[0][1] = 0;
    mat[1][0] = 0;
    mat[1][1] = 1000;

#ifndef NO_LAYER_EFFECTS
    layer_no = -1;
#endif
}

void AnimationInfo::deleteImage()
{
    if (!is_copy && image_surface) SDL_FreeSurface(image_surface);
    image_surface = NULL;
#ifdef BPP16
    if (!is_copy && alpha_buf) delete[] alpha_buf;
    alpha_buf = NULL;
#endif
}


void AnimationInfo::remove()
{
    image_name = "";
    deleteImage();
    removeTag();
}


void AnimationInfo::removeTag()
{
    //duration_list.clear();
    //color_list.clear();
    
    file_name        = "";
    mask_file_name   = "";
    current_cell     = 0;
    num_of_cells     = 0;
    remaining_time   = 0;
    is_animatable    = false;
    is_single_line   = true;
    is_tight_region  = true;
    is_ruby_drawable = false;
    skip_whitespace = true;
    is_centered_text = false;
    direction = 1;

    color.set(0);
}


// 0 ... restart at the end
// 1 ... stop at the end
// 2 ... reverse at the end
// 3 ... no animation
bool AnimationInfo::proceedAnimation()
{
    bool is_changed = false;

    if (loop_mode != 3 && num_of_cells > 1) {
        current_cell += direction;
        is_changed = true;
    }

    if (current_cell < 0) { // loop_mode must be 2
        current_cell = 1;
        direction = 1;
    }
    else if (current_cell >= num_of_cells) {
        if (loop_mode == 0) {
            current_cell = 0;
        }
        else if (loop_mode == 1) {
            current_cell = num_of_cells - 1;
            is_changed = false;
        }
        else {
            current_cell = num_of_cells - 2;
            direction = -1;
        }
    }

    remaining_time = duration_list[current_cell];

    return is_changed;
}


void AnimationInfo::setCell(int cell)
{
    if (cell < 0) cell = 0;
    else if (cell >= num_of_cells) cell = num_of_cells - 1;

    current_cell = cell;
}


int AnimationInfo::doClipping(SDL_Rect* dst, SDL_Rect* clip, SDL_Rect* clipped)
{
    if (clipped) clipped->x = clipped->y = 0;

    if (!dst
        || dst->x >= clip->x + clip->w || dst->x + dst->w <= clip->x
        || dst->y >= clip->y + clip->h || dst->y + dst->h <= clip->y)
        return -1;

    if (dst->x < clip->x) {
        dst->w -= clip->x - dst->x;
        if (clipped) clipped->x = clip->x - dst->x;

        dst->x = clip->x;
    }

    if (clip->x + clip->w < dst->x + dst->w) {
        dst->w = clip->x + clip->w - dst->x;
    }

    if (dst->y < clip->y) {
        dst->h -= clip->y - dst->y;
        if (clipped) clipped->y = clip->y - dst->y;

        dst->y = clip->y;
    }

    if (clip->y + clip->h < dst->y + dst->h) {
        dst->h = clip->y + clip->h - dst->y;
    }

    if (clipped) {
        clipped->w = dst->w;
        clipped->h = dst->h;
    }

    return 0;
}


SDL_Rect AnimationInfo::findOpaquePoint(SDL_Rect *clip)
//find the first opaque-enough pixel position for transbtn
{
    int cell_width = image_surface->w/num_of_cells;
    SDL_Rect cliprect = {0, 0, cell_width, image_surface->h};
    if (clip) cliprect = *clip;

#ifdef BPP16
    const int psize = 1;
    unsigned char *alphap = alpha_buf;
#else
    const int psize = 4;
    unsigned char *alphap = (unsigned char *)image_surface->pixels;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap += 3;
#endif
#endif

    SDL_Rect ret = {0, 0, 0, 0};

    for (int i=cliprect.y ; i<cliprect.h ; ++i){
        for (int j=cliprect.x ; j<cliprect.w ; ++j){
            int alpha = *(alphap + (image_surface->w * i + j) * psize);
            if (alpha > TRANSBTN_CUTOFF){
                ret.x = j;
                ret.y = i;
                //want to break out of the for loops
                i = cliprect.h;
                break;
            }
        }
    }
    //want to find a pixel that's opaque across all cells, if possible
    int xstart = ret.x;
    for (int i=ret.y ; i<cliprect.h ; ++i){
        for (int j=xstart ; j<cliprect.w ; ++j){
            bool is_opaque = true;
            for (int k=0 ; k<num_of_cells ; ++k){
                int alpha = *(alphap +
                              ((image_surface->w * i + cell_width * k + j) *
                               psize));
                if (alpha <= TRANSBTN_CUTOFF){
                    is_opaque = false;
                    break;
                }
            }
            if (is_opaque){
                ret.x = j;
                ret.y = i;
                //want to break out of the for loops
                i = cliprect.h;
                break;
            }
            xstart = cliprect.x;
        }
    }

    return ret;
}


int AnimationInfo::getPixelAlpha(int x, int y)
{
#ifdef BPP16
    unsigned char *alphap = alpha_buf + image_surface->w * y + x +
                            image_surface->w*current_cell/num_of_cells;
#else
    const int psize = 4;
    const int total_width = image_surface->w * psize;
    unsigned char *alphap = (unsigned char *)image_surface->pixels +
                            total_width * current_cell/num_of_cells +
                            total_width * y + x * psize;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    alphap += 3;
#endif
#endif
    return (int) *alphap;
}


void AnimationInfo::blendOnSurface(SDL_Surface* dst_surface, int dst_x,
                                   int dst_y, SDL_Rect &clip, int alpha)
{
    if (!image_surface || !dst_surface) return;

    SDL_Rect dst_rect = { dst_x, dst_y, pos.w, pos.h }, src_rect;
    if (doClipping(&dst_rect, &clip, &src_rect)) return;

    /* ---------------------------------------- */

    ++locked;

    SDL_LockSurface(dst_surface);
    SDL_LockSurface(image_surface);

#ifdef BPP16
    const int total_width = image_surface->pitch / 2;
#else
    const int total_width = image_surface->pitch / 4;
#endif
    ONSBuf* src_buffer = (ONSBuf*) image_surface->pixels +
                         total_width * src_rect.y +
                         image_surface->w * current_cell / num_of_cells +
                         src_rect.x;
    ONSBuf* dst_buffer = (ONSBuf*) dst_surface->pixels +
                         dst_surface->w * dst_rect.y + dst_rect.x;
#ifdef BPP16
    unsigned char* alphap = alpha_buf + image_surface->w * src_rect.y +
                            image_surface->w * current_cell / num_of_cells +
                            src_rect.x;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    unsigned char* alphap = (unsigned char*) src_buffer + 3;
#else
    unsigned char* alphap = (unsigned char*) src_buffer;
#endif
#endif

#ifndef BPP16
    if (blending_mode == BLEND_NORMAL) {
#endif
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            ONSBuf* srcmax = (ONSBuf*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                    SET_PIXEL(*src_buffer, 0xff);
                }
                src_buffer += total_width - dst_rect.w;
#ifdef BPP16
                alphap += image_surface->w - dst_rect.w;
#else
                alphap += (image_surface->w - dst_rect.w)*4;
#endif
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        } else if (alpha != 0) {
            ONSBuf* srcmax = (ONSBuf*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
#ifdef BPP16
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                    BLEND_PIXEL();
                }
                src_buffer += total_width - dst_rect.w;
                dst_buffer += dst_surface->w - dst_rect.w;
                alphap += image_surface->w - dst_rect.w;
#else
                if (src_buffer >= srcmax) goto break2;
                imageFilterBlend(dst_buffer, src_buffer, alphap, alpha,
                                 dst_rect.w);
                src_buffer += total_width;
                dst_buffer += dst_surface->w;
                alphap += (image_surface->w)*4;
#endif
            }
        }
#ifndef BPP16
    } else if (blending_mode == BLEND_ADD) {
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            // "add" the src pix value to the dst
            Uint8* srcmax = (Uint8*) ((Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h);
            Uint8* src_buf = (Uint8*) src_buffer;
            Uint8* dst_buf = (Uint8*) dst_buffer;

            for (int i=dst_rect.h ; i ; --i){
                if (src_buf >= srcmax) goto break2;
                imageFilterAddTo(dst_buf, src_buf, dst_rect.w*4);
                src_buf += total_width * 4;
                dst_buf += dst_surface->w * 4;
            }
        } else if (alpha != 0) {
            // gotta do additive alpha blending
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                        ADDBLEND_PIXEL();
                    }
                src_buffer += total_width - dst_rect.w;
                alphap += (image_surface->w - dst_rect.w)*4;
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        }
    } else if (blending_mode == BLEND_SUB) {
        if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
            // "subtract" the src pix value from the dst
            Uint8* srcmax = (Uint8*) ((Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h);
            Uint8* src_buf = (Uint8*) src_buffer;
            Uint8* dst_buf = (Uint8*) dst_buffer;

            for (int i=dst_rect.h ; i ; --i){
                if (src_buf >= srcmax) goto break2;
                imageFilterSubFrom(dst_buf, src_buf, dst_rect.w*4);
                src_buf += total_width * 4;
                dst_buf += dst_surface->w * 4;
            }
        } else if (alpha != 0) {
            // gotta do subtractive alpha blending
            Uint32* srcmax = (Uint32*)image_surface->pixels +
                image_surface->w * image_surface->h;

            for (int i=dst_rect.h ; i ; --i){
                for (int j=dst_rect.w ; j ; --j, src_buffer++, dst_buffer++){
                    // If we've run out of source area, ignore the remainder.
                    if (src_buffer >= srcmax) goto break2;
                        SUBBLEND_PIXEL();
                    }
                src_buffer += total_width - dst_rect.w;
                alphap += (image_surface->w - dst_rect.w)*4;
                dst_buffer += dst_surface->w  - dst_rect.w;
            }
        }
    }
#endif
break2:
    SDL_UnlockSurface( image_surface );
    SDL_UnlockSurface( dst_surface );

    --locked;
}


void AnimationInfo::blendOnSurface2(SDL_Surface* dst_surface, int dst_x,
                                    int dst_y, SDL_Rect &clip, int alpha)
{
    if (image_surface == NULL) return;
    if (scale_x == 0 || scale_y == 0) return;

    int i, x, y;

    // project corner point and calculate bounding box
    int min_xy[2] = { bounding_rect.x, bounding_rect.y };
    int max_xy[2] = { bounding_rect.x + bounding_rect.w - 1,
                      bounding_rect.y + bounding_rect.h - 1 };

    // clip bounding box
    if (max_xy[0] < clip.x) return;
    if (max_xy[0] >= clip.x + clip.w) max_xy[0] = clip.x + clip.w - 1;
    if (min_xy[0] >= clip.x + clip.w) return;
    if (min_xy[0] < clip.x) min_xy[0] = clip.x;
    if (max_xy[1] < clip.y) return;
    if (max_xy[1] >= clip.y + clip.h) max_xy[1] = clip.y + clip.h - 1;
    if (min_xy[1] >= clip.y + clip.h) return;
    if (min_xy[1] < clip.y) min_xy[1] = clip.y;

    SDL_LockSurface(dst_surface);
    SDL_LockSurface(image_surface);

#ifdef BPP16
    int total_width = image_surface->pitch / 2;
#else
    int total_width = image_surface->pitch / 4;
#endif
    // set pixel by inverse-projection with raster scan
    for (y = min_xy[1]; y <= max_xy[1]; y++) {
        // calculate the start and end point for each raster scan
        int raster_min = min_xy[0], raster_max = max_xy[0];
        for (i = 0; i < 4; i++) {
            if (corner_xy[i][1] == corner_xy[(i + 1) % 4][1])
                continue;
            x = ((corner_xy[(i + 1) % 4][0] - corner_xy[i][0]) *
                 (y - corner_xy[i][1]) /
                 (corner_xy[(i + 1) % 4][1] - corner_xy[i][1])) +
                corner_xy[i][0];
            if (corner_xy[(i + 1) % 4][1] - corner_xy[i][1] > 0) {
                if (raster_min < x) raster_min = x;
            }
            else {
                if (raster_max > x) raster_max = x;
            }
        }

        ONSBuf* dst_buffer = (ONSBuf*) dst_surface->pixels + dst_surface->w * y + raster_min;

        // inverse-projection
        int x_offset = inv_mat[0][1] * (y - dst_y) / 1000 + pos.w / 2;
        int y_offset = inv_mat[1][1] * (y - dst_y) / 1000 + pos.h / 2;
        for (x = raster_min - dst_x; x <= raster_max - dst_x; x++, dst_buffer++) {
            int x2 = inv_mat[0][0] * x / 1000 + x_offset;
            int y2 = inv_mat[1][0] * x / 1000 + y_offset;

            if (x2 < 0 || x2 >= pos.w
                || y2 < 0 || y2 >= pos.h) continue;

            ONSBuf* src_buffer = (ONSBuf*) image_surface->pixels + total_width * y2 + x2 + pos.w * current_cell;
#ifdef BPP16
            unsigned char* alphap = alpha_buf + image_surface->w * y2 + x2 + pos.w * current_cell;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            unsigned char* alphap = (unsigned char*) src_buffer + 3;
#else
            unsigned char* alphap = (unsigned char*) src_buffer;
#endif
#endif
#ifndef BPP16
            if (blending_mode == BLEND_NORMAL) {
#endif
                if ((trans_mode == TRANS_COPY) && (alpha == 256)) {
                    SET_PIXEL(*src_buffer, 0xff);
                } else {
                    BLEND_PIXEL();
                }
#ifndef BPP16
            } else if (blending_mode == BLEND_ADD) {
                ADDBLEND_PIXEL();
            } else if (blending_mode == BLEND_SUB) {
                SUBBLEND_PIXEL();
            }
#endif
        }
    }

    // unlock surface
    SDL_UnlockSurface(image_surface);
    SDL_UnlockSurface(dst_surface);
}


// used to draw characters on text_surface
// Alpha = 1 - (1-Da)(1-Sa)
// Color = (DaSaSc + Da(1-Sa)Dc + Sa(1-Da)Sc)/A
void AnimationInfo::blendText( SDL_Surface *surface, int dst_x, int dst_y,
                               SDL_Color &color, SDL_Rect *clip,
                               bool rotate_flag )
{
    if (image_surface == NULL || surface == NULL) return;

    SDL_Rect dst_rect = { dst_x, dst_y, surface->w, surface->h };
    if (rotate_flag){
        dst_rect.w = surface->h;
        dst_rect.h = surface->w;
    }
    SDL_Rect src_rect = { 0, 0, 0, 0 };
    SDL_Rect clipped_rect;

    /* ---------------------------------------- */
    /* 1st clipping */
    if (clip) {
        if (doClipping(&dst_rect, clip, &clipped_rect)) return;

        src_rect.x += clipped_rect.x;
        src_rect.y += clipped_rect.y;
    }

    /* ---------------------------------------- */
    /* 2nd clipping */
    SDL_Rect clip_rect = { 0, 0, image_surface->w, image_surface->h };
    if (doClipping(&dst_rect, &clip_rect, &clipped_rect)) return;

    src_rect.x += clipped_rect.x;
    src_rect.y += clipped_rect.y;

    /* ---------------------------------------- */

    SDL_LockSurface(surface);
    SDL_LockSurface(image_surface);

#ifdef BPP16
    int total_width = image_surface->pitch / 2;
    Uint32 src_color = ((color.r >> RLOSS) << RSHIFT) |
                       ((color.g >> GLOSS) << GSHIFT) |
                       (color.b >> BLOSS);
    src_color = (src_color | src_color << 16) & BLENDMASK;
#else
    int total_width = image_surface->pitch / 4;
    Uint32 src_color1 = color.r << RSHIFT | color.b;
    Uint32 src_color2 = color.g << GSHIFT;
#endif
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels +
                         total_width * dst_rect.y +
                         image_surface->w*current_cell/num_of_cells +
                         dst_rect.x;
#ifdef BPP16
    unsigned char *alphap = alpha_buf + image_surface->w * dst_rect.y +
                            image_surface->w*current_cell/num_of_cells +
                            dst_rect.x;
#endif
    if (!rotate_flag){
        unsigned char *src_buffer = (unsigned char*)surface->pixels +
                                    surface->pitch*src_rect.y + src_rect.x;
        for (int i=dst_rect.h; i>0; i--){
            for (int j=dst_rect.w; j>0; j--, dst_buffer++, src_buffer++){
                BLEND_PIXEL8_ALPHA();
            }
            dst_buffer += total_width - dst_rect.w;
#ifdef BPP16
            alphap += image_surface->w - dst_rect.w;
#endif
            src_buffer += surface->pitch - dst_rect.w;
        }
    }
    else{
        for (int i=0; i<dst_rect.h; i++){
            unsigned char *src_buffer = (unsigned char*)surface->pixels +
                                        surface->pitch*(surface->h -
                                                        src_rect.x - 1) +
                                        src_rect.y;
            for (int j=dst_rect.w; j>0; j--, dst_buffer++){
                BLEND_PIXEL8_ALPHA();
                src_buffer -= surface->pitch;
            }
            dst_buffer += total_width - dst_rect.w;
#ifdef BPP16
            alphap += image_surface->w - dst_rect.w;
#endif
        }
    }

    SDL_UnlockSurface(image_surface);
    SDL_UnlockSurface(surface);
}


void AnimationInfo::calcAffineMatrix()
{
    // calculate forward matrix
    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int cos_i = 1000, sin_i = 0;
    if (rot != 0){
        cos_i = (int)(1000.0 * cos(-M_PI * rot / 180));
        sin_i = (int)(1000.0 * sin(-M_PI * rot / 180));
    }
    mat[0][0] =  cos_i * scale_x / 100;
    mat[0][1] = -sin_i * scale_y / 100;
    mat[1][0] =  sin_i * scale_x / 100;
    mat[1][1] =  cos_i * scale_y / 100;

    // calculate bounding box
    int min_xy[2] = { 0, 0 }, max_xy[2] = { 0, 0 };
    for (int i = 0; i < 4; ++i) {
        int c_x = i < 2       ? -pos.w / 2 :  pos.w / 2;
        int c_y = (i + 1) & 2 ?  pos.h / 2 : -pos.h / 2;
        //Mion: need to make sure corners are in right order (UL,LL,LR,UR)
        if (scale_x < 0) c_x = -c_x;
        if (scale_y < 0) c_y = -c_y;
        corner_xy[i][0] = (mat[0][0] * c_x + mat[0][1] * c_y) / 1000 + pos.x;
        corner_xy[i][1] = (mat[1][0] * c_x + mat[1][1] * c_y) / 1000 + pos.y;

        if (i==0 || min_xy[0] > corner_xy[i][0]) min_xy[0] = corner_xy[i][0];
        if (i==0 || max_xy[0] < corner_xy[i][0]) max_xy[0] = corner_xy[i][0];
        if (i==0 || min_xy[1] > corner_xy[i][1]) min_xy[1] = corner_xy[i][1];
        if (i==0 || max_xy[1] < corner_xy[i][1]) max_xy[1] = corner_xy[i][1];
    }

    bounding_rect.x = min_xy[0];
    bounding_rect.y = min_xy[1];
    bounding_rect.w = max_xy[0] - min_xy[0] + 1;
    bounding_rect.h = max_xy[1] - min_xy[1] + 1;
    
    // calculate inverse matrix
    int denom = scale_x * scale_y;
    if (denom == 0) return;

    inv_mat[0][0] =  mat[1][1] * 10000 / denom;
    inv_mat[0][1] = -mat[0][1] * 10000 / denom;
    inv_mat[1][0] = -mat[1][0] * 10000 / denom;
    inv_mat[1][1] =  mat[0][0] * 10000 / denom;
}


SDL_Surface* AnimationInfo::allocSurface(int w, int h)
{
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, BPP, RMASK, GMASK, BMASK, AMASK);
}


void AnimationInfo::allocImage(int w, int h)
{
    if (!image_surface
        || image_surface->w != w
        || image_surface->h != h) {
        deleteImage();

        image_surface = allocSurface(w, h);
#ifdef BPP16
        if (image_surface)
        alpha_buf = new unsigned char[w * h];
#endif
    }

    abs_flag = true;
    pos.w = w / num_of_cells;
    pos.h = h;
}


void AnimationInfo::copySurface(SDL_Surface* surface, SDL_Rect* src_rect,
                                SDL_Rect* dst_rect)
{
    if (!image_surface || !surface) return;
    
    SDL_Rect _dst_rect = {0, 0};
    if (dst_rect) _dst_rect = *dst_rect;

    SDL_Rect _src_rect = {0, 0, surface->w, surface->h};
    if (src_rect) _src_rect = *src_rect;

    if (_src_rect.x >= surface->w) return;
    if (_src_rect.y >= surface->h) return;
    
    if (_src_rect.x+_src_rect.w >= surface->w)
        _src_rect.w = surface->w - _src_rect.x;
    if (_src_rect.y+_src_rect.h >= surface->h)
        _src_rect.h = surface->h - _src_rect.y;
        
    if (_dst_rect.x+_src_rect.w > image_surface->w)
        _src_rect.w = image_surface->w - _dst_rect.x;
    if (_dst_rect.y+_src_rect.h > image_surface->h)
        _src_rect.h = image_surface->h - _dst_rect.y;
        
    SDL_LockSurface( surface );
    SDL_LockSurface( image_surface );

    int i;
    for (i=0 ; i<_src_rect.h ; i++)
        memcpy( (ONSBuf*)((unsigned char*)image_surface->pixels + image_surface->pitch * (_dst_rect.y+i)) + _dst_rect.x,
                (ONSBuf*)((unsigned char*)surface->pixels + surface->pitch * (_src_rect.y+i)) + _src_rect.x,
                _src_rect.w*sizeof(ONSBuf) );
#if defined(BPP16)
    for (i=0 ; i<_src_rect.h ; i++)
        memset( alpha_buf + image_surface->w * (_dst_rect.y+i) + _dst_rect.x, 0xff, _src_rect.w );
#endif

    SDL_UnlockSurface(image_surface);
    SDL_UnlockSurface(surface);
}


void AnimationInfo::fill(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (!image_surface) return;

    SDL_LockSurface(image_surface);
    ONSBuf* dst_buffer = (ONSBuf*) image_surface->pixels;

#ifdef BPP16
    Uint32 rgb = ((r>>RLOSS) << RSHIFT) | ((g>>GLOSS) << GSHIFT) | (b>>BLOSS);
    unsigned char *alphap = alpha_buf;
    int dst_margin = image_surface->w % 2;
#else
    Uint32 rgb = (r << RSHIFT) | (g << GSHIFT) | b;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    unsigned char *alphap = (unsigned char *)dst_buffer + 3;
#else
    unsigned char *alphap = (unsigned char *)dst_buffer;
#endif
    int dst_margin = 0;
#endif

    for (int i=0 ; i<image_surface->h ; i++){
        for (int j=0 ; j<image_surface->w ; j++, dst_buffer++)
            SET_PIXEL(rgb, a);
        dst_buffer += dst_margin;
    }
    SDL_UnlockSurface( image_surface );
}


void AnimationInfo::setupImage( SDL_Surface *surface, SDL_Surface *surface_m,
                                bool has_alpha, int ratio1, int ratio2 )
{
    if (surface == NULL) return;

    SDL_LockSurface(surface);
    Uint32* buffer = (Uint32*) surface->pixels;
    SDL_PixelFormat *fmt = surface->format;

    int w = surface->w;
    int h = surface->h;
    int w2 = w / num_of_cells;
    if ((trans_mode == TRANS_ALPHA) && !has_alpha)
        w2 /= 2;
    w = w2 * num_of_cells;
    orig_pos.w = w;
    orig_pos.h = h;

    // Create a 32bpp buffer for image processing
#ifdef BPP16
    SDL_Surface *tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h,
                                             fmt->BitsPerPixel, fmt->Rmask,
                                             fmt->Gmask, fmt->Bmask, fmt->Amask);
    Uint32 *dst_buffer = (Uint32 *)tmp->pixels;
#else
    allocImage(w, h);
    SDL_Surface *tmp = image_surface;
    ONSBuf *dst_buffer = (ONSBuf *)image_surface->pixels;
#endif
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    unsigned char *alphap = (unsigned char *)dst_buffer + 3;
#else
    unsigned char *alphap = (unsigned char *)dst_buffer;
#endif

    Uint32 ref_color = 0;
    if (trans_mode == TRANS_TOPLEFT) {
        ref_color = *buffer;
    }
    else if (trans_mode == TRANS_TOPRIGHT) {
        ref_color = *(buffer + surface->w - 1);
    }
    else if (trans_mode == TRANS_DIRECT) {
        ref_color = direct_color.r << fmt->Rshift |
                    direct_color.g << fmt->Gshift |
                    direct_color.b << fmt->Bshift;
    }

    ref_color &= RGBMASK;

    int i, j, c;
    if ( trans_mode == TRANS_ALPHA ){
        if (has_alpha){
            for (i=h ; i>0 ; i--){
                for (j=w ; j>0 ; j--, buffer++, dst_buffer++)
                    SET_PIXEL32(*buffer, *buffer >> 24);
            }
        }
        else {
            const int cw = surface->w / num_of_cells;
            for (i=h ; i>0 ; i--){
                for (c=num_of_cells ; c>0 ; c--){
                    for (j=w2 ; j>0 ; j--, buffer++, dst_buffer++){
                        SET_PIXEL32(*buffer, (*(buffer+w2) & 0xff) ^ 0xff);
                    }
                    buffer += cw - w2;
                }
                buffer += surface->w - (cw * num_of_cells);
            }
        }
    }
    else if ( trans_mode == TRANS_MASK ){
        if (surface_m){
            SDL_LockSurface( surface_m );
            int mw = surface->w;
            int mh = surface->h;
            if (!has_alpha){
                for (i=0 ; i<h ; i++){
                    Uint32 *buffer_m = (Uint32 *)surface_m->pixels + mw*(i%mh);
                    for (c=num_of_cells ; c>0 ; c--){
                        for (j=0 ; j<w2 ; j++, buffer++, dst_buffer++){
                            SET_PIXEL32(*buffer, (*(buffer_m + j%mw) & 0xff) ^ 0xff);
                        }
                    }
                }
            } else {
                //if has_alpha, combine the pixel alpha with the mask value
                for (i=0 ; i<h ; i++){
                    Uint32 *buffer_m = (Uint32 *)surface_m->pixels + mw*(i%mh);
                    for (c=num_of_cells ; c>0 ; c--){
                        for (j=0 ; j<w2 ; j++, buffer++, dst_buffer++){
                            SET_PIXEL32(*buffer,
                                        (((*(buffer_m + j%mw) & 0xff) ^ 0xff) *
                                         (*buffer >> 24)) >> 8);
                        }
                    }
                }
            }
            SDL_UnlockSurface( surface_m );
        } else {
            for (i=h ; i>0 ; i--){
                for (j=w ; j>0 ; j--, buffer++, dst_buffer++)
                    SET_PIXEL32(*buffer, 0xff);
            }
        }
    }
    else if (has_alpha){
        for (i=h ; i>0 ; i--){
            for (j=w ; j>0 ; j--, buffer++, dst_buffer++)
                SET_PIXEL32(*buffer, *buffer >> 24);
        }
    }
    else if ( trans_mode == TRANS_TOPLEFT ||
              trans_mode == TRANS_TOPRIGHT ||
              trans_mode == TRANS_DIRECT ){
        for (i=h ; i>0 ; i--){
            for (j=w ; j>0 ; j--, buffer++, dst_buffer++){
                if ( (*buffer & RGBMASK) == ref_color ){
                    SET_PIXEL32(MEDGRAY, 0x00);
                }
                else{
                    SET_PIXEL32(*buffer, 0xff);
                }
            }
        }
    }
    else if ( trans_mode == TRANS_STRING ){
        for (i=h ; i>0 ; i--){
            for (j=w ; j>0 ; j--, buffer++, dst_buffer++)
                SET_PIXEL32(*buffer, *buffer >> 24);
        }
    }
    else { // TRANS_COPY
        for (i=h ; i>0 ; i--){
            for (j=w ; j>0 ; j--, buffer++, dst_buffer++)
                SET_PIXEL32(*buffer, 0xff);
        }
    }

    SDL_UnlockSurface( surface );

    if (ratio1 != ratio2) {
        SDL_Surface *src_s = tmp;

        w = ((src_s->w / num_of_cells) * ratio1 / ratio2) * num_of_cells;
        if (w >= 16384){
            //too wide for SDL_Surface pitch (Uint16) at 4bpp; size differently
            fprintf(stderr, " *** image '%s' is too wide to resize to (%d,%d); ",
                    (const char *)file_name, w, src_s->h * ratio1 / ratio2);
            int ratio3 = 16384 * ratio2 / src_s->w;
            w = ((src_s->w / num_of_cells) * ratio3 / ratio2) * num_of_cells;
            h = src_s->h * ratio3 / ratio2;
            if ( h == 0 ) h = 1;
            fprintf(stderr, "resizing to (%d,%d) instead *** \n", w, h);
        }else{
            if ( w == 0 ) w = num_of_cells;
            h = src_s->h * ratio1 / ratio2;
            if ( h == 0 ) h = 1;
        }
#ifdef BPP16
        tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h,
                                    fmt->BitsPerPixel, fmt->Rmask,
                                    fmt->Gmask, fmt->Bmask, fmt->Amask);
#else
        image_surface = NULL;
        allocImage(w, h);
        tmp = image_surface;
#endif
        resizeSurface( src_s, tmp, num_of_cells );
        SDL_FreeSurface( src_s );
    }
#ifdef BPP16
    allocImage(w, h);
    ONSBuf *img_buffer = (ONSBuf *)image_surface->pixels;
    alphap = alpha_buf;
    buffer = (Uint32 *)tmp->pixels;
    for (i=0 ; i<h ; i++){
        for (j=0 ; j<w ; j++, buffer++, img_buffer++)
            SET_PIXEL32TO16(*buffer, *buffer >> 24);
        img_buffer += w % 2;
    }
    SDL_FreeSurface( tmp );
#endif
}


bool AnimationInfo::update_showing()
{
    bool do_show = visible_ && enabled_;
    if (showing_ != do_show) {
        showing_ = do_show;
        return true;
    }
    return false;   
}


void AnimationInfo::setCpufuncs(unsigned int func)
{
    cpufuncs = func;
}


unsigned int AnimationInfo::getCpufuncs()
{
    return cpufuncs;
}


void AnimationInfo::imageFilterMean(unsigned char *src1, unsigned char *src2, unsigned char *dst, int length)
{
#if defined(USE_PPC_GFX)
    if(cpufuncs & CPUF_PPC_ALTIVEC) {
        imageFilterMean_Altivec(src1, src2, dst, length);
    } else {
        int n = length + 1;
        BASIC_MEAN();
    }
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterMean_SSE2(src1, src2, dst, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterMean_MMX(src1, src2, dst, length);

    } else {
        int n = length + 1;
        BASIC_MEAN();
    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_MEAN();
#endif
}


void AnimationInfo::imageFilterAddTo(unsigned char *dst, unsigned char *src, int length)
{
#if defined(USE_PPC_GFX)
    if(cpufuncs & CPUF_PPC_ALTIVEC) {
        imageFilterAddTo_Altivec(dst, src, length);
    } else {
        int n = length + 1;
        BASIC_ADDTO();
    }
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterAddTo_SSE2(dst, src, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterAddTo_MMX(dst, src, length);

    } else {
        int n = length + 1;
        BASIC_ADDTO();
    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_ADDTO();
#endif
}


void AnimationInfo::imageFilterSubFrom(unsigned char *dst, unsigned char *src, int length)
{
#if defined(USE_PPC_GFX)
    if(cpufuncs & CPUF_PPC_ALTIVEC) {
        imageFilterSubFrom_Altivec(dst, src, length);
    } else {
        int n = length + 1;
        BASIC_SUBFROM();
    }
#elif defined(USE_X86_GFX)

#ifndef MACOSX
    if (cpufuncs & CPUF_X86_SSE2) {
#endif // !MACOSX

        imageFilterSubFrom_SSE2(dst, src, length);

#ifndef MACOSX
    } else if (cpufuncs & CPUF_X86_MMX) {

        imageFilterSubFrom_MMX(dst, src, length);

    } else {
        int n = length + 1;
        BASIC_SUBFROM();
    }
#endif // !MACOSX

#else // no special gfx handling
    int n = length + 1;
    BASIC_SUBFROM();
#endif
}


void AnimationInfo::imageFilterBlend(Uint32 *dst_buffer, Uint32 *src_buffer,
                                     Uint8 *alphap, int alpha, int length)
{
    int n = length + 1;
    BASIC_BLEND();
}


#include "resize_image.h"

static unsigned char *resize_buffer = NULL;
static size_t resize_buffer_size = 0;

void AnimationInfo::resetResizeBuffer() {
    if (resize_buffer_size != 16){
        if (resize_buffer) delete[] resize_buffer;
        resize_buffer = new unsigned char[16];
        resize_buffer_size = 16;
    }
}


// resize 32bit surface to 32bit surface
int AnimationInfo::resizeSurface( SDL_Surface *src, SDL_Surface *dst, int num_cells )
{
    SDL_LockSurface( dst );
    SDL_LockSurface( src );
    Uint32 *src_buffer = (Uint32 *)src->pixels;
    Uint32 *dst_buffer = (Uint32 *)dst->pixels;

    /* size of tmp_buffer must be larger than 16 bytes */
    size_t len = src->w * (src->h+1) * 4 + 4;
    if (resize_buffer_size < len){
        if (resize_buffer) delete[] resize_buffer;
        resize_buffer = new unsigned char[len];
        resize_buffer_size = len;
    }
    resizeImage( (unsigned char*)dst_buffer, dst->w, dst->h, dst->w * 4,
                 (unsigned char*)src_buffer, src->w, src->h, src->w * 4,
                 4, resize_buffer, src->w * 4, num_cells );

    SDL_UnlockSurface( src );
    SDL_UnlockSurface( dst );

    return 0;
}
