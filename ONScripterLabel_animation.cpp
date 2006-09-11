/* -*- C++ -*-
 * 
 *  ONScripter_animation.cpp - Methods to manipulate AnimationInfo
 *
 *  Copyright (c) 2001-2005 Ogapee. All rights reserved.
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

int ONScripterLabel::proceedAnimation()
{
    int i, minimum_duration = -1;
    AnimationInfo *anim;
    
    for ( i=0 ; i<3 ; i++ ){
        anim = &tachi_info[i];
        if ( anim->visible && anim->is_animatable ){
            minimum_duration = estimateNextDuration( anim, anim->pos, minimum_duration );
        }
    }

    for ( i=MAX_SPRITE_NUM-1 ; i>=0 ; i-- ){
        anim = &sprite_info[i];
        if ( anim->visible && anim->is_animatable ){
            minimum_duration = estimateNextDuration( anim, anim->pos, minimum_duration );
        }
    }

    if ( !textgosub_label &&
         ( clickstr_state == CLICK_WAIT ||
           clickstr_state == CLICK_NEWPAGE ) ){
        
        if      ( clickstr_state == CLICK_WAIT )
            anim = &cursor_info[CURSOR_WAIT_NO];
        else if ( clickstr_state == CLICK_NEWPAGE )
            anim = &cursor_info[CURSOR_NEWPAGE_NO];

        if ( anim->visible && anim->is_animatable ){
            SDL_Rect dst_rect = anim->pos;
            if ( !anim->abs_flag ){
                dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
                dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
            }

            minimum_duration = estimateNextDuration( anim, dst_rect, minimum_duration );
        }
    }

    if ( minimum_duration == -1 ) minimum_duration = 0;

    return minimum_duration;
}

int ONScripterLabel::estimateNextDuration( AnimationInfo *anim, SDL_Rect &rect, int minimum )
{
    if ( anim->remaining_time == 0 ){
        if ( minimum == -1 ||
             minimum > anim->duration_list[ anim->current_cell ] )
            minimum = anim->duration_list[ anim->current_cell ];

        if ( anim->proceedAnimation() )
            flushDirect( rect, refreshMode() | (draw_cursor_flag?REFRESH_CURSOR_MODE:0) );
    }
    else{
        if ( minimum == -1 ||
             minimum > anim->remaining_time )
            minimum = anim->remaining_time;
    }

    return minimum;
}

void ONScripterLabel::resetRemainingTime( int t )
{
    int i;
    AnimationInfo *anim;
    
    for ( i=0 ; i<3 ; i++ ){
        anim = &tachi_info[i];
        if ( anim->visible && anim->is_animatable){
            anim->remaining_time -= t;
        }
    }
        
    for ( i=MAX_SPRITE_NUM-1 ; i>=0 ; i-- ){
        anim = &sprite_info[i];
        if ( anim->visible && anim->is_animatable ){
            anim->remaining_time -= t;
        }
    }

    if ( !textgosub_label &&
         ( clickstr_state == CLICK_WAIT ||
           clickstr_state == CLICK_NEWPAGE ) ){
        if ( clickstr_state == CLICK_WAIT )
            anim = &cursor_info[CURSOR_WAIT_NO];
        else if ( clickstr_state == CLICK_NEWPAGE )
            anim = &cursor_info[CURSOR_NEWPAGE_NO];
        
        if ( anim->visible && anim->is_animatable ){
            anim->remaining_time -= t;
        }
    }
}

void ONScripterLabel::setupAnimationInfo( AnimationInfo *anim, FontInfo *info )
{
    anim->deleteSurface();
    anim->abs_flag = true;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){
        FontInfo f_info = sentence_font;
        if (info) f_info = *info;

        if ( anim->font_size_xy[0] >= 0 ){ // in case of Sprite, not rclick menu
            f_info.setTateyokoMode(0);
            f_info.top_xy[0] = anim->pos.x * screen_ratio2 / screen_ratio1;
            f_info.top_xy[1] = anim->pos.y * screen_ratio2 / screen_ratio1;
            if (anim->is_single_line)
                f_info.setLineArea( strlen(anim->file_name)/2+1 );
            f_info.clear();
            
            f_info.pitch_xy[0] = f_info.pitch_xy[0] - f_info.font_size_xy[0] + anim->font_size_xy[0];
            f_info.font_size_xy[0] = anim->font_size_xy[0];
            f_info.pitch_xy[1] = f_info.pitch_xy[1] - f_info.font_size_xy[1] + anim->font_size_xy[1];
            f_info.font_size_xy[1] = anim->font_size_xy[1];
            if ( anim->font_pitch >= 0 )
                f_info.pitch_xy[0] = anim->font_pitch;
            f_info.ttf_font = NULL;
        }

        SDL_Rect pos;
        if (anim->is_tight_region){
        drawString( anim->file_name, anim->color_list[ anim->current_cell ], &f_info, false, NULL, &pos );
        }
        else{
            int xy_bak[2];
            xy_bak[0] = f_info.xy[0];
            xy_bak[1] = f_info.xy[1];
            
            int xy[2] = {0, 0};
            f_info.setXY(f_info.num_xy[0]-1, f_info.num_xy[1]-1);
            pos = f_info.calcUpdatedArea(xy, screen_ratio1, screen_ratio2);

            f_info.xy[0] = xy_bak[0];
            f_info.xy[1] = xy_bak[1];
        }
        
        if (info != NULL){
            info->xy[0] = f_info.xy[0];
            info->xy[1] = f_info.xy[1];
        }
        
        anim->allocImage( pos.w*anim->num_of_cells, pos.h );
        anim->fill( 0, 0, 0, 0 );
        
        f_info.setRubyOnFlag(false);
        f_info.top_xy[0] = f_info.top_xy[1] = 0;
        for ( int i=0 ; i<anim->num_of_cells ; i++ ){
            f_info.clear();
            drawString( anim->file_name, anim->color_list[i], &f_info, false, NULL, NULL, anim );
            f_info.top_xy[0] += anim->pos.w * screen_ratio2 / screen_ratio1;
        }
    }
    else{
        SDL_Surface *surface = loadImage( anim->file_name );

        SDL_Surface *surface_m = NULL;
        if (anim->trans_mode == AnimationInfo::TRANS_MASK)
            surface_m = loadImage( anim->mask_file_name );
        
        anim->setupImage(surface, surface_m);

        if ( surface ) SDL_FreeSurface(surface);
        if ( surface_m ) SDL_FreeSurface(surface_m);
    }

}

void ONScripterLabel::parseTaggedString( AnimationInfo *anim )
{
    if (anim->image_name == NULL) return;
    anim->removeTag();
    
    int i;
    char *buffer = anim->image_name;
    anim->num_of_cells = 1;
    anim->trans_mode = trans_mode;

    if ( buffer[0] == ':' ){
        while (*++buffer == ' ');
        
        if ( buffer[0] == 'a' ){
            anim->trans_mode = AnimationInfo::TRANS_ALPHA;
            buffer++;
        }
        else if ( buffer[0] == 'l' ){
            anim->trans_mode = AnimationInfo::TRANS_TOPLEFT;
            buffer++;
        }
        else if ( buffer[0] == 'r' ){
            anim->trans_mode = AnimationInfo::TRANS_TOPRIGHT;
            buffer++;
        }
        else if ( buffer[0] == 'c' ){
            anim->trans_mode = AnimationInfo::TRANS_COPY;
            buffer++;
        }
        else if ( buffer[0] == 's' ){
            anim->trans_mode = AnimationInfo::TRANS_STRING;
            buffer++;
            anim->num_of_cells = 0;
            if ( *buffer == '/' ){
                buffer++;
                script_h.getNext();
                
                script_h.pushCurrent( buffer );
                anim->font_size_xy[0] = script_h.readInt();
                anim->font_size_xy[1] = script_h.readInt();
                anim->font_pitch = script_h.readInt() + anim->font_size_xy[0];
                if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
                    script_h.readInt(); // 0 ... normal, 1 ... no anti-aliasing, 2 ... Fukuro
                }
                buffer = script_h.getNext();
                script_h.popCurrent();
            }
            else{
                anim->font_size_xy[0] = sentence_font.font_size_xy[0];
                anim->font_size_xy[1] = sentence_font.font_size_xy[1];
                anim->font_pitch = sentence_font.pitch_xy[0];
            }
            while(buffer[0] != '#' && buffer[0] != '\0') buffer++;
            i=0;
            while( buffer[i] == '#' ){
                anim->num_of_cells++;
                i += 7;
            }
            anim->color_list = new uchar3[ anim->num_of_cells ];
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                readColor( &anim->color_list[i], buffer );
                buffer += 7;
            }
        }
        else if ( buffer[0] == 'm' ){
            anim->trans_mode = AnimationInfo::TRANS_MASK;
            char *start = ++buffer;
            while(buffer[0] != ';' && buffer[0] != 0x0a && buffer[0] != '\0') buffer++;
            if (buffer[0] == ';')
                setStr( &anim->mask_file_name, start, buffer-start );
        }
        else if ( buffer[0] == '#' ){
            anim->trans_mode = AnimationInfo::TRANS_DIRECT;
            readColor( &anim->direct_color, buffer );
            buffer += 7;
        }
        else if ( buffer[0] == '!' ){
            anim->trans_mode = AnimationInfo::TRANS_PALLET;
            buffer++;
            anim->pallet_number = getNumberFromBuffer( (const char**)&buffer );
        }

        if (anim->trans_mode != AnimationInfo::TRANS_STRING)
            while(buffer[0] != '/' && buffer[0] != ';' && buffer[0] != '\0') buffer++;
    }

    if ( buffer[0] == '/' ){
        buffer++;
        anim->num_of_cells = getNumberFromBuffer( (const char**)&buffer );
        buffer++;
        if ( anim->num_of_cells == 0 ){
            fprintf( stderr, "ONScripterLabel::parseTaggedString  The number of cells is 0\n");
            return;
        }

        anim->duration_list = new int[ anim->num_of_cells ];

        if ( *buffer == '<' ){
            buffer++;
            for ( i=0 ; i<anim->num_of_cells ; i++ ){
                anim->duration_list[i] = getNumberFromBuffer( (const char**)&buffer );
                buffer++;
            }
            buffer++; // skip '>'
        }
        else{
            anim->duration_list[0] = getNumberFromBuffer( (const char**)&buffer );
            for ( i=1 ; i<anim->num_of_cells ; i++ )
                anim->duration_list[i] = anim->duration_list[0];
            buffer++;
        }
        
        anim->loop_mode = *buffer++ - '0'; // 3...no animation
        if ( anim->loop_mode != 3 ) anim->is_animatable = true;

        while(buffer[0] != ';' && buffer[0] != '\0') buffer++;
    }

    if ( buffer[0] == ';' ) buffer++;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING && buffer[0] == '$' ){
        script_h.pushCurrent( buffer );
        setStr( &anim->file_name, script_h.readStr() );
        script_h.popCurrent();
    }
    else{
        setStr( &anim->file_name, buffer );
    }
}

void ONScripterLabel::drawTaggedSurface( SDL_Surface *dst_surface, AnimationInfo *anim, SDL_Rect &clip )
{
    SDL_Rect poly_rect = anim->pos;
    if ( !anim->abs_flag ){
        poly_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        poly_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    anim->blendOnSurface( dst_surface, poly_rect.x, poly_rect.y,
                          clip, anim->trans );
}

void ONScripterLabel::stopAnimation( int click )
{
    int no;

    if ( !(event_mode & WAIT_TIMER_MODE) ) return;
    
    event_mode &= ~WAIT_TIMER_MODE;
    remaining_time = -1;
    if ( textgosub_label ) return;

    if      ( click == CLICK_WAIT )    no = CURSOR_WAIT_NO;
    else if ( click == CLICK_NEWPAGE ) no = CURSOR_NEWPAGE_NO;
    else return;

    if (cursor_info[no].image_surface == NULL) return;
    
    SDL_Rect dst_rect = cursor_info[ no ].pos;

    if ( !cursor_info[ no ].abs_flag ){
        dst_rect.x += sentence_font.x() * screen_ratio1 / screen_ratio2;
        dst_rect.y += sentence_font.y() * screen_ratio1 / screen_ratio2;
    }

    flushDirect( dst_rect, refreshMode() );
}
