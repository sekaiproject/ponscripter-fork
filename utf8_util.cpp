/* -*- C++ -*-
 * 
 *  utf8_util.cpp -- utility functions for handling Unicode text and
 *                   ligatures in Ponscripter
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

#include "utf8_util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct ligature {
    char bytes;
    char* in;
    unsigned short out;
    ligature* next;
} *ligs[0x80];

ligature*
GetLigatureRef(const char* string)
{
    ligature* lig;
    if ((lig = ligs[*(unsigned char*)string])) {
	do {
	    if (strncmp(string, lig->in, lig->bytes) == 0) return lig;
	} while ((lig = lig->next));
    }
    return 0;
}

void
AddLigature(const char* in, unsigned short out)
{
    const int c = *(unsigned char*)in;
    if (ligs[c]) {
	ligature* lig = ligs[c];
	do {
	    if (strcmp(in, lig->in) == 0) {
		lig->out = out;
		return;
	    }
	} while ((lig = lig->next));
    }	
    ligature* nl = new ligature;
    nl->bytes = strlen(in);
    nl->in = new char[nl->bytes];
    strcpy(nl->in, in);
    nl->out = out;
    nl->next = ligs[c];
    ligs[c] = nl;
}

void
ClearLigatures()
{
    for (int i = 0; i < 0x80; ++i) {
	while (ligs[i]) {
	    ligature* lig = ligs[i];
	    delete[] lig->in;
	    ligs[i] = lig->next;
	    delete lig;
	}
    }
}

void
DeleteLigature(const char* in)
{
    const int c = *(unsigned char*)in;
    if (ligs[c]) {
	ligature* lig = ligs[c];
	if (strcmp(lig->in, in) == 0) {
	    delete[] lig->in;
	    ligs[c] = lig->next;
	    delete lig;
	}
	else while (lig->next) {
	    if (strcmp(lig->next->in, in) == 0) {
		ligature* del = lig->next;
		lig->next = del->next;
		delete[] del->in;
		delete del;
		return;
	    }
	    lig = lig->next;
	}
    }
}

void
DefaultLigatures(int which)
{
    if (which & 1) {
	AddLigature("`",   0x2018);
	AddLigature("``",  0x201c);
	AddLigature("'",   0x2019);
	AddLigature("''",  0x201d);
    }
    if (which & 2) {
	AddLigature("...", 0x2026);
	AddLigature("--",  0x2013);
	AddLigature("---", 0x2014);
	AddLigature("(c)", 0x00a9);
	AddLigature("(r)", 0x00ae);
	AddLigature("(tm)",0x2122);
	AddLigature("++",  0x2020);
	AddLigature("+++", 0x2021);
	AddLigature("**",  0x2022);
	AddLigature("%_",  0x00a0);
	AddLigature("%.",  0x2009);
	AddLigature("%-",  0x2011);
    }
    if (which & 4) {
	AddLigature("ff",  0xfb00);
	AddLigature("fi",  0xfb01);
	AddLigature("fl",  0xfb02);
	AddLigature("ffi", 0xfb03);
	AddLigature("ffl", 0xfb04);
    }
}

void
DumpLigatures()
{
    printf("------------------------------------------------------------"
	   "-------------------\nDumping ligatures...\n");
    for (int i = 0; i < 0x80; ++i) {
	ligature* lig = ligs[i];
	if (lig) {
	    printf("Ligatures of '%c':\n", i);
	    while (lig) {
		printf("  %s -> U+%04x\n", lig->in, lig->out);
		lig = lig->next;
	    }
	}
    }
}

char
CharacterBytes(const char* string)
{
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char c = t[0];
    if (c < 0x80) {
	if (c >= 0x17 && c <= 0x1e) return 3; // size codes
	if (c == 0x1f) return 2; // extended codes
	if (c == '|') return t[1] == '|' ? 2 : 1 + CharacterBytes(string + 1);
	ligature* lig = GetLigatureRef(string);
	return lig ? lig->bytes : 1;
    }
    else {
	if ((c & 0xc0) == 0x80)
	    fprintf(stderr, "Warning: CharacterBytes called on incomplete "
		            "character\n");
	if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c)
	    return 3 + CharacterBytes(string + 3); // ZWNJ
	return c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4);
    }
}

unsigned short
UnicodeOfUTF8(const char* string)
{
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char c = t[0];
    if (c < 0x80) {
	if (c == '|') return (t[1] == '|') ? '|' : UnicodeOfUTF8(string + 1);
	ligature* lig = GetLigatureRef(string);
	return lig ? lig->out : c;
    }
    else {
	if ((c & 0xc0) == 0x80)
	    fprintf(stderr, "Warning: UnicodeOfUTF8 called on incomplete "
		            "character\n");
	if (c < 0xe0)
	    return (c - 0xc0) << 6 | t[1] & 0x7f;
	else if (c < 0xf0) {
	    if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c)
		return UnicodeOfUTF8(string + 3); // ZWNJ
	    return ((c - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f;
	}
	return (((c - 0xe0) << 6
	     | t[1] & 0x7f) << 6
	     | t[2] & 0x7f) << 6
	     | t[3] & 0x7f;
    }
}

const char*
PreviousCharacter(const char* string)
{
    unsigned char c;
    do { c = *(unsigned char*)(--string) & 0xc0; } while (c == 0x80);
    return string;
}

unsigned long int
UTF8Length(const char* string)
{
    unsigned long int rv = 0;
    while (*string) {
	++rv;
	string += CharacterBytes(string);
    }
    return rv;
}

int
UTF8OfUnicode(const unsigned short ch, char* out)
{
    unsigned char* b = (unsigned char*) out;
    if (ch <= 0x80) {
	*b++ = ch;
	*b = 0;
	return 1;
    }
    else if (ch < 0x800) {
	*b++ = 0xc0 | ch >> 6;
	*b++ = 0x80 | ch & 0x3f;
	*b = 0;
	return 2;
    }
    else {
	*b++ = 0xe0 | ch >> 12;
	*b++ = 0x80 | ch >> 6 & 0x3f;
	*b++ = 0x80 | ch & 0x3f;
	*b = 0;
	return 3;
    }
}

void
SetEncoding(int& encoding, const char flag)
{
    switch (flag) {
    case ' ': return;
    case 'd': encoding  =  Default; return;
    case 'r': encoding &= ~Italic;  return;
    case 'i': encoding ^=  Italic;  return;
    case 't': encoding &= ~Bold;    return;
    case 'b': encoding ^=  Bold;    return;
    case 'f': encoding &= ~Sans;    return;
    case 's': encoding ^=  Sans;    return;
    case '+': case '-':	case '*': case '/':
    case 'x': case 'y': case 'n': case 'u':
	fprintf(stderr, "Warning: tag ~%c~ cannot be used in this context\n",
		flag);
	return;
    case 'c':
    case 0:
	fprintf(stderr, "Error: non-matching ~tags~\n");
	exit(1);
    default:
	fprintf(stderr, "Warning: unknown tag ~%c~\n", flag);
    }
}

inline int
set_out(char* out, char val, char ext = 0)
{
    *out++ = val;
    if (ext) *out++ = ext;
    *out = 0;
    return ext ? 2 : 1;
}

inline int
set_int(char* out, char val, const char* src, int& in_len, int mulby = 1,
	int offset = 0)
{
    *out++ = val;
    ++src;
    int i = 0;
    while (*src >= '0' && *src <= '9') {
	++in_len;
	i = i * 10 + *src++ - '0';
    }
    i = i * mulby + offset;
    const char c1 = i & 0x7f, c2 = (i >> 7) & 0x7f;
    *out++ = c1 ? c1 : -1;
    *out++ = c2 ? c2 : -1;
    *out = 0;
    return 3;
}

int
TranslateTag(const char* flag, char* out, int& in_len)
{
    in_len = 1;
    switch (*flag) {
    case ' ': return 0;
    case 'd': return set_out(out, 0x10);
    case 'r': return set_out(out, 0x11);
    case 'i': return set_out(out, 0x12);
    case 't': return set_out(out, 0x13);
    case 'b': return set_out(out, 0x14);
    case 'f': return set_out(out, 0x15);
    case 's': return set_out(out, 0x16);
    case '=': return set_int(out, 0x17, flag, in_len);
    case '+': return set_int(out, 0x18, flag, in_len, -1, 8192);
    case '-': return set_int(out, 0x18, flag, in_len, 1, 8192);
    case '%': return set_int(out, 0x19, flag, in_len);
    case 'x':
	if (flag[1] == '+' || flag[1] == '-')	{
	    ++in_len;
	    return set_int(out, 0x1a, flag + 1, in_len,
			   flag[1] == '-' ? -1 : 1, 8192);
	}
	else return set_int(out, 0x1b, flag, in_len);
    case 'y': if (flag[1] == '+' || flag[1] == '-')	{
	    ++in_len;
	    return set_int(out, 0x1c, flag + 1, in_len,
			   flag[1] == '-' ? -1 : 1, 8192);
	}
	else return set_int(out, 0x1d, flag, in_len);
    case 'c': return set_int(out, 0x1e, flag, in_len);
    case 'n': return set_out(out, 0x1f, 0x10);
    case 'u': return set_out(out, 0x1f, 0x11);
    case 0:
	fprintf(stderr, "Error: non-matching ~tags~\n");
	exit(1);
    default:
	fprintf(stderr, "Warning: unknown tag ~%c~\n", *flag);
	*out = 0;
	return 0;
    }
}
