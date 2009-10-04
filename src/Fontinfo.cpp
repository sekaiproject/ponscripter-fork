/* -*- C++ -*-
 *
 *  Fontinfo.cpp - Font information storage class of Ponscripter
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this
 *  is a fork).
 *
 *  Most of this particular class has in fact been rewritten largely
 *  from scratch.
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

#include "Fontinfo.h"
#include "encoding.h"
#include "BaseReader.h"
#include "ScriptHandler.h"
#include "resources.h"
#include <math.h>

int screen_ratio1 = 1, screen_ratio2 = 1;

int Fontinfo::default_encoding = 0;

static class FontsStruct {
    friend void MapFont(int, const pstring&);
    friend void MapMetrics(int, const pstring&);

    static const int count = 8;
    bool isinit;
    pstring path, fallback;
    pstring mapping[count];
    pstring metrics[count];
    Font* font_[count];
public:
    Font* font(int style);

    void init(const pstring& basepath)
    {
	isinit = true;
	path = basepath;
	fallback = basepath + "default.ttf";
        for (int i = 0; i < count; ++i) {
            font_[i] = NULL;
            mapping[i].format("face%d.ttf", i);
        }
    }

    FontsStruct() : isinit(false) {}
    ~FontsStruct();
} Fonts;

FontsStruct::~FontsStruct()
{
    for (int i = 0; i < count; ++i) {
        if (font_[i]) delete font_[i];
    }
    FontFinished();
}

void InitialiseFontSystem(const pstring& basepath)
{
    FontInitialise();
    Fonts.init(basepath);
}

void MapFont(int id, const pstring& filename)
{
    Fonts.mapping[id] = filename;
}


void MapMetrics(int id, const pstring& filename)
{
    Fonts.metrics[id] = filename;
}


Font* FontsStruct::font(int style)
{
    if (!isinit) {
	fprintf(stderr, "ERROR: fonts struct not initialised\n");
	exit(1);
    }
    
    if (font_[style]) return font_[style];

    size_t len;
    pstring fpath = path + mapping[style];
    FILE* fp = fopen(fpath, "rb");
    if (fp) {
        fclose(fp);
	pstring mpath;
        const char* metnam = NULL;
        if (metrics[style]) {
	    mpath = path + metrics[style];
            fp = fopen(mpath, "rb");
            if (fp) {
                metnam = mpath;
                fclose(fp);
            }
        }

        font_[style] = new Font(fpath, metnam);
    }
    else {
	fpath = path + "fonts" + DELIMITER + mapping[style];
	fp = fopen(fpath, "rb");
	if (fp) {
	    fclose(fp);
	    pstring mpath;
	    const char* metnam = NULL;
	    if (metrics[style]) {
		mpath = path + "fonts" + DELIMITER + metrics[style];
		fp = fopen(mpath, "rb");
		if (fp) {
		    metnam = mpath;
		    fclose(fp);
		}
	    }
	    font_[style] = new Font(fpath, metnam);
	}
	else if ((len = ScriptHandler::cBR->getFileLength(mapping[style]))) {
	    Uint8 *data = new Uint8[len], *mdat = NULL;
	    ScriptHandler::cBR->getFile(mapping[style], data);
	    size_t mlen = 0;
	    if (metrics[style] &&
		(mlen = ScriptHandler::cBR->getFileLength(metrics[style]))) {
		mdat = new Uint8[mlen];
		ScriptHandler::cBR->getFile(metrics[style], mdat);
	    }
	    
	    font_[style] = new Font(data, len, mdat, mlen);
	}
	else {
	    const InternalResource *fres, *mres = NULL;
	    fres = getResource(mapping[style]);
	    if (metrics[style]) mres = getResource(metrics[style]);
	    
	    if (fres) font_[style] = new Font(fres, mres);
	}
    }

    // Fall back on default.ttf if no font was specified and
    // face$STYLE.ttf was not found.
    if (!font_[style] && (fp = fopen(fallback, "rb"))) {
	fclose(fp);
	font_[style] = new Font(fallback, (const char*) NULL);
    }

    if (font_[style]) {
        font_[style]->set_size(26);
        return font_[style];
    }

    fprintf(stderr, "Error: failed to open font %s\n",
	    (const char*) mapping[style]);
    exit(1);
}


Font* Fontinfo::font() const
{
    return Fonts.font(style);
}


Fontinfo::Fontinfo()
{
    on_color.set(0xff);
    off_color.set(0xaa);
    nofile_color.set(0x55, 0x55, 0x99);
    reset();
}


void Fontinfo::reset()
{
    is_vertical = false;
    is_bidirect = false;
    clear();
    font_size = 26;
    color.set(0xff);
    is_bold   = true;
    is_shadow = true;
    is_transparent = true;
    is_newline_accepted = false;
}


void Fontinfo::clear()
{
    if (is_vertical)
        SetXY(area_x-size()-pitch_x, 0);
    else if (is_bidirect)
        SetXY(area_x, 0);
    else
        SetXY(0, 0);
    indent = 0;
    style  = default_encoding;
    font_size_mod = 0;
}


float Fontinfo::em_width()
{
    doSize();
    return font()->advance('M');
}


int Fontinfo::line_space()
{
    doSize();
    return font()->lineskip();
}


float Fontinfo::GlyphAdvance(unsigned short unicode, unsigned short next)
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


bool Fontinfo::processCode(const char* text)
{
    if (*text >= 0x10 && *text < 0x20) {
        switch (*text) {
        case 0x10: style  = Default; return true;
        case 0x11: style &= ~Italic; return true;
        case 0x12: style ^= Italic;  return true;
        case 0x13: style &= ~Bold;   return true;
        case 0x14: style ^= Bold;    return true;
        case 0x15: style &= ~Sans;   return true;
        case 0x16: style ^= Sans;    return true;
        case 0x17: font_size_mod = get_int(text); return true;
        case 0x18: font_size_mod = size() + get_int(text) - 8192; return true;
        case 0x19: font_size_mod = font_size* get_int(text) / 100; return true;
        case 0x1a: pos_x += get_int(text) - 8192; return true;
        case 0x1b: pos_x  = get_int(text); return true;
        case 0x1c: pos_y += get_int(text) - 8192; return true;
        case 0x1d: pos_y  = get_int(text); return true;
        case 0x1e: style  = get_int(text); return true;
        case 0x1f:
            switch (text[1]) {
            case 0x10: if (is_vertical)
                           indent = pos_y;
                       else if (is_bidirect)
                           indent = area_x - pos_x;
                       else
                           indent = pos_x;
                       return true;
            case 0x11: indent = 0; return true;
            }
        }
    }

    return false;
}


float Fontinfo::StringAdvance(const char* string)
{
    // This relates to display, so we take ligatures into account.
    doSize();
    wchar unicode, next;
    int cb, nextcb;
    float orig_x   = pos_x;
    int   orig_mod = font_size_mod, orig_style = style, orig_y = pos_y;
    unicode = file_encoding->DecodeWithLigatures(string, *this, cb);
    while (*string) {
        next = file_encoding->DecodeWithLigatures(string + cb, *this, nextcb);
        if (!processCode(string))
            if (is_bidirect)
                pos_x -= GlyphAdvance(unicode, next);
            else
                pos_x += GlyphAdvance(unicode, next);
        string += cb;
        unicode = next;
	cb = nextcb;
    }
    float rv = pos_x - orig_x;
    font_size_mod = orig_mod;
    style = orig_style;
    pos_x = orig_x;
    pos_y = orig_y;
    return rv;
}


void Fontinfo::SetXY(float x, int y)
{
    if (x != -1) pos_x = x;

    if (y != -1) pos_y = y;
}


void Fontinfo::newLine()
{
    doSize();
    if (is_vertical){
        pos_x -= size() + pitch_x;
        pos_y = indent;
    }
    else if (is_bidirect){
        pos_x = area_x - indent;
        pos_y += line_space() + pitch_y;
    }
    else{
        pos_x = indent;
        pos_y += line_space() + pitch_y;
    }
    //pos_x = indent;
    //pos_y += line_space() + pitch_y;
}


void Fontinfo::setLineArea(int num)
{
    doSize();
    if (is_vertical) {
        area_x = size();
        area_y = num;
    }
    else {
        area_x = num;
        area_y = line_space();
    }
}


bool Fontinfo::isNoRoomFor(float margin)
{
    if (is_vertical)
        return pos_y + margin > area_y;
    else if (is_bidirect)
        return pos_x - margin < 0;
    else
        return pos_x + margin > area_x;
}

bool Fontinfo::isNoRoomForLines(int margin)
{
    if (is_vertical)
        return pos_x - (size() + pitch_x) * margin < 0;
    else
        return pos_y + (line_space() + pitch_y) * margin > area_y;
}

bool Fontinfo::isLineEmpty()
{
    if (is_vertical)
        return pos_y <= indent;
    else if (is_bidirect)
        return pos_x >= (area_x - indent);
    else
        return pos_x <= indent;
}


void Fontinfo::advanceBy(float offset)
{
    if (is_vertical)
        pos_y += offset;
    else if (is_bidirect)
        pos_x -= offset;
    else
        pos_x += offset;
}


SDL_Rect Fontinfo::getFullArea(int ratio1, int ratio2)
{
    SDL_Rect rect;
    rect.x = top_x * ratio1 / ratio2;
    rect.y = top_y * ratio1 / ratio2;
    rect.w = area_x * ratio1 / ratio2;
    rect.h = area_y * ratio1 / ratio2;
    return rect;
}


SDL_Rect Fontinfo::calcUpdatedArea(float start_x, int start_y,
				   int ratio1, int ratio2)
{
    doSize();
    SDL_Rect rect;

    if (start_y == pos_y) {
        // if single line, return minimum width
        rect.x = top_x + (long) floor(start_x);
        rect.w = (long) ceil(pos_x - start_x);
    }
    else {
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


void Fontinfo::addShadeArea(SDL_Rect &rect, int shade_distance[2])
{
    if (is_shadow) {
        if (shade_distance[0] > 0)
            rect.w += shade_distance[0];
        else {
            rect.x += shade_distance[0];
            rect.w -= shade_distance[0];
        }

        if (shade_distance[1] > 0)
            rect.h += shade_distance[1];
        else {
            rect.y += shade_distance[1];
            rect.h -= shade_distance[1];
        }
    }
}


int Fontinfo::doSize()
{
    const int sz = size();
    font()->set_size(sz);
    return sz;
}
