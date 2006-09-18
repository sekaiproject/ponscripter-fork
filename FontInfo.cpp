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
#include "BaseReader.h"
#include "ScriptHandler.h"
#include <stdio.h>

char* font_file = NULL;
int screen_ratio1 = 1, screen_ratio2 = 1;

int FontInfo::default_encoding = 0;

class FontsStruct {
	static const int count = 8;
	unsigned char* data[count];
	TTF_Font* font_[count];
public:
	TTF_Font* font(int style);

	FontsStruct() { for (int i = 0; i < count; ++i) { font_[i] = NULL; data[i] = NULL; } }
	~FontsStruct();
} Fonts;

FontsStruct::~FontsStruct()
{
	for (int i = 0; i < count; ++i) {
		if (data[i]) delete[] data[i];
	}
}

TTF_Font* FontsStruct::font(int style)
{
	if (font_[style]) return font_[style];
	char fn[32];
	size_t len;
	sprintf(fn, "face%d.ttf", style);
	FILE* fp = fopen(fn, "rb");
	if (fp) {
		fclose(fp);
		font_[style] = TTF_OpenFont(fn);
	}
	else if ((len = ScriptHandler::cBR->getFileLength(fn))) {
		data[style] = new unsigned char[len];
		ScriptHandler::cBR->getFile(fn, data[style]);
		SDL_RWops* rwfont = SDL_RWFromMem(data[style], len);
		font_[style] = TTF_OpenFontRW(rwfont, 0);
	}
	if (font_[style]) {
		TTF_SetSize(font_[style], 16);
		return font_[style];
	}
	fprintf(stderr, "Error: failed to open font %s\n", fn);
	exit(1);
}

TTF_Font* FontInfo::font()
{
	return Fonts.font(style);
}

FontInfo::FontInfo()
{
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

void FontInfo::clear()
{
	SetXY(0, 0);
	indent = 0;
	style = default_encoding;
}

int FontInfo::em_width()
{
	doSize();
	int rv;
	TTF_GlyphMetrics(font(), 'M', NULL, NULL, NULL, NULL, &rv);
	return rv;
}
int FontInfo::line_space()
{
	doSize();
	return font()->lineskip();
}

int FontInfo::GlyphAdvance(unsigned short unicode, unsigned short next)
{
	if (unicode >= 0x10 && unicode < 0x20) return 0;
	doSize();
	int rv;
	TTF_GlyphMetrics(font(), unicode, NULL, NULL, NULL, NULL, &rv);
#ifdef KERNING
	if (next) {
		FT_Face& face = font()->face;
		FT_Vector kern;
		FT_Error err = FT_Get_Kerning(face, FT_Get_Char_Index(face, unicode), FT_Get_Char_Index(face, next), FT_KERNING_DEFAULT, &kern);
		if (!err) rv += kern.x >> 6;
	}
#endif
	return rv + pitch_x;
}

int FontInfo::StringAdvance(const char* string) 
{
	doSize();
	int rv = 0;
	unsigned short unicode, next;
	unicode = UnicodeOfUTF8(string);
	while (*string) {
		string += CharacterBytes(string);
		next = UnicodeOfUTF8(string);
		rv += GlyphAdvance(unicode, next);
		unicode = next;
	}
	return rv;
}

void FontInfo::SetXY( int x, int y )
{
	if ( x != -1 ) pos_x = x;
	if ( y != -1 ) pos_y = y;
}

void FontInfo::newLine(const float proportion)
{
	doSize();
	pos_x = indent;
	pos_y += (int)((float)(line_space() + pitch_y) * proportion);
}

void FontInfo::setLineArea(int num)
{
	doSize();
	area_x = num;
	area_y = line_space();
}

bool FontInfo::isNoRoomFor(int margin)
{
	return pos_x + margin >= area_x;
}

bool FontInfo::isLineEmpty()
{
	return pos_x == indent;
}

void FontInfo::advanceBy(int offset)
{
	pos_x += offset;
}

SDL_Rect FontInfo::getFullArea(int ratio1, int ratio2)
{
	SDL_Rect rect;
	rect.x = top_x * ratio1 / ratio2;
	rect.y = top_y * ratio1 / ratio2;
	rect.w = area_x * ratio1 / ratio2;
	rect.h = area_y * ratio1 / ratio2;
	return rect;
}

SDL_Rect FontInfo::calcUpdatedArea(int start_xy[2], int ratio1, int ratio2)
{
	doSize();
	SDL_Rect rect;
	
	if (start_xy[1] == pos_y){ 
		// if single line, return minimum width
		rect.x = top_x + start_xy[0];
		rect.w = pos_x - start_xy[0];
	}
	else{
		// multi-line: return full width
		rect.x = top_x;
		rect.w = area_x;
	}
	rect.y = top_y + start_xy[1];
	rect.h = pos_y - start_xy[1] + line_space();

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

