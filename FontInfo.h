/* -*- C++ -*-
 *
 *  FontInfo.h - Font information storage class of Ponscripter
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

#ifndef __FONT_INFO_H__
#define __FONT_INFO_H__

#include <SDL.h>
#include "defs.h"
#include "font.h"

extern int screen_ratio1, screen_ratio2;

extern void MapFont(int id, const string& filename);
extern void MapMetrics(int id, const string& filename);

void InitialiseFontSystem(const string& basepath);

class FontInfo {
    float indent;
    float pos_x; int pos_y; // Current position
    int   font_size, font_size_mod;
    bool  is_vertical;
public:
    static int default_encoding;

    Font* font();

    rgb_t color;
    rgb_t on_color, off_color, nofile_color;
    int   top_x, top_y; // Top left origin
    int   area_x, area_y; // Size of the text windows
    int   pitch_x, pitch_y; // additional spacing
    int   wait_time;
    bool  is_bold;
    bool  is_shadow;
    bool  is_transparent;
    bool  is_newline_accepted;
    rgb_t window_color;

    int size() { return font_size_mod ? font_size_mod : font_size; }
    int base_size() { return font_size; }
    int mod_size() { return font_size_mod; }
    void set_size(int val) { font_size = val; }
    void set_mod_size(int val) { font_size_mod = val; }

    void setTateYoko(bool vertical) { is_vertical = vertical; clear(); }
    
    int style;

    float em_width();
    int line_space();

    int line_top(int line_number) {
        return (line_space() + pitch_y) * line_number;
    }

    void SetIndent(const unsigned short indent_char) {
        indent = GlyphAdvance(indent_char, 0);
    }

    void ClearIndent() { indent = 0; }

    FontInfo();
    void reset();

    float GetXOffset() const { return pos_x; }
    int GetYOffset() const { return pos_y; }
    float GetX() const { return pos_x + float (top_x); }
    int GetY() const { return pos_y + top_y; };

    void SetXY(float x = -1, int y = -1);
    void clear();
    void newLine();
    void setLineArea(int num);

    float GlyphAdvance(unsigned short unicode, unsigned short next = 0);
    float StringAdvance(const char* string);
    float StringAdvance(const string& s) {
	return StringAdvance(s.c_str());
    }

    bool isNoRoomFor(float margin = 0.0);
    bool isNoRoomForLines(int margin);
    bool isLineEmpty();
    bool processCode(const char* text);
    void advanceBy(float offset);

    SDL_Rect getFullArea(int ratio1, int ratio2);

    SDL_Rect calcUpdatedArea(float start_x, int start_y,
			     int ratio1, int ratio2);
    void addShadeArea(SDL_Rect &rect, int shade_distance[2]);

    int doSize();
};

#endif // __FONT_INFO_H__
