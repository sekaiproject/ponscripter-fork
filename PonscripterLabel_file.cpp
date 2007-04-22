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

#define SAVEFILE_MAGIC_NUMBER "ONS"
#define SAVEFILE_VERSION_MAJOR 2
#define SAVEFILE_VERSION_MINOR 4

#define READ_LENGTH 4096

inline string str(int i)
{
    char buf[1024];
    sprintf(buf, "%d", i);
    return string(buf);
}

void PonscripterLabel::searchSaveFile(SaveFileInfo &save_file_info, int no)
{
    string filename;

    script_h.getStringFromInteger(save_file_info.sjis_no, no, (num_save_file >= 10) ? 2 : 1);
#if defined (LINUX) || defined (MACOSX)
    filename = script_h.save_path + "save" + str(no) + ".dat";
    struct stat buf;
    struct tm*  tm;
    if (stat(filename.c_str(), &buf) != 0) {
        save_file_info.valid = false;
        return;
    }

    tm = localtime(&buf.st_mtime);

    save_file_info.month = tm->tm_mon + 1;
    save_file_info.day    = tm->tm_mday;
    save_file_info.hour   = tm->tm_hour;
    save_file_info.minute = tm->tm_min;
#elif defined (WIN32)
    filename = script_h.save_path + "save" + str(no) + ".dat";    
    HANDLE     handle;
    FILETIME   tm, ltm;
    SYSTEMTIME stm;

    handle = CreateFile(filename.c_str(), GENERIC_READ, 0, NULL,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        save_file_info.valid = false;
        return;
    }

    GetFileTime(handle, NULL, NULL, &tm);
    FileTimeToLocalFileTime(&tm, &ltm);
    FileTimeToSystemTime(&ltm, &stm);
    CloseHandle(handle);

    save_file_info.month = stm.wMonth;
    save_file_info.day    = stm.wDay;
    save_file_info.hour   = stm.wHour;
    save_file_info.minute = stm.wMinute;
#elif defined (PSP)
    filename = script_h.save_path + "save" + str(no) + ".dat";    
    SceIoStat buf;
    if (sceIoGetstat(filename.c_str(), &buf) < 0) {
        save_file_info.valid = false;
        return;
    }

    save_file_info.month = buf.st_mtime.month;
    save_file_info.day    = buf.st_mtime.day;
    save_file_info.hour   = buf.st_mtime.hour;
    save_file_info.minute = buf.st_mtime.minute;
#else
    filename = "save" + str(no) + ".dat";        
    FILE* fp;
    if ((fp = fopen(filename, "rb")) == NULL) {
        save_file_info.valid = false;
        return;
    }
    fclose(fp);

    save_file_info.month = 1;
    save_file_info.day    = 1;
    save_file_info.hour   = 0;
    save_file_info.minute = 0;
#endif
    save_file_info.valid = true;
    script_h.getStringFromInteger(save_file_info.sjis_month, save_file_info.month, 2);
    script_h.getStringFromInteger(save_file_info.sjis_day, save_file_info.day, 2);
    script_h.getStringFromInteger(save_file_info.sjis_hour, save_file_info.hour, 2);
    script_h.getStringFromInteger(save_file_info.sjis_minute, save_file_info.minute, 2, true);
}


int PonscripterLabel::loadSaveFile(int no)
{
    char filename[16];
    sprintf(filename, "save%d.dat", no);
    if (loadFileIOBuf(filename)) {
        fprintf(stderr, "can't open save file %s\n", filename);
        return -1;
    }

    char* str = NULL;
    int   i, j, k, address;
    int   file_version;

    /* ---------------------------------------- */
    /* Load magic number */
    for (i = 0; i < (int) strlen(SAVEFILE_MAGIC_NUMBER); i++)
        if (readChar() != SAVEFILE_MAGIC_NUMBER[i]) break;

    if (i != (int) strlen(SAVEFILE_MAGIC_NUMBER)) { // if not ONS save file
        file_io_buf_ptr = 0;
        // check for ONS version 0
        bool ons_ver0_flag = false;
        if (readInt() != 1) ons_ver0_flag = true;

        readInt();
        readInt();
        if (readInt() != 0) ons_ver0_flag = true;

        readInt();
        if (readInt() != 0xff) ons_ver0_flag = true;

        if (readInt() != 0xff) ons_ver0_flag = true;

        if (readInt() != 0xff) ons_ver0_flag = true;

        file_io_buf_ptr = 0;
        if (!ons_ver0_flag) {
            return loadSaveFile2(SAVEFILE_VERSION_MAJOR * 100 + SAVEFILE_VERSION_MINOR);
        }

        file_version = 0;
    }
    else {
        file_version  = readChar() * 100;
        file_version += readChar();
    }

    if (file_version > SAVEFILE_VERSION_MAJOR * 100 + SAVEFILE_VERSION_MINOR) {
        fprintf(stderr, "Save file is newer than %d.%d, please use the latest Ponscripter.\n", SAVEFILE_VERSION_MAJOR, SAVEFILE_VERSION_MINOR);
        return -1;
    }

    if (file_version >= 200)
        return loadSaveFile2(file_version);

    deleteNestInfo();

    /* ---------------------------------------- */
    /* Load text history */
// FIXME: this will (probably) NOT work with UTF-8 and proportional text...
    if (file_version >= 107) readInt();

    int text_history_num = readInt();
    for (i = 0; i < text_history_num; i++) {
        int num_xy[2];
        num_xy[0] = readInt();
        num_xy[1] = readInt();
        //current_text_buffer->num = (num_xy[0]*2+1)*num_xy[1];
        int xy[2];
        xy[0] = readInt();
        xy[1] = readInt();
        current_text_buffer->clear();

        char ch1, ch2;
        for (j = 0, k = 0; j < num_xy[0] * num_xy[1]; j++) {
            ch1 = readChar();
            ch2 = readChar();
            if (ch1 == ((char*) "@")[0]
                && ch2 == ((char*) "@")[1]) {
                k += 2;
            }
            else {
                if (ch1) {
                    current_text_buffer->addBuffer(ch1);
                    k++;
                }

                if (ch1 & 0x80 || ch2) {
                    current_text_buffer->addBuffer(ch2);
                    k++;
                }
            }

            if (k >= num_xy[0] * 2) {
                current_text_buffer->addBuffer(0x0a);
                k = 0;
            }
        }

        current_text_buffer = current_text_buffer->next;
        if (i == 0) {
            for (j = 0; j < max_text_buffer - text_history_num; j++)
                current_text_buffer = current_text_buffer->next;

            start_text_buffer = current_text_buffer;
        }
    }

    /* ---------------------------------------- */
    /* Load sentence font */
    j = readInt();
    //sentence_font.is_valid = (j==1)?true:false;
    sentence_font.set_size(readInt());
    if (file_version >= 100) {
        sentence_font.set_mod_size(readInt());
    }
    else {
        sentence_font.set_mod_size(0);
    }

    sentence_font.top_x  = readInt();
    sentence_font.top_y  = readInt();
    sentence_font.area_x = readInt();
    sentence_font.area_y = readInt();
    const int px = readInt();
    sentence_font.SetXY(px, readInt());
    sentence_font.pitch_x   = readInt();
    sentence_font.pitch_y   = readInt();
    sentence_font.wait_time = readInt();
    sentence_font.is_bold   = (readInt() == 1) ? true : false;
    sentence_font.is_shadow = (readInt() == 1) ? true : false;
    sentence_font.is_transparent = (readInt() == 1) ? true : false;

// FIXME: not sure what this does, but it'll be broken.
// Actually, looking at it, it doesn't appear to be essential...?
//    const char *buffer = current_text_buffer->contents.c_str();
//    int count = current_text_buffer->contents.size();
//    for (j=0, k=0, i=0 ; i<count ; i++){
//        if (j == sentence_font.pos_y &&
//            (k > sentence_font.pos_x ||
//             buffer[i] == 0x0a)) break;
//
//        if (buffer[i] == 0x0a){
//            j+=2;
//            k=0;
//        }
//        else
//            k++;
//    }
//    current_text_buffer->buffer2_count = i;

    /* Dummy, must be removed later !! */
    for (i = 0; i < 8; i++) {
        j = readInt();
        //sentence_font.window_color[i] = j;
    }

    /* Should be char, not integer !! */
    sentence_font.window_color.r = readInt();
    sentence_font.window_color.g = readInt();
    sentence_font.window_color.b = readInt();
    
    sentence_font_info.image_name = readStr();

    sentence_font_info.pos.x = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.y = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.w = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.h = readInt() * screen_ratio1 / screen_ratio2;

    if (!sentence_font.is_transparent) {
        parseTaggedString(&sentence_font_info);
        setupAnimationInfo(&sentence_font_info);
    }

    clickstr_state = readInt();
    new_line_skip_flag = (readInt() == 1) ? true : false;
    if (file_version >= 103) {
        erase_text_window_mode = readInt();
    }

    /* ---------------------------------------- */
    /* Load link label info */

    int offset = 0;
    while (1) {
        readStr(&str);
        current_label_info = script_h.lookupLabel(str);

        current_line = readInt() + 2;
        char* buf = current_label_info.label_header;
        while (buf < current_label_info.start_address) {
            if (*buf == 0x0a) current_line--;

            buf++;
        }

        offset = readInt();

        script_h.setCurrent(current_label_info.label_header);
        script_h.skipLine(current_line);

        if (file_version <= 104) {
            if (file_version >= 102)
                readInt();

            address = readInt();
        }
        else {
            offset += readInt();
        }

        if (readChar() == 0) break;

        last_nest_info->next = new NestInfo();
        last_nest_info->next->previous = last_nest_info;
        last_nest_info = last_nest_info->next;
        last_nest_info->next_script = script_h.getCurrent() + offset;
    }
    script_h.setCurrent(script_h.getCurrent() + offset);

    int tmp_event_mode = readChar();

    /* ---------------------------------------- */
    /* Load variables */
    readVariables(0, 200);

    /* ---------------------------------------- */
    /* Load monocro flag */
    monocro_flag = (readChar() == 1) ? true : false;
    if (file_version >= 101) {
        monocro_flag = (readChar() == 1) ? true : false;
    }

    monocro_color.r = readChar();
    monocro_color.g = readChar();
    monocro_color.b = readChar();
    
    if (file_version >= 101) {
	monocro_color.r = readChar();
	monocro_color.g = readChar();
	monocro_color.b = readChar();

        readChar(); // obsolete, need_refresh_flag
    }

    for (i = 0; i < 256; i++) {
        monocro_color_lut[i].r = (monocro_color.r * i) >> 8;
        monocro_color_lut[i].g = (monocro_color.g * i) >> 8;
        monocro_color_lut[i].b = (monocro_color.b * i) >> 8;
    }

    /* Load nega flag */
    if (file_version >= 104) {
        nega_mode = (unsigned char) readChar();
    }

    /* ---------------------------------------- */
    /* Load current images */
    bg_info.remove();
    bg_info.color.r = (unsigned char) readChar();
    bg_info.color.g = (unsigned char) readChar();
    bg_info.color.b = (unsigned char) readChar();
    bg_info.num_of_cells = 1;
    bg_info.file_name = readStr();
    setupAnimationInfo(&bg_info);
    bg_effect_image = (EFFECT_IMAGE) readChar();

    if (bg_effect_image == COLOR_EFFECT_IMAGE) {
        bg_info.allocImage(screen_width, screen_height);
        bg_info.fill(bg_info.color, 0xff);
        bg_info.pos.x = 0;
        bg_info.pos.y = 0;
    }

    bg_info.trans_mode = AnimationInfo::TRANS_COPY;

    for (i = 0; i < 3; i++)
        tachi_info[i].remove();

    for (i = 0; i < MAX_SPRITE_NUM; i++)
        sprite_info[i].remove();

    /* ---------------------------------------- */
    /* Load Tachi image and Sprite */
    for (i = 0; i < 3; i++) {
        tachi_info[i].image_name = readStr();
        if (tachi_info[i].image_name) {
            parseTaggedString(&tachi_info[i]);
            setupAnimationInfo(&tachi_info[i]);
            tachi_info[i].pos.x = screen_width * (i + 1) / 4 - tachi_info[i].pos.w / 2;
            tachi_info[i].pos.y = underline_value - tachi_info[i].image_surface->h + 1;
        }
    }

    /* ---------------------------------------- */
    /* Load current sprites */
    for (i = 0; i < 256; i++) {
        sprite_info[i].visible = (readInt() == 1) ? true : false;
        sprite_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;
        sprite_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;
        sprite_info[i].trans = readInt();
        sprite_info[i].image_name = readStr();
        if (sprite_info[i].image_name) {
            parseTaggedString(&sprite_info[i]);
            setupAnimationInfo(&sprite_info[i]);
        }
    }

    /* ---------------------------------------- */
    /* Load current playing CD track */
    stopCommand("stop");
    loopbgmstopCommand("loopbgmstop");

    current_cd_track = (Sint8) readChar();
    bool play_once_flag = (readChar() == 1) ? true : false;
    if (current_cd_track == -2) {
        midi_file_name = readStr();
        midi_play_loop_flag = !play_once_flag;
        music_file_name.clear();
        music_play_loop_flag = false;
    }
    else {
        music_file_name = readStr();
        if (music_file_name) {
            music_play_loop_flag = !play_once_flag;
            cd_play_loop_flag = false;
        }
        else {
            music_play_loop_flag = false;
            cd_play_loop_flag = !play_once_flag;
        }

        midi_file_name.clear();
        midi_play_loop_flag = false;
    }

    if (current_cd_track >= 0) {
        playCDAudio();
    }
    else if (midi_file_name && midi_play_loop_flag) {
        playSound(midi_file_name, SOUND_MIDI, midi_play_loop_flag);
    }
    else if (music_file_name && music_play_loop_flag) {
        playSound(music_file_name,
            SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3,
            music_play_loop_flag, MIX_BGM_CHANNEL);
    }

    /* ---------------------------------------- */
    /* Load rmode flag */
    rmode_flag = (readChar() == 1) ? true : false;

    /* ---------------------------------------- */
    /* Load text on flag */
    text_on_flag = (readChar() == 1) ? true : false;

    restoreTextBuffer();
    num_chars_in_sentence = 0;
    cached_text_buffer = current_text_buffer;

    display_mode = shelter_display_mode = TEXT_DISPLAY_MODE;

    event_mode = tmp_event_mode;
    if (event_mode & WAIT_BUTTON_MODE) event_mode = WAIT_SLEEP_MODE;

    // Re-execute the selectCommand, etc.

    if (event_mode & WAIT_SLEEP_MODE)
        event_mode &= ~WAIT_SLEEP_MODE;
    else
        event_mode |= WAIT_TIMER_MODE;

    if (event_mode & WAIT_INPUT_MODE) event_mode |= WAIT_TEXT_MODE;

    draw_cursor_flag = (clickstr_state == CLICK_NONE) ? false : true;

    return 0;
}


void PonscripterLabel::saveMagicNumber(bool output_flag)
{
    for (unsigned int i = 0; i < strlen(SAVEFILE_MAGIC_NUMBER); i++)
        writeChar(SAVEFILE_MAGIC_NUMBER[i], output_flag);

    writeChar(SAVEFILE_VERSION_MAJOR, output_flag);
    writeChar(SAVEFILE_VERSION_MINOR, output_flag);
}


int PonscripterLabel::saveSaveFile(int no)
{
    // make save data structure on memory
    if (no < 0 || saveon_flag && internal_saveon_flag) {
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

        char filename[16];
        sprintf(filename, "save%d.dat", no);

        memcpy(file_io_buf, save_data_buf, save_data_len);
        file_io_buf_ptr = save_data_len;
        if (saveFileIOBuf(filename)) {
            fprintf(stderr, "can't open save file %s for writing\n", filename);
            return -1;
        }

        size_t magic_len = strlen(SAVEFILE_MAGIC_NUMBER) + 2;
        sprintf(filename, "sav%csave%d.dat", DELIMITER, no);
        saveFileIOBuf(filename, magic_len);
    }

    return 0;
}
