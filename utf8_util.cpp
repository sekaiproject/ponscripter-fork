#include "utf8_util.h"
#include "stdlib.h"
#include "stdio.h"

#ifdef LIGATE_ALL
#ifndef LIGATE_FI
#define LIGATE_FI
#endif
#ifndef LIGATE_FL
#define LIGATE_FL
#endif
#ifndef LIGATE_FF
#define LIGATE_FF
#endif
#ifndef LIGATE_PUNCTUATION
#define LIGATE_PUNCTUATION
#endif
#endif
#if defined(LIGATE_ALL) || defined(LIGATE_FI) || defined(LIGATE_FL) || defined(LIGATE_FF) || defined(LIGATE_PUNCTUATION)
#define LIGATURES
#endif

char
CharacterBytes(const char* string)
{
	const unsigned char* t = (const unsigned char*) string;
	const unsigned char c = t[0];
	if (c < 0x80) {
#ifdef LIGATURES
		if (c == '|') return t[1] == '|' ? 2 : 1 + CharacterBytes(string + 1);
#endif
#ifdef LIGATE_FI
		if (c == 'f' && t[1] == 'i') return 2;
#endif
#ifdef LIGATE_FL
		if (c == 'f' && t[1] == 'l') return 2;
#endif
#ifdef LIGATE_FF
		if (c == 'f' && t[1] == 'f') return t[2] == 'i' || t[2] == 'l' ? 3 : 2;
#endif
#ifdef LIGATE_PUNCTUATION
		if (c == '.' && t[1] == '.' && t[2] == '.') return 3;
		if (c == '-' && t[1] == '-') return t[2] == '-' ? 3 : 2;
		if (c == '`') return (t[1] == '`') ? 2 : 1;
		if (c == '\'') return (t[1] == '\'') ? 2 : 1;
		if (c == '(' && (t[1] == 'c' || t[1] == 'r') && t[2] == ')') return 3;
		if (c == '(' && t[1] == 't' && t[2] == 'm' && t[3] == ')') return 4;
		if (c == '*' && t[1] == '*') return 2;
		if (c == '+' && t[1] == '+') return t[2] == '+' ? 3 : 2;
		if (c == '%' && (t[1] == '_' || t[1] == '.' || t[1] == '-')) return 2;
#endif
		return 1;
	}
	else {
		if ((c & 0xc0) == 0x80) fprintf(stderr, "Warning: CharacterBytes called on incomplete character\n");
		if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c) return 3 + CharacterBytes(string + 3); // ZWNJ
		return c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4);
	}
}

unsigned short
UnicodeOfUTF8(const char* string)
{
	const unsigned char* t = (const unsigned char*) string;
	const unsigned char c = t[0];
	if (c < 0x80) {
#ifdef LIGATURES
		if (c == '|') return (t[1] == '|') ? '|' : UnicodeOfUTF8(string + 1);
#endif
#ifdef LIGATE_FI
		if (c == 'f' && t[1] == 'i') return 0xfb01;
#endif
#ifdef LIGATE_FL
		if (c == 'f' && t[1] == 'l') return 0xfb02;
#endif
#ifdef LIGATE_FF
		if (c == 'f' && t[1] == 'f') return t[2] == 'i' ? 0xfb03 : (t[2] == 'l' ? 0xfb04 : 0xfb00);
#endif
#ifdef LIGATE_PUNCTUATION
		if (c == '.' && t[1] == '.' && t[2] == '.') return 0x2026;
		if (c == '-' && t[1] == '-') return t[2] == '-' ? 0x2014 : 0x2013;
		if (c == '`') return t[1] == '`' ? 0x201c : 0x2018;
		if (c == '\'') return t[1] == '\'' ? 0x201d : 0x2019;
		if (c == '(' && (t[1] == 'c' || t[1] == 'r') && t[2] == ')') return t[1] == 'c' ? 0x00a9: 0x00ae;
		if (c == '(' && t[1] == 't' && t[2] == 'm' && t[3] == ')') return 0x2122;
		if (c == '*' && t[1] == '*') return 0x2022;
		if (c == '+' && t[1] == '+') return t[2] == '+' ? 0x2021 : 0x2020;
		if (c == '%' && t[1] == '_') return 0xa0;
		if (c == '%' && t[1] == '.') return 0x2009;
		if (c == '%' && t[1] == '-') return 0x2011;
#endif
		return c;
	}
	else {
		if ((c & 0xc0) == 0x80) fprintf(stderr, "Warning: UnicodeOfUTF8 called on incomplete character\n");
		if (c < 0xe0)
			return (c - 0xc0) << 6 | t[1] & 0x7f;
		else if (c < 0xf0) {
			if (c == 0xe2 && t[1] == 0x80 && t[2] == 0x8c) return UnicodeOfUTF8(string + 3); // ZWNJ
			return ((c - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f;
		}
		return (((c - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f) << 6 | t[3] & 0x7f;
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
	case 'd': encoding  =  Default; return;
	case 'r': encoding &= ~Italic;  return;
	case 'i': encoding ^=  Italic;  return;
	case 't': encoding &= ~Bold;    return;
	case 'b': encoding ^=  Bold;    return;
	case 'f': encoding &= ~Sans;    return;
	case 's': encoding ^=  Sans;    return;
//	case 'o': encoding &= ~Altern;  return;
//	case 'l': encoding ^=  Altern;  return;
	case 0:
		fprintf(stderr, "Error: non-matching ~tags~\n");
		exit(1);
	default:
		fprintf(stderr, "Warning: unknown tag ~%c~\n", flag);
	}
}

char
TranslateTag(const char flag)
{
	switch (flag) {
	case 'd': return 0x10;
	case 'r': return 0x11;
	case 'i': return 0x12;
	case 't': return 0x13;
	case 'b': return 0x14;
	case 'f': return 0x15;
	case 's': return 0x16;
//	case 'o': return 0x17;
//	case 'l': return 0x18;
	case 0:
		fprintf(stderr, "Error: non-matching ~tags~\n");
		exit(1);
	default:
		fprintf(stderr, "Warning: unknown tag ~%c~\n", flag);
		return 0xf000;
	}
}
