/* -*- C++ -*-
 * 
 *  ONScripterLabel_image.cpp - Image processing in ONScripter
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

#include "ONScripterLabel.h"
#include "resize_image.h"

// resize 32bit surface to 32bit surface
int ONScripterLabel::resizeSurface( SDL_Surface *src, SDL_Surface *dst )
{
    SDL_LockSurface( dst );
    SDL_LockSurface( src );
    Uint32 *src_buffer = (Uint32 *)src->pixels;
    Uint32 *dst_buffer = (Uint32 *)dst->pixels;

    /* size of tmp_buffer must be larger than 16 bytes */
    size_t len = src->w * (src->h+1) * 4 + 4;
    if (resize_buffer_size < len){
        delete[] resize_buffer;
        resize_buffer = new unsigned char[len];
        resize_buffer_size = len;
    }
    resizeImage( (unsigned char*)dst_buffer, dst->w, dst->h, dst->w * 4,
                 (unsigned char*)src_buffer, src->w, src->h, src->w * 4,
                 4, resize_buffer, src->w * 4, false );

    SDL_UnlockSurface( src );
    SDL_UnlockSurface( dst );

    return 0;
}

#if defined(BPP16)
#define blend_pixel(){\
    Uint32 s1 = (*src1_buffer | *src1_buffer << 16) & 0x07e0f81f; \
    Uint32 s2 = (*src2_buffer | *src2_buffer << 16) & 0x07e0f81f; \
    src1_buffer++; \
    src2_buffer++; \
    mask_rb = (s1 + ((s2-s1) * mask2 >> 5)) & 0x07e0f81f; \
    *dst_buffer++ = mask_rb | mask_rb >> 16; \
}
#else
#define blend_pixel(){\
    mask_rb =  (((*src1_buffer & 0xff00ff) * mask1 + \
                 (*src2_buffer & 0xff00ff) * mask2) >> 8) & 0xff00ff;\
    mask  = (((*src1_buffer++ & 0x00ff00) * mask1 +\
              (*src2_buffer++ & 0x00ff00) * mask2) >> 8) & 0x00ff00;\
    *dst_buffer++ = mask_rb | mask;\
}
#endif

// alphaBlend
// dst: accumulation_surface
// src1: effect_src_surface
// src2: effect_dst_surface
void ONScripterLabel::alphaBlend( SDL_Surface *mask_surface,
                                  int trans_mode, Uint32 mask_value, SDL_Rect *clip )
{
    SDL_Rect rect = {0, 0, screen_width, screen_height};
    int i, j;
    Uint32 mask, mask1, mask2, mask_rb;
    ONSBuf *mask_buffer=NULL;

    /* ---------------------------------------- */
    /* clipping */
    if ( clip ){
        if ( AnimationInfo::doClipping( &rect, clip ) ) return;
    }

    /* ---------------------------------------- */

    SDL_LockSurface( effect_src_surface );
    SDL_LockSurface( effect_dst_surface );
    SDL_LockSurface( accumulation_surface );
    if ( mask_surface ) SDL_LockSurface( mask_surface );
    
    ONSBuf *src1_buffer = (ONSBuf *)effect_src_surface->pixels   + effect_src_surface->w * rect.y + rect.x;
    ONSBuf *src2_buffer = (ONSBuf *)effect_dst_surface->pixels   + effect_dst_surface->w * rect.y + rect.x;
    ONSBuf *dst_buffer  = (ONSBuf *)accumulation_surface->pixels + accumulation_surface->w * rect.y + rect.x;

    SDL_PixelFormat *fmt = accumulation_surface->format;
    Uint32 overflow_mask;
    if ( trans_mode == ALPHA_BLEND_FADE_MASK )
        overflow_mask = 0xffffffff;
    else
        overflow_mask = ~fmt->Bmask;

    mask_value >>= fmt->Bloss;
    mask2 = mask_value & fmt->Bmask;
    mask1 = mask2 ^ fmt->Bmask;

    for ( i=0; i<rect.h ; i++ ) {
        if (mask_surface) mask_buffer = (ONSBuf *)mask_surface->pixels + mask_surface->w * ((rect.y+i)%mask_surface->h);

        if ( trans_mode == ALPHA_BLEND_FADE_MASK ||
             trans_mode == ALPHA_BLEND_CROSSFADE_MASK ){
            for ( j=0 ; j<rect.w ; j++ ){
                mask = *(mask_buffer + (rect.x+j)%mask_surface->w) & fmt->Bmask;
                if ( mask_value > mask ){
                    mask2 = mask_value - mask;
                    if ( mask2 & overflow_mask ) mask2 = fmt->Bmask;
                }
                else{
                    mask2 = 0;
                }
                mask1 = mask2 ^ fmt->Bmask;
                blend_pixel();
            }
        }
        else{ // ALPHA_BLEND_CONST
            for ( j=0 ; j<rect.w ; j++ ){
                blend_pixel();
            }
        }
        src1_buffer += screen_width - rect.w;
        src2_buffer += screen_width - rect.w;
        dst_buffer  += screen_width - rect.w;
    }
    
    if ( mask_surface ) SDL_UnlockSurface( mask_surface );
    SDL_UnlockSurface( accumulation_surface );
    SDL_UnlockSurface( effect_dst_surface );
    SDL_UnlockSurface( effect_src_surface );
}

// alphaBlend32
// dst: ONSBuf surface (accumulation_surface)
// src: 8bit surface (TTF_RenderGlyph_Shaded())
void ONScripterLabel::alphaBlend32( SDL_Surface *dst_surface, SDL_Rect dst_rect,
                                    SDL_Surface *src_surface, SDL_Color &color, SDL_Rect *clip, bool rotate_flag )
{
    int i, j;
    int x2=0, y2=0;
    SDL_Rect clipped_rect;
    Uint32 mask, mask1, mask2, mask_rb;

    /* ---------------------------------------- */
    /* 1st clipping */
    if ( clip ){
        if ( AnimationInfo::doClipping( &dst_rect, clip, &clipped_rect ) ) return;

        x2 += clipped_rect.x;
        y2 += clipped_rect.y;
    }

    /* ---------------------------------------- */
    /* 2nd clipping */
    SDL_Rect clip_rect = {0, 0, dst_surface->w, dst_surface->h};
    if ( AnimationInfo::doClipping( &dst_rect, &clip_rect, &clipped_rect ) ) return;
    
    x2 += clipped_rect.x;
    y2 += clipped_rect.y;

    /* ---------------------------------------- */

    SDL_LockSurface( dst_surface );
    SDL_LockSurface( src_surface );

    unsigned char *src_buffer = (unsigned char*)src_surface->pixels + src_surface->pitch * y2 + x2;
    ONSBuf *dst_buffer = (AnimationInfo::ONSBuf *)dst_surface->pixels + dst_surface->w * dst_rect.y + dst_rect.x;
#if defined(BPP16)    
    Uint32 src_color = ((color.r & 0xf8) << 8 |
                        (color.g & 0xfc) << 3 |
                        (color.b & 0xf8) >> 3);
    src_color = (src_color | src_color << 16) & 0x07e0f81f;
#else
    Uint32 src_color1 = color.r << 16 | color.b;
    Uint32 src_color2 = color.g << 8;
#endif    

    SDL_PixelFormat *fmt = dst_surface->format;
    for ( i=0 ; i<dst_rect.h ; i++ ){
        if (rotate_flag)
            src_buffer = (unsigned char*)src_surface->pixels + src_surface->pitch*(src_surface->h - x2 - 1) + y2 + i;
        for ( j=0 ; j<dst_rect.w ; j++ ){

            mask2 = *src_buffer >> fmt->Bloss;
            mask1 = mask2 ^ fmt->Bmask;
            
#if defined(BPP16)
            Uint32 d1 = (*dst_buffer | *dst_buffer << 16) & 0x07e0f81f;

            mask_rb = (d1 + ((src_color-d1) * mask2 >> 5)) & 0x07e0f81f; // red, green and blue pixel
            *dst_buffer++ = mask_rb | mask_rb >> 16;
#else            
            mask_rb = (((*dst_buffer & 0xff00ff) * mask1 + 
                        src_color1 * mask2) >> 8) & 0xff00ff;
            mask = (((*dst_buffer & 0x00ff00) * mask1 +
                     src_color2 * mask2) >> 8) & 0x00ff00;
            *dst_buffer++ = mask_rb | mask;
#endif            
            if (rotate_flag)
                src_buffer -= src_surface->pitch;
            else
                src_buffer++;
        }
        if (!rotate_flag)
            src_buffer += src_surface->pitch - dst_rect.w;
        dst_buffer += dst_surface->w - dst_rect.w;
    }
    
    SDL_UnlockSurface( src_surface );
    SDL_UnlockSurface( dst_surface );
}

void ONScripterLabel::makeNegaSurface( SDL_Surface *surface, SDL_Rect &clip )
{
    SDL_LockSurface( surface );
    ONSBuf *buf = (ONSBuf *)surface->pixels + clip.y * surface->w + clip.x;

    ONSBuf mask = surface->format->Rmask | surface->format->Gmask | surface->format->Bmask;
    for ( int i=clip.y ; i<clip.y + clip.h ; i++ ){
        for ( int j=clip.x ; j<clip.x + clip.w ; j++ )
            *buf++ ^= mask;
        buf += surface->w - clip.w;
    }

    SDL_UnlockSurface( surface );
}

void ONScripterLabel::makeMonochromeSurface( SDL_Surface *surface, SDL_Rect &clip )
{
    SDL_LockSurface( surface );
    ONSBuf *buf = (ONSBuf *)surface->pixels + clip.y * surface->w + clip.x, c;

    SDL_PixelFormat *fmt = surface->format;
    for ( int i=clip.y ; i<clip.y + clip.h ; i++ ){
        for ( int j=clip.x ; j<clip.x + clip.w ; j++ ){
            c = ((((*buf & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss) * 77 +
                 (((*buf & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss) * 151 +
                 (((*buf & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss) * 28 ) >> 8; 
            *buf++ = ((monocro_color_lut[c][0] >> fmt->Rloss) << surface->format->Rshift |
                      (monocro_color_lut[c][1] >> fmt->Gloss) << surface->format->Gshift |
                      (monocro_color_lut[c][2] >> fmt->Bloss) << surface->format->Bshift);
        }
        buf += surface->w - clip.w;
    }

    SDL_UnlockSurface( surface );
}

void ONScripterLabel::refreshSurface( SDL_Surface *surface, SDL_Rect *clip_src, int refresh_mode )
{
    if (refresh_mode == REFRESH_NONE_MODE) return;

    SDL_Rect clip = {0, 0, surface->w, surface->h};
    if (clip_src) if ( AnimationInfo::doClipping( &clip, clip_src ) ) return;

    int i, top;

    SDL_FillRect( surface, &clip, SDL_MapRGB( surface->format, 0, 0, 0) );
    
    drawTaggedSurface( surface, &bg_info, clip );
    
    if ( !all_sprite_hide_flag ){
        if ( z_order < 10 && refresh_mode & REFRESH_SAYA_MODE )
            top = 9;
        else
            top = z_order;
        for ( i=MAX_SPRITE_NUM-1 ; i>top ; i-- ){
            if ( sprite_info[i].image_surface && sprite_info[i].visible ){
                drawTaggedSurface( surface, &sprite_info[i], clip );
            }
        }
    }

    for ( i=0 ; i<3 ; i++ ){
        if (human_order[2-i] >= 0 && tachi_info[human_order[2-i]].image_surface){
            drawTaggedSurface( surface, &tachi_info[human_order[2-i]], clip );
        }
    }

    if ( windowback_flag ){
        if ( nega_mode == 1 ) makeNegaSurface( surface, clip );
        if ( monocro_flag )   makeMonochromeSurface( surface, clip );
        if ( nega_mode == 2 ) makeNegaSurface( surface, clip );

        if (refresh_mode & REFRESH_SHADOW_MODE)
            shadowTextDisplay( surface, clip );
        if (refresh_mode & REFRESH_TEXT_MODE)
            text_info.blendOnSurface( surface, 0, 0, clip );
    }

    if ( !all_sprite_hide_flag ){
        if ( refresh_mode & REFRESH_SAYA_MODE )
            top = 10;
        else
            top = 0;
        for ( i=z_order ; i>=top ; i-- ){
            if ( sprite_info[i].image_surface && sprite_info[i].visible ){
                drawTaggedSurface( surface, &sprite_info[i], clip );
            }
        }
    }

    if ( !windowback_flag ){
        if ( nega_mode == 1 ) makeNegaSurface( surface, clip );
        if ( monocro_flag )   makeMonochromeSurface( surface, clip );
        if ( nega_mode == 2 ) makeNegaSurface( surface, clip );
    }
    
    if ( !( refresh_mode & REFRESH_SAYA_MODE ) ){
        for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
            if ( bar_info[i] ) {
                drawTaggedSurface( surface, bar_info[i], clip );
            }
        }
        for ( i=0 ; i<MAX_PARAM_NUM ; i++ ){
            if ( prnum_info[i] ){
                drawTaggedSurface( surface, prnum_info[i], clip );
            }
        }
    }

    if ( !windowback_flag ){
        if (refresh_mode & REFRESH_SHADOW_MODE)
            shadowTextDisplay( surface, clip );
        if (refresh_mode & REFRESH_TEXT_MODE)
            text_info.blendOnSurface( surface, 0, 0, clip );
    }

    if ( refresh_mode & REFRESH_CURSOR_MODE && !textgosub_label ){
        if ( clickstr_state == CLICK_WAIT )
            drawTaggedSurface( surface, &cursor_info[CURSOR_WAIT_NO], clip );
        else if ( clickstr_state == CLICK_NEWPAGE )
            drawTaggedSurface( surface, &cursor_info[CURSOR_NEWPAGE_NO], clip );
    }

    ButtonLink *p_button_link = root_button_link.next;
    while( p_button_link ){
        if (p_button_link->show_flag > 0){
            drawTaggedSurface( surface, p_button_link->anim[p_button_link->show_flag-1], clip );
        }
        p_button_link = p_button_link->next;
    }
}

void ONScripterLabel::refreshSprite( SDL_Surface *surface, int sprite_no,
                                     bool active_flag, int cell_no,
                                     SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect )
{
    if ( sprite_info[sprite_no].image_name && 
         ( sprite_info[ sprite_no ].visible != active_flag ||
           (cell_no >= 0 && sprite_info[ sprite_no ].current_cell != cell_no ) ||
           AnimationInfo::doClipping(check_src_rect, &sprite_info[ sprite_no ].pos) == 0 ||
           AnimationInfo::doClipping(check_dst_rect, &sprite_info[ sprite_no ].pos) == 0) )
    {
        if ( cell_no >= 0 )
            sprite_info[ sprite_no ].setCell(cell_no);

        sprite_info[ sprite_no ].visible = active_flag;

        dirty_rect.add( sprite_info[ sprite_no ].pos );
    }
}

void ONScripterLabel::createBackground()
{
    bg_effect_image = COLOR_EFFECT_IMAGE;

    if ( !strcmp( bg_info.file_name, "white" ) ){
        bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0xff;
    }
    else if ( !strcmp( bg_info.file_name, "black" ) ){
        bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0x00;
    }
    else{
        if ( bg_info.file_name[0] == '#' ){
            readColor( &bg_info.color, bg_info.file_name );
        }
        else{
            bg_info.color[0] = bg_info.color[1] = bg_info.color[2] = 0x00;
            setStr( &bg_info.image_name, bg_info.file_name );
            parseTaggedString( &bg_info );
            bg_info.trans_mode = AnimationInfo::TRANS_COPY;
            setupAnimationInfo( &bg_info );
            if (bg_info.image_surface){
                bg_info.pos.x = (screen_width - bg_info.image_surface->w) / 2;
                bg_info.pos.y = (screen_height - bg_info.image_surface->h) / 2;
            }
            bg_effect_image = BG_EFFECT_IMAGE;
        }
    }

    if (bg_effect_image == COLOR_EFFECT_IMAGE){
        bg_info.num_of_cells = 1;
        bg_info.trans_mode = AnimationInfo::TRANS_COPY;
        bg_info.pos.x = 0;
        bg_info.pos.y = 0;
        bg_info.allocImage( screen_width, screen_height );
        bg_info.fill(bg_info.color[0], bg_info.color[1], bg_info.color[2], 0xff);
    }
}
