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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EMBOLDEN
#define EMBOLDEN false
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_ALLOCA
#define ALLOCA(n) ((void*)alloca(n))
#define FREEA(p)
#else
#define ALLOCA(n) malloc(n)
#define FREEA(p) free(p)
#endif

#include <SDL.h>
#include <SDL_endian.h>
#include "ttf.h"

/* FIXME: Right now we assume the gray-scale renderer Freetype is using
   supports 256 shades of gray, but we should instead key off of num_grays
   in the result FT_Bitmap after the FT_Render_Glyph() call. */
#define NUM_GRAYS       256

#define CACHED_METRICS	0x10
#define CACHED_BITMAP	0x01
#define CACHED_PIXMAP	0x02


/* The FreeType font engine/library */
static FT_Library library;
static int TTF_initialized = 0;
static int TTF_byteswapped = 0;

/* UNICODE string utilities */
static __inline__ int UNICODE_strlen(const Uint16 *text)
{
	int size = 0;
	while ( *text++ ) {
		++size;
	}
	return size;
}
static __inline__ void UNICODE_strcpy(Uint16 *dst, const Uint16 *src, int swap)
{
	if ( swap ) {
		while ( *src ) {
			*dst = SDL_Swap16(*src);
			++src;
			++dst;
		}
		*dst = '\0';
	} else {
		while ( *src ) {
			*dst = *src;
			++src;
			++dst;
		}
		*dst = '\0';
	}
}

/* This function tells the library whether UNICODE text is generally
   byteswapped.  A UNICODE BOM character at the beginning of a string
   will override this setting for that string.
 */
void TTF_ByteSwappedUNICODE(int swapped)
{
	TTF_byteswapped = swapped;
}

static void TTF_SetFTError(const char *msg, FT_Error error)
{
	TTF_SetError(msg);
}

int TTF_Init( void )
{
	int status = 0;

	if ( ! TTF_initialized ) {
		FT_Error error = FT_Init_FreeType( &library );
		if ( error ) {
			TTF_SetFTError("Couldn't init FreeType engine", error);
			status = -1;
		}
	}
	if ( status == 0 ) {
		++TTF_initialized;
	}
	return status;
}

static unsigned long RWread(
	FT_Stream stream,
	unsigned long offset,
	unsigned char* buffer,
	unsigned long count
)
{
	SDL_RWops *src;

	src = (SDL_RWops *)stream->descriptor.pointer;
	SDL_RWseek( src, (int)offset, SEEK_SET );
	if ( count == 0 ) {
		return 0;
	}
	return SDL_RWread( src, buffer, 1, (int)count );
}

TTF_Font* TTF_OpenFontIndexRW( SDL_RWops *src, int freesrc, long index )
{
	TTF_Font* font;
	FT_Error error;
	FT_Stream stream;
	int position;

	if ( ! TTF_initialized ) {
		TTF_SetError( "Library not initialized" );
		return NULL;
	}

	/* Check to make sure we can seek in this stream */
	position = SDL_RWtell(src);
	if ( position < 0 ) {
		TTF_SetError( "Can't seek in stream" );
		return NULL;
	}

	font = (TTF_Font*) malloc(sizeof *font);
	if ( font == NULL ) {
		TTF_SetError( "Out of memory" );
		return NULL;
	}
	memset(font, 0, sizeof(*font));

	font->currsize = 0;
	font->src = src;
	font->freesrc = freesrc;

	stream = (FT_Stream)malloc(sizeof(*stream));
	if ( stream == NULL ) {
		TTF_SetError( "Out of memory" );
		TTF_CloseFont( font );
		return NULL;
	}
	memset(stream, 0, sizeof(*stream));

	stream->memory = NULL;
	stream->read = RWread;
	stream->descriptor.pointer = src;
	stream->pos = (unsigned long)position;
	SDL_RWseek(src, 0, SEEK_END);
	stream->size = (unsigned long)(SDL_RWtell(src) - position);
	SDL_RWseek(src, position, SEEK_SET);

	font->args.flags = FT_OPEN_STREAM;
	font->args.stream = stream;

	error = FT_Open_Face( library, &font->args, index, &font->face );
	if( error ) {
		TTF_SetFTError( "Couldn't load font file", error );
		TTF_CloseFont( font );
		return NULL;
	}

	return font;
}


TTF_Font* TTF_OpenFontRW( SDL_RWops *src, int freesrc )
{
	return TTF_OpenFontIndexRW(src, freesrc, 0);
}

TTF_Font* TTF_OpenFontIndex( const char *file, long index )
{
	SDL_RWops *rw = SDL_RWFromFile(file, "rb");
	if ( rw == NULL ) {
		TTF_SetError(SDL_GetError());
		return NULL;
	}
	return TTF_OpenFontIndexRW(rw, 1, index);
}

TTF_Font* TTF_OpenFont( const char *file )
{
	return TTF_OpenFontIndex(file, 0);
}

static void Flush_Glyph( c_glyph* glyph )
{
	glyph->stored = 0;
	glyph->index = 0;
	if( glyph->bitmap.buffer ) {
		free( glyph->bitmap.buffer );
		glyph->bitmap.buffer = 0;
	}
	if( glyph->pixmap.buffer ) {
		free( glyph->pixmap.buffer );
		glyph->pixmap.buffer = 0;
	}
	glyph->cached = 0;
}
	
static void Flush_Cache( TTF_Font* font )
{
	int i;
	int size = sizeof( font->cache ) / sizeof( font->cache[0] );

	for( i = 0; i < size; ++i ) {
		if( font->cache[i].cached ) {
			Flush_Glyph( &font->cache[i] );
		}

	}
	if( font->scratch.cached ) {
		Flush_Glyph( &font->scratch );
	}

}

static FT_Error Load_Glyph( TTF_Font* font, Uint16 ch, c_glyph* cached, int want )
{
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( !font || !font->face ) {
		return FT_Err_Invalid_Handle;
	}

	face = font->face;

	/* Load the glyph */
	if ( ! cached->index ) {
		cached->index = FT_Get_Char_Index( face, ch );
	}
	error = FT_Load_Glyph( face, cached->index, /*FT_LOAD_NO_HINTING*/ FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT );
	if( error ) {
		return error;
	}

	/* Get our glyph shortcuts */
	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	/* Get the glyph metrics if desired */
	if ( (want & CACHED_METRICS) && !(cached->stored & CACHED_METRICS) ) {
		/* Get the bounding box */
		cached->minx = FT_FLOOR(metrics->horiBearingX);
		cached->maxx = cached->minx + FT_CEIL(metrics->width);
		cached->maxy = FT_FLOOR(metrics->horiBearingY);
		cached->miny = cached->maxy - FT_CEIL(metrics->height);
		cached->yoffset = font->ascent() - cached->maxy;
		cached->advance = FT_CEIL(metrics->horiAdvance);
		cached->stored |= CACHED_METRICS;
	}

	if ( (want & CACHED_PIXMAP) && !(cached->stored & CACHED_PIXMAP) ) {
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;

		/* Render the glyph */
		error = FT_Render_Glyph( glyph, FT_RENDER_MODE_LIGHT );
		if( error ) {
			return error;
		}

		/* Copy over information to cache */
		src = &glyph->bitmap;
		dst = &cached->pixmap;
		memcpy( dst, src, sizeof( *dst ) );

		if (dst->rows != 0) {
			dst->buffer = (unsigned char *)malloc( dst->pitch * dst->rows );
			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;
				memcpy(dst->buffer+doffset,
				       src->buffer+soffset, src->pitch);
			}
		}

		/* Handle the bold style */
		if (EMBOLDEN) {
			int row;
			int col;
			Uint8* pixmap;

			/* The pixmap is a little hard, we have to add and clamp */
			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (Uint8*) dst->buffer + row * dst->pitch;
				for( col = dst->width - 1; col > 0; --col ) {
					const double px = (sqrt(pixmap[col]) * 16.0 + (double) pixmap[col]) / 2;
					pixmap[col] = (Uint8) px;
				}
			}
		}

		/* Mark that we rendered this format */
		cached->stored |= CACHED_PIXMAP;
	}

	/* We're done, mark this glyph cached */
	cached->cached = ch;

	return 0;
}

static FT_Error Find_Glyph( TTF_Font* font, Uint16 ch, int want )
{
	int retval = 0;

	if( ch < 256 ) {
		font->current = &font->cache[ch];
	} else {
		if ( font->scratch.cached != ch ) {
			Flush_Glyph( &font->scratch );
		}
		font->current = &font->scratch;
	}
	if ( (font->current->stored & want) != want ) {
		retval = Load_Glyph( font, ch, font->current, want );
	}
	return retval;
}

void TTF_CloseFont( TTF_Font* font )
{
	Flush_Cache( font );
	if ( font->face ) {
		FT_Done_Face( font->face );
	}
	if ( font->args.stream ) {
		free( font->args.stream );
	}
	if ( font->freesrc ) {
		SDL_RWclose( font->src );
	}
	free( font );
}

char *TTF_FontFaceFamilyName(TTF_Font *font)
{
	return(font->face->family_name);
}

char *TTF_FontFaceStyleName(TTF_Font *font)
{
	return(font->face->style_name);
}

int TTF_GlyphMetrics(TTF_Font *font, Uint16 ch,
                     int* minx, int* maxx, int* miny, int* maxy, int* advance)
{
	FT_Error error;

	error = Find_Glyph(font, ch, CACHED_METRICS);
	if ( error ) {
		TTF_SetFTError("Couldn't find glyph", error);
		return -1;
	}

	if ( minx ) {
		*minx = font->current->minx;
	}
	if ( maxx ) {
		*maxx = font->current->maxx;
	}
	if ( miny ) {
		*miny = font->current->miny;
	}
	if ( maxy ) {
		*maxy = font->current->maxy;
	}
	if ( advance ) {
		*advance = font->current->advance;
	}
	return 0;
}

SDL_Surface* TTF_RenderGlyph_Shaded( TTF_Font* font,
				     Uint16 ch,
				     SDL_Color fg,
				     SDL_Color bg )
{
	SDL_Surface* textbuf;
	SDL_Palette* palette;
	int index;
	int rdiff;
	int gdiff;
	int bdiff;
	Uint8* src;
	Uint8* dst;
	int row;
	FT_Error error;
	c_glyph* glyph;

	/* Get the glyph itself */
	error = Find_Glyph(font, ch, CACHED_METRICS|CACHED_PIXMAP);
	if( error ) {
		return NULL;
	}
	glyph = font->current;

	/* Create the target surface */
	textbuf = SDL_CreateRGBSurface( SDL_SWSURFACE,
					glyph->pixmap.width,
					glyph->pixmap.rows,
					8, 0, 0, 0, 0 );
	if( !textbuf ) {
		return NULL;
	}

	/* Fill the palette with NUM_GRAYS levels of shading from bg to fg */
	palette = textbuf->format->palette;
	rdiff = fg.r - bg.r;
	gdiff = fg.g - bg.g;
	bdiff = fg.b - bg.b;
	for( index = 0; index < NUM_GRAYS; ++index ) {
		palette->colors[index].r = bg.r + (index*rdiff) / (NUM_GRAYS-1);
		palette->colors[index].g = bg.g + (index*gdiff) / (NUM_GRAYS-1);
		palette->colors[index].b = bg.b + (index*bdiff) / (NUM_GRAYS-1);
	}

	/* Copy the character from the pixmap */
	src = glyph->pixmap.buffer;
	dst = (Uint8*) textbuf->pixels;
	for ( row = 0; row < textbuf->h; ++row ) {
		memcpy( dst, src, glyph->pixmap.pitch );
		src += glyph->pixmap.pitch;
		dst += textbuf->pitch;
	}

	return textbuf;
}

SDL_Surface *TTF_RenderGlyph_Blended(TTF_Font *font, Uint16 ch, SDL_Color fg)
{
	SDL_Surface *textbuf;
	Uint32 alpha;
	Uint32 pixel;
	Uint8 *src;
	Uint32 *dst;
	int row, col;
	FT_Error error;
	c_glyph *glyph;

	/* Get the glyph itself */
	error = Find_Glyph(font, ch, CACHED_METRICS|CACHED_PIXMAP);
	if ( error ) {
		return(NULL);
	}
	glyph = font->current;

	textbuf = SDL_CreateRGBSurface(SDL_SWSURFACE,
	              glyph->pixmap.width, glyph->pixmap.rows, 32,
                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if ( ! textbuf ) {
		return(NULL);
	}

	/* Copy the character from the pixmap */
	pixel = (fg.r<<16)|(fg.g<<8)|fg.b;
	SDL_FillRect(textbuf, NULL, pixel);	/* Initialize with fg and 0 alpha */

	for ( row=0; row<textbuf->h; ++row ) {
		/* Changed src to take pitch into account, not just width */
		src = glyph->pixmap.buffer + row * glyph->pixmap.pitch;
		dst = (Uint32 *)textbuf->pixels + row * textbuf->pitch/4;
		for ( col=0; col<glyph->pixmap.width; ++col ) {
			alpha = *src++;
			*dst++ = pixel | (alpha << 24);
		}
	}

	return(textbuf);
}

void TTF_SetSize( TTF_Font* font, int ptsize )
{
	if (!font) fprintf(stderr, "TTF_SetSize: font not initialised\n"); else
	if (ptsize != font->currsize) {
		Flush_Cache( font );
		/* Set the character size, in pixels */
		FT_Set_Char_Size( font->face, 0, ptsize * 64, 0, 0 );
		font->currsize = ptsize;
	}
}

void TTF_Quit( void )
{
	if ( TTF_initialized ) {
		if ( --TTF_initialized == 0 ) {
			FT_Done_FreeType( library );
		}
	}
}

int TTF_WasInit( void )
{
	return TTF_initialized;
}
