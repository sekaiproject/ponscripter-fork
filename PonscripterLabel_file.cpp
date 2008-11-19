/* -*- C++ -*-
 *
 *  PonscripterLabel_file.cpp - FILE I/O of Ponscripter
 *
 *  Copyright (c) 2001-2007 Ogapee (original ONScripter, of which this
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

#include "PonscripterLabel.h"

#if defined (LINUX) || defined (MACOSX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#elif defined (WIN32)
#include <windows.h>
#elif defined (MACOS9)
#include <DateTimeUtils.h>
#include <Files.h>
extern "C" void c2pstrcpy(Str255 dst, const char* src);

#elif defined (PSP)
#include <pspiofilemgr.h>
#endif

#define SAVEFILE_VERSION_MAJOR 2
#define SAVEFILE_VERSION_MINOR 6

#define READ_LENGTH 4096

void PonscripterLabel::searchSaveFile(SaveFileInfo &save_file_info, int no)
{
    save_file_info.num_str
	= script_h.stringFromInteger(no, num_save_file >= 10 ? 2 : 1);

    pstring filename;
    filename.format("%ssave%d.dat", (const char*) script_h.save_path, no);
#if defined (LINUX) || defined (MACOSX)
    struct stat buf;
    struct tm*  tm;
    if (stat(filename, &buf) != 0) {
        save_file_info.valid = false;
        return;
    }

    tm = localtime(&buf.st_mtime);

    save_file_info.month  = tm->tm_mon + 1;
    save_file_info.day    = tm->tm_mday;
    save_file_info.hour   = tm->tm_hour;
    save_file_info.minute = tm->tm_min;
#elif defined (WIN32)
    HANDLE     handle;
    FILETIME   tm, ltm;
    SYSTEMTIME stm;

    handle = CreateFile(filename, GENERIC_READ, 0, NULL,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        save_file_info.valid = false;
        return;
    }

    GetFileTime(handle, NULL, NULL, &tm);
    FileTimeToLocalFileTime(&tm, &ltm);
    FileTimeToSystemTime(&ltm, &stm);
    CloseHandle(handle);

    save_file_info.month  = stm.wMonth;
    save_file_info.day    = stm.wDay;
    save_file_info.hour   = stm.wHour;
    save_file_info.minute = stm.wMinute;
#elif defined (PSP)
    SceIoStat buf;
    if (sceIoGetstat(filename, &buf) < 0) {
        save_file_info.valid = false;
        return;
    }

    save_file_info.month  = buf.st_mtime.month;
    save_file_info.day    = buf.st_mtime.day;
    save_file_info.hour   = buf.st_mtime.hour;
    save_file_info.minute = buf.st_mtime.minute;
#else
    FILE* fp;
    if ((fp = fopen(filename, "rb")) == NULL) {
        save_file_info.valid = false;
        return;
    }
    fclose(fp);

    save_file_info.month  = 1;
    save_file_info.day    = 1;
    save_file_info.hour   = 0;
    save_file_info.minute = 0;
#endif
    save_file_info.valid = true;
}


int PonscripterLabel::loadSaveFile(int no)
{
    pstring filename;
    filename.format("save%d.dat", no);
    if (loadFileIOBuf(filename)) {
        fprintf(stderr, "can't open save file save%d.dat\n", no);
        return -1;
    }

    int file_version;

    /* ---------------------------------------- */
    /* Load magic number */
    SaveFileType file_type = NScripter;
    int c = readChar();
    if (c == 'O' || c == 'P') {
	int d = readChar();
	if (d == 'N' && readChar() == 'S')
	    file_type = c == 'O' ? ONScripter : Ponscripter;
    }
    
    if (file_type == NScripter) { // if not ONS save file
        file_io_buf_ptr = 0;
        // check for ONS version 0
        bool ons_ver0_flag = readInt() != 1;

        readInt();
        readInt();
        if (readInt() != 0) ons_ver0_flag = true;

        readInt();
        if (readInt() != 0xff) ons_ver0_flag = true;

        if (readInt() != 0xff) ons_ver0_flag = true;

        if (readInt() != 0xff) ons_ver0_flag = true;

        file_io_buf_ptr = 0;
        if (!ons_ver0_flag) {
            return loadSaveFile2(file_type, SAVEFILE_VERSION_MAJOR * 100 +
				 SAVEFILE_VERSION_MINOR);
        }

	file_type = ONScripter;
        file_version = 0;
    }
    else {
        file_version  = readChar() * 100;
        file_version += readChar();
    }

    if (file_version > SAVEFILE_VERSION_MAJOR * 100 + SAVEFILE_VERSION_MINOR) {
        fprintf(stderr, "Save file is newer than %d.%d, please use the "
		"latest Ponscripter.\n",
		SAVEFILE_VERSION_MAJOR, SAVEFILE_VERSION_MINOR);
        return -1;
    }

    if (file_version >= 200)
        return loadSaveFile2(file_type, file_version);

    fprintf(stderr, "Old ONScripter save files are not supported. Please use "
	    "ONScripter to finish your game, or start over.\n");
    return -1;
}


void PonscripterLabel::saveMagicNumber(bool output_flag)
{
    writeChar('P', output_flag);
    writeChar('N', output_flag);
    writeChar('S', output_flag);
    writeChar(SAVEFILE_VERSION_MAJOR, output_flag);
    writeChar(SAVEFILE_VERSION_MINOR, output_flag);
}


int PonscripterLabel::saveSaveFile(int no)
{
    // make save data structure on memory
    if (no < 0 || (saveon_flag && internal_saveon_flag)) {
        file_io_buf_ptr = 0;
        saveMagicNumber(false);
        saveSaveFile2(false);
        allocFileIOBuf();
        saveMagicNumber(true);
        saveSaveFile2(true);
        save_data_len = file_io_buf_ptr;
        memcpy(save_data_buf, file_io_buf, save_data_len);
    }

    if (no >= 0) {
        saveAll();

	pstring filename;
	filename.format("save%d.dat", no);
        memcpy(file_io_buf, save_data_buf, save_data_len);
        file_io_buf_ptr = save_data_len;
        if (saveFileIOBuf(filename)) {
            fprintf(stderr, "can't open save file save%d.dat for writing\n",
		    no);
            return -1;
        }

        size_t magic_len = 5;
	filename.format("sav" DELIMITER "save%d.dat", no);
        saveFileIOBuf(filename, magic_len);
    }

    return 0;
}
