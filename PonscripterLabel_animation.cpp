/* -*- C++ -*-
 * 
 *  Ponscripter_animation.cpp - Methods to manipulate AnimationInfo
 *
 *  Copyright (c) 2001-2005 Ogapee (original ONScripter, of which this
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

#include "PonscripterLabel.h"
#include "utf8_util.h"

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
                dst_rect.x += int(floor(sentence_font.GetX() * screen_ratio1 / screen_ratio2));
                dst_rect.y += sentence_font.GetY() * screen_ratio1 / screen_ratio2;
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

void downscale4x(SDL_Surface* src, SDL_Rect* srcpos, SDL_Surface* dst, SDL_Rect* dstpos)
{
    SDL_LockSurface(src);
    SDL_LockSurface(dst);
	
    const int sp = src->pitch;
    const int dp = dst->pitch;

    SDL_Rect sr, dr;
    if (srcpos) sr = *srcpos; else { sr.x = 0; sr.y = 0; sr.w = src->w; sr.h = src->h; }
    if (dstpos) dr = *dstpos; else { dr.x = 0; dr.y = 0; } dr.w = sr.w / 4; dr.h = sr.h / 4;

    Uint8* src_scan = (Uint8*) src->pixels + sr.y * sp + sr.x * 4;
    Uint8* dst_scan = (Uint8*) dst->pixels + dr.y * dp + dr.x * 4;
	
    for (int rows = dr.h; rows --> 0; src_scan += sp * 4, dst_scan += dp) {
	Uint32* src1 = (Uint32*) src_scan;
	Uint32* src2 = (Uint32*) (src_scan + sp);
	Uint32* src3 = (Uint32*) (src_scan + sp * 2);
	Uint32* src4 = (Uint32*) (src_scan + sp * 3);
	Uint32* dest = (Uint32*) dst_scan;
	int cols = dr.w;
	while (cols --> 0) {
	    Uint32 a, b, c, d;
	    Uint32 p;
#			define AddPx(s) p = *s; a += p >> 24; b += (p >> 16) & 0xff; c += (p >> 8) & 0xff; d += p & 0xff
	    p = *src1++; a = p >> 24; b = (p >> 16) & 0xff; c = (p >> 8) & 0xff; d = p & 0xff;
	    AddPx(src1++); AddPx(src1++); AddPx(src1++);
	    AddPx(src2++); AddPx(src2++); AddPx(src2++); AddPx(src2++);
	    AddPx(src3++); AddPx(src3++); AddPx(src3++); AddPx(src3++);
	    AddPx(src4++); AddPx(src4++); AddPx(src4++); AddPx(src4++);
	    a >>= 4; b >>= 4; c >>= 4; d >>= 4;
	    *dest++ = a << 24 | b << 16 | c << 8 | d;
	}
    }

    SDL_UnlockSurface(src);
    SDL_UnlockSurface(dst);
}

void ONScripterLabel::setupAnimationInfo( AnimationInfo *anim, FontInfo *info )
{
    anim->deleteSurface();
    anim->abs_flag = true;

    if ( anim->trans_mode == AnimationInfo::TRANS_STRING ){
        FontInfo f_info = sentence_font;
        if (info) f_info = *info;

	// handle private-use encodings
	{
	    std::string dest;
	    const char* buf = anim->file_name;
	    char ch = *buf;
	    if (ch == '^') {
		dest.push_back(ch);
		ch = *++buf;
	    }
	    while (ch) {
		char b2[5];
		if (ch == '~' && (ch = *++buf) != '~') {
		    while (ch != '~') {
			int l;
			TranslateTag(buf, b2, l);
			dest.append(b2);
			buf += l;
			ch = *buf;
		    }
		    ch = *++buf;
		    continue;
		}
		const unsigned short uc = UnicodeOfUTF8(buf);
		buf += CharacterBytes(buf);
		UTF8OfUnicode(uc, b2);
		dest.append(b2);
		ch = *buf;
	    }
	    setStr(&anim->file_name, dest.c_str());
	}

        if ( anim->font_size_x >= 0 ){ // in case of Sprite, not rclick menu
            f_info.top_x = anim->pos.x * screen_ratio2 / screen_ratio1;
            f_info.top_y = anim->pos.y * screen_ratio2 / screen_ratio1;
            f_info.clear();
            f_info.style = Default;
           
            f_info.set_size(anim->font_size_y);
            f_info.set_mod_size(0);
            if ( anim->font_pitch >= 0 )
                f_info.pitch_x = anim->font_pitch;
            if (anim->is_single_line) {
            	f_info.area_x = int(ceil(f_info.StringAdvance(anim->file_name)));
            	f_info.area_y = f_info.line_space();
            }
            if (anim->is_centered_text) {
            	anim->pos.x -= f_info.area_x / 2;
            	f_info.top_x = anim->pos.x * screen_ratio2 / screen_ratio1;
            }
        }

        SDL_Rect pos;
        if (anim->is_tight_region){
	    drawString( anim->file_name, anim->color_list[ anim->current_cell ], &f_info, false, NULL, &pos );
        }
        else{
            pos = f_info.getFullArea(screen_ratio1, screen_ratio2);
        }
        
        if (info != NULL){
            info->SetXY(f_info.GetXOffset(), f_info.GetYOffset());
        }
        
        anim->allocImage( pos.w*anim->num_of_cells, pos.h );
        anim->fill( 0, 0, 0, 0 );
        
        f_info.top_x = f_info.top_y = 0;
        for ( int i=0 ; i<anim->num_of_cells ; i++ ){
            f_info.clear();
            f_info.style = Default;
            drawString( anim->file_name, anim->color_list[i], &f_info, false, NULL, NULL, anim );
            f_info.top_x += anim->pos.w * screen_ratio2 / screen_ratio1;
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
        else if ( buffer[0] == 's' || buffer[0] == 'S' ){
            anim->trans_mode = AnimationInfo::TRANS_STRING;
            anim->is_centered_text = buffer[0] == 'S';
            buffer++;
            anim->num_of_cells = 0;
            if ( *buffer == '/' ){
                buffer++;
                script_h.getNext();
                
                script_h.pushCurrent( buffer );
                anim->font_size_x = script_h.readInt();
                anim->font_size_y = script_h.readInt();
                anim->font_pitch = script_h.readInt();
                if ( script_h.getEndStatus() & ScriptHandler::END_COMMA ){
                    script_h.readInt(); // 0 ... normal, 1 ... no anti-aliasing, 2 ... Fukuro
                }
                buffer = script_h.getNext();
                script_h.popCurrent();
            }
            else{
                anim->font_size_x = sentence_font.size();
                anim->font_size_y = sentence_font.size();
                anim->font_pitch = sentence_font.pitch_x;
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
            fprintf( stderr, "PonscripterLabel::parseTaggedString  The number of cells is 0\n");
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
        poly_rect.x += int(floor(sentence_font.GetX() * screen_ratio1 / screen_ratio2));
        poly_rect.y += sentence_font.GetY() * screen_ratio1 / screen_ratio2;
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
        dst_rect.x += int(floor(sentence_font.GetX() * screen_ratio1 / screen_ratio2));
        dst_rect.y += sentence_font.GetY() * screen_ratio1 / screen_ratio2;
    }

    flushDirect( dst_rect, refreshMode() );
}
