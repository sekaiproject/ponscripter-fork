/*
	C++ Freetype rendering code.
	Copyright 2006 Peter Jolly.

	Heavily based on, and much copied verbatim from, SDL_ttf, which is
	copyright (C) 1997-2004 Sam Lantinga.
	
	This library is free software; you can redistribute it and/or modify it 
	under the terms of the GNU General Public License as published by the Free 
	Software Foundation; either version 2 of the License, or (at your option) 
	any later version.
	
	This library is distributed in the hope that it will be useful, but WITHOUT 
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
	more details.
	
	You should have received a copy of the GNU General Public License along with 
	this library; if not, write to the Free Foundation, Inc., 59 Temple Place, 
	Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef FONT_H
#define FONT_H

#include <SDL.h>

enum HintingMode { NoHinting, LightHinting, FullHinting };

extern HintingMode hinting;
extern bool lightrender;
extern bool subpixel;

struct FontInternals;

class Font {
	FontInternals* priv;
public:
	Font(const char* filename, const char* metrics = NULL);
	Font(const Uint8* data, size_t len, const Uint8* mdat = NULL, size_t mlen = 0); // takes ownership of data and mdat
	~Font();

	void get_metrics(Uint16 ch, float* minx, float* maxx, float* miny, float* maxy);
	
	void set_size(int val);
	SDL_Surface* render_glyph(Uint16 ch, SDL_Color fg, SDL_Color bg, float x_fractional_part);
	
	int ascent();
	int lineskip();
	
	float advance(Uint16 ch);
	float kerning(Uint16 left, Uint16 right);
	
	bool has_char(Uint16 ch);
};

#endif
