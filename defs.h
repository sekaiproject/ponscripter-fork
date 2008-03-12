// -*- C++ -*-
// Global definitions not otherwise attributable to any class.

#ifndef __DEFS_H__
#define __DEFS_H__

#ifdef __GNUC__
#define USE_HASH
#endif

#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <utility>
#include <limits>
#include <vector>
#include <deque>
#include <map>
#include <set>

#include "pstring.h"
#ifdef USE_HASH
#include <ext/hash_map>
#include <ext/hash_set>
namespace __gnu_cxx {
    template<>
    struct hash<pstring> {
        size_t operator()(const pstring& s) const {
            return __stl_hash_string(s);
        }
    };
}
#endif

const int MAX_INT = std::numeric_limits<int>::max();

template <typename KT, typename VT>
struct dictionary {
#ifdef USE_HASH
    typedef __gnu_cxx::hash_map<KT, VT> t;
#else
    typedef std::map<KT, VT> t;
#endif
};

template <typename T>
struct set {
#ifdef USE_HASH
    typedef __gnu_cxx::hash_set<T> t;
#else
    typedef std::set<T> t;
#endif
};
typedef std::vector<int> h_index_t;

struct __attribute__((__packed__))
rgb_t {
    unsigned char r, g, b;

    void set(int all) { r = g = b = all; }
    void set(int red, int green, int blue) { r = red; g = green; b = blue; }

    rgb_t() {}
    rgb_t(int all) : r(all), g(all), b(all) {}
    rgb_t(int red, int green, int blue) : r(red), g(green), b(blue) {}
    rgb_t(const rgb_t& c) : r(c.r), g(c.g), b(c.b) {}
    
    rgb_t& operator=(const rgb_t& c) { set(c.r, c.g, c.b); return *this; }
    rgb_t& operator=(const SDL_Color& c) { set(c.r, c.g, c.b); return *this; }
};

// Print a Unicode character with C-style escaping.
inline
void wputc_escaped(int what, FILE* where = stdout)
{
    if (what < 0) {
	fputs("{END}", where);
    }
    else if (what == '\n') {
	fputs("\\n", where);
    }
    else if (what < 0x20) {
	fprintf(where, "\\x%02x", what);
    }
    else if (what == '\\' || what == '"') {
	fprintf(where, "\\%c", (char) what);
    }
    else {
	UTF8Encoding enc;
	fputs(enc.Encode(what), where);
    }
}

// Print an encoded string with C-style escaping.
inline
void print_escaped(const pstring& what, FILE* where = stdout, bool newline = 0)
{
    fputc('"', where);
    for (pstrIter it(what); it.get() >= 0; it.next())
	wputc_escaped(it.get());
    fputc('"', where);
    if (newline) {
	fputc('\n', where);
	fflush(where);
    }
}

// Random number generation
void init_rnd();
int get_rnd(int lower, int upper);

#endif
