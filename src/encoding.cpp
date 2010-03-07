/* -*- C++ -*-
 *
 *  encoding.cpp -- ligature handling, general encoding and UTF8
 *                  implementation
 *
 *  Copyright (c) 2007-2009 Peter Jolly
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

Encoding *file_encoding = 0; // the encoding used by the script file

struct ligature {
    wchar codepoint;
    int seqlen;
    ligature() : codepoint(0), seqlen(0) {}
};

class ligatures {
    typedef std::map<char, ligatures*> map;
    typedef map::iterator mit;
    typedef std::pair<char, ligatures*> clunk;
    typedef std::pair<mit, bool> ins;
    ligature val;
    map children;
    
public:
    void clear();
    ligature* find(const char* seq, const Fontinfo* face);
    void add(const char* seq, wchar value, int depth = 0);
    void del(const char* seq);
    
    ~ligatures() { clear(); }
};
static ligatures ligs;

void
ligatures::clear()
{
    children.clear();
}
    
ligature*
ligatures::find(const char* seq, const Fontinfo* face)
{
    ligature* v = 0;
    mit lit = children.find(*seq);
    if (lit != children.end())
        v = lit->second->find(seq + 1, face);
    if (!v && val.codepoint &&
        (!face || face->font()->has_char(val.codepoint)))
        v = &val;
    return v;
}

void
ligatures::add(const char* seq, wchar value, int depth)
{
    if (*seq) {
        ins p = children.insert(clunk(*seq, 0));
        if (!p.first->second) p.first->second = new ligatures;
        p.first->second->add(seq + 1, value, depth + 1);
    }
    else {
        val.codepoint = value;
        val.seqlen = depth;
    }
}

void
ligatures::del(const char* seq)
{
    if (*seq) {
        mit e = children.find(*seq);
        if (e != children.end()) {
            e->second->del(seq + 1);
            if (!e->second->val.codepoint && e->second->children.empty())
                children.erase(e);
        }
    }
    else {
        val.codepoint = 0;
    }
}


void
AddLigature(const pstring& in, wchar out)
{
    ligs.add(in, out);
}

void
ClearLigatures()
{
    ligs.clear();
}

void
DeleteLigature(const pstring& in)
{
    ligs.del(in);
}
    
void
DefaultLigatures(int which)
{
    if (which & 1) {
        AddLigature("`",    0x2018);
        AddLigature("``",   0x201c);
        AddLigature("'",    0x2019);
        AddLigature("''",   0x201d);
    }

    if (which & 2) {
        AddLigature("...",  0x2026);
        AddLigature("--",   0x2013);
        AddLigature("---",  0x2014);
        AddLigature("(c)",  0x00a9);
        AddLigature("(r)",  0x00ae);
        AddLigature("(tm)", 0x2122);
        AddLigature("++",   0x2020);
        AddLigature("+++",  0x2021);
        AddLigature("**",   0x2022);
        AddLigature("%_",   0x00a0);
        AddLigature("%.",   0x2009);
        AddLigature("%-",   0x2011);
    }

    if (which & 4) {
        AddLigature("ff",   0xfb00);
        AddLigature("fi",   0xfb01);
        AddLigature("fl",   0xfb02);
        AddLigature("ffi",  0xfb03);
        AddLigature("ffl",  0xfb04);
    }

    if (which & 8) {
        AddLigature("#@",   '@');
        AddLigature("#/",   '/');
        AddLigature("#\\",  '\\');
        AddLigature("#_",   '_');
        AddLigature("#^",   '^');
        AddLigature("#`",   '`');
        AddLigature("#!",   '!');
        AddLigature("##",   '#');
    }
}

wchar
Encoding::DecodeWithLigatures(const char* str, const Fontinfo& info, int& bytes)
{
//    wchar rv = Decode_impl(str, bytes, &info);
//    // If likely a ligature, check for its presence in the target font.
//    if (bytes == 1 || info.font()->has_char(rv)) return rv;
//    // If the character wasn't found, try again discounting ligatures.
//    return Decode_impl(str, bytes, 0);
    // Ligature presence is taken care of in ligature search now.
    return Decode_impl(str, bytes, &info);
}


const pstring Encoding::which() const { return name; }

int
UTF8Encoding::Charsz_impl(const char* string, const Fontinfo* fi)
{
    if (!string) return 0;
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char  c = t[0];
    if (c < 0x80) {

        if (!UseTags()) return 1;
        if (c >= 0x17 && c <= 0x1e) return 3; // size codes
        if (c == 0x1f) return 2;

	// tags disabled?
	if (!fi)
	    return 1;
	
        // extended codes
        if (c == '|')
	    return t[1] == '|' ? 2 : 1 + Charsz_impl(string + 1, fi);

        const ligature* lig = ligs.find(string, fi);
        return lig ? lig->seqlen : 1;
    }
    else {
        if ((c & 0xc0) == 0x80)
            fprintf(stderr, "Warning: NextCharSize called on incomplete "
                            "character\n");

        const ligature* lig = ligs.find(string, fi);
        if (lig) return lig->seqlen;

        // ZWNJ
        if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c)
            return 3 + Charsz_impl(string + 3, fi);

        return c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4);
    }
}


wchar
UTF8Encoding::Decode_impl(const char* string, int& bytes,
                          const Fontinfo* fi)
{
    bytes = 0;
    if (!string) return 0;
    const unsigned char* t = (const unsigned char*) string;
    const unsigned char  c = t[0];
    if (c < 0x80) {

        // return if tags and ligatures are disabled
        if (!UseTags()) {
            bytes = 1;
            return c;
        }

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
        if (!fi) {
            bytes = 1;
            return c;
        }

        if (c == '|' && t[1] == '|') { bytes = 2; return '|'; }
        if (c == '|') {
            wchar wc = Decode_impl(string + 1, bytes, fi);
            ++bytes;
            return wc;
        }

        const ligature* lig = ligs.find(string, fi);
        if (lig) {
            bytes = lig->seqlen;
            return lig->codepoint;
        }
        bytes = 1;
        return c;
    }
    else {
        if ((c & 0xc0) == 0x80) {
            size_t len = strlen(string);
            int save = -1;
            if (len > 128) {
                save = string[128];
                *(char*)(string + 128) = 0;
            }
            fprintf(stderr, "Warning: UTF8Encoding::Decode called on "
                    "incomplete character (string: %s)\n", string);
            if (save) *(char*)(string + 128) = save;
        }

        wchar wc = 0;

        if (UseTags() && fi) {
            const ligature* lig = ligs.find(string, fi);
            if (lig) {
                bytes = lig->seqlen;
                return lig->codepoint;
            }
        }

        if (c < 0xe0) {
            bytes = 2;
            wc = ((c - 0xc0) << 6) | (t[1] & 0x7f);
        }
        else if (c < 0xf0) {
            // ZWNJ
            if ((c == 0xe2) && (t[1] == 0x80) && (t[2] == 0x8c)) {
                wc = Decode_impl(string + 3, bytes, fi);
                bytes += 3;
                return wc;
            }

            bytes = 3;
            wc = ((((c - 0xe0) << 6) | (t[1] & 0x7f)) << 6) | (t[2] & 0x7f);
        } else {
            bytes = 4;
            wc = (( (( ((c - 0xe0) << 6) |
                      (t[1] & 0x7f)) << 6) |
                    (t[2] & 0x7f)) << 6) |
                 (t[3] & 0x7f);
        }
        return wc;

    }
}


const char*
UTF8Encoding::Previous(const char* currpos, const char* strstart)
{
    if (currpos <= strstart + 1 || !currpos) return strstart;
    unsigned char c;
    do {
        c = *(unsigned char*)(--currpos) & 0xc0;
    } while ((c == 0x80) && (currpos > strstart));
    return currpos;
}


size_t
Encoding::CharacterCount(const char* string, const Fontinfo* fi)
{
    size_t rv = 0;
    while (*string) {
        ++rv;
        string += NextCharSize(string, fi);
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
        *b++ = 0xc0 | (ch >> 6);
        *b++ = 0x80 | (ch & 0x3f);
        *b = 0;
        return 2;
    }
    else {
        *b++ = 0xe0 | (ch >> 12);
        *b++ = 0x80 | ((ch >> 6) & 0x3f);
        *b++ = 0x80 | (ch & 0x3f);
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
        rv += (unsigned char)(0xc0 | (ch >> 6));
        rv += (unsigned char)(0x80 | (ch & 0x3f));
    }
    else {
        rv += (unsigned char)(0xe0 | (ch >> 12));
        rv += (unsigned char)(0x80 | ((ch >> 6) & 0x3f));
        rv += (unsigned char)(0x80 | (ch & 0x3f));
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


//Return whether the given Unicode character is of type 'Separator, Space'
bool isSpace(wchar input)
{
    if ((input == 0x0020) || (input == 0x00a0) ||
        (input == 0x1680) || (input == 0x180e) ||
        ((input >= 0x2000) && (input <= 0x200a)) ||
        (input == 0x202f) || (input == 0x205f) || (input == 0x3000)) {
        return true;
    }
    return false;
}

//Return whether the given Unicode character is of type 'Mark,Nonspacing'
bool isNonspacing(wchar input)
{
    if (((input >= 0x0300) && (input <= 0x036f)) ||
        ((input >= 0x0483) && (input <= 0x0487)) ||
        ((input >= 0x0591) && (input <= 0x05bf)) ||
        ((input >= 0x05c1) && (input <= 0x05c5)) ||
        (input == 0x05c7) ||
        ((input >= 0x0610) && (input <= 0x061a)) ||
        ((input >= 0x064b) && (input <= 0x065e)) ||
        (input == 0x0670) ||
        ((input >= 0x06d6) && (input <= 0x06e4)) ||
        (input == 0x06e7) || (input == 0x06e8) ||
        ((input >= 0x06ea) && (input <= 0x06ed)) ||
        (input == 0x0711) ||
        ((input >= 0x0730) && (input <= 0x074a)) ||
        ((input >= 0x07a6) && (input <= 0x07b0)) ||
        ((input >= 0x07eb) && (input <= 0x07f3)) ||
        (input == 0x0901) || (input == 0x0902) || (input == 0x093c) ||
        ((input >= 0x0941) && (input <= 0x0948)) ||
        (input == 0x094d) ||
        ((input >= 0x0951) && (input <= 0x0954)) ||
        (input == 0x0962) || (input == 0x0963) ||
        (input == 0x0981) || (input == 0x09bc) ||
        ((input >= 0x09c1) && (input <= 0x09c4)) ||
        (input == 0x09cd) || (input == 0x09e2) || (input == 0x09e3) ||
        (input == 0x0a01) || (input == 0x0a02) || (input == 0x0a3c) ||
        (input == 0x0a41) || (input == 0x0a42) ||
        (input == 0x0a47) || (input == 0x0a48) ||
        ((input >= 0x0a4b) && (input <= 0x0a4d)) ||
        (input == 0x0a51) || (input == 0x0a70) || (input == 0x0a71) ||
        (input == 0x0a75) || (input == 0x0a81) ||
        (input == 0x0a82) || (input == 0x0abc) ||
        ((input >= 0x0ac1) && (input <= 0x0ac5)) ||
        (input == 0x0ac7) || (input == 0x0ac8) || (input == 0x0acd) ||
        (input == 0x0ae2) || (input == 0x0ae3) || (input == 0x0b01) ||
        (input == 0x0b3c) || (input == 0x0b3f) ||
        ((input >= 0x0b41) && (input <= 0x0b44)) ||
        (input == 0x0b4d) || (input == 0x0b56) || (input == 0x0b62) ||
        (input == 0x0b63) || (input == 0x0b82) || (input == 0x0bc0) ||
        (input == 0x0bcd) || (input == 0x0c3e) ||
        (input == 0x0c3f) || (input == 0x0c40) ||
        ((input >= 0x0c46) && (input <= 0x0c48)) ||
        ((input >= 0x0c4a) && (input <= 0x0c4d)) ||
        (input == 0x0c55) || (input == 0x0c56) || (input == 0x0c62) ||
        (input == 0x0c63) || (input == 0x0cbc) || (input == 0x0cbf) ||
        (input == 0x0cc6) || (input == 0x0ccc) || (input == 0x0ccd) ||
        (input == 0x0ce2) || (input == 0x0ce3) ||
        ((input >= 0x0d41) && (input <= 0x0d44)) ||
        (input == 0x0d4d) || (input == 0x0d62) ||
        (input == 0x0d63) || (input == 0x0dca) ||
        ((input >= 0x0dd2) && (input <= 0x0dd4)) ||
        (input == 0x0dd6) || (input == 0x0e31) ||
        ((input >= 0x0e34) && (input <= 0x0e3a)) ||
        ((input >= 0x0e47) && (input <= 0x0e4e)) ||
        (input == 0x0eb1) ||
        ((input >= 0x0eb4) && (input <= 0x0eb9)) ||
        (input == 0x0ebb) || (input == 0x0ebc) ||
        ((input >= 0x0ec8) && (input <= 0x0ecd)) ||
        (input == 0x0f18) || (input == 0x0f19) || (input == 0x0f35) ||
        (input == 0x0f37) || (input == 0x0f39) ||
        ((input >= 0x0f71) && (input <= 0x0f7e)) ||
        ((input >= 0x0f80) && (input <= 0x0f84)) ||
        (input == 0x0f86) || (input == 0x0f87) ||
        ((input >= 0x0f90) && (input <= 0x0f97)) ||
        ((input >= 0x0f99) && (input <= 0x0fbc)) ||
        (input == 0x0fc6) ||
        ((input >= 0x102d) && (input <= 0x1030)) ||
        ((input >= 0x1032) && (input <= 0x1037)) ||
        (input == 0x1039) || (input == 0x103a) || (input == 0x103d) ||
        (input == 0x103e) || (input == 0x1058) || (input == 0x1059) ||
        ((input >= 0x105e) && (input <= 0x1060)) ||
        ((input >= 0x1071) && (input <= 0x1074)) ||
        (input == 0x1082) || (input == 0x1085) || (input == 0x1086) ||
        (input == 0x108d) || (input == 0x135f) ||
        ((input >= 0x1712) && (input <= 0x1714)) ||
        ((input >= 0x1732) && (input <= 0x1734)) ||
        (input == 0x1752) || (input == 0x1753) ||
        (input == 0x1772) || (input == 0x1773) ||
        ((input >= 0x17b7) && (input <= 0x17bd)) ||
        (input == 0x17c6) ||
        ((input >= 0x17c9) && (input <= 0x17d3)) ||
        (input == 0x17dd) ||
        ((input >= 0x180b) && (input <= 0x180d)) ||
        (input == 0x18a9) ||
        ((input >= 0x1920) && (input <= 0x1922)) ||
        (input == 0x1927) || (input == 0x1928) || (input == 0x1932) ||
        ((input >= 0x1939) && (input <= 0x193b)) ||
        (input == 0x1a17) || (input == 0x1a18) ||
        ((input >= 0x1b00) && (input <= 0x1b03)) ||
        (input == 0x1b34) ||
        ((input >= 0x1b37) && (input <= 0x1b3a)) ||
        (input == 0x1b3c) || (input == 0x1b42) ||
        ((input >= 0x1b6b) && (input <= 0x1b73)) ||
        (input == 0x1b80) || (input == 0x1b81) ||
        ((input >= 0x1ba2) && (input <= 0x1ba5)) ||
        (input == 0x1ba8) || (input == 0x1ba9) ||
        ((input >= 0x1c2c) && (input <= 0x1c33)) ||
        (input == 0x1c36) || (input == 0x1c37) ||
        ((input >= 0x1dc0) && (input <= 0x1de6)) ||
        (input == 0x1dfe) || (input == 0x1dff) ||
        ((input >= 0x20d0) && (input <= 0x20dc)) ||
        (input == 0x20e1) ||
        ((input >= 0x20e5) && (input <= 0x20f0)) ||
        ((input >= 0x2de0) && (input <= 0x2dff)) ||
        ((input >= 0x302a) && (input <= 0x302f)) ||
        (input == 0x3099) || (input == 0x309a) ||
        (input == 0xa66f) || (input == 0xa67c) || (input == 0xa67d) ||
        (input == 0xa802) || (input == 0xa806) || (input == 0xa80b) ||
        (input == 0xa825) || (input == 0xa826) || (input == 0xa8c4) ||
        ((input >= 0xa926) && (input <= 0xa92d)) ||
        ((input >= 0xa947) && (input <= 0xa951)) ||
        ((input >= 0xaa29) && (input <= 0xaa2e)) ||
        (input == 0xaa31) || (input == 0xaa32) || (input == 0xaa35) ||
        (input == 0xaa36) || (input == 0xaa43) ||
        (input == 0xaa4c) || (input == 0xfb1e) ||
        ((input >= 0xfe00) && (input <= 0xfe0f)) ||
        ((input >= 0xfe20) && (input <= 0xfe26)) ) {
        return true;
    }
    return false;
}
