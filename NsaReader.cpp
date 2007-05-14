/* -*- C++ -*-
 *
 *  NsaReader.cpp - Reader from a NSA archive
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

#include "NsaReader.h"
#include <string.h>

NsaReader::NsaReader(const string& path, const unsigned char* key_table)
    : SarReader(path, key_table),
      sar_flag(true),
      num_of_nsa_archives(0),
      nsa_archive_ext(key_table ? "___" : "nsa")
{}


NsaReader::~NsaReader()
{ }


int NsaReader::open(const string& nsa_path, int archive_type)
{
    if (!SarReader::open("arc.sar")) return 0;
    sar_flag = false;

    string archive_name = nsa_path + "arc." + nsa_archive_ext;
    if ((archive_info.file_handle = fileopen(archive_name, "rb")) == NULL) {
        fprintf(stderr, "can't open file %s\n", archive_name.c_str());
        return -1;
    }

    archive_info.file_name = archive_name;
    readArchive(&archive_info, archive_type);

    for (int i = 0; i < MAX_EXTRA_ARCHIVE; ++i) {
	string arcname2("arc" + str(i + 1));
	archive_name = nsa_path + arcname2 + "." + nsa_archive_ext;
        if ((archive_info2[i].file_handle = fileopen(archive_name, "rb")) == NULL)
            return 0;
        archive_info2[i].file_name = arcname2;
        num_of_nsa_archives = i + 1;
        readArchive(&archive_info2[i], archive_type);
    }

    return 0;
}


int NsaReader::getNumFiles()
{
    int total = archive_info.num_of_files, i;

    for (i = 0; i < num_of_nsa_archives; i++)
	total += archive_info2[i].num_of_files;

    return total;
}


size_t NsaReader::getFileLengthSub(ArchiveInfo* ai, const string& file_name)
{
    unsigned int i = getIndexFromFile(ai, file_name);

    if (i == ai->num_of_files) return 0;

    if (ai->fi_list[i].compression_type == NO_COMPRESSION) {
        int type = getRegisteredCompressionType(file_name);
        if (type == NBZ_COMPRESSION || type == SPB_COMPRESSION)
            return getDecompressedFileLength(type, ai->file_handle,
					     ai->fi_list[i].offset);
    }

    return ai->fi_list[i].original_length;
}


size_t NsaReader::getFileLength(const string& file_name)
{
    if (sar_flag) return SarReader::getFileLength(file_name);

    size_t ret;
    int i;

    if ((ret = DirectReader::getFileLength(file_name))) return ret;

    if ((ret = getFileLengthSub(&archive_info, file_name))) return ret;

    for (i = 0; i < num_of_nsa_archives; i++) {
        if ((ret = getFileLengthSub(&archive_info2[i], file_name))) return ret;
    }

    return 0;
}


size_t NsaReader::getFile(const string& file_name, unsigned char* buffer,
			  int* location)
{
    size_t ret;

    if (sar_flag)
	return SarReader::getFile(file_name, buffer, location);

    if ((ret = DirectReader::getFile(file_name, buffer, location)))
	return ret;

    if ((ret = getFileSub(&archive_info, file_name, buffer))) {
        if (location) *location = ARCHIVE_TYPE_NSA;

        return ret;
    }

    for (int i = 0; i < num_of_nsa_archives; i++) {
        if ((ret = getFileSub(&archive_info2[i], file_name, buffer))) {
            if (location) *location = ARCHIVE_TYPE_NSA;

            return ret;
        }
    }

    return 0;
}


NsaReader::FileInfo NsaReader::getFileByIndex(unsigned int index)
{
    int i;

    if (index < archive_info.num_of_files) return archive_info.fi_list[index];

    index -= archive_info.num_of_files;

    for (i = 0; i < num_of_nsa_archives; i++) {
        if (index < archive_info2[i].num_of_files)
	    return archive_info2[i].fi_list[index];

        index -= archive_info2[i].num_of_files;
    }

    fprintf(stderr, "NsaReader::getFileByIndex  Index %d is out of range\n", index);

    return archive_info.fi_list[0];
}
