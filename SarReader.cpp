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

#include "SarReader.h"
#define WRITE_LENGTH 4096

SarReader::SarReader(const string& path, const unsigned char* key_table)
    : DirectReader(path, key_table),
      num_of_sar_archives(0)
{
    root_archive_info   = last_archive_info = &archive_info;
}


SarReader::~SarReader()
{
    close();
}


int SarReader::open(const string& name, int archive_type)
{
    ArchiveInfo* info = new ArchiveInfo();

    if ((info->file_handle = fileopen(name, "rb")) == NULL) {
        delete info;
        return -1;
    }

    info->file_name = name;

    readArchive(info);

    last_archive_info->next = info;
    last_archive_info = last_archive_info->next;
    num_of_sar_archives++;

    return 0;
}


int SarReader::readArchive(ArchiveInfo* ai, int archive_type)
{
    unsigned int i = 0;

    /* Read header */
    if (archive_type == ARCHIVE_TYPE_NS2) {
        i = readChar(ai->file_handle); // FIXME: for what ?
    }

    if (archive_type == ARCHIVE_TYPE_NS3) {
        i = readChar(ai->file_handle); // FIXME: for what ?
        i = readChar(ai->file_handle); // FIXME: for what ?
    }

    ai->num_of_files = readShort(ai->file_handle);
    ai->fi_list = new FileInfo[ai->num_of_files];

    ai->base_offset = readLong(ai->file_handle);
    if (archive_type == ARCHIVE_TYPE_NS2)
        ai->base_offset++;

    if (archive_type == ARCHIVE_TYPE_NS3)
        ai->base_offset += 2;

    for (i = 0; i < ai->num_of_files; i++) {
        unsigned char ch;

	ai->fi_list[i].name.clear();
        while ((ch = key_table[fgetc(ai->file_handle)])) {
            ai->fi_list[i].name.push_uchar(ch);
        }

        if (archive_type >= ARCHIVE_TYPE_NSA)
            ai->fi_list[i].compression_type = readChar(ai->file_handle);
        else
            ai->fi_list[i].compression_type = NO_COMPRESSION;

        ai->fi_list[i].offset = readLong(ai->file_handle) + ai->base_offset;
        ai->fi_list[i].length = readLong(ai->file_handle);

        if (archive_type >= ARCHIVE_TYPE_NSA) {
            ai->fi_list[i].original_length = readLong(ai->file_handle);
        }
        else {
            ai->fi_list[i].original_length = ai->fi_list[i].length;
        }

        /* Registered Plugin check */
        if (ai->fi_list[i].compression_type == NO_COMPRESSION)
            ai->fi_list[i].compression_type =
		getRegisteredCompressionType(ai->fi_list[i].name);

        if (ai->fi_list[i].compression_type == NBZ_COMPRESSION
            || ai->fi_list[i].compression_type == SPB_COMPRESSION) {
            ai->fi_list[i].original_length =
		getDecompressedFileLength(ai->fi_list[i].compression_type,
					  ai->file_handle,
					  ai->fi_list[i].offset);
        }
    }

    return 0;
}


int SarReader::close()
{
    ArchiveInfo* info = archive_info.next;

    for (int i = 0; i < num_of_sar_archives; i++) {
        if (info->file_handle) {
            fclose(info->file_handle);
            delete[] info->fi_list;
        }

        last_archive_info = info;
        info = info->next;
        delete last_archive_info;
    }

    return 0;
}


int SarReader::getNumFiles()
{
    ArchiveInfo* info = archive_info.next;
    int num = 0;

    for (int i = 0; i < num_of_sar_archives; i++) {
        num += info->num_of_files;
        info = info->next;
    }

    return num;
}


int SarReader::getIndexFromFile(ArchiveInfo* ai, string file_name)
{
    unsigned int i;

    file_name.replace(wchar('/'), wchar('\\'));
    
    for (i = 0; i < ai->num_of_files; i++)
	if (file_name.icompare(ai->fi_list[i].name) == 0) break;

    return i;
}


size_t SarReader::getFileLength(const string& file_name)
{
    size_t ret;
    if ((ret = DirectReader::getFileLength(file_name))) return ret;

    ArchiveInfo* info = archive_info.next;
    unsigned int j = 0;
    for (int i = 0; i < num_of_sar_archives; i++) {
        j = getIndexFromFile(info, file_name);
        if (j != info->num_of_files) break;

        info = info->next;
    }

    if (!info) return 0;

    if (info->fi_list[j].compression_type == NO_COMPRESSION) {
        int type = getRegisteredCompressionType(file_name);
        if (type == NBZ_COMPRESSION || type == SPB_COMPRESSION)
            return getDecompressedFileLength(type, info->file_handle,
					     info->fi_list[j].offset);
    }

    return info->fi_list[j].original_length;
}


size_t SarReader::getFileSub(ArchiveInfo* ai, const string& file_name,
			     unsigned char* buf)
{
    unsigned int i = getIndexFromFile(ai, file_name);
    if (i == ai->num_of_files) return 0;

    int type = ai->fi_list[i].compression_type;
    if (type == NO_COMPRESSION) type = getRegisteredCompressionType(file_name);

    if (type == NBZ_COMPRESSION) {
        return decodeNBZ(ai->file_handle, ai->fi_list[i].offset, buf);
    }
    else if (type == LZSS_COMPRESSION) {
        return decodeLZSS(ai, i, buf);
    }
    else if (type == SPB_COMPRESSION) {
        return decodeSPB(ai->file_handle, ai->fi_list[i].offset, buf);
    }

    fseek(ai->file_handle, ai->fi_list[i].offset, SEEK_SET);
    size_t ret = fread(buf, 1, ai->fi_list[i].length, ai->file_handle);
    for (size_t j = 0; j < ret; j++) buf[j] = key_table[buf[j]];

    return ret;
}


size_t SarReader::getFile(const string& file_name, unsigned char* buf,
			  int* location)
{
    size_t ret;
    if ((ret = DirectReader::getFile(file_name, buf, location))) return ret;

    ArchiveInfo* info = archive_info.next;
    size_t j = 0;
    for (int i = 0; i < num_of_sar_archives; i++) {
        if ((j = getFileSub(info, file_name, buf)) > 0) break;

        info = info->next;
    }

    if (location) *location = ARCHIVE_TYPE_SAR;

    return j;
}


SarReader::FileInfo SarReader::getFileByIndex(unsigned int index)
{
    ArchiveInfo* info = archive_info.next;
    for (int i = 0; i < num_of_sar_archives; i++) {
        if (index < info->num_of_files) return info->fi_list[index];

        index -= info->num_of_files;
        info = info->next;
    }

    fprintf(stderr, "SarReader::getFileByIndex  Index %d is out of range\n", index);

    return archive_info.fi_list[index];
}
