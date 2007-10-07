/* -*- C++ -*-
 *
 *  cp932_encoding.cpp -- Shift_JIS support
 *
 *  Copyright (c) 2007 Peter Jolly
 *
 *  Based in part on the similar routines used in Elliot Glaysher's
 *  RLVM.
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

#include "defs.h"
#include "cp932_tables.h"

// NB. We currently don't support ligatures in CP932.

int
CP932Encoding::CharacterBytes(const char* string)
{
    const unsigned char c = *(unsigned char*) string;
    return (c < 0x7f || (c >= 0xa1 && c <= 0xdf)) ? 1 : 2;
}


wchar
CP932Encoding::Decode(const char* string, int& bytes)
{
    bytes = 0;
    if (!string) return 0;
    unsigned char* us = (unsigned char*) string;
    leading character = fmcp932_tbl[us[0]];
    if (character.tbl) {
	bytes = 2;
	return character.tbl[us[1]];
    }
    bytes = 1;
    return character.sbc;
}


const char*
CP932Encoding::Previous(const char* currpos, const char* strstart)
{
    int cb = CharacterBytes(strstart);
    while (strstart + cb < currpos) {
	strstart += cb;
	cb = CharacterBytes(strstart);
    }
    return strstart;
}


int
CP932Encoding::Encode(wchar ch, char* out)
{
    wchar* table = tocp932_tbl[ch >> 8];
    ch = table ? table[ch & 0xff] : 0;
    if (ch > 255) {
	out[0] = char(ch >> 8);
	out[1] = char(ch & 0xff);
	out[2] = 0;
	return 2;
    }
    else {
	out[0] = char(ch);
	out[1] = 0;
	return 1;
    }
}

string
CP932Encoding::Encode(const wchar ch)
{
    char c[3];
    Encode(ch, c);
    return string(c);
}
