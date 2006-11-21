/*
	BASED ON:

    SDL_ttf:  A companion library to SDL for working with TrueType (tm) fonts
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/*
	Heavily modified September 2006 by Peter Jolly.  This file is in no
	way compatible with or to be mistaken for the regular SDL_ttf library,
	and should not be reused outside Ponscripter if you're just looking
	for SDL_ttf -- use the real SDL_ttf library instead.
*/

#ifndef _SDL_TTF_H
#define _SDL_TTF_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_IDS_H

#include <SDL.h>

/* Cached glyph information */
struct c_glyph {
	int stored;
	FT_UInt index;
	FT_Bitmap bitmap;
	FT_Bitmap pixmap;
	int minx;
	int maxx;
	int miny;
	int maxy;
	int yoffset;
	int advance;
	Uint16 cached;
};

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

/* The structure used to hold internal font information */
struct TTF_Font {
	FT_Face face;
	/* Cache for style-transformed glyphs */
	c_glyph *current;
	c_glyph cache[256];
	c_glyph scratch;
	/* We are responsible for closing the font stream */
	SDL_RWops *src;
	int freesrc;
	FT_Open_Args args;
	
	int currsize;
	bool embolden;
	
	FT_Face& get_face() { return face; }
	int ascent() { return FT_CEIL(FT_MulFix(face->ascender, face->size->metrics.y_scale)); }
	int descent() { return FT_CEIL(FT_MulFix(face->descender, face->size->metrics.y_scale)); }
	int height() { return ascent() - descent() + 1; }
	int lineskip() { return FT_CEIL(FT_MulFix(face->height, face->size->metrics.y_scale)); }
	bool fixed() { return FT_IS_FIXED_WIDTH(face); }
};

/* ZERO WIDTH NO-BREAKSPACE (Unicode byte order mark) */
#define UNICODE_BOM_NATIVE	0xFEFF
#define UNICODE_BOM_SWAPPED	0xFFFE

/* This function tells the library whether UNICODE text is generally
   byteswapped.  A UNICODE BOM character in a string will override
   this setting for the remainder of that string.
*/
void SDLCALL TTF_ByteSwappedUNICODE(int swapped);

/* Initialize the TTF engine - returns 0 if successful, -1 on error */
int SDLCALL TTF_Init(void);

void TTF_SetSize( TTF_Font* font, int ptsize );

/* Open a font file. */
TTF_Font * SDLCALL TTF_OpenFont(const char *file);
TTF_Font * SDLCALL TTF_OpenFontIndex(const char *file, long index);
TTF_Font * SDLCALL TTF_OpenFontRW(SDL_RWops *src, int freesrc);
TTF_Font * SDLCALL TTF_OpenFontIndexRW(SDL_RWops *src, int freesrc, long index);

/* Get the font face attributes, if any */
int SDLCALL TTF_FontFaceIsFixedWidth(TTF_Font *font);
char * SDLCALL TTF_FontFaceFamilyName(TTF_Font *font);
char * SDLCALL TTF_FontFaceStyleName(TTF_Font *font);

/* Get the metrics (dimensions) of a glyph
   To understand what these metrics mean, here is a useful link:
    http://freetype.sourceforge.net/freetype2/docs/tutorial/step2.html
 */
int SDLCALL TTF_GlyphMetrics(TTF_Font *font, Uint16 ch,
				     int *minx, int *maxx,
                                     int *miny, int *maxy, int *advance);

/* Create an 8-bit palettized surface and render the given glyph at
   high quality with the given font and colors.  The 0 pixel is background,
   while other pixels have varying degrees of the foreground color.
   The glyph is rendered without any padding or centering in the X
   direction, and aligned normally in the Y direction.
   This function returns the new surface, or NULL if there was an error.
*/
SDL_Surface * SDLCALL TTF_RenderGlyph_Shaded(TTF_Font *font,
				Uint16 ch, SDL_Color fg, SDL_Color bg);

/* Create a 32-bit ARGB surface and render the given glyph at high quality,
   using alpha blending to dither the font with the given color.
   The glyph is rendered without any padding or centering in the X
   direction, and aligned normally in the Y direction.
   This function returns the new surface, or NULL if there was an error.
*/
SDL_Surface * SDLCALL TTF_RenderGlyph_Blended(TTF_Font *font,
						Uint16 ch, SDL_Color fg);

/* Close an opened font file */
void SDLCALL TTF_CloseFont(TTF_Font *font);

/* De-initialize the TTF engine */
void SDLCALL TTF_Quit(void);

/* Check if the TTF engine is initialized */
int SDLCALL TTF_WasInit(void);

/* We'll use SDL for reporting errors */
#define TTF_SetError	SDL_SetError
#define TTF_GetError	SDL_GetError

#endif /* _SDL_TTF_H */
