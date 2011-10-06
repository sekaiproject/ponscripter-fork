//$Id:$ -*- C++ -*-
/*
 *  DirectReader.cpp - Reader from independent files
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

#include "DirectReader.h"
#include <stdio.h>
#include <bzlib.h>
#if !defined (WIN32) && !defined (PSP) && !defined (__OS2__)
#include <dirent.h>
#endif

#ifdef WIN32
//Mion: support for non-ASCII (SJIS) filenames
#include <wchar.h>
#endif

#ifdef UTF8_FILESYSTEM
#ifdef MACOSX
#include <CoreFoundation/CoreFoundation.h>
#else
#include <iconv.h>
static int iconv_ref_count = 0;
static iconv_t iconv_cd = NULL;
#endif
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#define READ_LENGTH 4096

#define EI 8
#define EJ 4
#define P 1  /* If match length <= P then output one character */
#define N (1 << EI)  /* buffer size */
#define F ((1 << EJ) + P)  /* lookahead buffer size */

DirectReader::DirectReader(DirPaths *path, const unsigned char* key_table)
{
#if defined (UTF8_FILESYSTEM) && !defined (MACOSX)
    if (iconv_cd == NULL) iconv_cd = iconv_open("UTF-8", "SJIS");

    iconv_ref_count++;
#endif

    if ( path != NULL )
        archive_path = path;
    else
        archive_path = new DirPaths("");

    int i;
    if (key_table) {
        key_table_flag = true;
        for (i = 0; i < 256; i++) this->key_table[i] = key_table[i];
    }
    else {
        key_table_flag = false;
        for (i = 0; i < 256; i++) this->key_table[i] = (unsigned char) i;
    }

    read_buf = new unsigned char[READ_LENGTH];
    decomp_buffer = new unsigned char[N * 2];
    decomp_buffer_len = N * 2;

    last_registered_compression_type = &root_registered_compression_type;
    registerCompressionType("SPB", SPB_COMPRESSION);
    registerCompressionType("JPG", NO_COMPRESSION);
    registerCompressionType("GIF", NO_COMPRESSION);
}


DirectReader::~DirectReader()
{
#if defined (UTF8_FILESYSTEM) && !defined (MACOSX)
    if (--iconv_ref_count == 0) {
        iconv_close(iconv_cd);
        iconv_cd = NULL;
    }

#endif
    delete[] read_buf;
    delete[] decomp_buffer;

    last_registered_compression_type = root_registered_compression_type.next;
    while (last_registered_compression_type) {
        RegisteredCompressionType* cur = last_registered_compression_type;
        last_registered_compression_type = last_registered_compression_type->next;
        delete cur;
    }
}

bool NonAsciiFilename(const pstring& filename)
{
    int len = filename.length();
    const unsigned char* c = filename;
    for (int i = 0; i < len; ++i) {
        if (c[i] >= 0x80) {
            return true;
        }
    }
    return false;
}

FILE* DirectReader::fileopen(pstring path, const char* mode)
{
    pstring full_path = "";
    FILE* fp = NULL;

#if defined (UTF8_FILESYSTEM) && !defined (WIN32)
    //preconvert Shift-JIS filename to UTF-8
    //(assumes path uses the script file encoding)
    if ((file_encoding->which() == "cp932") &&
        NonAsciiFilename(path)) {
        path = convertFromSJISToUTF8(path);
    }
#endif

    // Check each archive path until found
    //(full_path probably needs to be ASCII, needs testing...)
    for (int n=-1; n<archive_path->get_num_paths(); n++) {
        if (n == -1) {
            if (archive_path->get_num_paths() == 0) {
                full_path = ".";
            } else {
                n++;
                full_path = archive_path->get_path(n);
            }
        } else
            full_path = archive_path->get_path(n);

        // If the file is trivially found, open it and return the handle.
//printf("DReader::fileopen: about to try '" + full_path + path + "'\n");
        fp = fopen(full_path + path, mode);
        if (fp) return fp;

#ifdef WIN32
        pstring file_full_path = full_path + path;
        // Windows uses UTF-16, so convert for Japanese characters
        if (NonAsciiFilename(file_full_path)) {
            UINT cpage = 932;
            if (file_encoding->which() != "cp932") {
                cpage = CP_UTF8;
            }
            wchar_t *u16_tmp, *umode;
            //convert the file path to from Shift-JIS/UTF-8 to Wide chars (Unicode)
            int wc_size = MultiByteToWideChar(cpage, 0, (const char*)file_full_path, -1, NULL, 0);
            u16_tmp = new wchar_t[wc_size];
            MultiByteToWideChar(cpage, 0, (const char*)file_full_path, -1, u16_tmp, wc_size);
            //need to convert the file mode too
            wc_size = MultiByteToWideChar(cpage, 0, (char*)mode, -1, NULL, 0);
            umode = new wchar_t[wc_size];
            MultiByteToWideChar(cpage, 0, (char*)mode, -1, umode, wc_size);
            fp = _wfopen( u16_tmp, umode );
            //printf("checking utf16 filename: %s\n", fp ? "found" : "not found");
            delete[] u16_tmp;
            delete[] umode;
            if (fp) return fp;
        }
#endif

#if !defined (WIN32) && !defined (PSP) && !defined (__OS2__)
        // For systems that are case-sensitive, split the path into
        // directories and check each for correct case, correcting
        // the case in memory as appropriate, until we either
        // find the file or show that it doesn't exist.

        full_path.rtrim(DELIMITER);

        // Get the constituent parts of the file path.
        CBStringList parts = path.split(DELIMITER);

        // Correct the case of each.
        bool found = false;
        for (CBStringList::iterator it = parts.begin(); it != parts.end(); ++it) {
            found = false;
            DIR* dp = opendir(full_path);
            if (!dp) {
                fp = NULL;
                break;
            }
            dirent* entry;
            while ((entry = readdir(dp))) {
                pstring item = entry->d_name;
                if (it->caselessEqual(item)) {
                    found = true;
                    full_path += DELIMITER;
                    full_path += item;
                    break;
                }
            }
            closedir(dp);
        }
        if (!found) continue;
        fp = fopen(full_path, mode);
        if (fp) return fp;
#endif
    }
    return fp;
}


unsigned char DirectReader::readChar(FILE* fp)
{
    unsigned char ret;

    fread(&ret, 1, 1, fp);
    return key_table[ret];
}


unsigned short DirectReader::readShort(FILE* fp)
{
    int ret;
    unsigned char buf[2];

    fread(&buf, 1, 2, fp);
    ret = key_table[buf[0]] << 8 | key_table[buf[1]];
    return (unsigned short) ret;
}


unsigned long DirectReader::readLong(FILE* fp)
{
    unsigned long ret;
    unsigned char buf[4];

    fread(&buf, 1, 4, fp);
    ret = key_table[buf[0]];
    ret = ret << 8 | key_table[buf[1]];
    ret = ret << 8 | key_table[buf[2]];
    ret = ret << 8 | key_table[buf[3]];
    return ret;
}


int DirectReader::open(const pstring& name, int archive_type)
{
    return 0;
}


int DirectReader::close()
{
    return 0;
}


int DirectReader::getNumFiles()
{
    return 0;
}


void DirectReader::registerCompressionType(const pstring& ext, int type)
{
    last_registered_compression_type->next
	= new RegisteredCompressionType(ext, type);
    last_registered_compression_type
	= last_registered_compression_type->next;
}


int DirectReader::getRegisteredCompressionType(pstring filename)
{
    pstring ext = file_extension(filename);
    ext.toupper();
    RegisteredCompressionType* reg = root_registered_compression_type.next;
    while (reg) {
        if (ext == reg->ext) return reg->type;
        reg = reg->next;
    }
    return NO_COMPRESSION;
}


DirectReader::FileInfo DirectReader::getFileByIndex(unsigned int index)
{
    DirectReader::FileInfo fi;
    memset(&fi, 0, sizeof(DirectReader::FileInfo));
    return fi;
}


FILE* DirectReader::getFileHandle(pstring filename, int& compression_type,
                                  size_t* length)
{
    FILE* fp = NULL;
    *length = 0;
    compression_type = NO_COMPRESSION;
    filename.findreplace("/", DELIMITER);
    filename.findreplace("\\", DELIMITER);
    if ((filename.length() >= 3) && ((fp = fileopen(filename, "rb")) != NULL)) {
        compression_type = getRegisteredCompressionType(filename);
        if (compression_type == NBZ_COMPRESSION ||
            compression_type == SPB_COMPRESSION) {
            *length = getDecompressedFileLength(compression_type, fp, 0);
        }
        else {
            fseek(fp, 0, SEEK_END);
            *length = ftell(fp);
            fseek(fp, 0, SEEK_SET);
        }
    }
    return fp;
}


size_t DirectReader::getFileLength(const pstring& file_name)
{
    int compression_type;
    size_t len;
    FILE* fp = getFileHandle(file_name, compression_type, &len);
    if (fp) fclose(fp);
    return len;
}


size_t DirectReader::getFile(const pstring& file_name, unsigned char* buffer,
			     int* location)
{
    int compression_type;
    size_t len, c, total = 0;
    FILE*  fp = getFileHandle(file_name, compression_type, &len);

    if (fp) {
        if (compression_type & NBZ_COMPRESSION)
	    return decodeNBZ(fp, 0, buffer);
        else if (compression_type & SPB_COMPRESSION)
	    return decodeSPB(fp, 0, buffer);

        total = len;
        while (len > 0) {
            if (len > READ_LENGTH) c = READ_LENGTH;
            else c = len;

            len -= c;
            fread(buffer, 1, c, fp);
            buffer += c;
        }
        fclose(fp);
        if (location) *location = ARCHIVE_TYPE_NONE;
    }

    return total;
}


pstring DirectReader::convertFromSJISToUTF8(const pstring& src)
{
#if defined(UTF8_FILESYSTEM)
    //not sure how efficient this is...
    CP932Encoding cp932;
    UTF8Encoding utf8;
    const char* c = src;
    pstring dst = "";
    while (*c) {
        int b;
        dst += utf8->Encode(cp932.DecodeChar(c, b));
        c += b;
    }
#elif defined(WIN32)
    const char *src_ptr = src;
    int wc_size = MultiByteToWideChar(932, 0, src_ptr, -1, NULL, 0);
    wchar_t *u16_tmp = new wchar_t[wc_size];
    MultiByteToWideChar(932, 0, src_ptr, -1, u16_tmp, wc_size);
    int mb_size = WideCharToMultiByte(CP_UTF8, 0, u16_tmp, wc_size, NULL, 0, NULL, NULL);
    char *dst_buf = new char[mb_size];
    WideCharToMultiByte(CP_UTF8, 0, u16_tmp, wc_size, dst_buf, mb_size, NULL, NULL);
    pstring dst = pstring(dst_buf);
    delete[] u16_tmp;
    delete[] dst_buf;
#endif //UTF8_FILESYSTEM, WIN32

    return dst;
}


size_t DirectReader::decodeNBZ(FILE* fp, size_t offset, unsigned char* buf)
{
    if (key_table_flag)
        fprintf(stderr, "may not decode NBZ with key_table enabled.\n");

    unsigned int original_length, count;
    BZFILE* bfp;
    void*   unused;
    int err, len, nunused;

    fseek(fp, offset, SEEK_SET);
    original_length = count = readLong(fp);

    bfp = BZ2_bzReadOpen(&err, fp, 0, 0, NULL, 0);
    if (bfp == NULL || err != BZ_OK) return 0;

    while (err == BZ_OK && count > 0) {
        if (count >= READ_LENGTH)
            len = BZ2_bzRead(&err, bfp, buf, READ_LENGTH);
        else
            len = BZ2_bzRead(&err, bfp, buf, count);

        count -= len;
        buf += len;
    }

    BZ2_bzReadGetUnused(&err, bfp, &unused, &nunused);
    BZ2_bzReadClose(&err, bfp);

    return original_length - count;
}


int DirectReader::getbit(FILE* fp, int n)
{
    int i, x = 0;
    static int getbit_buf;

    for (i = 0; i < n; i++) {
        if (getbit_mask == 0) {
            if (getbit_len == getbit_count) {
                getbit_len = fread(read_buf, 1, READ_LENGTH, fp);
                if (getbit_len == 0) return EOF;

                getbit_count = 0;
            }

            getbit_buf  = key_table[read_buf[getbit_count++]];
            getbit_mask = 128;
        }

        x <<= 1;
        if (getbit_buf & getbit_mask) x++;

        getbit_mask >>= 1;
    }

    return x;
}


size_t DirectReader::decodeSPB(FILE* fp, size_t offset, unsigned char* buf)
{
    unsigned int   count;
    unsigned char* pbuf, * psbuf;
    size_t i, j, k;
    int c, n, m;

    getbit_mask = 0;
    getbit_len  = getbit_count = 0;

    fseek(fp, offset, SEEK_SET);
    size_t width  = readShort(fp);
    size_t height = readShort(fp);

    size_t width_pad = (4 - width * 3 % 4) % 4;

    size_t total_size = (width * 3 + width_pad) * height + 54;

    /* ---------------------------------------- */
    /* Write header */
    memset(buf, 0, 54);
    buf[0]  = 'B'; buf[1] = 'M';
    buf[2]  = total_size & 0xff;
    buf[3]  = (total_size >> 8) & 0xff;
    buf[4]  = (total_size >> 16) & 0xff;
    buf[5]  = (total_size >> 24) & 0xff;
    buf[10] = 54; // offset to the body
    buf[14] = 40; // header size
    buf[18] = width & 0xff;
    buf[19] = (width >> 8) & 0xff;
    buf[22] = height & 0xff;
    buf[23] = (height >> 8) & 0xff;
    buf[26] = 1; // the number of the plane
    buf[28] = 24; // bpp
    buf[34] = total_size - 54; // size of the body

    buf += 54;

    if (decomp_buffer_len < width * height + 4) {
        if (decomp_buffer) delete[] decomp_buffer;

        decomp_buffer_len = width * height + 4;
        decomp_buffer = new unsigned char[decomp_buffer_len];
    }

    for (i = 0; i < 3; i++) {
        count = 0;
        decomp_buffer[count++] = c = getbit(fp, 8);
        while (count < (unsigned) (width * height)) {
            n = getbit(fp, 3);
            if (n == 0) {
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                decomp_buffer[count++] = c;
                continue;
            }
            else if (n == 7) {
                m = getbit(fp, 1) + 1;
            }
            else {
                m = n + 2;
            }

            for (j = 0; j < 4; j++) {
                if (m == 8) {
                    c = getbit(fp, 8);
                }
                else {
                    k = getbit(fp, m);
                    if (k & 1) c += (k >> 1) + 1;
                    else c -= (k >> 1);
                }

                decomp_buffer[count++] = c;
            }
        }

        pbuf  = buf + (width * 3 + width_pad) * (height - 1) + i;
        psbuf = decomp_buffer;

        for (j = 0; j < height; j++) {
            if (j & 1) {
                for (k = 0; k < width; k++, pbuf -= 3) *pbuf = *psbuf++;

                pbuf -= width * 3 + width_pad - 3;
            }
            else {
                for (k = 0; k < width; k++, pbuf += 3) *pbuf = *psbuf++;

                pbuf -= width * 3 + width_pad + 3;
            }
        }
    }

    return total_size;
}


size_t DirectReader::decodeLZSS(ArchiveInfo* ai, int no, unsigned char* buf)
{
    unsigned int count = 0;
    int i, j, k, r, c;

    getbit_mask = 0;
    getbit_len  = getbit_count = 0;

    fseek(ai->file_handle, ai->fi_list[no].offset, SEEK_SET);
    memset(decomp_buffer, 0, N - F);
    r = N - F;

    while (count < ai->fi_list[no].original_length) {
        if (getbit(ai->file_handle, 1)) {
            if ((c = getbit(ai->file_handle, 8)) == EOF) break;

            buf[count++] = c;
            decomp_buffer[r++] = c;  r &= (N - 1);
        }
        else {
            if ((i = getbit(ai->file_handle, EI)) == EOF) break;

            if ((j = getbit(ai->file_handle, EJ)) == EOF) break;

            for (k = 0; k <= j + 1; k++) {
                c = decomp_buffer[(i + k) & (N - 1)];
                buf[count++] = c;
                decomp_buffer[r++] = c;  r &= (N - 1);
            }
        }
    }

    return count;
}


size_t DirectReader::getDecompressedFileLength(int type, FILE* fp, size_t offset)
{
    fpos_t pos;
    size_t length = 0;
    fgetpos(fp, &pos);
    fseek(fp, offset, SEEK_SET);

    if (type == NBZ_COMPRESSION) {
        length = readLong(fp);
    }
    else if (type == SPB_COMPRESSION) {
        size_t width     = readShort(fp);
        size_t height    = readShort(fp);
        size_t width_pad = (4 - width * 3 % 4) % 4;

        length = (width * 3 + width_pad) * height + 54;
    }

    fsetpos(fp, &pos);

    return length;
}
