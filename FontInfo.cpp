/* -*- C++ -*-
 * 
 *  FontInfo.cpp - Font information storage class of Ponscripter
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this
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

#include "FontInfo.h"
#include "utf8_util.h"
#include "BaseReader.h"
#include "ScriptHandler.h"
#include "resources.h"
#include <stdio.h>
#include <string>
#include <math.h>

int screen_ratio1 = 1, screen_ratio2 = 1;

int FontInfo::default_encoding = 0;

static class FontsStruct {
    friend void MapFont(int, const char*);
    friend void MapMetrics(int, const char*);
    static const int count = 8;
    std::string mapping[count];
    std::string metrics[count];
    Font* font_[count];
public:
    Font* font(int style);

    FontsStruct() {
	char fn[32];
	for (int i = 0; i < count; ++i) {
	    font_[i] = NULL;
	    sprintf(fn, "face%d.ttf", i);
	    mapping[i] = fn;
	}
    }
    ~FontsStruct();
} Fonts;

FontsStruct::~FontsStruct()
{
    for (int i = 0; i < count; ++i) {
	if (font_[i]) delete font_[i];
    }
}

void MapFont(int id, const char* filename) {
    Fonts.mapping[id] = filename;
}

void MapMetrics(int id, const char* filename) {
    Fonts.metrics[id] = filename;
}

Font* FontsStruct::font(int style)
{
    if (font_[style]) return font_[style];
    size_t len;
    FILE* fp = fopen(mapping[style].c_str(), "rb");
    if (fp) {
	fclose(fp);
	const char* metnam = NULL;
	if (!metrics[style].empty()) {
	    fp = fopen(metrics[style].c_str(), "rb");
	    if (fp) {
		metnam = metrics[style].c_str();
		fclose(fp);
	    }
	}
	font_[style] = new Font(mapping[style].c_str(), metnam);
    }
    else if ((len = ScriptHandler::cBR->getFileLength(mapping[style].c_str()))) {
	Uint8 *data = new Uint8[len], *mdat = NULL;
	ScriptHandler::cBR->getFile(mapping[style].c_str(), data);
	size_t mlen = 0;
	if (!metrics[style].empty() && (mlen = ScriptHandler::cBR->getFileLength(metrics[style].c_str()))) {
	    mdat = new Uint8[mlen];
	    ScriptHandler::cBR->getFile(metrics[style].c_str(), mdat);
	}
	font_[style] = new Font(data, len, mdat, mlen);
    }
    else {
	const InternalResource *fres, *mres = NULL;
	fres = getResource(mapping[style].c_str());
	if (!metrics[style].empty()) mres = getResource(metrics[style].c_str());
	if (fres) font_[style] = new Font(fres, mres);
    }
    if (font_[style]) {
	font_[style]->set_size(26);
	return font_[style];
    }
    fprintf(stderr, "Error: failed to open font %s\n", mapping[style].c_str());
    exit(1);
}

Font* FontInfo::font()
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
	
    font_size = 26;
	
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
    font_size_mod = 0;
}

float FontInfo::em_width()
{
    doSize();
    return font()->advance('M');
}

int FontInfo::line_space()
{
    doSize();
    return font()->lineskip();
}

float FontInfo::GlyphAdvance(unsigned short unicode, unsigned short next)
{
    if (unicode >= 0x10 && unicode < 0x20) return 0;
    doSize();
    float adv = font()->advance(unicode);
#ifdef KERNING
    if (next) adv += font()->kerning(unicode, next);
#endif
    return adv + float(pitch_x);
}

int get_int(const char* text)
{
    int c1 = text[1], c2 = text[2];
    if (c1 == -1) c1 = 0;
    if (c2 == -1) c2 = 0;
    return c1 | (c2 << 7);
}

bool FontInfo::processCode(const char* text)
{
    if (*text >= 0x10 && *text < 0x20) {
	switch (*text) {
	case 0x10: style  =  Default; return true;
	case 0x11: style &= ~Italic;  return true;
	case 0x12: style ^=  Italic;  return true;
	case 0x13: style &= ~Bold;    return true;
	case 0x14: style ^=  Bold;    return true;
	case 0x15: style &= ~Sans;    return true;
	case 0x16: style ^=  Sans;    return true;
	case 0x17: font_size_mod = get_int(text); return true;
	case 0x18: font_size_mod = size() + get_int(text) - 8192; return true;
	case 0x19: font_size_mod = font_size * get_int(text) / 100; return true;
	case 0x1a: pos_x += get_int(text) - 8192; return true;
	case 0x1b: pos_x  = get_int(text); return true;
	case 0x1c: pos_y += get_int(text) - 8192; return true;
	case 0x1d: pos_y  = get_int(text); return true;
	case 0x1e: style  = get_int(text); return true;
	case 0x1f: 
	    switch (text[1]) {
	    case 0x10: indent = pos_x; return true;
	    case 0x11: indent = 0; return true;
	    }
	}
    }
    return false;
}

float FontInfo::StringAdvance(const char* string) 
{
    doSize();
    unsigned short unicode, next;
    float orig_x = pos_x;
    int orig_mod = font_size_mod, orig_style = style, orig_y = pos_y;
    unicode = UnicodeOfUTF8(string);
    while (*string) {
	int cb = CharacterBytes(string);
	next = UnicodeOfUTF8(string + cb);
	if (!processCode(string)) pos_x += GlyphAdvance(unicode, next);
	unicode = next;
	string += cb;
    }
    float rv = pos_x - orig_x;
    font_size_mod = orig_mod;
    style = orig_style;
    pos_x = orig_x;
    pos_y = orig_y;
    return rv;
}

void FontInfo::SetXY( float x, int y )
{
    if ( x != -1 ) pos_x = x;
    if ( y != -1 ) pos_y = y;
}

void FontInfo::newLine()
{
    doSize();
    pos_x = indent;
    pos_y += line_space() + pitch_y;
}

void FontInfo::setLineArea(int num)
{
    doSize();
    area_x = num;
    area_y = line_space();
}

bool FontInfo::isNoRoomFor(float margin)
{
    return pos_x + margin >= area_x;
}

bool FontInfo::isLineEmpty()
{
    return pos_x == indent;
}

void FontInfo::advanceBy(float offset)
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

SDL_Rect FontInfo::calcUpdatedArea(float start_x, int start_y, int ratio1, int ratio2)
{
    doSize();
    SDL_Rect rect;
	
    if (start_y == pos_y){ 
	// if single line, return minimum width
	rect.x = top_x + (long) floor(start_x);
	rect.w = (long) ceil(pos_x - start_x);
    }
    else{
	// multi-line: return full width
	rect.x = top_x;
	rect.w = area_x;
    }
    rect.y = top_y + start_y;
    rect.h = pos_y - start_y + line_space();

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

int FontInfo::doSize() { 
    const int sz = size();
    font()->set_size(sz);
    return sz;
}
