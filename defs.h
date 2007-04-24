// Global definitions not otherwise attributable to any class.

#ifndef __DEFS_H__
#define __DEFS_H__

#include <SDL.h>
#include "pstring.h"

#ifdef __GNU_C__
#include <ext/hash_map>
#include <ext/hash_set>
#else
#include <map>
#include <set>
#endif

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

inline string str(int i)
{
    char buf[1024];
    sprintf(buf, "%d", i);
    return string(buf);
}

inline string str(size_t i)
{
    char buf[1024];
    sprintf(buf, "%lu", i);
    return string(buf);
}


struct __attribute__((__packed__))
rgb_t {
    unsigned char r, g, b;

    void set(int all) { r = g = b = all; }
    void set(int red, int green, int blue) { r = red; g = green; b = blue; }

    rgb_t() {};
    rgb_t(int red, int green, int blue) : r(red), g(green), b(blue) {}
    rgb_t(const rgb_t& c) : r(c.r), g(c.g), b(c.b) {}
    
    rgb_t& operator=(const rgb_t& c) { set(c.r, c.g, c.b); return *this; }
    rgb_t& operator=(const SDL_Color& c) { set(c.r, c.g, c.b); return *this; }
};

#endif
