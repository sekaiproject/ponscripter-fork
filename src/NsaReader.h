/* -*- C++ -*-
 *
 *  NsaReader.h - Reader from a NSA archive
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

#ifndef __NSA_READER_H__
#define __NSA_READER_H__

#include "SarReader.h"
#define MAX_EXTRA_ARCHIVE 9

class NsaReader : public SarReader {
public:
    NsaReader(DirPaths *path = NULL, const unsigned char* key_table = NULL);
    ~NsaReader();

    int open(const pstring& nsa_path = "", int archive_type = ARCHIVE_TYPE_NSA);
    pstring getArchiveName() const { return "nsa"; }
    int getNumFiles();

    size_t getFileLength(const pstring& file_name);
    size_t getFile(const pstring& file_name, unsigned char* buf,
		   int* location = NULL);
    FileInfo getFileByIndex(unsigned int index);

private:
    bool sar_flag;
    struct ArchiveInfo archive_info2[MAX_EXTRA_ARCHIVE];
    int num_of_nsa_archives;
    pstring nsa_archive_ext;

    size_t getFileLengthSub(ArchiveInfo* ai, const pstring& file_name);
};

#endif // __NSA_READER_H__
