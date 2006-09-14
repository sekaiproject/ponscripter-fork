#ifndef __UTF8_UTIL__
#define __UTF8_UTIL__

#include "stdio.h"

inline char
CharacterBytes(const char* string)
{
	const unsigned char c = *(const unsigned char*) string;
	if ((c & 0xc0) == 0x80) fprintf(stderr, "Warning: CharacterBytes called on incomplete character\n");
	return c < 0x80 ? 1 : (c < 0xe0 ? 2 : (c < 0xf0 ? 3 : 4));
}

inline unsigned short
UnicodeOfUTF8(const char* string)
{
	const unsigned char* t = (const unsigned char*) string;
	if ((t[0] & 0xc0) == 0x80) fprintf(stderr, "Warning: UnicodeOfUTF8 called on incomplete character\n");
	if (t[0] < 0x80)
		return t[0];
	else if (t[0] < 0xe0)
		return (t[0] - 0xc0) << 6 | t[1] & 0x7f;
	else if (t[0] < 0xf0)
		return ((t[0] - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f;
	return (((t[0] - 0xe0) << 6 | t[1] & 0x7f) << 6 | t[2] & 0x7f) << 6 | t[3] & 0x7f;
}

void
UTF8OfUnicode(const unsigned short ch, char* out);

const char*
PreviousCharacter(const char* string);

unsigned long int
UTF8Length(const char* string);

unsigned short
get_encoded_char(const char encoding, const unsigned short original);

#endif
