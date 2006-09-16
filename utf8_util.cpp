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

char
CharacterBytes(const char* string)
{
	const unsigned char c = *(const unsigned char*) string;
	if ((c & 0xc0) == 0x80) fprintf(stderr, "Warning: CharacterBytes called on incomplete character\n");
#ifdef LIGATE_FI
	if (c == 'f' && string[1] == 'i') return 2;
#endif
#ifdef LIGATE_FL
	if (c == 'f' && string[1] == 'l') return 2;
#endif
#ifdef LIGATE_FF
	if (c == 'f' && string[1] == 'f') return (string[2] == 'i' || string[2] == 'l') ? 3 : 2;
#endif
#ifdef LIGATE_PUNCTUATION
	if (c == '.' && string[1] == '.' && string[2] == '.') return 3;
	if (c == '-' && string[1] == '-') return (string[2] == '-') ? 3 : 2;
	if (c == '`') return string[1] == '`' ? 2 : 1;
	if (c == '\'') return string[1] == '\'' ? 2 : 1;
#endif
	return c < 0x80 ? 1 : (c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4));
}

unsigned short
UnicodeOfUTF8(const char* string)
{
	const unsigned char* t = (const unsigned char*) string;
	if ((t[0] & 0xc0) == 0x80) fprintf(stderr, "Warning: UnicodeOfUTF8 called on incomplete character\n");
#ifdef LIGATE_FI
	if (t[0] == 'f' && t[1] == 'i') return 0xfb01;
#endif
#ifdef LIGATE_FL
	if (t[0] == 'f' && t[1] == 'l') return 0xfb02;
#endif
#ifdef LIGATE_FF
	if (t[0] == 'f' && t[1] == 'f') return t[2] == 'i' ? 0xfb03 : (t[2] == 'l' ? 0xfb04 : 0xfb00);
#endif
#ifdef LIGATE_PUNCTUATION
	if (t[0] == '.' && t[1] == '.' && t[2] == '.') return 0x2026;
	if (t[0] == '-' && t[1] == '-') return t[2] == '-' ? 0x2014 : 0x2013;
	if (t[0] == '`') return t[1] == '`' ? 0x201c : 0x2018;
	if (t[0] == '\'') return t[1] == '\'' ? 0x201d : 0x2019;
#endif
	if (t[0] < 0x80)
		return t[0];
	else if (t[0] < 0xe0)
		return (t[0] - 0xc0) << 6 | t[1] & 0x7f;
	else if (t[0] < 0xf0)
		return ((t[0] - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f;
	return (((t[0] - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f) << 6 | t[3] & 0x7f;
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

static const unsigned short extra_chars[] = {
	0x2019, 0x2018, 0x2044, 0x0192, 0x201c, 0x2039, 0x203a, 0xfb01, 0xfb02, 
	0x2013, 0x2020, 0x2021, 0x2022, 0x201a, 0x201e, 0x201d, 0x2026, 0x2030, 
	0x02c6, 0x02dc, 0x02d8, 0x02d9, 0x02da, 0x02dd, 0x02db, 0x02c7, 0x2014, 
	0x0141, 0x0152, 0x0131, 0x0142, 0x0153, 0x0160, 0x0178, 0x017d, 0x0161, 
	0x017e, 0x2122, 0x2212, 0x20ac, 0xfb00, 0xfb03, 0xfb04, 0
};

static const unsigned short offset[] = {
	0x0000, // Roman
	0xe000, // Italic
	0xe128, // Bold
	0xe250, // Bold Italic
	0xe378, // Sans
	0xe4a0, // Sans Italic
	0xe5c8, // Sans Bold
	0xe6f0  // Sans Bold Italic
};

// Used to support proprietary encoding of italic, etc. in private use area.
unsigned short
get_encoded_char(const int encoding, const unsigned short original)
{
	if (encoding == 0) return original;

	if (encoding & Altern && original >= '0' && original <= '9') {
		if (encoding ^ Altern == Default) return original + 0xe000 - '0';
		return original + offset[encoding ^ Altern] + 0x20;
	}	
	
	if (encoding == 'o') return (original >= '0' && original <= '9') ? original + (0xe000 - '0') : original;
	unsigned short compact_enc;
	if (original <= 0xff)
		compact_enc = original;
	else {
		compact_enc = 0;
		for (int idx = 0; extra_chars[idx]; ++idx) if (extra_chars[idx] == original) {
			compact_enc = 0x101 + idx;
			break;
		}
	}
	return compact_enc 
	     ? compact_enc + offset[encoding & ~Altern] 
	     : original;
}

void SetEncoding(int& encoding, const char flag)
{
	switch (flag) {
	case 'd': encoding  =  Default; return;
	case 'r': encoding &= ~Italic;  return;
	case 'i': encoding ^=  Italic;  return;
	case 't': encoding &= ~Bold;    return;
	case 'b': encoding ^=  Bold;    return;
	case 'f': encoding &= ~Sans;    return;
	case 's': encoding ^=  Sans;    return;
	case 'o': encoding &= ~Altern;  return;
	case 'l': encoding ^=  Altern;  return;
	case 0:
		fprintf(stderr, "Error: non-matching ~tags~\n");
		exit(1);
	default:
		fprintf(stderr, "Warning: unknown tag ~%c~\n", flag);
	}
}
