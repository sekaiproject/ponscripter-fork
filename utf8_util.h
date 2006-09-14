#ifndef __UTF8_UTIL__
#define __UTF8_UTIL__

#include "stdio.h"

char
CharacterBytes(const char* string);

unsigned short
UnicodeOfUTF8(const char* string);

int
UTF8OfUnicode(const unsigned short ch, char* out);

const char*
PreviousCharacter(const char* string);

unsigned long int
UTF8Length(const char* string);

unsigned short
get_encoded_char(const char encoding, const unsigned short original);

#endif
