// -*- C++ -*-
// Global definitions not otherwise attributable to any class.

#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdio.h>

#include <algorithm>
#include <utility>
#include <limits>
#include <string>
#include <vector>
#include <map>
#ifdef __GNU_C__
#include <ext/hash_map>
#include <ext/hash_set>
#else
#include <set>
#endif

#include <SDL.h>

#include "pstring.h"

const int MAX_INT = std::numeric_limits<int>::max();

template <typename KT, typename VT>
struct dictionary {
#ifdef __GNU_C__
    typedef __gnu_cxx::hash_map<KT, VT> t;
#else
    typedef std::map<KT, VT> t;
#endif
};

template <typename T>
struct set {
#ifdef __GNU_C__
    typedef __gnu_cxx::hash_set<T> t;
#else
    typedef std::set<T> t;
#endif
};

template<typename T> inline T pred(T t) { return --t; }
template<typename T> inline T succ(T t) { return ++t; }

inline string lstr(int i, int len, int min, int radix = 10)
{
    char buf[1024];
    sprintf(buf, radix == 16 ? "%*.*x" : "%*.*d", len, min, i);
    return string(buf);
}

inline string nstr(int i, int len, bool zero = false, int radix = 10)
{
    char buf[1024];
    sprintf(buf, radix == 16 ? (zero ? "%0*x" : "%*x")
	                     : (zero ? "%0*d" : "%*d"), len, i);
    return string(buf);
}

inline string str(int i, int radix = 10)
{
    return nstr(i, 1, false, radix);
}

inline string lstr(size_t i, int len, int min, int radix = 10)
{
    char buf[1024];
    sprintf(buf, radix == 16 ? "%*.*lx" : "%*.*lu", len, min, i);
    return string(buf);
}

inline string nstr(size_t i, int len, bool zero = false, int radix = 10)
{
    char buf[1024];
    sprintf(buf, radix == 16 ? (zero ? "%0*lx" : "%*lx")
	                     : (zero ? "%0*lu" : "%*lu"), len, i);
    return string(buf);
}

inline string str(size_t i, int radix = 10)
{
    return nstr(i, 1, false, radix);
}


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

#endif
