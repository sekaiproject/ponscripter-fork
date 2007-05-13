// -*- c++ -*-
// Wrapper for std::string that implements saner semantics and adds
// loads of handy extensions.  OK, it's a dumping-ground and a mess.  :)

// This wrapper treats NULL as an empty string (except in comparisons,
// where NULL < ""), instead of dying horribly whenever it sees one like
// std::string does.

// In bool contexts, strings are true if non-empty.

// Strings can be instantiated from signed or unsigned chars; u_str() is
// c_str() for unsigned chars, and udata() is data().

// Extensions include a Perl-like shift/unshift/push/pop set, a
// vector-like back(), split/uppercase/replace functions, a set of
// UTF-8-aware iterators, and so on.

// IO is provided for C streams.
// FILE* >> string // reads an entire line, sans trailing eol
// FILE* << string // prints string, but no newline
// FILE* << eol    // eol is defined, just like for iostreams
// (I hate hideous bloated iostreams, but the syntax can be convenient!)

#ifndef PSTRING_H
#define PSTRING_H

class string;
#include "encoding.h"

template<typename T> inline T pred(T t) { return --t; }
template<typename T> inline T succ(T t) { return ++t; }

class string {
public:
    typedef std::string::size_type size_type;
    typedef char& reference;
    typedef const char& const_reference;
    typedef std::string::iterator iterator;
    typedef std::string::const_iterator const_iterator;
    typedef std::string::reverse_iterator reverse_iterator;
    typedef std::string::const_reverse_iterator const_reverse_iterator;
    static const size_type npos;

    typedef std::vector<string> vector;
private:
    std::string c;
    string(const std::string s, size_type pos, size_type n)
	: c(s.substr(pos, n)) {}
    friend void swap(string& s1, string& s2);

public:
    iterator begin() { return c.begin(); }
    iterator end() { return c.end(); }
    const_iterator begin() const { return c.begin(); }
    const_iterator end() const { return c.end(); }
    reverse_iterator rbegin() { return c.rbegin(); }
    reverse_iterator rend() { return c.rend(); }
    const_reverse_iterator rbegin() const { return c.rbegin(); }
    const_reverse_iterator rend() const { return c.rend(); }

    size_type size() const { return c.size(); }
    bool empty() const { return c.empty(); }

    reference operator[](size_type n) { return c[n]; }
    const_reference operator[](size_type n) const { return c[n]; }

    const char* c_str() const { return c.c_str(); }
    const char* data() const { return c.data(); }
    
    const unsigned char* u_str() const
	{ return (const unsigned char*) c.c_str(); }
    const unsigned char* udata() const
	{ return (const unsigned char*) c.data(); }    

    string() {}
    string(const string& s) : c(s.c) {}
    string(const string& s, size_type p, size_type n) : c(s.c, p, n) {}
    string(const char* s) : c(s ? s : "") {}
    string(const char* s, size_type n) : c(s ? s : "", n) {}
    string(const unsigned char* s) : c(s ? (char*) s : "") {}
    string(const unsigned char* s, size_type n) : c(s ? (char*) s : "", n) {}
    string(size_type n, char c) : c(n, c) {}
    string(size_type n, unsigned char c) : c(n, (char) c) {}    
    template <typename T> string(T first, T last) : c(first, last) {}

    void reserve(size_t n) { c.reserve(n); }
    void swap(string& o) { c.swap(o.c); }

    iterator insert(iterator p, const char& e)
	{ return c.insert(p, e); }
    template <class T> void insert(iterator p, T f, T l)
	{ c.insert(p, f, l); }
    void insert(iterator p, size_type n, const char& e)
	{ c.insert(p, n, e); }
    string& insert(size_type pos, const string& s)
	{ c.insert(pos, s.c); return *this; }
    string& insert(size_type pos, const string& s, size_type pos1, size_type n)
	{ c.insert(pos, s.c, pos1, n); return *this; }
    string& insert(size_type pos, const char* s)
	{ if (s) c.insert(pos, s); return *this; }
    string& insert(size_type pos, const char* s, size_type n)
	{ if (s) c.insert(pos, s, n); return *this; }
    string& insert(size_type pos, size_type n, char e)
	{ c.insert(pos, n, e); return *this; }

    string& append(const string& s)
	{ c.append(s.c); return *this; }
    string& append(const string& s, size_type pos, size_type n)
	{ c.append(s.c, pos, n); return *this; }
    string& append(const char* s)
	{ if (s) c.append(s); return *this; }
    string& append(const char* s, size_type n)
	{ if (s) c.append(s, n); return *this; }
    string& append(wchar e)
	{ return append(encoding->Encode(e)); }
    string& append(size_type n, char e)
	{ c.append(n, e); return *this; }
    template <class T> string& append(T first, T last)
	{ c.append(first, last); return *this; }

    void push_back(char e) { c.push_back(e); }
    void push_back(wchar e) { append(e); }

    string& operator+=(const string& s) { return append(s); }
    string& operator+=(const char* s)   { return append(s); }
    string& operator+=(char e)          { push_back(e); return *this; }
    string& operator+=(wchar e)         { return append(e); }

    iterator erase(iterator p)
	{ return c.erase(p); }
    iterator erase(iterator first, iterator last)
	{ return c.erase(first, last); }
    string& erase(size_type pos = 0)
	{ c.erase(pos); return *this; }
    string& erase(size_type pos, size_type n)
	{ c.erase(pos, n); return *this; }

    void clear() { c.clear(); }
    void resize(size_type n, char e = 0) { c.resize(n, e); }

    string& assign(const string& s)
	{ if (&s != this) c.assign(s.c); return *this; }
    string& assign(const string& s, size_type pos, size_type n)
	{ c.assign(s.c, pos, n); return *this; }
    string& assign(const char* s, size_type n)
	{ if (s) c.assign(s, n); else c.clear(); return *this; }
    string& assign(const char* s)
	{ if (s) c.assign(s); else c.clear(); return *this; }
    string& assign(size_type n, char e)
	{ c.assign(n, e); return *this; }
    template <class T> string& assign(T first, T last)
	{ c.assign(first, last); return *this; }

    string& operator=(const string& s) { return assign(s); }
    string& operator=(const char* s) { return assign(s); }
    string& operator=(char e) { c = e; return *this; }

    string& replace(size_type pos, size_type n, const string& s)
	{ c.replace(pos, n, s.c); return *this; }
    string& replace(size_type pos, size_type n, const string& s, 
		    size_type pos1, size_type n1)
	{ c.replace(pos, n, s.c, pos1, n1); return *this; }
    string& replace(size_type pos, size_type n, const char* s, size_type n1)
	{ if (s) c.replace(pos, n, s, n1); else erase(pos, n);
	  return *this; }
    string& replace(size_type pos, size_type n, const char* s)
	{ if (s) c.replace(pos, n, s); else erase(pos, n);
	  return *this; }
    string& replace(size_type pos, size_type n, size_type n1, char e)
	{ c.replace(pos, n, n1, e); return *this; }
    string& replace(iterator first, iterator last, const string& s)
	{ c.replace(first, last, s.c); return *this; }
    string& replace(iterator first, iterator last, const char* s, size_type n)
	{ if (s) c.replace(first, last, s, n); else erase(first, last);
	  return *this; }
    string& replace(iterator first, iterator last, const char* s)
	{ if (s) c.replace(first, last, s); else erase(first, last);
	  return *this; }
    string& replace(iterator first, iterator last, size_type n, char e)
	{ c.replace(first, last, n, e); return *this; }
    template <class T> string& replace(iterator first, iterator last, T f, T l)
	{ c.replace(first, last, f, l); return *this; }

    size_type copy(char* buf, size_type n, size_type pos = 0) const
	{ return c.copy(buf, n, pos); }

    size_type find(const string& s, size_type pos = 0) const
        { return c.find(s.c, pos); }
    size_type find(const char* s, size_type pos, size_type n) const
        { return s ? c.find(s, pos, n) : npos; }
    size_type find(const char* s, size_type pos = 0) const
        { return s ? c.find(s, pos) : npos; }
    size_type find(char e, size_type pos = 0) const
        { return c.find(e, pos); }
    size_type rfind(const string& s, size_type pos = npos) const
        { return c.rfind(s.c, pos); }
    size_type rfind(const char* s, size_type pos, size_type n) const
        { return s ? c.rfind(s, pos, n) : npos; }	
    size_type rfind(const char* s, size_type pos = npos) const
        { return s ? c.rfind(s, pos) : npos; }
    size_type rfind(char e, size_type pos = npos) const 
        { return c.rfind(e, pos); }
    size_type find_first_of(const string& s, size_type pos = 0) const
        { return c.find_first_of(s.c, pos); }
    size_type find_first_of(const char* s, size_type pos, size_type n) const 
        { return s ? c.find_first_of(s, pos, n) : npos; }
    size_type find_first_of(const char* s, size_type pos = 0) const 
        { return s ? c.find_first_of(s, pos) : npos; }
    size_type find_first_of(char e, size_type pos = 0) const
        { return c.find_first_of(e, pos); }
    size_type find_first_not_of(const string& s, size_type pos = 0) const
        { return c.find_first_not_of(s.c, pos); }
    size_type find_first_not_of(const char* s, size_type pos, size_type n) const
        { return s ? c.find_first_not_of(s, pos, n) : npos; }
    size_type find_first_not_of(const char* s, size_type pos = 0) const 
        { return s ? c.find_first_not_of(s, pos) : npos; }
    size_type find_first_not_of(char e, size_type pos = 0) const 
        { return c.find_first_not_of(e, pos); }
    size_type find_last_of(const string& s, size_type pos = npos) const 
        { return c.find_last_of(s.c, pos); }
    size_type find_last_of(const char* s, size_type pos, size_type n) const 
        { return s ? c.find_last_of(s, pos, n) : npos; }
    size_type find_last_of(const char* s, size_type pos = npos) const 
        { return s ? c.find_last_of(s, pos) : npos; }
    size_type find_last_of(char e, size_type pos = npos) const 
        { return c.find_last_of(e, pos); }
    size_type find_last_not_of(const string& s, size_type pos = npos) const 
        { return c.find_last_not_of(s.c, pos); }
    size_type find_last_not_of(const char* s, size_type pos, size_type n) const 
        { return s ? c.find_last_not_of(s, pos, n) : npos; }
    size_type find_last_not_of(const char* s, size_type pos = npos) const 
        { return s ? c.find_last_not_of(s, pos) : npos; }
    size_type find_last_not_of(char e, size_type pos = npos) const
        { return c.find_last_not_of(e, pos); }

    string substr(size_type pos = 0, size_type n = npos) const
	{ if (pos >= c.size()) return "";
	  if (pos + n >= c.size()) n = c.size() - pos;
	  return n ? string(c, pos, n) : ""; }
		
    int compare(const string& s) const
	{ return c.compare(s.c); }
    int compare(size_type pos, size_type n, const string& s) const
	{ return c.compare(pos, n, s.c); }
    int compare(size_type pos, size_type n, const string& s,
		size_type pos1, size_type n1) const
	{ return c.compare(pos, n, s.c, pos1, n1); }
    int compare(const char* s) const
	{ return s ? c.compare(s) : 1; }
    int compare(size_type pos, size_type n, const char* s,
		size_type len = npos) const
	{ return s ? c.compare(pos, n, s, len) : 1; }

    // Extensions

    operator bool() const { return !empty(); }
    void push_uchar(unsigned char e) { c.push_back(char(e)); }

    char& back() { return c[c.size() - 1]; }
    char back() const { return c[c.size() - 1]; }    
    wchar wback() const { return *(pred(wend())); }

    // Non-case-sensitive comparisons (encoding-aware)
    int icompare(size_type pos, size_type n, const string& s,
		 size_type pos1, size_type n1) const {
	const char *a1 = c.c_str() + pos,
	           *a2 = c.c_str() + (n > c.size() ? c.size() : n),
		   *b1 = s.c.c_str() + pos1,
		   *b2 = s.c.c_str() + (n1 > s.c.size() ? s.c.size() : n1);
	while (a1 < a2 && b1 < b2) {
	    wchar ac = encoding->Decode(a1);
	    wchar bc = encoding->Decode(b1);
	    if (ac >= 'A' && ac <= 'Z') ac += 32;
	    if (bc >= 'A' && bc <= 'Z') bc += 32;
	    if (ac < bc) return -1;
	    if (ac > bc) return 1;
	    a1 += encoding->CharacterBytes(a1);
	    b1 += encoding->CharacterBytes(b1);
	}
	return 0;
    }
    int icompare(const string& s) const
	{ return icompare(0, npos, s, 0, npos); }
    int icompare(size_type pos, size_type n, const string& s) const
	{ return icompare(pos, n, s, pos, n); }
    int icompare(const char* s) const
	{ return icompare(0, npos, s, npos); }
    int icompare(size_type pos, size_type n, const char* s,
		size_type len = npos) const
	{ return s ? icompare(pos, n, string(s), 0, len) : 1; }
    
    // Perl-like stuff
    char pop() { char e = back(); c.resize(c.size() - 1); return e; }
    string& push(char e) { c.push_back(e); return *this; }
    char shift() { char e = c[0]; c.erase(0, 1); return e; }
    string& unshift(char e) { c.insert(0, 1, e); return *this; }
    size_type chomp() {
	size_type rv = 0, back = c.size();
	while (back > 0 && c[back - 1] == 0x0a) ++rv, --back;
	c.resize(back);
	return rv;
    }

    wchar wpop()
	{ const char* p = encoding->Previous(c.c_str() + c.size(), c.c_str());
	  wchar e = encoding->Decode(p);
	  c.erase(p - c.c_str());
	  return e; }
    string& wpush(wchar e)
	{ append(e); return *this; }
    wchar wshift()
	{ char e = encoding->Decode(c_str());
	  c.erase(encoding->CharacterBytes(c_str()));
	  return e; }
    string& wunshift(wchar e)
	{ char buf[32];
	  std::string out(buf, encoding->Encode(e, buf));
	  out.append(c);
	  c.swap(out);
	  return *this; }
    
    // Unicode handling

    // wsize(): number of characters (rather than bytes) in string
    size_type wsize() { return encoding->CharacterCount(c_str()); }

    // witerator: encoding-aware const iterator
    class witerator {
	friend class string;
	const char *min, *max, *pos;
    public:
	witerator() : min(0), max(0), pos(0) {}
	witerator(const string& o, const char* p)
	    : min(o.c_str()), max(min + o.size()), pos(p) {}
	witerator(const witerator& i) : min(i.min), max(i.max), pos(i.pos) {}
	witerator& operator=(const witerator& i)
	    { pos = i.pos; min = i.min; max = i.max; return *this; }
	bool operator==(const witerator& i) const
	    { return pos == i.pos; }
	bool operator!=(const witerator& i) const
	    { return pos != i.pos; }
	bool operator<=(const witerator& i) const
	    { return !i.pos ? true : (!pos ? false : pos <= i.pos); }
	bool operator<(const witerator& i) const
	    { return !i.pos ? pos != 0 : (!pos ? false : pos < i.pos); }
	bool operator>=(const witerator& i) const
	    { return !pos ? true : (!i.pos ? false : pos >= i.pos); }
	bool operator>(const witerator& i) const
	    { return !i.pos ? false : (!pos ? true : pos > i.pos); }
	wchar operator*() const
	    { return encoding->Decode(pos); }
	witerator& operator++()
	    { pos += encoding->CharacterBytes(pos); if (pos > max) pos = 0;
	      return *this;}
	witerator operator++(int)
	    { witerator rv(*this); operator++(); return rv; }
	witerator& operator--()
	    { pos = encoding->Previous(pos, min); return *this; }
	witerator operator--(int)
	    { witerator rv(*this); operator--(); return rv; }
	witerator& operator+=(size_type n)
	    { while (n--) operator++(); return *this; }
	witerator& operator-=(size_type n)
	    { while (n--) operator--(); return *this; }
	witerator operator+(size_type n)
	    { witerator rv(*this); while (n--) ++rv; return rv; }
	witerator operator-(size_type n)
	    { witerator rv(*this); while (n--) --rv; return rv; }
	long operator-(const witerator& o)
	    { return pos - o.pos; }
    };

    witerator wbegin() const { return witerator(*this, c_str()); }
    witerator wend() const { return witerator(*this, c_str() + size()); }

    // Miscellanea

    // Split string on every instance of delimiter, up to max items.
    vector split(const string& delimiter, int max = 0)
    {
	vector rv;
	size_type spos = 0, epos;
	while (--max && (epos = find(delimiter, spos)) != npos) {
	    rv.push_back(substr(spos, epos - spos));
	    spos = epos + delimiter.size();
	}
	rv.push_back(substr(spos, size() - spos));
	return rv;
    }

    // Split string on every instance of delimiter, taking encoding
    // into account, up to max items.
    vector wsplit(wchar delimiter, int max = 0)
    {
	vector rv;
	size_type spos = 0, epos = 0;
	while (--max && spos < size()) {
	    const char* c = c_str();
	    while (epos < size() && encoding->Decode(c + epos) != delimiter)
		epos += encoding->CharacterBytes(c + epos);
	    if (epos >= size()) break;
	    rv.push_back(substr(spos, epos - spos));
	    spos = epos += encoding->CharacterBytes(c + epos);
	}
	rv.push_back(substr(spos, size() - spos));
	return rv;
    }

    // Trim spaces and tabs.
    
    void ltrim()
    {
	size_type end = c.find_first_not_of(" \t\r\n");
	if (end == npos)
	    c.clear();
	else
	    c.erase(0, end);
    }

    void rtrim()
    {
	size_type end = c.find_last_not_of(" \t\r\n");
	if (end == npos)
	    c.clear();
	else
	    c.erase(end + 1);
    }
    
    void trim()
    {
	rtrim(); ltrim();
    }

    // Case folding (ASCII only; FIXME: NOT encoding-aware!)
    void uppercase()
    {
	for (std::string::iterator it = c.begin(); it != c.end(); ++it)
	    if (*it >= 'a' && *it <= 'z') *it -= 32;
    }
    void lowercase()
    {
	for (std::string::iterator it = c.begin(); it != c.end(); ++it)
	    if (*it >= 'A' && *it <= 'Z') *it += 32;
    }

    // Width folding
    void hantozen();
    void zentohan();
    
    // Character-based replacement
    void replace(char what, char with)
    {
	for (std::string::iterator it = c.begin(); it != c.end(); ++it)
	    if (*it == what) *it = with;
    }
    void replace(wchar what, wchar with)
    {
	std::string out;
	char buf[32];
	for (witerator it = wbegin(); it != wend(); ++it)
	    out.append(buf, encoding->Encode(*it == what ? with : *it, buf));
	c.swap(out);
    }

    // Parse tags in current string; return new string with tags
    // converted to bytecode
    string parseTags()
    {
	string rv;
	witerator it = wbegin();
	if (*it == encoding->TextMarker()) rv += *it++;
	while (it.pos) {
	    if (encoding->UseTags() && *it == '~' && *++it != '~') {
		while (*it != '~') {
		    int l;
		    rv += encoding->TranslateTag(it.pos, l);
		    it.pos += l;
		}
		++it;
	    }
	    else {
		rv += *it++;
	    }
	}
	return rv;
    }
};

inline string operator+(const string& s1, const string& s2)
    { return string(s1) += s2; }
inline string operator+(const char* s1, const string& s2)
    { return string(s1) += s2; }
inline string operator+(const string& s1, const char* s2)
    { return string(s1) += s2; }
inline string operator+(char e, const string& s2)
    { return string(1, e) += s2; }
inline string operator+(const string& s1, char e)
    { return string(s1) += e; }

inline bool operator==(const string& s1, const string& s2)
    { return s1.compare(s2) == 0; }
//inline bool operator==(const char* s1, const string& s2)
//    { return s2.compare(s1) == 0; }
//inline bool operator==(const string& s1, const char* s2)
//    { return s1.compare(s2) == 0; }
inline bool operator!=(const string& s1, const string& s2)
    { return s1.compare(s2); }
//inline bool operator!=(const char* s1, const string& s2)
//    { return s2.compare(s1); }
//inline bool operator!=(const string& s1, const char* s2)
//    { return s1.compare(s2); }
inline bool operator<(const string& s1, const string& s2)
    { return s1.compare(s2) < 0; }
inline bool operator<(const char* s1, const string& s2)
    { return s2.compare(s1) >= 0; }
inline bool operator<(const string& s1, const char* s2)
    { return s1.compare(s2) < 0; }

inline void swap(string& s1, string& s2)
    { swap(s1.c, s2.c); }

inline FILE*& operator>>(FILE*& f, string& dest) {
    dest.clear();
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') dest += char(c);
    return f;
}

extern const string eol;

inline FILE*& operator<<(FILE*& dst, const string& src) {
    fputs(src.c_str(), dst);
    return dst;
}

#endif
