/* -*- C++ -*-
 * 
 *  FontInfo.cpp - Font information storage class of PONScripter
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this is a fork).
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

#include "FontInfo.h"
#include "utf8_util.h"
#include <stdio.h>
#include <SDL_ttf.h>

#ifdef USE_INTERNAL_FONT
#include "resources.h"
#endif

char* font_file = NULL;
int screen_ratio1 = 1, screen_ratio2 = 1;

static struct FontContainer{
    FontContainer *next;
    int size;
    TTF_Font *font;

    FontContainer(){
        size = 0;
        next = NULL;
        font = NULL;
    };
} root_font_container;

FontInfo::FontInfo()
{
    ttf_font = NULL;

    reset();
}

void FontInfo::reset()
{
    clear();

    color[0]        = color[1]        = color[2]        = 0xff;
    on_color[0]     = on_color[1]     = on_color[2]     = 0xff;
    off_color[0]    = off_color[1]    = off_color[2]    = 0xaa;
    nofile_color[0] = 0x55;
    nofile_color[1] = 0x55;
    nofile_color[2] = 0x99;

    is_bold = true;
    is_shadow = true;
    is_transparent = true;
    is_newline_accepted = false;
}

void *FontInfo::openFont()
{
    int font_size = font_size_x < font_size_y ? font_size_x : font_size_y;

    FontContainer *fc = &root_font_container;
    while( fc->next ){
        if ( fc->next->size == font_size ) break;
        fc = fc->next;
    }
    if ( !fc->next ){
        fc->next = new FontContainer();
        fc->next->size = font_size;
        if ( font_file ) {
	        FILE *fp = fopen( font_file, "r" );
	        if ( fp == NULL ) return NULL;
	        fclose( fp );
	        fc->next->font = TTF_OpenFont( font_file, font_size * screen_ratio1 / screen_ratio2 );
	    }
	    else {
#ifdef USE_INTERNAL_FONT
	    	SDL_RWops* rwfont = SDL_RWFromConstMem( internal_font_buffer, internal_font_size );
	    	fc->next->font = TTF_OpenFontRW( rwfont, 0, font_size * screen_ratio1 / screen_ratio2 );
#else
			return NULL;
#endif
	    }
    }

	if (fc->next->font) {
		line_space_ = TTF_FontLineSkip(fc->next->font);
		TTF_GlyphMetrics(fc->next->font, 'M', NULL, NULL, NULL, NULL, &em_width_);
	}

    ttf_font = (void*)fc->next->font;
    
    return fc->next->font;
}

int FontInfo::em_width()
{
	bool was_open = ttf_font;
	if (!ttf_font) openFont();
	int rv = em_width_;
	if (!was_open) ttf_font = NULL;
	return rv;
}
int FontInfo::line_space()
{
	bool was_open = ttf_font;
	if (!ttf_font) openFont();
	int rv = line_space_;
	if (!was_open) ttf_font = NULL;
	return rv;
}

int FontInfo::GlyphAdvance(unsigned short unicode)
{
	int rv;
	bool was_open = ttf_font;
	if (!ttf_font) openFont();
	TTF_GlyphMetrics((TTF_Font*) ttf_font, unicode, NULL, NULL, NULL, NULL, &rv);
	if (!was_open) ttf_font = NULL;
	return rv + pitch_x;
}

int FontInfo::StringAdvance(const char* string) 
{
	int rv = 0;
	bool was_open = ttf_font;
	if (!ttf_font) openFont();
	while (*string) {
		unsigned short unicode = UnicodeOfUTF8(string);
		string += CharacterBytes(string);
		rv += GlyphAdvance(unicode);
	}
	if (!was_open) ttf_font = NULL;
	return rv;
}

int FontInfo::getRemainingLine()
{
    return area_y - pos_y;
}

int FontInfo::GetX()
{
    return pos_x + top_x;
}

int FontInfo::GetY()
{
    return pos_y * (line_space() + pitch_y) + top_y;
}

void FontInfo::SetXY( int x, int y )
{
    if ( x != -1 ) pos_x = x;
    if ( y != -1 ) pos_y = y;
}

void FontInfo::clear()
{
    SetXY(0, 0);
}

void FontInfo::newLine()
{
    pos_x = 0;
    pos_y += 1;
}

void FontInfo::setLineArea(int num)
{
    area_x = num;
    area_y = 1;
}

bool FontInfo::isNoRoomFor(int margin)
{
    return pos_x + margin >= area_x;
}

bool FontInfo::isLineEmpty()
{
    return pos_x == 0;
}

void FontInfo::advanceBy(int offset)
{
    pos_x += offset;
}


SDL_Rect FontInfo::calcUpdatedArea(int start_xy[2], int ratio1, int ratio2)
{
    SDL_Rect rect;
    
    if (start_xy[1] == pos_y){
        rect.x = top_x + start_xy[0];
        rect.w = pos_x - start_xy[0];
    }
    else{
        rect.x = top_x;
        rect.w = area_x;
    }
    const int lsp = line_space() + pitch_y;
    rect.y = top_y + start_xy[1] * lsp;
    rect.h = lsp * (pos_y - start_xy[1] + 2);

    rect.x = rect.x * ratio1 / ratio2;
    rect.y = rect.y * ratio1 / ratio2;
    rect.w = rect.w * ratio1 / ratio2;
    rect.h = rect.h * ratio1 / ratio2;
    
    return rect;
}

void FontInfo::addShadeArea(SDL_Rect &rect, int shade_distance[2])
{
    if (is_shadow){
        if (shade_distance[0]>0)
            rect.w += shade_distance[0];
        else{
            rect.x += shade_distance[0];
            rect.w -= shade_distance[0];
        }
        if (shade_distance[1]>0)
            rect.h += shade_distance[1];
        else{
            rect.y += shade_distance[1];
            rect.h -= shade_distance[1];
        }
    }
}

