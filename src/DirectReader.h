//$Id:$ -*- C++ -*-
/*
 *  DirectReader.h - Reader from independent files
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

#ifndef __DIRECT_READER_H__
#define __DIRECT_READER_H__

#include "BaseReader.h"
#include "DirPaths.h"

#define MAX_FILE_NAME_LENGTH 256

class DirectReader : public BaseReader {
public:
    DirectReader(DirPaths *path = NULL, const unsigned char* key_table = 0);
    ~DirectReader();

    int open(const pstring& name = "", int archive_type = ARCHIVE_TYPE_NONE);
    int close();

    pstring getArchiveName() const { return "direct"; }
    int getNumFiles();
    void registerCompressionType(const pstring& ext, int type);

    FileInfo getFileByIndex(unsigned int index);
    size_t getFileLength(const pstring& file_name);
    size_t getFile(const pstring& file_name, unsigned char* buffer,
                   int* location = NULL);

//    static string convertFromSJISToEUC(string buf);
    static pstring convertFromSJISToUTF8(const pstring& src);

protected:
    DirPaths *archive_path;
    unsigned char key_table[256];
    bool   key_table_flag;
    int    getbit_mask;
    size_t getbit_len, getbit_count;
    unsigned char* read_buf;
    unsigned char* decomp_buffer;
    size_t decomp_buffer_len;

    // TODO: replace with map
    struct RegisteredCompressionType {
        RegisteredCompressionType* next;
        pstring ext;
        int type;
        RegisteredCompressionType() {
            next = NULL;
        };
        RegisteredCompressionType(pstring new_ext, int type)
            : ext(new_ext)
        {
            ext.toupper();
            this->type = type;
            this->next = NULL;
        };
    } root_registered_compression_type, *last_registered_compression_type;

    FILE* fileopen(pstring path, const char* mode);
    unsigned char readChar(FILE* fp);
    unsigned short readShort(FILE* fp);
    unsigned long readLong(FILE* fp);
    size_t decodeNBZ(FILE* fp, size_t offset, unsigned char* buf);
    int getbit(FILE* fp, int n);
    size_t decodeSPB(FILE* fp, size_t offset, unsigned char* buf);
    size_t decodeLZSS(ArchiveInfo* ai, int no, unsigned char* buf);
    int getRegisteredCompressionType(pstring filename);
    size_t getDecompressedFileLength(int type, FILE* fp, size_t offset);

private:
    FILE* getFileHandle(pstring filename, int& compression_type, size_t* length);
};

#endif // __DIRECT_READER_H__
