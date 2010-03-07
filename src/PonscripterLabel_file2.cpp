/* -*- C++ -*-
 *
 *  PonscripterLabel_file2.cpp - FILE I/O of Ponscripter
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

#include "PonscripterLabel.h"

int PonscripterLabel::loadSaveFile2(SaveFileType file_type, int file_version)
{
    deleteNestInfo();

    int i, j;

    readInt(); // 1

    if (file_type == Ponscripter) {
        if (file_version >= 206) {
            int i = readInt() - 1; // length of Ponscripter data
            Fontinfo::default_encoding = readInt();
            while (i--) readInt(); // unknown Ponscripter data
        }
        else {
            Fontinfo::default_encoding = readInt();
        }
    }
    
    sentence_font.is_bold = readInt() == 1;
    sentence_font.is_shadow = readInt() == 1;

    readInt(); // 0
    
    rmode_flag = readInt() == 1;
    sentence_font.color.r = readInt();
    sentence_font.color.g = readInt();
    sentence_font.color.b = readInt();
    cursor_info[0].remove();
    cursor_info[0].image_name = readStr();
    if (cursor_info[0].image_name) {
	parseTaggedString(&cursor_info[0]);
	setupAnimationInfo(&cursor_info[0]);
	if (cursor_info[0].image_surface)
	    cursor_info[0].visible(true);
    }

    cursor_info[1].remove();
    cursor_info[1].image_name = readStr();
    if (cursor_info[1].image_name) {
	parseTaggedString(&cursor_info[1]);
	setupAnimationInfo(&cursor_info[1]);
	if (cursor_info[1].image_surface)
	    cursor_info[1].visible(true);
    }

    window_effect.effect   = readInt();
    window_effect.duration = readInt();
    window_effect.anim.image_name = readStr(); // probably

    sentence_font.clear();
    sentence_font.top_x	 = readInt();
    sentence_font.top_y	 = readInt();
    sentence_font.area_x = readInt();
    sentence_font.area_y = readInt();
    sentence_font.set_size(readInt());
    sentence_font.set_mod_size(readInt());
    sentence_font.pitch_x = readInt();
    sentence_font.pitch_y = readInt();
    sentence_font.window_color.b = readChar();
    sentence_font.window_color.g = readChar();
    sentence_font.window_color.r = readChar();
    sentence_font.is_transparent = readChar() == 0;
    
    sentence_font.wait_time  = readInt();
    sentence_font_info.pos.x = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.y = readInt() * screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.w = (readInt() + 1 -	sentence_font_info.pos.x *
				screen_ratio1 / screen_ratio2) *
	screen_ratio1 / screen_ratio2;
    sentence_font_info.pos.h = (readInt() + 1 - sentence_font_info.pos.y *
				screen_ratio1 / screen_ratio2) *
	screen_ratio1 / screen_ratio2;
    sentence_font_info.image_name = readStr();
    if (!sentence_font.is_transparent && sentence_font_info.image_name) {
	parseTaggedString(&sentence_font_info);
	setupAnimationInfo(&sentence_font_info);
    }

    cursor_info[0].abs_flag = readInt() != 1;
    cursor_info[1].abs_flag = readInt() != 1;	 

    cursor_info[0].pos.x = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[1].pos.x = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[0].pos.y = readInt() * screen_ratio1 / screen_ratio2;
    cursor_info[1].pos.y = readInt() * screen_ratio1 / screen_ratio2;

    // load background surface
    bg_info.remove();
    bg_info.file_name = readStr();
    createBackground();

    for (i = 0; i < 3; i++) {
	tachi_info[i].remove();
	tachi_info[i].image_name = readStr();
	if (tachi_info[i].image_name) {
	    parseTaggedString(&tachi_info[i]);
	    setupAnimationInfo(&tachi_info[i]);
	}
    }

    for (i = 0; i < 3; i++)
	tachi_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;

    for (i = 0; i < 3; i++)
	tachi_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;

    readInt(); // 0
    readInt(); // 0
    readInt(); // 0

    if (file_version >= 203) {
	readInt(); // -1
	readInt(); // -1
	readInt(); // -1
    }
    
    for (i = 0; i < MAX_SPRITE_NUM; i++) {
	sprite_info[i].remove();
	sprite_info[i].image_name = readStr();
	if (sprite_info[i].image_name) {
	    parseTaggedString(&sprite_info[i]);
	    setupAnimationInfo(&sprite_info[i]);
	}

	sprite_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;
	sprite_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;
        int visible_flags = readInt();
	sprite_info[i].visible(visible_flags & 1);
	sprite_info[i].enabled(visible_flags & 2);
        sprite_info[i].enablemode = visible_flags >> 2;
	sprite_info[i].current_cell = readInt();
	if (file_version >= 203) {
	    int trans = readInt();
	    if (trans == -1)
	        trans = 256;
	    sprite_info[i].trans = trans;
	}
    }

    readVariables(0, script_h.global_variable_border);

    // nested info
    int num_nest = readInt();
    if (num_nest < 0) {
        // New, simpler, extensible format
        while (num_nest++ < 0) {
            int mode = readInt();
            const char* addr = script_h.getAddress(readInt());
            switch (mode) {
            case NestInfo::LABEL:
                nest_infos.push_back(NestInfo(script_h, addr));
                break;

            case NestInfo::TEXTGOSUB:
                nest_infos.push_back(NestInfo(script_h, addr, readInt()));
                break;

            case NestInfo::FOR: {
		NestInfo info(Expression(script_h, Expression::Int, true,
					 readInt()), addr);
		info.to	  = readInt();
		info.step = readInt();
                nest_infos.push_back(info);
                break;
            }

            default:
                errorAndExit("encountered unknown nesting mode");
            }
        }
    }
    else if (num_nest > 0) {
        // Legacy save format; TODO: remove support when no longer needed
	file_io_buf_ptr += (num_nest - 1) * 4;
	while (num_nest > 0) {
	    i = readInt();
	    if (i > 0) {
		nest_infos.insert(nest_infos.begin(),
				  NestInfo(script_h, script_h.getAddress(i)));
		file_io_buf_ptr -= 8;
		num_nest--;
	    }
	    else {
		file_io_buf_ptr -= 16;
		NestInfo info(Expression(script_h, Expression::Int, true,
					 readInt()),
			      script_h.getAddress(-i));
		info.to	  = readInt();
		info.step = readInt();
		nest_infos.insert(nest_infos.begin(), info);
		file_io_buf_ptr -= 16;
		num_nest -= 4;
	    }
	}
	num_nest = readInt();
	file_io_buf_ptr += num_nest * 4;
    }

    monocro_flag = readInt() == 1;
    monocro_color.b = readInt();
    monocro_color.g = readInt();
    monocro_color.r = readInt();

    for (i = 0; i < 256; i++) {
	monocro_color_lut[i].r = (monocro_color.r * i) >> 8;
	monocro_color_lut[i].g = (monocro_color.g * i) >> 8;
	monocro_color_lut[i].b = (monocro_color.b * i) >> 8;
    }

    nega_mode = readInt();

    // ----------------------------------------
    // Sound
    stopCommand("stop");
    loopbgmstopCommand("loopbgmstop");

    midi_file_name = readStr(); // MIDI file
    wave_file_name = readStr(); // wave, waveloop
    i = readInt();
    if (i >= 0) current_cd_track = i;

    // play, playonce MIDI
    if (readInt() == 1) {
	midi_play_loop_flag = true;
	current_cd_track = -2;
	playSound(midi_file_name, SOUND_MIDI, midi_play_loop_flag);
    }
    else
	midi_play_loop_flag = false;

    // wave, waveloop
    wave_play_loop_flag = readInt() == 1;

    if (wave_file_name && wave_play_loop_flag)
	playSound(wave_file_name, SOUND_WAVE | SOUND_OGG,
		  wave_play_loop_flag, MIX_WAVE_CHANNEL);

    // play, playonce
    cd_play_loop_flag = readInt() == 1;

    if (current_cd_track >= 0) playCDAudio();

    // bgm, mp3, mp3loop
    music_play_loop_flag = readInt() == 1;
    mp3save_flag = readInt() == 1;

    music_file_name = readStr();
    playSound(music_file_name,
	      SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3 | SOUND_MIDI,
	      music_play_loop_flag, MIX_BGM_CHANNEL);

    erase_text_window_mode = readInt();
    readInt(); // 1

    barclearCommand("barclear");
    for (i = 0; i < MAX_PARAM_NUM; i++) {
	j = readInt();
	if (j != 0) {
	    bar_info[i] = new AnimationInfo();
	    bar_info[i]->trans_mode   = AnimationInfo::TRANS_COPY;
	    bar_info[i]->num_of_cells = 1;

	    bar_info[i]->param = j;
	    bar_info[i]->pos.x = readInt() * screen_ratio1 / screen_ratio2;
	    bar_info[i]->pos.y = readInt() * screen_ratio1 / screen_ratio2;
	    bar_info[i]->max_width = readInt() * screen_ratio1 / screen_ratio2;
	    bar_info[i]->pos.h = readInt() * screen_ratio1 / screen_ratio2;
	    bar_info[i]->max_param = readInt();
	    bar_info[i]->color.b = readChar();
	    bar_info[i]->color.g = readChar();
	    bar_info[i]->color.r = readChar();
	    
	    readChar(); // 0x00

	    int w = bar_info[i]->max_width * bar_info[i]->param
		  / bar_info[i]->max_param;
	    if (bar_info[i]->max_width > 0 && w > 0) {
		bar_info[i]->pos.w = w;
		bar_info[i]->allocImage(bar_info[i]->pos.w, bar_info[i]->pos.h);
		bar_info[i]->fill(bar_info[i]->color, 0xff);
	    }
	}
	else {
	    readInt(); // -1
	    readInt(); // 0
	    readInt(); // 0
	    readInt(); // 0
	    readInt(); // 0
	    readInt(); // 0
	}
    }

    prnumclearCommand("prnumclear");
    for (i = 0; i < MAX_PARAM_NUM; i++) {
	j = readInt();
	if (prnum_info[i]) {
	    prnum_info[i] = new AnimationInfo();
	    prnum_info[i]->trans_mode	= AnimationInfo::TRANS_STRING;
	    prnum_info[i]->num_of_cells = 1;
	    prnum_info[i]->color_list.resize(1);

	    prnum_info[i]->param = j;
	    prnum_info[i]->pos.x = readInt() * screen_ratio1 / screen_ratio2;
	    prnum_info[i]->pos.y = readInt() * screen_ratio1 / screen_ratio2;
	    prnum_info[i]->font_size_x = readInt();
	    prnum_info[i]->font_size_y = readInt();
	    prnum_info[i]->color_list[0].b = readChar();
	    prnum_info[i]->color_list[0].g = readChar();
	    prnum_info[i]->color_list[0].r = readChar();

	    readChar(); // 0x00

	    prnum_info[i]->file_name =
		script_h.stringFromInteger(prnum_info[i]->param, 3);
	    setupAnimationInfo(prnum_info[i]);
	}
	else {
	    readInt(); // -1
	    readInt(); // 0
	    readInt(); // 0
	    readInt(); // 0
	    readInt(); // 0
	}
    }

    readInt(); // 1
    readInt(); // 0
    readInt(); // 1
    btndef_info.remove();
    btndef_info.image_name = readStr();
    if (btndef_info.image_name) {
	parseTaggedString(&btndef_info);
	setupAnimationInfo(&btndef_info);
	SDL_SetAlpha(btndef_info.image_surface, DEFAULT_BLIT_FLAG,
		     SDL_ALPHA_OPAQUE);
    }

    if (file_version >= 202)
	readArrayVariable();

    readInt(); // 0
    if (readChar() == 1) erase_text_window_mode = 2;

    readChar(); // 0
    readChar(); // 0
    readChar(); // 0
    loop_bgm_name[0] = readStr();
    loop_bgm_name[1] = readStr();
    if (loop_bgm_name[0]) {
	playSound(loop_bgm_name[1], SOUND_PRELOAD | SOUND_WAVE | SOUND_OGG,
		  false, MIX_LOOPBGM_CHANNEL1);

	playSound(loop_bgm_name[0], SOUND_WAVE | SOUND_OGG,
		  false, MIX_LOOPBGM_CHANNEL0);
    }

    if (file_type != Ponscripter && file_version >= 201) {
	// Ruby support has been stripped out.
	readInt();
	readInt();
	readInt();
	readStr();
    }

    if (file_version >= 204){
	readInt();
	
	for (i = 0; i < MAX_SPRITE2_NUM; ++i) {
	    sprite2_info[i].remove();
	    sprite2_info[i].image_name = readStr();
	    if (sprite2_info[i].image_name) {
		parseTaggedString(&sprite2_info[i]);
		setupAnimationInfo(&sprite2_info[i]);
	    }
	    sprite2_info[i].pos.x = readInt() * screen_ratio1 / screen_ratio2;
	    sprite2_info[i].pos.y = readInt() * screen_ratio1 / screen_ratio2;
	    sprite2_info[i].scale_x = readInt();
	    sprite2_info[i].scale_y = readInt();
	    sprite2_info[i].rot = readInt();

            int visible_flags = readInt();
            sprite2_info[i].visible(visible_flags & 1);
            sprite2_info[i].enabled(visible_flags & 2);
            sprite2_info[i].enablemode = visible_flags >> 2;

	    j = readInt();
	    sprite2_info[i].trans = j == -1 ? 256 : j;
	    sprite2_info[i].blending_mode = readInt();
	}

	readInt();
	readInt();
	readInt();
	readInt();
	readInt();
	readInt();
    }
    
    int text_num = readInt();
    start_text_buffer = current_text_buffer;
    for (i = 0; i < text_num; i++) {
	clearCurrentTextBuffer();
	char c;
	while ((c = readChar())) current_text_buffer->addBuffer(c);
	if (file_version == 203) readChar();
	current_text_buffer = current_text_buffer->next;
    }
    clearCurrentTextBuffer();

    if (file_version >= 204) { readInt(); readInt(); }
    
    i = readInt();
    current_label_info = script_h.getLabelByLine(i);
    current_line = i - current_label_info.start_line;
    const char* buf = script_h.getAddressByLine(i);

    j = readInt();
    for (i = 0; i < j; i++) {
	while (*buf != ':') buf++;
	buf++;
    }

    script_h.setCurrent(buf);

    display_mode = shelter_display_mode = NORMAL_DISPLAY_MODE;

    clickstr_state = CLICK_NONE;
    event_mode = 0; //WAIT_SLEEP_MODE;
    draw_cursor_flag = false;

    return 0;
}


void PonscripterLabel::saveSaveFile2(bool output_flag)
{
    int i;

    writeInt(1, output_flag);

    // Ponscripter additions
    writeInt(1, output_flag);
    writeInt(Fontinfo::default_encoding, output_flag);
    // End Ponscripter additions
    
    writeInt((sentence_font.is_bold ? 1 : 0), output_flag);
    writeInt((sentence_font.is_shadow ? 1 : 0), output_flag);

    writeInt(0, output_flag);
    
    writeInt((rmode_flag) ? 1 : 0, output_flag);
    writeInt(sentence_font.color.r, output_flag);
    writeInt(sentence_font.color.g, output_flag);
    writeInt(sentence_font.color.b, output_flag);
    writeStr(cursor_info[0].image_name, output_flag);
    writeStr(cursor_info[1].image_name, output_flag);

    writeInt(window_effect.effect, output_flag);
    writeInt(window_effect.duration, output_flag);
    writeStr(window_effect.anim.image_name, output_flag); // probably

    writeInt(sentence_font.top_x, output_flag);
    writeInt(sentence_font.top_y, output_flag);
    writeInt(sentence_font.area_x, output_flag);
    writeInt(sentence_font.area_y, output_flag);
    writeInt(sentence_font.base_size(), output_flag);
    writeInt(sentence_font.mod_size(), output_flag);
    writeInt(sentence_font.pitch_x, output_flag);
    writeInt(sentence_font.pitch_y, output_flag);
    writeChar(sentence_font.window_color.b, output_flag);
    writeChar(sentence_font.window_color.g, output_flag);
    writeChar(sentence_font.window_color.r, output_flag);
    
    writeChar((sentence_font.is_transparent) ? 0x00 : 0xff, output_flag);
    writeInt(sentence_font.wait_time, output_flag);
    writeInt(sentence_font_info.pos.x * screen_ratio2 / screen_ratio1,
	     output_flag);
    writeInt(sentence_font_info.pos.y * screen_ratio2 / screen_ratio1,
	     output_flag);
    writeInt(sentence_font_info.pos.w * screen_ratio2 / screen_ratio1 +
	     sentence_font_info.pos.x * screen_ratio2 / screen_ratio1 - 1,
	     output_flag);
    writeInt(sentence_font_info.pos.h * screen_ratio2 / screen_ratio1 +
	     sentence_font_info.pos.y * screen_ratio2 / screen_ratio1 - 1,
	     output_flag);
    writeStr(sentence_font_info.image_name, output_flag);

    writeInt((cursor_info[0].abs_flag) ? 0 : 1, output_flag);
    writeInt((cursor_info[1].abs_flag) ? 0 : 1, output_flag);
    writeInt(cursor_info[0].pos.x * screen_ratio2 / screen_ratio1, output_flag);
    writeInt(cursor_info[1].pos.x * screen_ratio2 / screen_ratio1, output_flag);
    writeInt(cursor_info[0].pos.y * screen_ratio2 / screen_ratio1, output_flag);
    writeInt(cursor_info[1].pos.y * screen_ratio2 / screen_ratio1, output_flag);

    writeStr(bg_info.file_name, output_flag);
    for (i = 0; i < 3; i++)
	writeStr(tachi_info[i].image_name, output_flag);

    for (i = 0; i < 3; i++)
	writeInt(tachi_info[i].pos.x * screen_ratio2 / screen_ratio1,
		 output_flag);

    for (i = 0; i < 3; i++)
	writeInt(tachi_info[i].pos.y * screen_ratio2 / screen_ratio1,
		 output_flag);

    writeInt(0, output_flag);
    writeInt(0, output_flag);
    writeInt(0, output_flag);

    writeInt(-1, output_flag);
    writeInt(-1, output_flag);
    writeInt(-1, output_flag);
    
    for (i = 0; i < MAX_SPRITE_NUM; i++) {
	writeStr(sprite_info[i].image_name, output_flag);
	writeInt(sprite_info[i].pos.x * screen_ratio2 / screen_ratio1,
		 output_flag);
	writeInt(sprite_info[i].pos.y * screen_ratio2 / screen_ratio1,
		 output_flag);
	writeInt(sprite_info[i].savestate(), output_flag);
	writeInt(sprite_info[i].current_cell, output_flag);
        if (sprite_info[i].trans == 256)
            writeInt(-1, output_flag);
        else
            writeInt(sprite_info[i].trans, output_flag);
    }

    writeVariables(0, script_h.global_variable_border, output_flag);

    // nested info
    int num_nest = 0;
    NestInfo::iterator info = nest_infos.begin();
    while (info != nest_infos.end()) {
        ++num_nest;
	++info;
    }
    writeInt(-num_nest, output_flag);
    info = nest_infos.begin();
    while (info != nest_infos.end()) {
        writeInt(info->nest_mode, output_flag);
        writeInt(script_h.getOffset(info->next_script), output_flag);
        switch (info->nest_mode) {
        case NestInfo::LABEL:
            break;

        case NestInfo::TEXTGOSUB:
            writeInt(info->to, output_flag);
            break;
        
	case NestInfo::FOR:
	    writeInt(info->var.var_no(), output_flag);
	    writeInt(info->to, output_flag);
	    writeInt(info->step, output_flag);
            break;

        default:
            errorAndExit("encountered unhandled nesting mode");
	}
	++info;
    }

    writeInt(monocro_flag ? 1 : 0, output_flag);
    writeInt(monocro_color.b, output_flag);
    writeInt(monocro_color.g, output_flag);
    writeInt(monocro_color.r, output_flag);

    writeInt(nega_mode, output_flag);

    // sound
    writeStr(midi_file_name, output_flag); // MIDI file

    writeStr(wave_file_name, output_flag); // wave, waveloop

    if (current_cd_track >= 0)	// play CD
	writeInt(current_cd_track, output_flag);
    else
	writeInt(-1, output_flag);

    writeInt(midi_play_loop_flag ? 1 : 0, output_flag); // play, playonce MIDI
    writeInt(wave_play_loop_flag ? 1 : 0, output_flag); // wave, waveloop
    writeInt(cd_play_loop_flag ? 1 : 0, output_flag); // play, playonce
    writeInt(music_play_loop_flag ? 1 : 0, output_flag); // bgm, mp3, mp3loop
    writeInt(mp3save_flag ? 1 : 0, output_flag);
    writeStr(mp3save_flag ? music_file_name : pstring(""), output_flag);

    writeInt((erase_text_window_mode > 0) ? 1 : 0, output_flag);
    writeInt(1, output_flag);

    for (i = 0; i < MAX_PARAM_NUM; i++) {
	if (bar_info[i]) {
	    writeInt(bar_info[i]->param, output_flag);
	    writeInt(bar_info[i]->pos.x * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(bar_info[i]->pos.y * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(bar_info[i]->max_width * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(bar_info[i]->pos.h * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(bar_info[i]->max_param, output_flag);
	    writeChar(bar_info[i]->color.b, output_flag);
	    writeChar(bar_info[i]->color.g, output_flag);
	    writeChar(bar_info[i]->color.r, output_flag);

	    writeChar(0x00, output_flag);
	}
	else {
	    writeInt(0, output_flag);
	    writeInt(-1, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	}
    }

    for (i = 0; i < MAX_PARAM_NUM; i++) {
	if (prnum_info[i]) {
	    writeInt(prnum_info[i]->param, output_flag);
	    writeInt(prnum_info[i]->pos.x * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(prnum_info[i]->pos.y * screen_ratio2 / screen_ratio1,
		     output_flag);
	    writeInt(prnum_info[i]->font_size_x, output_flag);
	    writeInt(prnum_info[i]->font_size_y, output_flag);
	    writeChar(prnum_info[i]->color_list[0].b, output_flag);
	    writeChar(prnum_info[i]->color_list[0].g, output_flag);
	    writeChar(prnum_info[i]->color_list[0].r, output_flag);

	    writeChar(0x00, output_flag);
	}
	else {
	    writeInt(0, output_flag);
	    writeInt(-1, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	    writeInt(0, output_flag);
	}
    }

    writeInt(1, output_flag);
    writeInt(0, output_flag);
    writeInt(1, output_flag);
    writeStr(btndef_info.image_name, output_flag);

    writeArrayVariable(output_flag);

    writeInt(0, output_flag);
    writeChar((erase_text_window_mode == 2) ? 1 : 0, output_flag);
    writeChar(0, output_flag);
    writeChar(0, output_flag);
    writeChar(0, output_flag);
    writeStr(loop_bgm_name[0], output_flag);
    writeStr(loop_bgm_name[1], output_flag);

    // Ruby is gone, gone, gone.
    //writeInt(0, output_flag);
    //writeInt(0, output_flag);
    //writeInt(0, output_flag);
    //writeStr("", output_flag);

    writeInt(0, output_flag);

    for (i = 0; i < MAX_SPRITE2_NUM; ++i) {
	writeStr(sprite2_info[i].image_name, output_flag);
	writeInt(sprite2_info[i].pos.x * screen_ratio2 / screen_ratio1,
		 output_flag);
	writeInt(sprite2_info[i].pos.y * screen_ratio2 / screen_ratio1,
		 output_flag);
	writeInt(sprite2_info[i].scale_x, output_flag);
	writeInt(sprite2_info[i].scale_y, output_flag);
	writeInt(sprite2_info[i].rot, output_flag);
	writeInt(sprite2_info[i].savestate(), output_flag);
	if (sprite2_info[i].trans == 256)
	    writeInt(-1, output_flag);
	else
	    writeInt(sprite2_info[i].trans, output_flag);
	writeInt(sprite2_info[i].blending_mode, output_flag);
    }

    writeInt(0, output_flag);
    writeInt(0, output_flag);
    writeInt(0, output_flag);
    writeInt(0, output_flag);
    writeInt(0, output_flag);
    writeInt(0, output_flag);

    TextBuffer* tb = current_text_buffer;
    int text_num = 0;
    while (tb != start_text_buffer) {
	tb = tb->previous;
	text_num++;
    }
    writeInt(text_num, output_flag);

    for (i = 0; i < text_num; i++) {
	const char* buf = tb->contents;
	while (*buf) writeChar(*buf++, output_flag);
	writeChar(0, output_flag);
	tb = tb->next;
    }

    writeInt(0, output_flag);
    writeInt(0, output_flag);

    writeInt(current_label_info.start_line + current_line, output_flag);
    const char* buf =
	script_h.getAddressByLine(current_label_info.start_line + current_line);

    i = 0;
    if (!script_h.isText()) {
	while (buf != script_h.getCurrent()) {
	    if (*buf == ':') i++;
	    buf++;
	}
    }

    writeInt(i, output_flag);
}
