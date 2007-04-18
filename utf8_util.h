/* -*- C++ -*-
 *
 *  utf8_util.cpp -- utility functions for handling Unicode text and
 *                   ligatures in Ponscripter
 *
 *  Copyright (c) 2007 Peter Jolly
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

#ifndef __UTF8_UTIL__
#define __UTF8_UTIL__

#include "stdio.h"
#include "pstring.h"

// Style bits
const int Default = 0;
const int Italic  = 1;
const int Bold    = 2;
const int Sans    = 4;

char CharacterBytes(const char* string);

unsigned short UnicodeOfUTF8(const char* string);

int UTF8OfUnicode(const unsigned short ch, char* out);
string UTF8OfUnicode(const unsigned short ch);

const char* PreviousCharacter(const char* string);

unsigned long int UTF8Length(const char* string);

void SetEncoding(int& encoding, const char flag);

string TranslateTag(const char* flag, int& in_len);

void AddLigature(const char* in, unsigned short out);
void DeleteLigature(const char* in);
void DefaultLigatures(int which);
void ClearLigatures();
void DumpLigatures();

#endif
