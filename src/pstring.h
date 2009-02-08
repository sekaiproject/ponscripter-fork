// -*- c++ -*-
// Utility functions and classes for strings.

#include "bstrwrap.h"
typedef CBString pstring;

#ifndef PSTRING_H
#define PSTRING_H

#include <SDL.h>
#include "encoding.h"

// Encoding-aware function to replace ASCII characters in a string.
inline void
replace_ascii(pstring& string, char what, char with, const Fontinfo* fi = 0)
{
    if (what != with) {
	char* s = string.mutable_data();
	const char* e = s + string.length();
	while (s < e) {
	    int cs = encoding->NextCharSize(s, fi);
	    if (cs == 1 && *s == what) *s = with;
	    s += cs;
	}
    }
}


// External iterator.
class pstrIter {
    const pstring& src;
    const char* pos;
    const char* end;
    int curr;
    int csize;
    const Fontinfo* font;
public:
    inline pstrIter(const pstring& target, const Fontinfo* fi = 0);

    // Current contents as character, or -1 if none.
    inline int get() const { return curr; }

    // Current contents as encoded character.
    inline const pstring getstr() const;

    // Pointer to start of current character.
    inline const char* getptr() const;

    // Advance to next character.
    inline void next();

    // Advance by N bytes.
    inline void forward(int n);
};

pstrIter::pstrIter(const pstring& target, const Fontinfo* fi) : src(target)
{
    pos = src;
    end = pos + target.length();
    font = fi;
    next();
}

const char* pstrIter::getptr() const
{
    return pos - csize;
}

const pstring pstrIter::getstr() const
{
    return src.midstr(getptr() - (const char*) src, csize);
}

void pstrIter::forward(int n)
{
    pos += n - csize;
    next();
}

void pstrIter::next()
{
//printf("pstrIter::next - pos %08lx, end %08lx", (size_t) pos, (size_t) end);
    if (pos < end) {
	curr = encoding->DecodeChar(pos, csize, font);
	pos += csize;
    }
    else {
	csize = 0;
	curr = -1;
    }
//printf(" => curr = %d\n", curr);
}

pstring zentohan(const pstring&);
pstring hantozen(const pstring&);

inline pstring
file_extension(const pstring& filename) {
    int dot = filename.reversefind('.', filename.length());
    return filename.midstr(dot + 1, filename.length());
}

inline SDL_RWops*
rwops(pstring& str)
{
    return SDL_RWFromMem((void*) str.mutable_data(), str.length());
}


// Parse tags in a pstring; return new pstring with tags converted to
// bytecode.
// TODO: needs testing!
pstring parseTags(const pstring& src);

#endif
