/* -*- C++ -*-
 *
 *  PonscripterLabel_command.cpp - Command executer of Ponscripter
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
#include "version.h"

#if defined (MACOSX) && (SDL_COMPILEDVERSION >= 1208)
#include <CoreFoundation/CoreFoundation.h>
#endif

#define CONTINUOUS_PLAY

extern SDL_TimerID timer_mp3fadeout_id;
extern "C" Uint32 SDLCALL mp3fadeoutCallback(Uint32 interval, void* param);

int PonscripterLabel::waveCommand(const string& cmd)
{
    wavestopCommand("wavestop");

    wave_file_name = script_h.readStr();
    playSound(wave_file_name, SOUND_WAVE | SOUND_OGG, cmd == "waveloop",
	      MIX_WAVE_CHANNEL);

    return RET_CONTINUE;
}


int PonscripterLabel::wavestopCommand(const string& cmd)
{
    if (wave_sample[MIX_WAVE_CHANNEL]) {
        Mix_Pause(MIX_WAVE_CHANNEL);
        Mix_FreeChunk(wave_sample[MIX_WAVE_CHANNEL]);
        wave_sample[MIX_WAVE_CHANNEL] = NULL;
    }

    wave_file_name.clear();

    return RET_CONTINUE;
}


int PonscripterLabel::waittimerCommand(const string& cmd)
{
    int count = script_h.readInt() + internal_timer - SDL_GetTicks();
    startTimer(count);

    return RET_WAIT;
}


int PonscripterLabel::waitCommand(const string& cmd)
{
    int count = script_h.readInt();

    if (skip_flag || draw_one_page_flag || ctrl_pressed_status || skip_to_wait)
	return RET_CONTINUE;

    startTimer(count);
    return RET_WAIT;
}


int PonscripterLabel::vspCommand(const string& cmd)
{
    int no = script_h.readInt();
    int v  = script_h.readInt();
    sprite_info[no].visible = (v == 1) ? true : false;
    dirty_rect.add(sprite_info[no].pos);

    return RET_CONTINUE;
}


int PonscripterLabel::voicevolCommand(const string& cmd)
{
    voice_volume = script_h.readInt();

    if (wave_sample[0]) Mix_Volume(0, se_volume * 128 / 100);

    return RET_CONTINUE;
}


int PonscripterLabel::vCommand(const string& cmd)
{
    string buf = "wav";
    buf += DELIMITER;
    buf.append(script_h.getStringBuffer(), 1);
    playSound(buf, SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

    return RET_CONTINUE;
}


int PonscripterLabel::trapCommand(const string& cmd)
{
    if (cmd == "lr_trap") {
        trap_mode = TRAP_LEFT_CLICK | TRAP_RIGHT_CLICK;
    }
    else if (cmd == "trap") {
        trap_mode = TRAP_LEFT_CLICK;
    }

    if (script_h.compareString("off")) {
        script_h.readLabel();
        trap_mode = TRAP_NONE;
        return RET_CONTINUE;
    }

    string buf = script_h.readStr();
    if (buf[0] == '*') {
	buf.shift();
        trap_dist = buf;
    }
    else {
        printf("trapCommand: [%s] is not supported\n", buf.c_str());
    }

    return RET_CONTINUE;
}


int PonscripterLabel::textspeedCommand(const string& cmd)
{
    sentence_font.wait_time = script_h.readInt();

    return RET_CONTINUE;
}


int PonscripterLabel::textshowCommand(const string& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE
	                     | REFRESH_SHADOW_MODE
	                     | REFRESH_TEXT_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::textonCommand(const string& cmd)
{
    text_on_flag = true;
    if (!(display_mode & TEXT_DISPLAY_MODE)) {
        dirty_rect.fill(screen_width, screen_height);
        display_mode = TEXT_DISPLAY_MODE;
        flush(refreshMode());
    }

    return RET_CONTINUE;
}


int PonscripterLabel::textoffCommand(const string& cmd)
{
    text_on_flag = false;
    if (display_mode & TEXT_DISPLAY_MODE) {
        dirty_rect.fill(screen_width, screen_height);
        display_mode = NORMAL_DISPLAY_MODE;
        flush(refreshMode());
    }

    return RET_CONTINUE;
}


int PonscripterLabel::texthideCommand(const string& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::textclearCommand(const string& cmd)
{
    newPage(false);
    return RET_CONTINUE;
}


int PonscripterLabel::texecCommand(const string& cmd)
{
    if (textgosub_clickstr_state == CLICK_NEWPAGE) {
        newPage(true);
        clickstr_state = CLICK_NONE;
    }
    else if (textgosub_clickstr_state == (CLICK_WAIT | CLICK_EOL)) {
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            indent_offset = 0;
            line_enter_status = 0;
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::tateyokoCommand(const string& cmd)
{
    // Ignored in this version

    return RET_CONTINUE;
}


int PonscripterLabel::talCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    char loc = script_h.readLabel()[0];
    int  no  = -1, trans = 0;
    if (loc == 'l') no = 0;
    else if (loc == 'c') no = 1;
    else if (loc == 'r') no = 2;

    if (no >= 0) {
        trans = script_h.readInt();
        if (trans > 256) trans = 256;
        else if (trans < 0) trans = 0;
    }

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), NULL, TACHI_EFFECT_IMAGE);
    }
    else {
        if (no >= 0) {
            tachi_info[no].trans = trans;
            dirty_rect.add(tachi_info[no].pos);
        }

        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::tablegotoCommand(const string& cmd)
{
    int count = 0;
    int no = script_h.readInt();

    while (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        const char* buf = script_h.readStr();
        if (count++ == no) {
            setCurrentLabel(buf + 1);
            break;
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::systemcallCommand(const string& cmd)
{
    system_menu_mode = getSystemCallNo(script_h.readLabel());
    enterSystemCall();
    advancePhase();

    return RET_WAIT;
}


int PonscripterLabel::strspCommand(const string& cmd)
{
    int sprite_no = script_h.readInt();
    AnimationInfo* ai = &sprite_info[sprite_no];
    ai->removeTag();
    ai->file_name = script_h.readStr();

    FontInfo fi;
    fi.is_newline_accepted = true;
    ai->pos.x = script_h.readInt();
    ai->pos.y = script_h.readInt();
    fi.area_x = script_h.readInt();
    fi.area_y = script_h.readInt();
    int s1 = script_h.readInt(), s2 = script_h.readInt();
    fi.set_size(s1 > s2 ? s1 : s2);
    fi.set_mod_size(0);
    fi.pitch_x   = script_h.readInt();
    fi.pitch_y   = script_h.readInt();
    fi.is_bold   = script_h.readInt() ? true : false;
    fi.is_shadow = script_h.readInt() ? true : false;

    char* buffer = script_h.getNext();
    while (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        ai->num_of_cells++;
        script_h.readStr();
    }
    if (ai->num_of_cells == 0) {
        ai->num_of_cells = 1;
        ai->color_list = new rgb_t[ai->num_of_cells];
	ai->color_list[0].set(0xff);
    }
    else {
        ai->color_list = new rgb_t[ai->num_of_cells];
        script_h.setCurrent(buffer);
        for (int i = 0; i < ai->num_of_cells; i++)
	    ai->color_list[i] = readColour(script_h.readStr());
    }

    ai->trans_mode = AnimationInfo::TRANS_STRING;
    ai->trans = 256;
    ai->visible = true;
    ai->is_single_line  = false;
    ai->is_tight_region = false;
    setupAnimationInfo(ai, &fi);
    return RET_CONTINUE;
}


int PonscripterLabel::stopCommand(const string& cmd)
{
    stopBGM(false);
    wavestopCommand("wavestop");

    return RET_CONTINUE;
}


int PonscripterLabel::sp_rgb_gradationCommand(const string& cmd)
{
    int no = script_h.readInt();
    int upper_r  = script_h.readInt();
    int upper_g  = script_h.readInt();
    int upper_b  = script_h.readInt();
    int lower_r  = script_h.readInt();
    int lower_g  = script_h.readInt();
    int lower_b  = script_h.readInt();
    ONSBuf key_r = script_h.readInt();
    ONSBuf key_g = script_h.readInt();
    ONSBuf key_b = script_h.readInt();
    Uint32 alpha = script_h.readInt();

    AnimationInfo* si;
    if (no == -1) si = &sentence_font_info;
    else si = &sprite_info[no];

    SDL_Surface* surface = si->image_surface;
    if (surface == NULL) return RET_CONTINUE;

    SDL_PixelFormat* fmt = surface->format;

    ONSBuf key_mask = (key_r >> fmt->Rloss) << fmt->Rshift |
                      (key_g >> fmt->Gloss) << fmt->Gshift |
                      (key_b >> fmt->Bloss) << fmt->Bshift;
    ONSBuf rgb_mask = fmt->Rmask | fmt->Gmask | fmt->Bmask;

    SDL_LockSurface(surface);
    // check upper and lower bound
    int  i, j;
    int  upper_bound  = 0, lower_bound = 0;
    bool is_key_found = false;
    for (i = 0; i < surface->h; i++) {
        ONSBuf* buf = (ONSBuf*) surface->pixels + surface->w * i;
        for (j = 0; j < surface->w; j++, buf++) {
            if ((*buf & rgb_mask) == key_mask) {
                if (is_key_found == false) {
                    is_key_found = true;
                    upper_bound  = lower_bound = i;
                }
                else {
                    lower_bound = i;
                }

                break;
            }
        }
    }

    // replace pixels of the key-color with the specified color in gradation
    for (i = upper_bound; i <= lower_bound; i++) {
        ONSBuf* buf = (ONSBuf*) surface->pixels + surface->w * i;
#ifdef BPP16
        unsigned char* alphap = si->alpha_buf + surface->w * i;
#else
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        unsigned char* alphap = (unsigned char*) buf + 3;
#else
        unsigned char* alphap = (unsigned char*) buf;
#endif
#endif
        Uint32 color = alpha << surface->format->Ashift;
        if (upper_bound != lower_bound) {
            color |= (((lower_r - upper_r) * (i - upper_bound) / (lower_bound - upper_bound) + upper_r) >> fmt->Rloss) << fmt->Rshift;
            color |= (((lower_g - upper_g) * (i - upper_bound) / (lower_bound - upper_bound) + upper_g) >> fmt->Gloss) << fmt->Gshift;
            color |= (((lower_b - upper_b) * (i - upper_bound) / (lower_bound - upper_bound) + upper_b) >> fmt->Bloss) << fmt->Bshift;
        }
        else {
            color |= (upper_r >> fmt->Rloss) << fmt->Rshift;
            color |= (upper_g >> fmt->Gloss) << fmt->Gshift;
            color |= (upper_b >> fmt->Bloss) << fmt->Bshift;
        }

        for (j = 0; j < surface->w; j++, buf++) {
            if ((*buf & rgb_mask) == key_mask) {
                *buf    = color;
                *alphap = alpha;
            }

#ifdef BPP16
            alphap++;
#else
            alphap += 4;
#endif
        }
    }

    SDL_UnlockSurface(surface);

    if (si->visible)
        dirty_rect.add(si->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::spstrCommand(const string& cmd)
{
    decodeExbtnControl(script_h.readStr());

    return RET_CONTINUE;
}


int PonscripterLabel::spreloadCommand(const string& cmd)
{
    int no = script_h.readInt();
    AnimationInfo* si;
    if (no == -1) si = &sentence_font_info;
    else si = &sprite_info[no];

    parseTaggedString(si);
    setupAnimationInfo(si);

    if (si->visible)
        dirty_rect.add(si->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::splitCommand(const string& cmd)
{
    string buf = script_h.readStr();
    string delimiter = script_h.readStr();
    delimiter.erase(encoding->CharacterBytes(delimiter.c_str()));
    std::vector<string> parts = buf.split(delimiter);
    printf("Splitting %s on %s: %lu parts\n", buf.c_str(), delimiter.c_str(), parts.size());
    std::vector<string>::const_iterator it = parts.begin();
    while (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
	script_h.readVariable();
        if (script_h.current_variable.type & ScriptHandler::VAR_INT ||
            script_h.current_variable.type & ScriptHandler::VAR_ARRAY) {
	    int val = it == parts.end() ? 0 : atoi(it->c_str());
            script_h.setInt(&script_h.current_variable, val);
        }
        else if (script_h.current_variable.type & ScriptHandler::VAR_STR) {
            script_h.variable_data[script_h.current_variable.var_no].str =
		it == parts.end() ? "" : *it;
        }
	++it;
    }
    return RET_CONTINUE;
}


int PonscripterLabel::spclclkCommand(const string& cmd)
{
    if (!force_button_shortcut_flag)
        spclclk_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::spbtnCommand(const string& cmd)
{
    bool cellcheck_flag = false;

    if (cmd == "cellcheckspbtn")
        cellcheck_flag = true;

    int sprite_no = script_h.readInt();
    int no = script_h.readInt();

    if (cellcheck_flag) {
        if (sprite_info[sprite_no].num_of_cells < 2) return RET_CONTINUE;
    }
    else {
        if (sprite_info[sprite_no].num_of_cells == 0) return RET_CONTINUE;
    }

    ButtonElt button;
    button.button_type = ButtonElt::SPRITE_BUTTON;
    button.sprite_no = sprite_no;

    if (sprite_info[sprite_no].image_surface
        || sprite_info[sprite_no].trans_mode == AnimationInfo::TRANS_STRING) {
        button.image_rect = button.select_rect = sprite_info[sprite_no].pos;
        sprite_info[sprite_no].visible = true;
    }

    buttons[no] = button;

    return RET_CONTINUE;
}


int PonscripterLabel::skipoffCommand(const string& cmd)
{
    skip_flag = false;

    return RET_CONTINUE;
}


int PonscripterLabel::sevolCommand(const string& cmd)
{
    se_volume = script_h.readInt();

    for (int i = 1; i < ONS_MIX_CHANNELS; i++)
        if (wave_sample[i]) Mix_Volume(i, se_volume * 128 / 100);

    if (wave_sample[MIX_LOOPBGM_CHANNEL0])
	Mix_Volume(MIX_LOOPBGM_CHANNEL0, se_volume * 128 / 100);

    if (wave_sample[MIX_LOOPBGM_CHANNEL1])
	Mix_Volume(MIX_LOOPBGM_CHANNEL1, se_volume * 128 / 100);

    return RET_CONTINUE;
}


void PonscripterLabel::setwindowCore()
{
    sentence_font.top_x  = script_h.readInt();
    sentence_font.top_y  = script_h.readInt();
    sentence_font.area_x = script_h.readInt();
    sentence_font.area_y = script_h.readInt();
    int s1 = script_h.readInt(), s2 = script_h.readInt();
    sentence_font.set_size(s1 > s2 ? s1 : s2);
    sentence_font.set_mod_size(0);
    sentence_font.pitch_x   = script_h.readInt();
    sentence_font.pitch_y   = script_h.readInt();
    sentence_font.wait_time = script_h.readInt();
    sentence_font.is_bold   = script_h.readInt() ? true : false;
    sentence_font.is_shadow = script_h.readInt() ? true : false;

    const char* buf = script_h.readStr();
    dirty_rect.add(sentence_font_info.pos);
    if (buf[0] == '#') {
        sentence_font.is_transparent = true;
        sentence_font.window_color = readColour(buf);

        sentence_font_info.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.w = script_h.readInt() * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.x + 1;
        sentence_font_info.pos.h = script_h.readInt() * screen_ratio1 / screen_ratio2 - sentence_font_info.pos.y + 1;
    }
    else {
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName(buf);
        parseTaggedString(&sentence_font_info);
        setupAnimationInfo(&sentence_font_info);
        sentence_font_info.pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
        sentence_font_info.pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
#if 0
        if (sentence_font_info.image_surface) {
            sentence_font_info.pos.w = sentence_font_info.image_surface->w * screen_ratio1 / screen_ratio2;
            sentence_font_info.pos.h = sentence_font_info.image_surface->h * screen_ratio1 / screen_ratio2;
        }

#endif
        sentence_font.window_color.set(0xff);
    }
}


int PonscripterLabel::setwindow3Command(const string& cmd)
{
    setwindowCore();

    clearCurrentTextBuffer();
    indent_offset = 0;
    line_enter_status = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    flush(refreshMode(), &sentence_font_info.pos);
    
    return RET_CONTINUE;
}


int PonscripterLabel::setwindow2Command(const string& cmd)
{
    const char* buf = script_h.readStr();
    if (buf[0] == '#') {
        sentence_font.is_transparent = true;
        sentence_font.window_color = readColour(buf);
    }
    else {
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName(buf);
        parseTaggedString(&sentence_font_info);
        setupAnimationInfo(&sentence_font_info);
    }

    repaintCommand("repaint");

    return RET_CONTINUE;
}


int PonscripterLabel::setwindowCommand(const string& cmd)
{
    setwindowCore();

    lookbackflushCommand("lookbackflush");
    indent_offset = 0;
    line_enter_status = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    flush(refreshMode(), &sentence_font_info.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::setcursorCommand(const string& cmd)
{
    int no = script_h.readInt();
    string cur = script_h.readStr();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    loadCursor(no, cur.c_str(), x, y, cmd == "abssetcursor");

    return RET_CONTINUE;
}


int PonscripterLabel::selectCommand(const string& cmd)
{
    int ret = enterTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int select_mode = SELECT_GOTO_MODE;

    if (cmd == "selnum")
        select_mode = SELECT_NUM_MODE;
    else if (cmd == "selgosub")
        select_mode = SELECT_GOSUB_MODE;
    else if (cmd == "select")
        select_mode = SELECT_GOTO_MODE;
    else if (cmd == "csel")
        select_mode = SELECT_CSEL_MODE;

    if (select_mode == SELECT_NUM_MODE) {
        script_h.readVariable();
        script_h.pushVariable();
    }

    // If waiting for a selection to be made...
    if (event_mode & WAIT_BUTTON_MODE) {
	const int button = current_button_state.button - 1;
        if (button < 0) return RET_WAIT | RET_REREAD;

	playSound(selectvoice_file_name[SELECTVOICE_SELECT],
		  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

        event_mode = IDLE_EVENT_MODE;

        deleteButtons();

        if (select_mode == SELECT_GOTO_MODE) {
            setCurrentLabel(select_links[button].label);
        }
        else if (select_mode == SELECT_GOSUB_MODE) {
            gosubReal(select_links[button].label,
		      select_label_info.next_script);
        }
        else { // selnum
            script_h.setInt(&script_h.pushed_variable, button);
            current_label_info =
		script_h.getLabelByAddress(select_label_info.next_script);
            current_line =
		script_h.getLineByAddress(select_label_info.next_script);
            script_h.setCurrent(select_label_info.next_script);
        }

        select_links.clear();

        newPage(true);

        return RET_CONTINUE;
    }
    // Otherwise, if this is initialising a select point...
    else {
        bool comma_flag = true;
        if (select_mode == SELECT_CSEL_MODE) {
            saveoffCommand("saveoff");
        }

        shortcut_mouse_line = buttons.end();

        float old_x = sentence_font.GetXOffset();
        int   old_y = sentence_font.GetYOffset();

	playSound(selectvoice_file_name[SELECTVOICE_OPEN],
		  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

        select_links.clear();

        while (1) {
            if (script_h.getNext()[0] != 0x0a && comma_flag == true) {
                string text = script_h.readStr();
                comma_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
                if (select_mode != SELECT_NUM_MODE && !comma_flag)
		    errorAndExit(cmd + ": comma is needed here.");

		string label;
                if (select_mode != SELECT_NUM_MODE)
                    label = script_h.readStr() + 1;

		select_links.push_back(SelectElt(text, label));
		
                comma_flag = script_h.getEndStatus() & ScriptHandler::END_COMMA;
            }
            else if (script_h.getNext()[0] == 0x0a) {
                char* buf = script_h.getNext() + 1; // consume eol
                while (*buf == ' ' || *buf == '\t') buf++;

                if (comma_flag && *buf == ',')
                    errorAndExit(cmd + ": double comma.");

                bool comma2_flag = false;
                if (*buf == ',') {
                    comma2_flag = true;
                    buf++;
                    while (*buf == ' ' || *buf == '\t') buf++;
                }

                script_h.setCurrent(buf);

                if (*buf == 0x0a) {
                    comma_flag |= comma2_flag;
                    continue;
                }

                if (!comma_flag && !comma2_flag) {
                    select_label_info.next_script = buf;
                    break;
                }

                comma_flag = true;
            }
            else { // if select ends at the middle of the line
                select_label_info.next_script = script_h.getNext();
                break;
            }
        }

        if (select_mode != SELECT_CSEL_MODE) {
            int counter = 1;
	    for (SelectElt::iterator it = select_links.begin();
		 it != select_links.end(); ++it) {
		if (it->text)
		    buttons[counter] = getSelectableSentence(it->text,
							     &sentence_font);
		++counter;
	    }
        }

        if (select_mode == SELECT_CSEL_MODE) {
            setCurrentLabel("customsel");
            return RET_CONTINUE;
        }

        skip_flag = false;
        automode_flag = false;
        sentence_font.SetXY(old_x, old_y);

        flush(refreshMode());

        event_mode = WAIT_TEXT_MODE | WAIT_BUTTON_MODE | WAIT_TIMER_MODE;
        advancePhase();
        refreshMouseOverButton();

        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::savetimeCommand(const string& cmd)
{
    int no = script_h.readInt();

    SaveFileInfo info;
    searchSaveFile(info, no);

    script_h.readVariable();
    if (!info.valid) {
        script_h.setInt(&script_h.current_variable, 0);
        for (int i = 0; i < 3; i++)
            script_h.readVariable();

        return RET_CONTINUE;
    }

    script_h.setInt(&script_h.current_variable, info.month);
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, info.day);
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, info.hour);
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, info.minute);

    return RET_CONTINUE;
}


int PonscripterLabel::savescreenshotCommand(const string& cmd)
{
    //if (cmd == "savescreenshot") { }
    //else if (cmd == "savescreenshot2") { }

    const char* buf = script_h.readStr();

    char* ext = strrchr(buf, '.');
    if (ext && (!strcmp(ext + 1, "BMP") || !strcmp(ext + 1, "bmp"))) {
	string filename = archive_path + buf;
	for (string::iterator i = filename.begin(); i != filename.end(); ++i)
	    if (*i == '/' || *i == '\\')
		*i = DELIMITER;

        SDL_SaveBMP(screenshot_surface, filename.c_str());
    }
    else
        printf("%s: file %s is not supported.\n", cmd.c_str(), buf);

    return RET_CONTINUE;
}


int PonscripterLabel::saveonCommand(const string& cmd)
{
    saveon_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::saveoffCommand(const string& cmd)
{
    saveon_flag = false;

    return RET_CONTINUE;
}


int PonscripterLabel::savegameCommand(const string& cmd)
{
    int no = script_h.readInt();

    if (no < 0)
        errorAndExit("savegame: the specified number is less than 0.");
    else {
        shelter_event_mode = event_mode;
        saveSaveFile(no);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::savefileexistCommand(const string& cmd)
{
    script_h.readInt();
    script_h.pushVariable();
    int no = script_h.readInt();

    SaveFileInfo info;
    searchSaveFile(info, no);

    script_h.setInt(&script_h.pushed_variable, (info.valid == true) ? 1 : 0);

    return RET_CONTINUE;
}


int PonscripterLabel::rndCommand(const string& cmd)
{
    int upper, lower;

    if (cmd == "rnd2") {
        script_h.readInt();
        script_h.pushVariable();

        lower = script_h.readInt();
        upper = script_h.readInt();
    }
    else {
        script_h.readInt();
        script_h.pushVariable();

        lower = 0;
        upper = script_h.readInt() - 1;
    }

    script_h.setInt(&script_h.pushed_variable,
		    lower + (int)((double)(upper - lower + 1) * rand() /
				  (RAND_MAX + 1.0)));

    return RET_CONTINUE;
}


int PonscripterLabel::rmodeCommand(const string& cmd)
{
    rmode_flag = script_h.readInt() == 1;

    return RET_CONTINUE;
}


int PonscripterLabel::resettimerCommand(const string& cmd)
{
    internal_timer = SDL_GetTicks();
    return RET_CONTINUE;
}


int PonscripterLabel::resetCommand(const string& cmd)
{
    resetSub();
    clearCurrentTextBuffer();

    setCurrentLabel("start");
    saveSaveFile(-1);

    return RET_CONTINUE;
}


int PonscripterLabel::repaintCommand(const string& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::quakeCommand(const string& cmd)
{
    int quake_type;

    if (cmd == "quakey") {
        quake_type = 0;
    }
    else if (cmd == "quakex") {
        quake_type = 1;
    }
    else {
        quake_type = 2;
    }

    tmp_effect.no = script_h.readInt();
    tmp_effect.duration = script_h.readInt();
    if (tmp_effect.duration < tmp_effect.no * 4)
	tmp_effect.duration = tmp_effect.no * 4;

    tmp_effect.effect = CUSTOM_EFFECT_NO + quake_type;

    if (ctrl_pressed_status || skip_to_wait) {
        dirty_rect.fill(screen_width, screen_height);
        SDL_BlitSurface(accumulation_surface, NULL, effect_dst_surface, NULL);
        return RET_CONTINUE;
    }

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(tmp_effect, NULL, DIRECT_EFFECT_IMAGE);
    }
    else {
        dirty_rect.fill(screen_width, screen_height);
        SDL_BlitSurface(accumulation_surface, NULL, effect_dst_surface, NULL);

        return setEffect(tmp_effect); // 2 is dummy value
    }
}


int PonscripterLabel::puttextCommand(const string& cmd)
{
    int ret = enterTextDisplayMode(false);
    if (ret != RET_NOMATCH) return ret;

    script_h.readStr();
    script_h.addStringBuffer(0x0a);
    if (script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR
        && string_buffer_offset == 0)
        string_buffer_offset = 1;

    // skip the heading ^

    ret = processText();
    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a) {
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
            //for (int i=0 ; i<indent_offset ; i++){
            //    current_text_buffer->addBuffer(((char*)"Å@")[0]);
            //    current_text_buffer->addBuffer(((char*)"Å@")[1]);
            //    sentence_font.advanceCharInHankaku(2);
            //}
        }
    }

    if (ret != RET_CONTINUE) {
        ret &= ~RET_NOREAD;
        return ret | RET_REREAD;
    }

    string_buffer_offset = 0;

    return RET_CONTINUE;
}


int PonscripterLabel::prnumclearCommand(const string& cmd)
{
    for (int i = 0; i < MAX_PARAM_NUM; i++) {
        if (prnum_info[i]) {
            dirty_rect.add(prnum_info[i]->pos);
            delete prnum_info[i];
            prnum_info[i] = NULL;
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::prnumCommand(const string& cmd)
{
    int no = script_h.readInt();
    if (prnum_info[no]) {
        dirty_rect.add(prnum_info[no]->pos);
        delete prnum_info[no];
    }

    prnum_info[no] = new AnimationInfo();
    prnum_info[no]->trans_mode   = AnimationInfo::TRANS_STRING;
    prnum_info[no]->num_of_cells = 1;
    prnum_info[no]->setCell(0);
    prnum_info[no]->color_list = new rgb_t[prnum_info[no]->num_of_cells];

    prnum_info[no]->param = script_h.readInt();
    prnum_info[no]->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->font_size_x = script_h.readInt();
    prnum_info[no]->font_size_y = script_h.readInt();

    prnum_info[no]->color_list[0] = readColour(script_h.readStr());

    prnum_info[no]->file_name =
	script_h.stringFromInteger(prnum_info[no]->param, 3);

    setupAnimationInfo(prnum_info[no]);
    dirty_rect.add(prnum_info[no]->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::printCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), NULL, TACHI_EFFECT_IMAGE);
    }
    else {
        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::playstopCommand(const string& cmd)
{
    stopBGM(false);
    return RET_CONTINUE;
}


int PonscripterLabel::playCommand(const string& cmd)
{
    bool loop_flag = cmd != "playonce";

    const char* buf = script_h.readStr();
    if (buf[0] == '*') {
        cd_play_loop_flag = loop_flag;
        int new_cd_track = atoi(buf + 1);
#ifdef CONTINUOUS_PLAY
        if (current_cd_track != new_cd_track) {
#endif
        stopBGM(false);
        current_cd_track = new_cd_track;
        playCDAudio();
#ifdef CONTINUOUS_PLAY
    }

#endif
    }
    else { // play MIDI
        stopBGM(false);

        midi_file_name = buf;
        midi_play_loop_flag = loop_flag;
        if (playSound(midi_file_name, SOUND_MIDI, midi_play_loop_flag) != SOUND_MIDI) {
            fprintf(stderr, "can't play MIDI file %s\n", midi_file_name.c_str());
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::ofscopyCommand(const string& cmd)
{
    SDL_BlitSurface(screen_surface, NULL, accumulation_surface, NULL);

    return RET_CONTINUE;
}


int PonscripterLabel::negaCommand(const string& cmd)
{
    nega_mode = script_h.readInt();

    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::mspCommand(const string& cmd)
{
    int no = script_h.readInt();
    dirty_rect.add(sprite_info[no].pos);
    sprite_info[no].pos.x += script_h.readInt() * screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y += script_h.readInt() * screen_ratio1 / screen_ratio2;
    dirty_rect.add(sprite_info[no].pos);
    if (script_h.getEndStatus() & ScriptHandler::END_COMMA)
        sprite_info[no].trans += script_h.readInt();

    if (sprite_info[no].trans > 256) sprite_info[no].trans = 256;
    else if (sprite_info[no].trans < 0) sprite_info[no].trans = 0;

    return RET_CONTINUE;
}


int PonscripterLabel::mpegplayCommand(const string& cmd)
{
    string name = script_h.readStr();

    bool click_flag = script_h.readInt() == 1;

    stopBGM(false);
    if (playMPEG(name.c_str(), click_flag))
	endCommand("end");

    return RET_CONTINUE;
}


int PonscripterLabel::mp3volCommand(const string& cmd)
{
    music_volume = script_h.readInt();

    if (mp3_sample)
	SMPEG_setvolume(mp3_sample, music_volume);

    return RET_CONTINUE;
}


int PonscripterLabel::mp3fadeoutCommand(const string& cmd)
{
    mp3fadeout_start    = SDL_GetTicks();
    mp3fadeout_duration = script_h.readInt();

    timer_mp3fadeout_id = SDL_AddTimer(20, mp3fadeoutCallback, NULL);

    event_mode |= WAIT_TIMER_MODE;
    return RET_WAIT;
}


int PonscripterLabel::mp3Command(const string& cmd)
{
    bool loop_flag = false;
    if (cmd == "mp3save") {
        mp3save_flag = true;
    }
    else if (cmd == "bgmonce") {
        mp3save_flag = false;
    }
    else if (cmd == "mp3loop" || cmd == "bgm") {
        mp3save_flag = true;
        loop_flag = true;
    }
    else {
        mp3save_flag = false;
    }

    stopBGM(false);
    music_play_loop_flag = loop_flag;

    music_file_name = script_h.readStr();
    playSound(music_file_name,
	      SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3 | SOUND_MIDI,
	      music_play_loop_flag, MIX_BGM_CHANNEL);

    return RET_CONTINUE;
}


int PonscripterLabel::movemousecursorCommand(const string& cmd)
{
    int x = script_h.readInt();
    int y = script_h.readInt();

    SDL_WarpMouse(x, y);

    return RET_CONTINUE;
}


int PonscripterLabel::monocroCommand(const string& cmd)
{
    if (script_h.compareString("off")) {
        script_h.readLabel();
        monocro_flag = false;
    }
    else {
        monocro_flag = true;
        monocro_color = readColour(script_h.readStr());

        for (int i = 0; i < 256; i++) {
            monocro_color_lut[i].r = (monocro_color.r * i) >> 8;
            monocro_color_lut[i].g = (monocro_color.g * i) >> 8;
            monocro_color_lut[i].b = (monocro_color.b * i) >> 8;
        }
    }

    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::menu_windowCommand(const string& cmd)
{
    if (fullscreen_mode) {
#if !defined (PSP)
        if (!SDL_WM_ToggleFullScreen(screen_surface)) {
            SDL_FreeSurface(screen_surface);
            screen_surface = SDL_SetVideoMode(screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG);
            SDL_Rect rect = { 0, 0, screen_width, screen_height };
            flushDirect(rect, refreshMode());
        }

#endif
        fullscreen_mode = false;
    }

    return RET_CONTINUE;
}


int PonscripterLabel::menu_fullCommand(const string& cmd)
{
    if (!fullscreen_mode) {
#if !defined (PSP)
        if (!SDL_WM_ToggleFullScreen(screen_surface)) {
            SDL_FreeSurface(screen_surface);
            screen_surface = SDL_SetVideoMode(screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG | SDL_FULLSCREEN);
            SDL_Rect rect = { 0, 0, screen_width, screen_height };
            flushDirect(rect, refreshMode());
        }

#endif
        fullscreen_mode = true;
    }

    return RET_CONTINUE;
}


int PonscripterLabel::menu_automodeCommand(const string& cmd)
{
    automode_flag = true;
    skip_flag = false;
    printf("menu_automode: change to automode\n");

    return RET_CONTINUE;
}


int PonscripterLabel::lspCommand(const string& cmd)
{
    int no = script_h.readInt();
    if (sprite_info[no].visible)
        dirty_rect.add(sprite_info[no].pos);

    sprite_info[no].visible = cmd != "lsph";

    const char* buf = script_h.readStr();
    sprite_info[no].setImageName(buf);

    sprite_info[no].pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    if (script_h.getEndStatus() & ScriptHandler::END_COMMA)
        sprite_info[no].trans = script_h.readInt();
    else
        sprite_info[no].trans = 256;

    parseTaggedString(&sprite_info[no]);
    setupAnimationInfo(&sprite_info[no]);
    if (sprite_info[no].visible)
        dirty_rect.add(sprite_info[no].pos);

    return RET_CONTINUE;
}


int PonscripterLabel::loopbgmstopCommand(const string& cmd)
{
    if (wave_sample[MIX_LOOPBGM_CHANNEL0]) {
        Mix_Pause(MIX_LOOPBGM_CHANNEL0);
        Mix_FreeChunk(wave_sample[MIX_LOOPBGM_CHANNEL0]);
        wave_sample[MIX_LOOPBGM_CHANNEL0] = NULL;
    }

    if (wave_sample[MIX_LOOPBGM_CHANNEL1]) {
        Mix_Pause(MIX_LOOPBGM_CHANNEL1);
        Mix_FreeChunk(wave_sample[MIX_LOOPBGM_CHANNEL1]);
        wave_sample[MIX_LOOPBGM_CHANNEL1] = NULL;
    }

    loop_bgm_name[0].clear();

    return RET_CONTINUE;
}


int PonscripterLabel::loopbgmCommand(const string& cmd)
{
    loop_bgm_name[0] = script_h.readStr();
    loop_bgm_name[1] = script_h.readStr();    

    playSound(loop_bgm_name[1],
        SOUND_PRELOAD | SOUND_WAVE | SOUND_OGG, false, MIX_LOOPBGM_CHANNEL1);
    playSound(loop_bgm_name[0],
        SOUND_WAVE | SOUND_OGG, false, MIX_LOOPBGM_CHANNEL0);

    return RET_CONTINUE;
}


int PonscripterLabel::lookbackflushCommand(const string& cmd)
{
    current_text_buffer = current_text_buffer->next;
    for (int i = 0; i < max_text_buffer - 1; i++) {
        current_text_buffer->clear();
        current_text_buffer = current_text_buffer->next;
    }

    clearCurrentTextBuffer();
    start_text_buffer = current_text_buffer;

    return RET_CONTINUE;
}


int PonscripterLabel::lookbackbuttonCommand(const string& cmd)
{
    for (int i = 0; i < 4; i++) {
        lookback_info[i].image_name = script_h.readStr();
        parseTaggedString(&lookback_info[i]);
        setupAnimationInfo(&lookback_info[i]);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::logspCommand(const string& cmd)
{
    int sprite_no = script_h.readInt();

    AnimationInfo &si = sprite_info[sprite_no];
    if (si.visible) dirty_rect.add(si.pos);

    si.remove();
    si.file_name = script_h.readStr();

    si.pos.x = script_h.readInt();
    si.pos.y = script_h.readInt();

    si.trans_mode = AnimationInfo::TRANS_STRING;
    if (cmd == "logsp2") {
        si.font_size_x = script_h.readInt();
        si.font_size_y = script_h.readInt();
        si.font_pitch  = script_h.readInt() + si.font_size_x;
        script_h.readInt(); // dummy read for y pitch
    }
    else {
        si.font_size_x = sentence_font.size();
        si.font_size_y = sentence_font.size();
        si.font_pitch  = sentence_font.pitch_x;
    }

    char* current = script_h.getNext();
    int num = 0;
    while (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        script_h.readStr();
        num++;
    }

    script_h.setCurrent(current);
    if (num == 0) {
        si.num_of_cells = 1;
        si.color_list = new rgb_t[si.num_of_cells];
        si.color_list[0].set(0xff);
    }
    else {
        si.num_of_cells = num;
        si.color_list = new rgb_t[si.num_of_cells];
        for (int i = 0; i < num; i++) {
            si.color_list[i] = readColour(script_h.readStr());
        }
    }

    si.is_single_line  = false;
    si.is_tight_region = false;
    sentence_font.is_newline_accepted = true;
    setupAnimationInfo(&si);
    sentence_font.is_newline_accepted = false;
    si.visible = true;
    dirty_rect.add(si.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::locateCommand(const string& cmd)
{
    int x = script_h.readInt();
    int y = script_h.readInt();
    sentence_font.SetXY(x, y);
    return RET_CONTINUE;
}


int PonscripterLabel::loadgameCommand(const string& cmd)
{
    int no = script_h.readInt();

    if (no < 0)
        errorAndExit("loadgame: no < 0.");

    if (loadSaveFile(no)) return RET_CONTINUE;
    else {
        dirty_rect.fill(screen_width, screen_height);
        flush(refreshMode());

        saveon_flag = true;
        internal_saveon_flag = true;
        skip_flag = false;
        automode_flag = false;
        deleteButtons();
        select_links.clear();
        key_pressed_flag = false;
        text_on_flag  = false;
        indent_offset = 0;
        line_enter_status = 0;
        string_buffer_offset = 0;

        refreshMouseOverButton();

        if (loadgosub_label)
            gosubReal(loadgosub_label, script_h.getCurrent());

        readToken();

        if (event_mode & WAIT_INPUT_MODE) return RET_WAIT | RET_NOREAD;

        return RET_CONTINUE | RET_NOREAD;
    }
}


int PonscripterLabel::ldCommand(const string& cmd)
{
    // TEST rca spriteanim patch
#ifdef NO_RCA_CHANGES
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;
#endif

    char loc = script_h.readLabel()[0];
    int no = -1;
    if (loc == 'l') no = 0;
    else if (loc == 'c') no = 1;
    else if (loc == 'r') no = 2;

    const char* buf = NULL;
    if (no >= 0) buf = script_h.readStr();

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), NULL, TACHI_EFFECT_IMAGE);
    }
    else {
        if (no >= 0) {
            dirty_rect.add(tachi_info[no].pos);
            tachi_info[no].setImageName(buf);
            parseTaggedString(&tachi_info[no]);
            setupAnimationInfo(&tachi_info[no]);
            if (tachi_info[no].image_surface) {
                tachi_info[no].visible = true;
                tachi_info[no].pos.x = screen_width * (no + 1) / 4 - tachi_info[no].pos.w / 2;
                tachi_info[no].pos.y = underline_value - tachi_info[no].image_surface->h + 1;
                dirty_rect.add(tachi_info[no].pos);
            }
        }

        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::jumpfCommand(const string& cmd)
{
    char* buf = script_h.getNext();
    while (*buf != '\0' && *buf != '~') buf++;
    if (*buf == '~') buf++;

    script_h.setCurrent(buf);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    return RET_CONTINUE;
}


int PonscripterLabel::jumpbCommand(const string& cmd)
{
    script_h.setCurrent(last_tilde.next_script);
    current_label_info = script_h.getLabelByAddress(last_tilde.next_script);
    current_line = script_h.getLineByAddress(last_tilde.next_script);

    return RET_CONTINUE;
}


int PonscripterLabel::ispageCommand(const string& cmd)
{
    script_h.readInt();

    if (textgosub_clickstr_state == CLICK_NEWPAGE)
        script_h.setInt(&script_h.current_variable, 1);
    else
        script_h.setInt(&script_h.current_variable, 0);

    return RET_CONTINUE;
}


int PonscripterLabel::isfullCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, fullscreen_mode ? 1 : 0);

    return RET_CONTINUE;
}


int PonscripterLabel::isskipCommand(const string& cmd)
{
    script_h.readInt();

    if (automode_flag)
        script_h.setInt(&script_h.current_variable, 2);
    else if (skip_flag)
        script_h.setInt(&script_h.current_variable, 1);
    else if (ctrl_pressed_status)
        script_h.setInt(&script_h.current_variable, 3);
    else
        script_h.setInt(&script_h.current_variable, 0);

    return RET_CONTINUE;
}


int PonscripterLabel::isdownCommand(const string& cmd)
{
    script_h.readInt();

    if (current_button_state.down_flag)
        script_h.setInt(&script_h.current_variable, 1);
    else
        script_h.setInt(&script_h.current_variable, 0);

    return RET_CONTINUE;
}


int PonscripterLabel::inputCommand(const string& cmd)
{
    script_h.readStr();

    if (script_h.current_variable.type != ScriptHandler::VAR_STR)
        errorAndExit("input: no string variable.");

    int no = script_h.current_variable.var_no;

    script_h.readStr(); // description
    script_h.variable_data[no].str = script_h.readStr();

    printf("*** inputCommand(): $%d is set to the default value: %s\n",
	   no, script_h.variable_data[no].str.c_str());
    script_h.readInt(); // maxlen
    script_h.readInt(); // widechar flag
    if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        script_h.readInt(); // window width
        script_h.readInt(); // window height
        script_h.readInt(); // text box width
        script_h.readInt(); // text box height
    }

    return RET_CONTINUE;
}


int PonscripterLabel::indentCommand(const string& cmd)
{
    indent_offset = script_h.readInt();
    fprintf(stderr, " warning: [indent] command is broken\n");
    return RET_CONTINUE;
}


int PonscripterLabel::humanorderCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    const char* buf = script_h.readStr();
    int i;
    for (i = 0; i < 3; i++) {
        if (buf[i] == 'l') human_order[i] = 0;
        else if (buf[i] == 'c') human_order[i] = 1;
        else if (buf[i] == 'r') human_order[i] = 2;
        else human_order[i] = -1;
    }

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), &bg_info, bg_effect_image);
    }
    else {
        for (i = 0; i < 3; i++)
            dirty_rect.add(tachi_info[i].pos);

        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::getzxcCommand(const string& cmd)
{
    getzxc_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getvoicevolCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, voice_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getversionCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, NSC_VERSION);

    return RET_CONTINUE;
}


int PonscripterLabel::gettimerCommand(const string& cmd)
{
    script_h.readInt();

    if (cmd == "gettimer") {
        script_h.setInt(&script_h.current_variable, SDL_GetTicks() - internal_timer);
    }
    else {
        script_h.setInt(&script_h.current_variable, btnwait_time);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::gettextCommand(const string& cmd)
{
    script_h.readStr();
    int no = script_h.current_variable.var_no;

    char* buf = new char[current_text_buffer->contents.size() + 1];
    unsigned int i, j;
    for (i = 0, j = 0; i < current_text_buffer->contents.size(); i++) {
        if (current_text_buffer->contents[i] != 0x0a)
            buf[j++] = current_text_buffer->contents[i];
    }

    buf[j] = '\0';

    script_h.variable_data[no].str = buf;
    delete[] buf;

    return RET_CONTINUE;
}


int PonscripterLabel::gettagCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::LABEL)
        errorAndExit("gettag: not in a subroutine, e.g. pretextgosub");

    bool end_flag = false;
    char* buf = nest_infos.back().next_script;
    while (*buf == ' ' || *buf == '\t') buf++;
    if (zenkakko_flag && encoding->Decode(buf) == 0x3010 /*Åy */)
        buf += encoding->CharacterBytes(buf);
    else if (*buf == '[')
        buf++;
    else
        end_flag = true;

    int end_status;
    do {
        script_h.readVariable();
        end_status = script_h.getEndStatus();
        script_h.pushVariable();

        if (script_h.pushed_variable.type & ScriptHandler::VAR_INT
            || script_h.pushed_variable.type & ScriptHandler::VAR_ARRAY) {
            if (end_flag)
                script_h.setInt(&script_h.pushed_variable, 0);
            else {
                script_h.setInt(&script_h.pushed_variable, script_h.parseInt(&buf));
            }
        }
        else if (script_h.pushed_variable.type & ScriptHandler::VAR_STR) {
            if (end_flag)
                script_h.variable_data[script_h.pushed_variable.var_no].str.clear();
            else {
                const char* buf_start = buf;
                while (*buf != '/'
                       && (!zenkakko_flag || encoding->Decode(buf) != 0x3011 /* Åz*/)
                       && *buf != ']') {
                    buf += encoding->CharacterBytes(buf);
                }
                script_h.variable_data[script_h.pushed_variable.var_no].str.assign(buf_start, buf - buf_start);
            }
        }

        if (*buf == '/')
            buf++;
        else
            end_flag = true;
    }
    while (end_status & ScriptHandler::END_COMMA);

    if (zenkakko_flag && encoding->Decode(buf) == 0x3010 /*Åy */)
	buf += encoding->CharacterBytes(buf);
    else if (*buf == ']') buf++;

    while (*buf == ' ' || *buf == '\t') buf++;
    nest_infos.back().next_script = buf;

    return RET_CONTINUE;
}


int PonscripterLabel::gettabCommand(const string& cmd)
{
    gettab_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getspsizeCommand(const string& cmd)
{
    int no = script_h.readInt();

    script_h.readVariable();
    script_h.setInt(&script_h.current_variable,
		    sprite_info[no].pos.w * screen_ratio2 / screen_ratio1);
    script_h.readVariable();
    script_h.setInt(&script_h.current_variable,
		    sprite_info[no].pos.h * screen_ratio2 / screen_ratio1);
    if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        script_h.readVariable();
        script_h.setInt(&script_h.current_variable, sprite_info[no].num_of_cells);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::getspmodeCommand(const string& cmd)
{
    script_h.readVariable();
    script_h.pushVariable();

    int no = script_h.readInt();
    script_h.setInt(&script_h.pushed_variable, sprite_info[no].visible ? 1 : 0);

    return RET_CONTINUE;
}


int PonscripterLabel::getsevolCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, se_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getscreenshotCommand(const string& cmd)
{
    int w = script_h.readInt();
    int h = script_h.readInt();
    if (w == 0) w = 1;

    if (h == 0) h = 1;

    if (screenshot_surface
        && screenshot_surface->w != w
        && screenshot_surface->h != h) {
        SDL_FreeSurface(screenshot_surface);
        screenshot_surface = NULL;
    }

    if (screenshot_surface == NULL)
        screenshot_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

    SDL_Surface* surface = SDL_ConvertSurface(screen_surface, image_surface->format, SDL_SWSURFACE);
    resizeSurface(surface, screenshot_surface);
    SDL_FreeSurface(surface);

    return RET_CONTINUE;
}


int PonscripterLabel::getpageupCommand(const string& cmd)
{
    getpageup_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getpageCommand(const string& cmd)
{
    getpageup_flag   = true;
    getpagedown_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getretCommand(const string& cmd)
{
    script_h.readVariable();

    if (script_h.current_variable.type == ScriptHandler::VAR_INT
        || script_h.current_variable.type == ScriptHandler::VAR_ARRAY) {
        script_h.setInt(&script_h.current_variable, getret_int);
    }
    else if (script_h.current_variable.type == ScriptHandler::VAR_STR) {
        int no = script_h.current_variable.var_no;
        script_h.variable_data[no].str = getret_str;
    }
    else errorAndExit("getret: no variable.");

    return RET_CONTINUE;
}


int PonscripterLabel::getregCommand(const string& cmd)
{
    script_h.readVariable();

    if (script_h.current_variable.type != ScriptHandler::VAR_STR)
        errorAndExit("getreg: no string variable.");

    int no = script_h.current_variable.var_no;

    const char* buf = script_h.readStr();
    char path[256], key[256];
    strcpy(path, buf);
    buf = script_h.readStr();
    strcpy(key, buf);

    printf("  reading Registry file for [%s] %s\n", path, key);

    FILE* fp;
    if ((fp = fopen(registry_file.c_str(), "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n", registry_file.c_str());
        return RET_CONTINUE;
    }

    char reg_buf[256], reg_buf2[256];
    bool found_flag = false;
    while (fgets(reg_buf, 256, fp) && !found_flag) {
        if (reg_buf[0] == '[') {
            unsigned int c = 0;
            while (reg_buf[c] != ']' && reg_buf[c] != '\0') c++;
            if (!strncmp(reg_buf + 1, path, (c - 1 > strlen(path)) ? (c - 1) : strlen(path))) {
                while (fgets(reg_buf2, 256, fp)) {
                    script_h.pushCurrent(reg_buf2);
                    buf = script_h.readStr();
                    if (strncmp(buf,
                            key,
                            (strlen(buf) > strlen(key)) ? strlen(buf) : strlen(key))) {
                        script_h.popCurrent();
                        continue;
                    }

                    if (!script_h.compareString("=")) {
                        script_h.popCurrent();
                        continue;
                    }

                    script_h.setCurrent(script_h.getNext() + 1);

                    script_h.variable_data[no].str = script_h.readStr();
                    script_h.popCurrent();
                    printf("  $%d = %s\n", no, script_h.variable_data[no].str.c_str());
                    found_flag = true;
                    break;
                }
            }
        }
    }

    if (!found_flag) fprintf(stderr, "  The key is not found.\n");

    fclose(fp);

    return RET_CONTINUE;
}


int PonscripterLabel::getmp3volCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, music_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getmouseposCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, current_button_state.x * screen_ratio2 / screen_ratio1);

    script_h.readInt();
    script_h.setInt(&script_h.current_variable, current_button_state.y * screen_ratio2 / screen_ratio1);

    return RET_CONTINUE;
}


int PonscripterLabel::getlogCommand(const string& cmd)
{
    script_h.readVariable();
    script_h.pushVariable();

    int page_no = script_h.readInt();

    TextBuffer* t_buf = current_text_buffer;
    while (t_buf != start_text_buffer && page_no > 0) {
        page_no--;
        t_buf = t_buf->previous;
    }

    if (page_no > 0)
        script_h.variable_data[script_h.pushed_variable.var_no].str.clear();
    else
        script_h.variable_data[script_h.pushed_variable.var_no].str =
	    t_buf->contents;

    return RET_CONTINUE;
}


int PonscripterLabel::getinsertCommand(const string& cmd)
{
    getinsert_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getfunctionCommand(const string& cmd)
{
    getfunction_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getenterCommand(const string& cmd)
{
    if (!force_button_shortcut_flag)
        getenter_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getcursorposCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable,
		    int(floor(sentence_font.GetX())));

    script_h.readInt();
    script_h.setInt(&script_h.current_variable, sentence_font.GetY());

    return RET_CONTINUE;
}


int PonscripterLabel::getcursorCommand(const string& cmd)
{
    if (!force_button_shortcut_flag)
        getcursor_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getcselstrCommand(const string& cmd)
{
    script_h.readVariable();
    script_h.pushVariable();

    int csel_no = script_h.readInt();
    if (csel_no >= (int)select_links.size())
	errorAndExit("getcselstr: no select link");

    script_h.variable_data[script_h.pushed_variable.var_no].str =
	select_links[csel_no].text;

    return RET_CONTINUE;
}


int PonscripterLabel::getcselnumCommand(const string& cmd)
{
    script_h.readInt();
    script_h.setInt(&script_h.current_variable, (int)select_links.size());
    return RET_CONTINUE;
}


int PonscripterLabel::gameCommand(const string& cmd)
{
    int i;
    current_mode = NORMAL_MODE;

    /* ---------------------------------------- */
    if (!lookback_info[0].image_surface) {
        lookback_info[0].image_name = DEFAULT_LOOKBACK_NAME0;
        parseTaggedString(&lookback_info[0]);
        setupAnimationInfo(&lookback_info[0]);
    }

    if (!lookback_info[1].image_surface) {
        lookback_info[1].image_name = DEFAULT_LOOKBACK_NAME1;
        parseTaggedString(&lookback_info[1]);
        setupAnimationInfo(&lookback_info[1]);
    }

    if (!lookback_info[2].image_surface) {
        lookback_info[2].image_name = DEFAULT_LOOKBACK_NAME2;
        parseTaggedString(&lookback_info[2]);
        setupAnimationInfo(&lookback_info[2]);
    }

    if (!lookback_info[3].image_surface) {
        lookback_info[3].image_name = DEFAULT_LOOKBACK_NAME3;
        parseTaggedString(&lookback_info[3]);
        setupAnimationInfo(&lookback_info[3]);
    }

    /* ---------------------------------------- */
    /* Load default cursor */
    loadCursor(CURSOR_WAIT_NO, DEFAULT_CURSOR_WAIT, 0, 0);
    loadCursor(CURSOR_NEWPAGE_NO, DEFAULT_CURSOR_NEWPAGE, 0, 0);

    /* ---------------------------------------- */
    /* Initialize text buffer */
    text_buffer = new TextBuffer[max_text_buffer];
    for (i = 0; i < max_text_buffer - 1; i++) {
        text_buffer[i].next = &text_buffer[i + 1];
        text_buffer[i + 1].previous = &text_buffer[i];
    }

    text_buffer[0].previous = &text_buffer[max_text_buffer - 1];
    text_buffer[max_text_buffer - 1].next = &text_buffer[0];
    start_text_buffer = current_text_buffer = &text_buffer[0];

    clearCurrentTextBuffer();

    /* ---------------------------------------- */
    /* Initialize local variables */
    for (i = 0; i < script_h.global_variable_border; i++)
        script_h.variable_data[i].reset(false);

    setCurrentLabel("start");
    saveSaveFile(-1);

    return RET_CONTINUE;
}


int PonscripterLabel::fileexistCommand(const string& cmd)
{
    script_h.readInt();
    script_h.pushVariable();
    const char* buf = script_h.readStr();

    script_h.setInt(&script_h.pushed_variable,
		    (ScriptHandler::cBR->getFileLength(buf) > 0) ? 1 : 0);

    return RET_CONTINUE;
}


int PonscripterLabel::exec_dllCommand(const string& cmd)
{
    const char* buf = script_h.readStr();
    char dll_name[256];
    unsigned int c = 0;
    while (buf[c] != '/') {
        dll_name[c] = buf[c];
        c++;
    }
    dll_name[c] = '\0';

    printf("  reading %s for %s\n", dll_file.c_str(), dll_name);

    FILE* fp;
    if ((fp = fopen(dll_file.c_str(), "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n", dll_file.c_str());
        return RET_CONTINUE;
    }

    char dll_buf[256], dll_buf2[256];
    bool found_flag = false;
    while (fgets(dll_buf, 256, fp) && !found_flag) {
        if (dll_buf[0] == '[') {
            c = 0;
            while (dll_buf[c] != ']' && dll_buf[c] != '\0') c++;
            if (!strncmp(dll_buf + 1, dll_name, (c - 1 > strlen(dll_name)) ? (c - 1) : strlen(dll_name))) {
                found_flag = true;
                while (fgets(dll_buf2, 256, fp)) {
                    c = 0;
                    while (dll_buf2[c] == ' ' || dll_buf2[c] == '\t') c++;
                    if (!strncmp(&dll_buf2[c], "str", 3)) {
                        c += 3;
                        while (dll_buf2[c] == ' ' || dll_buf2[c] == '\t') c++;
                        if (dll_buf2[c] != '=') continue;

                        c++;
                        while (dll_buf2[c] != '"') c++;
                        unsigned int c2 = ++c;
                        while (dll_buf2[c2] != '"' && dll_buf2[c2] != '\0') c2++;
                        dll_buf2[c2] = '\0';
                        getret_str = dll_buf2 + c;
                        printf("  getret_str = %s\n", getret_str.c_str());
                    }
                    else if (!strncmp(&dll_buf2[c], "ret", 3)) {
                        c += 3;
                        while (dll_buf2[c] == ' ' || dll_buf2[c] == '\t') c++;
                        if (dll_buf2[c] != '=') continue;

                        c++;
                        while (dll_buf2[c] == ' ' || dll_buf2[c] == '\t') c++;
                        getret_int = atoi(&dll_buf2[c]);
                        printf("  getret_int = %d\n", getret_int);
                    }
                    else if (dll_buf2[c] == '[')
                        break;
                }
            }
        }
    }

    if (!found_flag) fprintf(stderr, "  The DLL is not found in %s.\n", dll_file.c_str());

    fclose(fp);

    return RET_CONTINUE;
}


int PonscripterLabel::exbtnCommand(const string& cmd)
{
    int sprite_no = -1, no = 0;
    ButtonElt* button = &exbtn_d_button;

    if (cmd != "exbtn_d") {
        bool cellcheck_flag = cmd == "cellcheckexbtn";
        sprite_no = script_h.readInt();
        no = script_h.readInt();
        if (cellcheck_flag && (sprite_info[sprite_no].num_of_cells < 2)
            || !cellcheck_flag && (sprite_info[sprite_no].num_of_cells == 0)) {
            script_h.readStr();
            return RET_CONTINUE;
        }
	button = &buttons[no];
    }

    button->button_type = ButtonElt::EX_SPRITE_BUTTON;
    button->sprite_no = sprite_no;
    button->exbtn_ctl = script_h.readStr();

    if (sprite_no >= 0
        && (sprite_info[sprite_no].image_surface ||
            sprite_info[sprite_no].trans_mode == AnimationInfo::TRANS_STRING)) {
        button->image_rect = button->select_rect = sprite_info[sprite_no].pos;
        sprite_info[sprite_no].visible = true;
    }

    return RET_CONTINUE;
}


int PonscripterLabel::erasetextwindowCommand(const string& cmd)
{
    erase_text_window_mode = script_h.readInt();
    dirty_rect.add(sentence_font_info.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::endCommand(const string& cmd)
{
    quit();
    exit(0);
    return RET_CONTINUE; // dummy
}


int PonscripterLabel::dwavestopCommand(const string& cmd)
{
    int ch = script_h.readInt();

    if (wave_sample[ch]) {
        Mix_Pause(ch);
        Mix_FreeChunk(wave_sample[ch]);
        wave_sample[ch] = NULL;
    }

    return RET_CONTINUE;
}


int PonscripterLabel::dwaveCommand(const string& cmd)
{
    int play_mode  = WAVE_PLAY;
    bool loop_flag = false;

    if (cmd == "dwaveloop") {
        loop_flag = true;
    }
    else if (cmd == "dwaveload") {
        play_mode = WAVE_PRELOAD;
    }
    else if (cmd == "dwaveplayloop") {
        play_mode = WAVE_PLAY_LOADED;
        loop_flag = true;
    }
    else if (cmd == "dwaveplay") {
        play_mode = WAVE_PLAY_LOADED;
        loop_flag = false;
    }

    int ch = script_h.readInt();
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS - 1;

    if (play_mode == WAVE_PLAY_LOADED) {
        Mix_PlayChannel(ch, wave_sample[ch], loop_flag ? -1 : 0);
    }
    else {
        const char* buf = script_h.readStr();
        int fmt = SOUND_WAVE | SOUND_OGG;
        if (play_mode == WAVE_PRELOAD) fmt |= SOUND_PRELOAD;

        playSound(buf, fmt, loop_flag, ch);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::dvCommand(const string& cmd)
{
    string buf = "voice";
    buf += DELIMITER;
    buf.append(cmd, 2);
    playSound(buf, SOUND_WAVE | SOUND_OGG, false, 0);
    return RET_CONTINUE;
}


int PonscripterLabel::drawtextCommand(const string& cmd)
{
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    text_info.blendOnSurface(accumulation_surface, 0, 0, clip);

    return RET_CONTINUE;
}


int PonscripterLabel::drawsp3Command(const string& cmd)
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int mat[2][2];
    mat[0][0] = script_h.readInt();
    mat[0][1] = script_h.readInt();
    mat[1][0] = script_h.readInt();
    mat[1][1] = script_h.readInt();

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    si.blendOnSurface2(accumulation_surface, x, y, alpha, mat);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawsp2Command(const string& cmd)
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int scale_x = script_h.readInt();
    int scale_y = script_h.readInt();
    int rot = script_h.readInt();

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    if (scale_x == 0 || scale_y == 0) return RET_CONTINUE;

    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int mat[2][2];
    int cos_i = 1000, sin_i = 0;
    if (rot != 0) {
        cos_i = (int) (1000.0 * cos(-M_PI * rot / 180));
        sin_i = (int) (1000.0 * sin(-M_PI * rot / 180));
    }

    mat[0][0] = cos_i * scale_x / 100;
    mat[0][1] = -sin_i * scale_y / 100;
    mat[1][0] = sin_i * scale_x / 100;
    mat[1][1] = cos_i * scale_y / 100;
    si.blendOnSurface2(accumulation_surface, x, y, alpha, mat);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawspCommand(const string& cmd)
{
    int sprite_no = script_h.readInt();
    int cell_no = script_h.readInt();
    int alpha = script_h.readInt();
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    si.blendOnSurface(accumulation_surface, x, y, clip, alpha);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawfillCommand(const string& cmd)
{
    int r = script_h.readInt();
    int g = script_h.readInt();
    int b = script_h.readInt();

    SDL_FillRect(accumulation_surface, NULL, SDL_MapRGBA(accumulation_surface->format, r, g, b, 0xff));

    return RET_CONTINUE;
}


int PonscripterLabel::drawclearCommand(const string& cmd)
{
    SDL_FillRect(accumulation_surface, NULL, SDL_MapRGBA(accumulation_surface->format, 0, 0, 0, 0xff));

    return RET_CONTINUE;
}


int PonscripterLabel::drawbgCommand(const string& cmd)
{
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    bg_info.blendOnSurface(accumulation_surface, bg_info.pos.x, bg_info.pos.y, clip);

    return RET_CONTINUE;
}


int PonscripterLabel::drawbg2Command(const string& cmd)
{
    int x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    int scale_x = script_h.readInt();
    int scale_y = script_h.readInt();
    int rot = script_h.readInt();

    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int mat[2][2];
    int cos_i = 1000, sin_i = 0;
    if (rot != 0) {
        cos_i = (int) (1000.0 * cos(-M_PI * rot / 180));
        sin_i = (int) (1000.0 * sin(-M_PI * rot / 180));
    }

    mat[0][0] = cos_i * scale_x / 100;
    mat[0][1] = -sin_i * scale_y / 100;
    mat[1][0] = sin_i * scale_x / 100;
    mat[1][1] = cos_i * scale_y / 100;

    bg_info.blendOnSurface2(accumulation_surface, x, y,
        256, mat);

    return RET_CONTINUE;
}


int PonscripterLabel::drawCommand(const string& cmd)
{
    SDL_Rect rect = { 0, 0, screen_width, screen_height };
    flushDirect(rect, REFRESH_NONE_MODE);
    dirty_rect.clear();

    return RET_CONTINUE;
}


int PonscripterLabel::delayCommand(const string& cmd)
{
    int t = script_h.readInt();

    if (event_mode & WAIT_INPUT_MODE) {
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else {
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        startTimer(t);
        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::defineresetCommand(const string& cmd)
{
    script_h.reset();
    ScriptParser::reset();
    reset();

    setCurrentLabel("define");

    return RET_CONTINUE;
}


int PonscripterLabel::cspCommand(const string& cmd)
{
    int no = script_h.readInt();

    if (no == -1)
        for (int i = 0; i < MAX_SPRITE_NUM; i++) {
            if (sprite_info[i].visible)
                dirty_rect.add(sprite_info[i].pos);

            if (sprite_info[i].image_name) {
                sprite_info[i].pos.x = -1000 * screen_ratio1 / screen_ratio2;
                sprite_info[i].pos.y = -1000 * screen_ratio1 / screen_ratio2;
            }

            buttonsRemoveSprite(i);
            sprite_info[i].remove();
        }
    else if (no >= 0 && no < MAX_SPRITE_NUM) {
        if (sprite_info[no].visible)
            dirty_rect.add(sprite_info[no].pos);

        buttonsRemoveSprite(no);
        sprite_info[no].remove();
    }

    return RET_CONTINUE;
}


int PonscripterLabel::cselgotoCommand(const string& cmd)
{
    int csel_no = script_h.readInt();
    if (csel_no >= (int)select_links.size())
	errorAndExit("cselgoto: no select link");

    setCurrentLabel(select_links[csel_no].label);
    select_links.clear();
    newPage(true);

    return RET_CONTINUE;
}


int PonscripterLabel::cselbtnCommand(const string& cmd)
{
    int csel_no   = script_h.readInt();
    int button_no = script_h.readInt();

    FontInfo csel_info = sentence_font;
    csel_info.top_x = script_h.readInt();
    csel_info.top_y = script_h.readInt();

    if (csel_no >= (int)select_links.size()) return RET_CONTINUE;
    const string& text = select_links[csel_no].text;
    if (!text) return RET_CONTINUE;

    csel_info.setLineArea(int(ceil(csel_info.StringAdvance(text))));
    csel_info.clear();
    buttons[button_no] = getSelectableSentence(text, &csel_info);
    buttons[button_no].sprite_no = csel_no;

    return RET_CONTINUE;
}


int PonscripterLabel::clickCommand(const string& cmd)
{
    if (event_mode & WAIT_INPUT_MODE) {
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else {
        skip_flag  = false;
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::clCommand(const string& cmd)
{
    // TEST rca spriteanim patch
#ifdef NO_RCA_CHANGES
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;
#endif

    char loc = script_h.readLabel()[0];

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), NULL, TACHI_EFFECT_IMAGE);
    }
    else {
        if (loc == 'l' || loc == 'a') {
            dirty_rect.add(tachi_info[0].pos);
            tachi_info[0].remove();
        }

        if (loc == 'c' || loc == 'a') {
            dirty_rect.add(tachi_info[1].pos);
            tachi_info[1].remove();
        }

        if (loc == 'r' || loc == 'a') {
            dirty_rect.add(tachi_info[2].pos);
            tachi_info[2].remove();
        }

        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::chvolCommand(const string& cmd)
{
    int ch  = script_h.readInt();
    int vol = script_h.readInt();

    if (wave_sample[ch]) {
        Mix_Volume(ch, vol * 128 / 100);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::checkpageCommand(const string& cmd)
{
    script_h.readVariable();
    script_h.pushVariable();

    if (script_h.pushed_variable.type != ScriptHandler::VAR_INT
        && script_h.pushed_variable.type != ScriptHandler::VAR_ARRAY)
        errorAndExit("checkpage: no variable.");

    int page_no = script_h.readInt();

    TextBuffer* t_buf = current_text_buffer;
    while (t_buf != start_text_buffer && page_no > 0) {
        page_no--;
        t_buf = t_buf->previous;
    }

    if (page_no > 0)
        script_h.setInt(&script_h.pushed_variable, 0);
    else
        script_h.setInt(&script_h.pushed_variable, 1);

    return RET_CONTINUE;
}


int PonscripterLabel::cellCommand(const string& cmd)
{
    int sprite_no = script_h.readInt();
    int no = script_h.readInt();

    sprite_info[sprite_no].setCell(no);
    dirty_rect.add(sprite_info[sprite_no].pos);

    return RET_CONTINUE;
}


int PonscripterLabel::captionCommand(const string& cmd)
{
    const char* buf = script_h.readStr();
    SDL_WM_SetCaption(buf, buf);
    return RET_CONTINUE;
}


int PonscripterLabel::btnwaitCommand(const string& cmd)
{
    bool del_flag = false, textbtn_flag = false;

    if (cmd == "btnwait2") {
        if (erase_text_window_mode > 0) display_mode = NORMAL_DISPLAY_MODE;
    }
    else if (cmd == "btnwait") {
        del_flag = true;
        if (erase_text_window_mode > 0) display_mode = NORMAL_DISPLAY_MODE;
    }
    else if (cmd == "textbtnwait") {
        textbtn_flag = true;
    }

    script_h.readInt();

    if (event_mode & WAIT_BUTTON_MODE
        || (textbtn_flag && (skip_flag || (draw_one_page_flag && clickstr_state == CLICK_WAIT) || ctrl_pressed_status))) {
        btnwait_time  = SDL_GetTicks() - internal_button_timer;
        btntime_value = 0;
        num_chars_in_sentence = 0;

        if (textbtn_flag && (skip_flag || (draw_one_page_flag && clickstr_state == CLICK_WAIT) || ctrl_pressed_status))
            current_button_state.button = 0;

        script_h.setInt(&script_h.current_variable, current_button_state.button);

        if (current_button_state.button >= 1 && del_flag)
            deleteButtons();

        event_mode = IDLE_EVENT_MODE;
        disableGetButtonFlag();

	for (ButtonElt::iterator it = buttons.begin(); it != buttons.end();
	     ++it)
	    it->second.show_flag = 0;

        return RET_CONTINUE;
    }
    else {
        shortcut_mouse_line = buttons.begin();
        skip_flag = false;

        if (exbtn_d_button.exbtn_ctl) {
            SDL_Rect check_src_rect = { 0, 0, screen_width, screen_height };
            decodeExbtnControl(exbtn_d_button.exbtn_ctl, &check_src_rect);
        }

	for (ButtonElt::iterator it = buttons.begin(); it != buttons.end();
	     ++it) {
	    it->second.show_flag = 0;
	    if (it->second.isTmpSprite())
		it->second.show_flag = 1;
	    else if (it->second.anim[1] && !it->second.isSprite())
		it->second.show_flag = 2;
	}

        flush(refreshMode());

        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();

        if (btntime_value > 0) {
            if (btntime2_flag) event_mode |= WAIT_VOICE_MODE;
            startTimer(btntime_value);
        }

        internal_button_timer = SDL_GetTicks();

        if (textbtn_flag) {
            event_mode |= WAIT_TEXTBTN_MODE;
            if (btntime_value == 0) {
                if (automode_flag) {
                    event_mode |= WAIT_VOICE_MODE;
                    if (automode_time < 0)
                        startTimer(-automode_time * num_chars_in_sentence);
                    else
                        startTimer(automode_time);
                }
		else if (autoclick_time > 0)
		    startTimer(autoclick_time);
            }
        }

        if ((event_mode & WAIT_TIMER_MODE) == 0) {
            event_mode |= WAIT_TIMER_MODE;
            advancePhase();
        }

        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::btntimeCommand(const string& cmd)
{
    btntime2_flag = cmd == "btntime2";
    btntime_value = script_h.readInt();

    return RET_CONTINUE;
}


int PonscripterLabel::btndownCommand(const string& cmd)
{
    btndown_flag = script_h.readInt() == 1;

    return RET_CONTINUE;
}


int PonscripterLabel::btndefCommand(const string& cmd)
{
    if (script_h.compareString("clear")) {
        script_h.readLabel();
    }
    else {
        string buf = script_h.readStr();

        btndef_info.remove();

        if (buf) {
            btndef_info.setImageName(buf);
            parseTaggedString(&btndef_info);
            btndef_info.trans_mode = AnimationInfo::TRANS_COPY;
            setupAnimationInfo(&btndef_info);
	    if (btndef_info.image_surface)
		SDL_SetAlpha(btndef_info.image_surface, DEFAULT_BLIT_FLAG,
			     SDL_ALPHA_OPAQUE);
	    else
		fprintf(stderr, "Could not create button: %s not found\n",
			buf.c_str());
        }
    }

//printf("deleteButtons()... ");fflush(stdout);
    deleteButtons();
//printf("OK\n");

    disableGetButtonFlag();

    return RET_CONTINUE;
}


int PonscripterLabel::btnCommand(const string& cmd)
{
    SDL_Rect src_rect;

    const int no = script_h.readInt();

    ButtonElt* button = &buttons[no];
    
    button->image_rect.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.w = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->image_rect.h = script_h.readInt() * screen_ratio1 / screen_ratio2;
    button->select_rect = button->image_rect;

    src_rect.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    src_rect.y = script_h.readInt() * screen_ratio1 / screen_ratio2;
    if (btndef_info.image_surface
        && src_rect.x + button->image_rect.w > btndef_info.image_surface->w) {
        button->image_rect.w = btndef_info.image_surface->w - src_rect.x;
    }

    if (btndef_info.image_surface
        && src_rect.y + button->image_rect.h > btndef_info.image_surface->h) {
        button->image_rect.h = btndef_info.image_surface->h - src_rect.y;
    }

    src_rect.w = button->image_rect.w;
    src_rect.h = button->image_rect.h;

    button->anim[0] = new AnimationInfo();
    button->anim[0]->num_of_cells = 1;
    button->anim[0]->trans_mode = AnimationInfo::TRANS_COPY;
    button->anim[0]->pos.x = button->image_rect.x;
    button->anim[0]->pos.y = button->image_rect.y;
    button->anim[0]->allocImage(button->image_rect.w, button->image_rect.h);
    button->anim[0]->copySurface(btndef_info.image_surface, &src_rect);

    return RET_CONTINUE;
}


int PonscripterLabel::brCommand(const string& cmd)
{
    int delta = cmd == "br2" ? script_h.readInt() : 50;

    int ret = enterTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int cs = sentence_font.mod_size(),
	ns = sentence_font.base_size() * delta / 100;
    sentence_font.set_mod_size(ns);
    sentence_font.newLine();
    sentence_font.set_mod_size(cs);

    current_text_buffer->addBuffer(0x17);
    current_text_buffer->addBuffer(ns & 0x7f);
    current_text_buffer->addBuffer((ns >> 7) & 0x7f);
    current_text_buffer->addBuffer(0x0a);
    current_text_buffer->addBuffer(0x17);
    current_text_buffer->addBuffer(cs & 0x7f);
    current_text_buffer->addBuffer((cs >> 7) & 0x7f);

    return RET_CONTINUE;
}


int PonscripterLabel::bltCommand(const string& cmd)
{
    int dx, dy, dw, dh;
    int sx, sy, sw, sh;

    dx = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dy = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dw = script_h.readInt() * screen_ratio1 / screen_ratio2;
    dh = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sx = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sy = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sw = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sh = script_h.readInt() * screen_ratio1 / screen_ratio2;

    if (btndef_info.image_surface == NULL) return RET_CONTINUE;

    if (dw == 0 || dh == 0 || sw == 0 || sh == 0) return RET_CONTINUE;

    if (sw == dw && sw > 0 && sh == dh && sh > 0) {
        SDL_Rect src_rect = { sx, sy, sw, sh };
        SDL_Rect dst_rect = { dx, dy, dw, dh };

        SDL_BlitSurface(btndef_info.image_surface, &src_rect, screen_surface, &dst_rect);
        SDL_UpdateRect(screen_surface, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
        dirty_rect.clear();
    }
    else {
        SDL_LockSurface(accumulation_surface);
        SDL_LockSurface(btndef_info.image_surface);
        ONSBuf* dst_buf = (ONSBuf*) accumulation_surface->pixels;
        ONSBuf* src_buf = (ONSBuf*) btndef_info.image_surface->pixels;
#ifdef BPP16
        int dst_width = accumulation_surface->pitch / 2;
        int src_width = btndef_info.image_surface->pitch / 2;
#else
        int dst_width = accumulation_surface->pitch / 4;
        int src_width = btndef_info.image_surface->pitch / 4;
#endif

        int start_y = dy, end_y = dy + dh;
        if (dh < 0) {
            start_y = dy + dh;
            end_y = dy;
        }

        if (start_y < 0) start_y = 0;

        if (end_y > screen_height) end_y = screen_height;

        int start_x = dx, end_x = dx + dw;
        if (dw < 0) {
            start_x = dx + dw;
            end_x = dx;
        }

        if (start_x < 0) start_x = 0;

        if (end_x >= screen_width) end_x = screen_width;

        dst_buf += start_y * dst_width;
        for (int i = start_y; i < end_y; i++) {
            int y = sy + sh * (i - dy) / dh;
            for (int j = start_x; j < end_x; j++) {
                int x = sx + sw * (j - dx) / dw;
                if (x < 0 || x >= btndef_info.image_surface->w
                    || y < 0 || y >= btndef_info.image_surface->h)
                    *(dst_buf + j) = 0;
                else
                    *(dst_buf + j) = *(src_buf + y * src_width + x);
            }

            dst_buf += dst_width;
        }

        SDL_UnlockSurface(btndef_info.image_surface);
        SDL_UnlockSurface(accumulation_surface);

        SDL_Rect dst_rect = { start_x, start_y, end_x - start_x, end_y - start_y };
        flushDirect((SDL_Rect &)dst_rect, REFRESH_NONE_MODE);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::bgcopyCommand(const string& cmd)
{
    SDL_BlitSurface(screen_surface, NULL, accumulation_surface, NULL);
    bg_effect_image = BG_EFFECT_IMAGE;

    bg_info.num_of_cells = 1;
    bg_info.trans_mode = AnimationInfo::TRANS_COPY;
    bg_info.pos.x = 0;
    bg_info.pos.y = 0;
    bg_info.copySurface(accumulation_surface, NULL);

    return RET_CONTINUE;
}


int PonscripterLabel::bgCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    const char* buf;
    if (script_h.compareString("white")) {
        buf = "white";
        script_h.readLabel();
    }
    else if (script_h.compareString("black")) {
        buf = "black";
        script_h.readLabel();
    }
    else {
        buf = script_h.readStr();
        bg_info.file_name = buf;
    }

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), &bg_info, bg_effect_image);
    }
    else {
        for (int i = 0; i < 3; i++)
            tachi_info[i].remove();

        bg_info.remove();
        bg_info.file_name = buf;

        createBackground();
        dirty_rect.fill(screen_width, screen_height);

        return setEffect(parseEffect(true));
    }
}


int PonscripterLabel::barclearCommand(const string& cmd)
{
    for (int i = 0; i < MAX_PARAM_NUM; i++) {
        if (bar_info[i]) {
            dirty_rect.add(bar_info[i]->pos);
            delete bar_info[i];
            bar_info[i] = NULL;
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::barCommand(const string& cmd)
{
    int no = script_h.readInt();
    if (bar_info[no]) {
        dirty_rect.add(bar_info[no]->pos);
        bar_info[no]->remove();
    }
    else {
        bar_info[no] = new AnimationInfo();
    }

    bar_info[no]->trans_mode   = AnimationInfo::TRANS_COPY;
    bar_info[no]->num_of_cells = 1;
    bar_info[no]->setCell(0);

    bar_info[no]->param = script_h.readInt();
    bar_info[no]->pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    bar_info[no]->max_width = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.h = script_h.readInt() * screen_ratio1 / screen_ratio2;
    bar_info[no]->max_param = script_h.readInt();

    bar_info[no]->color = readColour(script_h.readStr());

    int w = bar_info[no]->max_width * bar_info[no]->param / bar_info[no]->max_param;
    if (bar_info[no]->max_width > 0 && w > 0) {
        bar_info[no]->pos.w = w;
        bar_info[no]->allocImage(bar_info[no]->pos.w, bar_info[no]->pos.h);
        bar_info[no]->fill(bar_info[no]->color, 0xff);
        dirty_rect.add(bar_info[no]->pos);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::aviCommand(const string& cmd)
{
    string name = script_h.readStr();
    bool click_flag = script_h.readInt() == 1;

    stopBGM(false);
    playAVI(name.c_str(), click_flag);

    return RET_CONTINUE;
}


int PonscripterLabel::automode_timeCommand(const string& cmd)
{
    automode_time = script_h.readInt();

    return RET_CONTINUE;
}


int PonscripterLabel::autoclickCommand(const string& cmd)
{
    autoclick_time = script_h.readInt();

    return RET_CONTINUE;
}


int PonscripterLabel::amspCommand(const string& cmd)
{
    int no = script_h.readInt();
    dirty_rect.add(sprite_info[no].pos);
    sprite_info[no].pos.x = script_h.readInt() * screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y = script_h.readInt() * screen_ratio1 / screen_ratio2;

    if (script_h.getEndStatus() & ScriptHandler::END_COMMA)
        sprite_info[no].trans = script_h.readInt();

    if (sprite_info[no].trans > 256) sprite_info[no].trans = 256;
    else if (sprite_info[no].trans < 0) sprite_info[no].trans = 0;

    dirty_rect.add(sprite_info[no].pos);

    return RET_CONTINUE;
}


int PonscripterLabel::allspresumeCommand(const string& cmd)
{
    all_sprite_hide_flag = false;
    for (int i = 0; i < MAX_SPRITE_NUM; i++) {
        if (sprite_info[i].visible)
            dirty_rect.add(sprite_info[i].pos);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::allsphideCommand(const string& cmd)
{
    all_sprite_hide_flag = true;
    for (int i = 0; i < MAX_SPRITE_NUM; i++) {
        if (sprite_info[i].visible)
            dirty_rect.add(sprite_info[i].pos);
    }

    return RET_CONTINUE;
}
