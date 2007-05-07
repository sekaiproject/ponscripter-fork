/* -*- C++ -*-
 *
 *  SarReader.cpp - Reader from a SAR archive
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this
 *  is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
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

#ifndef __SAR_READER_H__
#define __SAR_READER_H__

#include "DirectReader.h"

class SarReader : public DirectReader {
public:
    SarReader(const string& path = "", const unsigned char* key_table = NULL);
    ~SarReader();

    int open(const string& name = "", int archive_type = ARCHIVE_TYPE_SAR);
    int close();
    string getArchiveName() const { return "sar"; }
    int getNumFiles();

    size_t getFileLength(const string& file_name);
    size_t getFile(const string& file_name, unsigned char* buf,
		   int* location = NULL);
    FileInfo getFileByIndex(unsigned int index);

protected:
    ArchiveInfo  archive_info;
    ArchiveInfo* root_archive_info, * last_archive_info;
    int num_of_sar_archives;

    int readArchive(ArchiveInfo* ai, int archive_type = ARCHIVE_TYPE_SAR);
    int getIndexFromFile(ArchiveInfo* ai, string file_name);
    size_t getFileSub(ArchiveInfo* ai, const string& file_name,
		      unsigned char* buf);
};

#endif // __SAR_READER_H__
