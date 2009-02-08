/* -*- C++ -*-
 *
 *  encoding.h -- utility functions for handling Unicode text and
 *                ligatures in Ponscripter
 *
 *  Copyright (c) 2007 Peter Jolly
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

#ifndef __ENCODING_H__
#define __ENCODING_H__

typedef unsigned short wchar;

#include <stack>
#include "stdio.h"
#include "pstring.h"
class Fontinfo;

// Style bits
const int Default = 0;
const int Italic  = 1;
const int Bold    = 2;
const int Sans    = 4;

// Ligature control
void AddLigature(const pstring& in, wchar out);
void DeleteLigature(const pstring& in);
void DefaultLigatures(int which);
void ClearLigatures();

// For methods taking Fontinfo*, pass NULL if ligature information is not
// required, or a Fontinfo to get info on ligatures for that font.
class Encoding {
    const char textchar;
    const char* name;
    bool tagscurr;
protected:    
    virtual wchar Decode_impl(const char*, int&, const Fontinfo*) = 0;
    virtual int Charsz_impl(const char*, const Fontinfo*) = 0;
public:
    char TextMarker() const { return textchar; }
    bool UseTags() const { return tagscurr; }

    int NextCharSize(const char* str, const Fontinfo* fi = 0)
	{ return Charsz_impl(str, fi); }
    int NextCharSizeWithLigatures(const char* str, const Fontinfo* fi)
	{ return Charsz_impl(str, fi); }

    virtual int Encode(wchar input, char* output) = 0;
    virtual pstring Encode(wchar input) = 0;

    wchar DecodeChar(const char* str, const Fontinfo* fi = 0)
	{ int bytes; return Decode_impl(str, bytes, fi); }
    wchar DecodeChar(const char* str, int& bytes, const Fontinfo* fi = 0)
	{ return Decode_impl(str, bytes, fi); }
    wchar DecodeWithLigatures(const char* str, const Fontinfo& info,
			      int& bytes);
    wchar DecodeWithLigatures(const char* str, const Fontinfo& info)
	{ int bytes; return DecodeWithLigatures(str, info, bytes); }
    
    // Previous is O(1) for UTF-8, but may be O(n) for encodings like CP932.
    // It never takes ligatures into account.
    virtual const char* Previous(const char* currpos, const char* strstart) = 0;

    size_t CharacterCount(const char* str, const Fontinfo* fi);
    void SetStyle(int& style, const char flag);
    pstring TranslateTag(const char* flags, int& in_len);

    const pstring which() const;
    
    Encoding(char tc, bool ut, const char* n)
	: textchar(tc), name(n), tagscurr(ut) {}
    virtual ~Encoding() {}
};

class UTF8Encoding : public Encoding {
protected:
    wchar Decode_impl(const char* str, int& bytes, const Fontinfo* fi);
    int Charsz_impl(const char* str, const Fontinfo* fi);
public:
    int Encode(wchar input, char* output);
    pstring Encode(wchar input);
    const char* Previous(const char* str, const char* min = 0);

    UTF8Encoding() : Encoding('^', true, "utf8") {}
};

class CP932Encoding : public Encoding {
protected:
    wchar Decode_impl(const char* str, int& bytes, const Fontinfo* fi);
    int Charsz_impl(const char* str, const Fontinfo* fi);
public:
    int Encode(wchar input, char* output);
    pstring Encode(wchar input);
    const char* Previous(const char* str, const char* min = 0);

    CP932Encoding() : Encoding('`', false, "cp932") {}
};

extern Encoding* encoding; // Some uses of global state are less evil

#endif
