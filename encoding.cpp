/* -*- C++ -*-
 *
 *  encoding.cpp -- ligature handling, general encoding and UTF8
 *                  implementation
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

#include "defs.h"
#include "Fontinfo.h"

Encoding* encoding = 0; // initialised in ScriptHandler::readScript

struct ligature {
    typedef std::vector<ligature> vec;
    typedef vec::iterator iterator;
    typedef std::map<char, vec> map;
    pstring in;
    wchar out;
    ligature(pstring i, wchar o) : in(i), out(o) {}

    static const ligature undef;
};
static ligature::map ligs;
const ligature ligature::undef("", 0);

const ligature&
GetLigatureRef(const char* string)
{
    ligature::vec& v = ligs[string[0]];
    for (ligature::iterator it = v.begin(); it != v.end(); ++it) {
	int is = it->in.length();
	const char* in = it->in;
	while (--is && in[is] == string[is]);
	if (is == 0) return *it;
    }
    return ligature::undef;
}

void
AddLigature(const pstring& in, wchar out)
{
    ligature::vec& v = ligs[in[0]];
    for (ligature::iterator it = v.begin(); it != v.end(); ++it) {
	if (it->in == in) {
	    it->out = out;
	    return;
	}
    }
    v.insert(v.begin(), ligature(in, out));
}

void
ClearLigatures()
{
    ligs.clear();
}


void
DeleteLigature(const pstring& in)
{
    ligature::map::iterator lit = ligs.find(in[0]);
    if (lit == ligs.end()) return;
    ligature::vec& v = lit->second;
    for (ligature::iterator it = v.begin(); it != v.end(); ++it) {
	if (it->in == in) {
	    v.erase(it);
	    return;
	}
    }
}

void
DefaultLigatures(int which)
{
    if (which & 1) {
        AddLigature("`", 0x2018);
        AddLigature("``", 0x201c);
        AddLigature("'", 0x2019);
        AddLigature("''", 0x201d);
    }

    if (which & 2) {
        AddLigature("...", 0x2026);
        AddLigature("--", 0x2013);
        AddLigature("---", 0x2014);
        AddLigature("(c)", 0x00a9);
        AddLigature("(r)", 0x00ae);
        AddLigature("(tm)", 0x2122);
        AddLigature("++", 0x2020);
        AddLigature("+++", 0x2021);
        AddLigature("**", 0x2022);
        AddLigature("%_", 0x00a0);
        AddLigature("%.", 0x2009);
        AddLigature("%-", 0x2011);
    }

    if (which & 4) {
        AddLigature("ff", 0xfb00);
        AddLigature("fi", 0xfb01);
        AddLigature("fl", 0xfb02);
        AddLigature("ffi", 0xfb03);
        AddLigature("ffl", 0xfb04);
    }
}

wchar
Encoding::DecodeWithLigatures(const char* str, const Fontinfo& info, int& bytes)
{
    wchar rv = Decode_impl(str, bytes, true);
    // If likely a ligature, check for its presence in the target font.
    if (bytes == 1 || info.font()->has_char(rv)) return rv;
    // If the character wasn't found, try again discounting ligatures.
    return Decode_impl(str, bytes, false);
}


const pstring Encoding::which() const { return name; }

int
UTF8Encoding::Charsz_impl(const char* string, bool withligs)
{
    if (!string) return 0;
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char  c = t[0];
    if (c < 0x80) {
        if (c >= 0x17 && c <= 0x1e) return 3; // size codes
        if (c == 0x1f) return 2;

	// tags disabled?
	if (!withligs)
	    return 1;
	
        // extended codes
        if (c == '|')
	    return t[1] == '|' ? 2 : 1 + Charsz_impl(string + 1, true);

        const ligature& lig = GetLigatureRef(string);
        return lig.out ? lig.in.length() : 1;
    }
    else {
        if ((c & 0xc0) == 0x80)
            fprintf(stderr, "Warning: NextCharSize called on incomplete "
                            "character\n");
        // ZWNJ
        if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c)
            return 3 + Charsz_impl(string + 3, withligs);

        return c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4);
    }
}


wchar
UTF8Encoding::Decode_impl(const char* string, int& bytes, bool withligs)
{
    bytes = 0;
    if (!string) return 0;
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char  c = t[0];
    if (c < 0x80) {
	// Encoded tags.  Return the tag ID and skip the payload; it's
	// up to callers to extract that if they want it.
	// (pstrIter::getstr() will return complete tags.)
        if (c >= 0x17 && c <= 0x1e) {
	    bytes = 3;
	    return c;
	}
        if (c == 0x1f) {
	    bytes = 2;
	    return c;
	}
	
	// If ligatures are disabled, all other ASCII characters are
	// treated as themselves.
	if (!withligs) {
	    bytes = 1;
	    return c;
	}

        if (c == '|' && t[1] == '|') { bytes = 2; return '|'; }
	if (c == '|') {
	    wchar c = Decode_impl(string + 1, bytes, true);
	    ++bytes;
	    return c;
	}

        const ligature& lig = GetLigatureRef(string);
	if (lig.out) {
	    bytes = lig.in.length();
	    return lig.out;
	}
	bytes = 1;
        return c;
    }
    else {
        if ((c & 0xc0) == 0x80) {
	    size_t len = strlen(string);
	    int save = -1;
	    if (len > 128) { save = string[128]; *(char*)(string + 128) = 0; }
            fprintf(stderr, "Warning: UTF8Encoding::Decode called on "
		    "incomplete character (string: %s)\n", string);
	    if (save) *(char*)(string + 128) = save;
	}

        if (c < 0xe0) {
	    bytes = 2;
            return (c - 0xc0) << 6 | t[1] & 0x7f;
	}
        else if (c < 0xf0) {
            // ZWNJ
            if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c) {
		wchar c = Decode_impl(string + 3, bytes, withligs);
                bytes += 3;
		return c;
	    }

	    bytes = 3;
            return ((c - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f;
        }

	bytes = 4;
        return (((c - 0xe0) << 6
                 | t[1] & 0x7f) << 6
                | t[2] & 0x7f) << 6
               | t[3] & 0x7f;
    }
}


const char*
UTF8Encoding::Previous(const char* currpos, const char* strstart)
{
    if (currpos <= strstart + 1 || !currpos) return strstart;
    unsigned char c;
    do {
	c = *(unsigned char*)(--currpos) & 0xc0;
    } while (c == 0x80 && currpos > strstart);
    return currpos;
}


size_t
Encoding::CharacterCount(const char* string, bool withligs)
{
    size_t rv = 0;
    while (*string) {
        ++rv;
        string += NextCharSize(string, withligs);
    }
    return rv;
}


int
UTF8Encoding::Encode(wchar ch, char* out)
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

pstring
UTF8Encoding::Encode(wchar ch)
{
    pstring rv;
    if (ch <= 0x80) {
        rv += (unsigned char) ch;
    }
    else if (ch < 0x800) {
	rv += (unsigned char)(0xc0 | ch >> 6);
	rv += (unsigned char)(0x80 | ch & 0x3f);
    }
    else {
	rv += (unsigned char)(0xe0 | ch >> 12);
	rv += (unsigned char)(0x80 | ch >> 6 & 0x3f);
        rv += (unsigned char)(0x80 | ch & 0x3f);
    }
    return rv;
}


void
Encoding::SetStyle(int& style, const char flag)
{
    switch (flag) {
    case ' ': return;
    case 'd': style  = Default; return;
    case 'r': style &= ~Italic; return;
    case 'i': style ^= Italic;  return;
    case 't': style &= ~Bold;   return;
    case 'b': style ^= Bold;    return;
    case 'f': style &= ~Sans;   return;
    case 's': style ^= Sans;    return;
    case '+': case '-':   case '*': case '/':
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


pstring
set_int(char val, const char* src, int& in_len, int mulby = 1, int offset = 0)
{
    pstring rv(val);
    ++src;
    int i = 0;
    while (*src >= '0' && *src <= '9') {
        ++in_len;
        i = i * 10 + *src++ - '0';
    }
    i = i * mulby + offset;
    char c1 = i & 0x7f,
	 c2 = (i >> 7) & 0x7f;
    rv += char(c1 ? c1 : -1);
    rv += char(c2 ? c2 : -1);
    return rv;
}


pstring
Encoding::TranslateTag(const char* flag, int& in_len)
{
    in_len = 1;
    switch (*flag) {
    case ' ': return "";
    case 'd': return "\x10";
    case 'r': return "\x11";
    case 'i': return "\x12";
    case 't': return "\x13";
    case 'b': return "\x14";
    case 'f': return "\x15";
    case 's': return "\x16";
    case '=': return set_int(0x17, flag, in_len);
    case '+': return set_int(0x18, flag, in_len, -1, 8192);
    case '-': return set_int(0x18, flag, in_len, 1, 8192);
    case '%': return set_int(0x19, flag, in_len);
    case 'x':
        if (flag[1] == '+' || flag[1] == '-') {
            ++in_len;
            return set_int(0x1a, flag + 1, in_len,
			   flag[1] == '-' ? -1 : 1, 8192);
        }
        else
	    return set_int(0x1b, flag, in_len);

    case 'y': if (flag[1] == '+' || flag[1] == '-') {
            ++in_len;
            return set_int(0x1c, flag + 1, in_len,
			   flag[1] == '-' ? -1 : 1, 8192);
	}
        else return set_int(0x1d, flag, in_len);

    case 'c': return set_int(0x1e, flag, in_len);
    case 'n': return "\x1f\x10";
    case 'u': return "\x1f\x11";
    case 0:
        fprintf(stderr, "Error: non-matching ~tags~\n");
        exit(1);
    default:
        fprintf(stderr, "Warning: unknown tag ~%c~\n", *flag);
        return "";
    }
}
