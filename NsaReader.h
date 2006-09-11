/* -*- C++ -*-
 *
 *  NsaReader.h - Reader from a NSA archive
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NSA_READER_H__
#define __NSA_READER_H__

#include "SarReader.h"
#define MAX_EXTRA_ARCHIVE 9

class NsaReader : public SarReader
{
public:
    NsaReader( char *path=NULL, const unsigned char *key_table=NULL );
    ~NsaReader();

    int open( char *nsa_path=NULL, int archive_type = ARCHIVE_TYPE_NSA );
    char *getArchiveName() const;
    int getNumFiles();
    
    size_t getFileLength( const char *file_name );
    size_t getFile( const char *file_name, unsigned char *buf, int *location=NULL );
    struct FileInfo getFileByIndex( unsigned int index );

    int openForConvert( char *nsa_name, int archive_type=ARCHIVE_TYPE_NSA );
    int writeHeader( FILE *fp, int archive_type=ARCHIVE_TYPE_NSA );
    size_t putFile( FILE *fp, int no, size_t offset, size_t length, size_t original_length, int compression_type, bool modified_flag, unsigned char *buffer );
    
private:
    bool sar_flag;
    struct ArchiveInfo archive_info2[MAX_EXTRA_ARCHIVE];
    int num_of_nsa_archives;
    char *nsa_archive_ext;

    size_t getFileLengthSub( ArchiveInfo *ai, const char *file_name );
};

#endif // __NSA_READER_H__
