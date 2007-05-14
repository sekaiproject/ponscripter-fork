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

    wave_file_name = script_h.readStrValue();
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
    startTimer(script_h.readIntValue() + internal_timer - SDL_GetTicks());
    return RET_WAIT;
}


int PonscripterLabel::waitCommand(const string& cmd)
{
    int count = script_h.readIntValue();
    if (skip_flag || draw_one_page_flag || ctrl_pressed_status || skip_to_wait)
	return RET_CONTINUE;
    startTimer(count);
    return RET_WAIT;
}


int PonscripterLabel::vspCommand(const string& cmd)
{
    int no = script_h.readIntValue();
    sprite_info[no].visible = script_h.readIntValue() == 1;
    dirty_rect.add(sprite_info[no].pos);
    return RET_CONTINUE;
}


int PonscripterLabel::voicevolCommand(const string& cmd)
{
    voice_volume = script_h.readIntValue();
    if (wave_sample[0]) Mix_Volume(0, se_volume * 128 / 100);
    return RET_CONTINUE;
}


int PonscripterLabel::vCommand(const string& cmd)
{
    string lcmd = cmd;
    lcmd.shift();
    playSound("wav" DELIMITER + lcmd, SOUND_WAVE | SOUND_OGG, false,
	      MIX_WAVE_CHANNEL);
    return RET_CONTINUE;
}


int PonscripterLabel::trapCommand(const string& cmd)
{
    if (cmd == "lr_trap")
        trap_mode = TRAP_LEFT_CLICK | TRAP_RIGHT_CLICK;
    else if (cmd == "trap")
        trap_mode = TRAP_LEFT_CLICK;

    Expression e = script_h.readStrExpr();
    if (e.is_bareword("off"))
        trap_mode = TRAP_NONE;
    else if (e.is_label())
        trap_dist = e.as_string();
    else
        printf("%s: [%s] is not supported\n", cmd.c_str(),
	       e.debug_string().c_str());

    return RET_CONTINUE;
}


int PonscripterLabel::textspeedCommand(const string& cmd)
{
    sentence_font.wait_time = script_h.readIntValue();
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
    sentence_font.setTateYoko(script_h.readIntValue());
    return RET_CONTINUE;
}


int PonscripterLabel::talCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    Expression loc = script_h.readStrExpr();
    
    int  no  = -1, trans = 0;
    if (loc.is_bareword("l")) no = 0;
    else if (loc.is_bareword("c")) no = 1;
    else if (loc.is_bareword("r")) no = 2;

    if (no >= 0) {
        trans = script_h.readIntValue();
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
    int no = script_h.readIntValue();
    while (script_h.hasMoreArgs()) {
        string buf = script_h.readStrValue();
        if (count++ == no) {
	    setCurrentLabel(buf);
	    break;
	}
    }
    return RET_CONTINUE;
}


int PonscripterLabel::systemcallCommand(const string& cmd)
{
    system_menu_mode = getSystemCallNo(script_h.readStrValue());
    enterSystemCall();
    advancePhase();
    return RET_WAIT;
}


int PonscripterLabel::strspCommand(const string& cmd)
{
    int sprite_no = script_h.readIntValue();
    AnimationInfo* ai = &sprite_info[sprite_no];
    ai->removeTag();
    ai->file_name = script_h.readStrValue();

    FontInfo fi;
    fi.is_newline_accepted = true;
    ai->pos.x = script_h.readIntValue();
    ai->pos.y = script_h.readIntValue();
    fi.area_x = script_h.readIntValue();
    fi.area_y = script_h.readIntValue();
    int s1 = script_h.readIntValue(), s2 = script_h.readIntValue();
    fi.set_size(s1 > s2 ? s1 : s2);
    fi.set_mod_size(0);
    fi.pitch_x   = script_h.readIntValue();
    fi.pitch_y   = script_h.readIntValue();
    fi.is_bold   = script_h.readIntValue();
    fi.is_shadow = script_h.readIntValue();

    if (script_h.hasMoreArgs()) {
	ai->color_list.clear();
	while (script_h.hasMoreArgs())
	    ai->color_list.push_back(readColour(script_h.readStrValue()));
	ai->num_of_cells = ai->color_list.size();
    }
    else {
	ai->num_of_cells = 1;
	ai->color_list.assign(1, rgb_t(0xff));
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
    int no = script_h.readIntValue();
    int upper_r  = script_h.readIntValue();
    int upper_g  = script_h.readIntValue();
    int upper_b  = script_h.readIntValue();
    int lower_r  = script_h.readIntValue();
    int lower_g  = script_h.readIntValue();
    int lower_b  = script_h.readIntValue();
    ONSBuf key_r = script_h.readIntValue();
    ONSBuf key_g = script_h.readIntValue();
    ONSBuf key_b = script_h.readIntValue();
    Uint32 alpha = script_h.readIntValue();

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
    decodeExbtnControl(script_h.readStrValue());
    return RET_CONTINUE;
}


int PonscripterLabel::spreloadCommand(const string& cmd)
{
    int no = script_h.readIntValue();
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
    string buf = script_h.readStrValue();
    string delimiter = script_h.readStrValue();
    delimiter.erase(encoding->CharacterBytes(delimiter.c_str()));
    string::vector parts = buf.split(delimiter);
    string::vector::const_iterator it = parts.begin();
    while (script_h.hasMoreArgs()) {
	Expression e = script_h.readExpr();
	if (e.is_numeric())
	    e.mutate(it == parts.end() ? 0 : atoi(it->c_str()));
	else
	    e.mutate(it == parts.end() ? "" : *it);
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

    int sprite_no = script_h.readIntValue();
    int no = script_h.readIntValue();

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
    se_volume = script_h.readIntValue();

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
    sentence_font.top_x  = script_h.readIntValue();
    sentence_font.top_y  = script_h.readIntValue();
    sentence_font.area_x = script_h.readIntValue();
    sentence_font.area_y = script_h.readIntValue();
    int s1 = script_h.readIntValue(), s2 = script_h.readIntValue();
    sentence_font.set_size(s1 > s2 ? s1 : s2);
    sentence_font.set_mod_size(0);
    sentence_font.pitch_x   = script_h.readIntValue();
    sentence_font.pitch_y   = script_h.readIntValue();
    sentence_font.wait_time = script_h.readIntValue();
    sentence_font.is_bold   = script_h.readIntValue();
    sentence_font.is_shadow = script_h.readIntValue();

    // Handle NScripter games: window size defined in characters
    if (!script_h.is_ponscripter) {
	sentence_font.area_x *= s1 + sentence_font.pitch_x;
	sentence_font.area_y *= s2 + sentence_font.pitch_y;
    }

    string back = script_h.readStrValue();
    dirty_rect.add(sentence_font_info.pos);
    float r = float(screen_ratio1) / float(screen_ratio2);
    if (back[0] == '#') {
        sentence_font.is_transparent = true;
        sentence_font.window_color = readColour(back);
        sentence_font_info.pos.x = int(script_h.readIntValue() * r);
        sentence_font_info.pos.y = int(script_h.readIntValue() * r);
        sentence_font_info.pos.w = int(script_h.readIntValue() * r);
        sentence_font_info.pos.h = int(script_h.readIntValue() * r);
    }
    else {
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName(back);
        parseTaggedString(&sentence_font_info);
        setupAnimationInfo(&sentence_font_info);
        sentence_font_info.pos.x = int(script_h.readIntValue() * r);
	sentence_font_info.pos.y = int(script_h.readIntValue() * r);
#if 0
        if (sentence_font_info.image_surface) {
            sentence_font_info.pos.w = int(sentence_font_info.image_surface->w * r);
            sentence_font_info.pos.h = int(sentence_font_info.image_surface->h * r);
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
    string back = script_h.readStrValue();
    if (back[0] == '#') {
        sentence_font.is_transparent = true;
        sentence_font.window_color = readColour(back);
    }
    else {
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName(back);
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
    int no = script_h.readIntValue();
    string cur = script_h.readStrValue();
    int x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

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
            gosubReal(select_links[button].label, select_label_next_script);
        }
        else { // selnum
	    script_h.readIntExpr().mutate(button);
            current_label_info = script_h.getLabelByAddress(select_label_next_script);
            current_line = script_h.getLineByAddress(select_label_next_script);
            script_h.setCurrent(select_label_next_script);
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
	else if (select_mode == SELECT_NUM_MODE) {
	    script_h.readIntExpr();
	}
	
        shortcut_mouse_line = buttons.end();

        float old_x = sentence_font.GetXOffset();
        int   old_y = sentence_font.GetYOffset();

	playSound(selectvoice_file_name[SELECTVOICE_OPEN],
		  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

        select_links.clear();

        while (1) {
            if (script_h.getNext()[0] != 0x0a && comma_flag == true) {
                string text = script_h.readStrValue();
                comma_flag = script_h.hasMoreArgs();
                if (select_mode != SELECT_NUM_MODE && !comma_flag)
		    errorAndExit(cmd + ": comma is needed here.");

		string label;
                if (select_mode != SELECT_NUM_MODE)
                    label = script_h.readStrValue();

		select_links.push_back(SelectElt(text, label));
		
                comma_flag = script_h.hasMoreArgs();
            }
            else if (script_h.getNext()[0] == 0x0a) {
                const char* buf = script_h.getNext() + 1; // consume eol
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
                    select_label_next_script = buf;
                    break;
                }

                comma_flag = true;
            }
            else { // if select ends at the middle of the line
                select_label_next_script = script_h.getNext();
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
    SaveFileInfo info;
    searchSaveFile(info, script_h.readIntValue());
    if (!info.valid) {
	script_h.readIntExpr().mutate(0);
        for (int i = 0; i < 3; i++)
            script_h.readIntExpr();

        return RET_CONTINUE;
    }
    script_h.readIntExpr().mutate(info.month);
    script_h.readIntExpr().mutate(info.day);
    script_h.readIntExpr().mutate(info.hour);
    script_h.readIntExpr().mutate(info.minute);
    return RET_CONTINUE;
}


int PonscripterLabel::savescreenshotCommand(const string& cmd)
{
    string filename = script_h.readStrValue();
    string ext = filename.substr(filename.rfind('.'));
    ext.lowercase();
    if (ext == ".bmp") {
	filename = archive_path + filename;
	filename.replace('/', DELIMITER[0]);
	filename.replace('\\', DELIMITER[0]);	
        SDL_SaveBMP(screenshot_surface, filename.c_str());
    }
    else
        printf("%s: %s files are not supported.\n", cmd.c_str(), ext.c_str());

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
    int no = script_h.readIntValue();
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
    Expression e = script_h.readIntExpr();
    SaveFileInfo info;
    searchSaveFile(info, script_h.readIntValue());
    e.mutate(info.valid);
    return RET_CONTINUE;
}


int PonscripterLabel::rndCommand(const string& cmd)
{
    int upper, lower;
    Expression e = script_h.readIntExpr();
    if (cmd == "rnd2") {
        lower = script_h.readIntValue();
        upper = script_h.readIntValue();
    }
    else {
        lower = 0;
        upper = script_h.readIntValue() - 1;
    }
    e.mutate(lower + int(double(upper - lower + 1) * rand() /
			 (RAND_MAX + 1.0)));
    return RET_CONTINUE;
}


int PonscripterLabel::rmodeCommand(const string& cmd)
{
    rmode_flag = script_h.readIntValue() == 1;
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

    tmp_effect.no = script_h.readIntValue();
    tmp_effect.duration = script_h.readIntValue();
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

    string s = script_h.readStrValue() + "\n";
    if (s[0] == encoding->TextMarker()) s.shift();

    // FIXME: processText() should take the text to process as a parameter
    script_h.getStringBuffer() = s;
    string_buffer_offset = 0;
    ret = processText();
    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a) {
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
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
    int no = script_h.readIntValue();
    if (prnum_info[no]) {
        dirty_rect.add(prnum_info[no]->pos);
        delete prnum_info[no];
    }

    prnum_info[no] = new AnimationInfo();
    prnum_info[no]->trans_mode   = AnimationInfo::TRANS_STRING;
    prnum_info[no]->num_of_cells = 1;
    prnum_info[no]->setCell(0);
    prnum_info[no]->color_list.resize(1);

    prnum_info[no]->param = script_h.readIntValue();
    prnum_info[no]->pos.x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->pos.y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    prnum_info[no]->font_size_x = script_h.readIntValue();
    prnum_info[no]->font_size_y = script_h.readIntValue();

    prnum_info[no]->color_list[0] = readColour(script_h.readStrValue());

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
    string buf = script_h.readStrValue();
    if (buf[0] == '*') {
	cd_play_loop_flag = cmd != "playonce";
	buf.shift();
        int new_cd_track = atoi(buf.c_str());
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
        midi_play_loop_flag = cmd != "playonce";
        if (playSound(midi_file_name, SOUND_MIDI,
		      midi_play_loop_flag) != SOUND_MIDI)
            fprintf(stderr, "can't play MIDI file %s\n",
		    midi_file_name.c_str());
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
    nega_mode = script_h.readIntValue();
    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());
    return RET_CONTINUE;
}


int PonscripterLabel::mspCommand(const string& cmd)
{
    int no = script_h.readIntValue();
    dirty_rect.add(sprite_info[no].pos);
    sprite_info[no].pos.x += script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y += script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    dirty_rect.add(sprite_info[no].pos);
    if (script_h.hasMoreArgs())
        sprite_info[no].trans += script_h.readIntValue();

    if (sprite_info[no].trans > 256) sprite_info[no].trans = 256;
    else if (sprite_info[no].trans < 0) sprite_info[no].trans = 0;

    return RET_CONTINUE;
}


int PonscripterLabel::mpegplayCommand(const string& cmd)
{
    string name = script_h.readStrValue();
    stopBGM(false);
    if (playMPEG(name, script_h.readIntValue() == 1)) endCommand("end");
    return RET_CONTINUE;
}


int PonscripterLabel::mp3volCommand(const string& cmd)
{
    music_volume = script_h.readIntValue();

    if (mp3_sample)
	SMPEG_setvolume(mp3_sample, music_volume);

    return RET_CONTINUE;
}


int PonscripterLabel::mp3fadeoutCommand(const string& cmd)
{
    mp3fadeout_start    = SDL_GetTicks();
    mp3fadeout_duration = script_h.readIntValue();

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

    music_file_name = script_h.readStrValue();
    playSound(music_file_name,
	      SOUND_WAVE | SOUND_OGG_STREAMING | SOUND_MP3 | SOUND_MIDI,
	      music_play_loop_flag, MIX_BGM_CHANNEL);

    return RET_CONTINUE;
}


int PonscripterLabel::movemousecursorCommand(const string& cmd)
{
    int x = script_h.readIntValue();
    int y = script_h.readIntValue();

    SDL_WarpMouse(x, y);

    return RET_CONTINUE;
}


int PonscripterLabel::monocroCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    if (e.is_bareword("off")) {
        monocro_flag = false;
    }
    else {
        monocro_flag = true;
        monocro_color = readColour(e.as_string());
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
    int no = script_h.readIntValue();
    if (sprite_info[no].visible)
        dirty_rect.add(sprite_info[no].pos);

    sprite_info[no].visible = cmd != "lsph";

    sprite_info[no].setImageName(script_h.readStrValue());

    sprite_info[no].pos.x = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    if (script_h.hasMoreArgs())
        sprite_info[no].trans = script_h.readIntValue();
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
    loop_bgm_name[0] = script_h.readStrValue();
    loop_bgm_name[1] = script_h.readStrValue();    

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
        lookback_info[i].image_name = script_h.readStrValue();
        parseTaggedString(&lookback_info[i]);
        setupAnimationInfo(&lookback_info[i]);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::logspCommand(const string& cmd)
{
    int sprite_no = script_h.readIntValue();

    AnimationInfo &si = sprite_info[sprite_no];
    if (si.visible) dirty_rect.add(si.pos);

    si.remove();
    si.file_name = script_h.readStrValue();

    si.pos.x = script_h.readIntValue();
    si.pos.y = script_h.readIntValue();

    si.trans_mode = AnimationInfo::TRANS_STRING;
    if (cmd == "logsp2") {
        si.font_size_x = script_h.readIntValue();
        si.font_size_y = script_h.readIntValue();
        si.font_pitch  = script_h.readIntValue() + si.font_size_x;
        script_h.readIntValue(); // dummy read for y pitch
    }
    else {
        si.font_size_x = sentence_font.size();
        si.font_size_y = sentence_font.size();
        si.font_pitch  = sentence_font.pitch_x;
    }

    if (script_h.hasMoreArgs()) {
	si.color_list.clear();
	while (script_h.hasMoreArgs())
	    si.color_list.push_back(readColour(script_h.readStrValue()));
    }
    else {
        si.color_list.assign(1, rgb_t(0xff));
    }
    si.num_of_cells = si.color_list.size();

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
    int x = script_h.readIntValue();
    int y = script_h.readIntValue();
    if (!script_h.is_ponscripter) {
	x *= sentence_font.size() + sentence_font.pitch_x;
	y *= sentence_font.line_space() + sentence_font.pitch_y;
    }
    sentence_font.SetXY(x, y);
    return RET_CONTINUE;
}


int PonscripterLabel::loadgameCommand(const string& cmd)
{
    int no = script_h.readIntValue();

    if (no < 0)
        errorAndExit("loadgame: no < 0.");

    if (loadSaveFile(no) != 0) {
	// failed
	return RET_CONTINUE;
    }
    else {
	// succeeded
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

    Expression loc = script_h.readStrExpr();
    int no = -1;
    if (loc.is_bareword("l")) no = 0;
    else if (loc.is_bareword("c")) no = 1;
    else if (loc.is_bareword("r")) no = 2;

    string buf;
    if (no >= 0) buf = script_h.readStrValue();

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
    const char* buf = script_h.getNext();
    while (*buf != '\0' && *buf != '~') buf++;
    if (*buf == '~') buf++;

    script_h.setCurrent(buf);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    return RET_CONTINUE;
}


int PonscripterLabel::jumpbCommand(const string& cmd)
{
    script_h.setCurrent(last_tilde);
    current_label_info = script_h.getLabelByAddress(last_tilde);
    current_line = script_h.getLineByAddress(last_tilde);

    return RET_CONTINUE;
}


int PonscripterLabel::ispageCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(textgosub_clickstr_state == CLICK_NEWPAGE);
    return RET_CONTINUE;
}


int PonscripterLabel::isfullCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(fullscreen_mode);
    return RET_CONTINUE;
}


int PonscripterLabel::isskipCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(automode_flag         ? 2 :
				  (skip_flag            ? 1 :
				   (ctrl_pressed_status ? 3 : 
				                          0)));
    return RET_CONTINUE;
}


int PonscripterLabel::isdownCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(current_button_state.down_flag);
    return RET_CONTINUE;
}


int PonscripterLabel::inputCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();

    script_h.readStrValue(); // description

    e.mutate(script_h.readStrValue());
    printf("%s: %s is set to the default value, %s\n", cmd.c_str(),
	   e.debug_string().c_str(), e.as_string().c_str());

    script_h.readIntValue(); // maxlen
    script_h.readIntValue(); // widechar flag
    if (script_h.hasMoreArgs()) {
        script_h.readIntValue(); // window width
        script_h.readIntValue(); // window height
        script_h.readIntValue(); // text box width
        script_h.readIntValue(); // text box height
    }

    return RET_CONTINUE;
}


int PonscripterLabel::indentCommand(const string& cmd)
{
    indent_offset = script_h.readIntValue();
    fprintf(stderr, " warning: [indent] command is broken\n");
    return RET_CONTINUE;
}


int PonscripterLabel::humanorderCommand(const string& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    string buf = script_h.readStrValue();
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
    script_h.readIntExpr().mutate(voice_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getversionCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(NSC_VERSION);
    return RET_CONTINUE;
}


int PonscripterLabel::gettimerCommand(const string& cmd)
{
    if (cmd == "gettimer")
	script_h.readIntExpr().mutate(SDL_GetTicks() - internal_timer);
    else
	script_h.readIntExpr().mutate(btnwait_time);	

    return RET_CONTINUE;
}


int PonscripterLabel::gettextCommand(const string& cmd)
{
    string buf;
    remove_copy(current_text_buffer->contents.begin(),
		current_text_buffer->contents.end(),
		buf.begin(),
		0x0a);
    script_h.readStrExpr().mutate(buf);
    return RET_CONTINUE;
}


int PonscripterLabel::gettagCommand(const string& cmd)
{
    if (nest_infos.empty() || nest_infos.back().nest_mode != NestInfo::LABEL)
        errorAndExit("gettag: not in a subroutine, e.g. pretextgosub");

    bool end_flag = false;
    const char* buf = nest_infos.back().next_script;
    while (*buf == ' ' || *buf == '\t') buf++;
    if (zenkakko_flag && encoding->Decode(buf) == 0x3010 /*y */)
        buf += encoding->CharacterBytes(buf);
    else if (*buf == '[')
        buf++;
    else
        end_flag = true;

    bool more_args;
    do {
	Expression e = script_h.readExpr();
        more_args = script_h.hasMoreArgs();

	if (e.is_numeric())
	    e.mutate(end_flag ? 0 : script_h.parseInt(&buf));
	else if (end_flag)
	    e.mutate("");
	else {
	    const char* buf_start = buf;
	    while (*buf != '/' &&
		   (!zenkakko_flag || encoding->Decode(buf) != 0x3011) &&
		   *buf != ']')
		buf += encoding->CharacterBytes(buf);
	    e.mutate(string(buf_start, buf - buf_start));
	}
        if (*buf == '/')
            buf++;
        else
            end_flag = true;
    }
    while (more_args);

    if (zenkakko_flag && encoding->Decode(buf) == 0x3010 /*y */)
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
    int no = script_h.readIntValue();

    script_h.readIntExpr().mutate(sprite_info[no].pos.w *
				  screen_ratio2 / screen_ratio1);
    script_h.readIntExpr().mutate(sprite_info[no].pos.h *
				  screen_ratio2 / screen_ratio1);
    if (script_h.hasMoreArgs())
        script_h.readIntExpr().mutate(sprite_info[no].num_of_cells);

    return RET_CONTINUE;
}


int PonscripterLabel::getspmodeCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(sprite_info[script_h.readIntValue()].visible);
    return RET_CONTINUE;
}


int PonscripterLabel::getsevolCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(se_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getscreenshotCommand(const string& cmd)
{
    int w = script_h.readIntValue();
    int h = script_h.readIntValue();
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    if (screenshot_surface &&
        screenshot_surface->w != w &&
        screenshot_surface->h != h) {
        SDL_FreeSurface(screenshot_surface);
        screenshot_surface = NULL;
    }

    if (screenshot_surface == NULL)
        screenshot_surface =
	    SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00ff0000,
				 0x0000ff00, 0x000000ff, 0xff000000);

    SDL_Surface* surface =
	SDL_ConvertSurface(screen_surface,image_surface->format, SDL_SWSURFACE);

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
    Expression e = script_h.readExpr();
    if (e.is_numeric()) e.mutate(getret_int); else e.mutate(getret_str);
    return RET_CONTINUE;
}


int PonscripterLabel::getregCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();

    string path = "[" + script_h.readStrValue() + "]";
    string key = script_h.readStrValue();

    FILE* fp;
    if ((fp = fopen(registry_file.c_str(), "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n", registry_file.c_str());
        return RET_CONTINUE;
    }
    string reg_buf;
    while (!feof(fp)) {
	fp >> reg_buf;
	if (reg_buf == path) {
	    while (!feof(fp)) {
		fp >> reg_buf;
		string::iterator s, f;
		s = reg_buf.begin();
		while (s != reg_buf.end() && *s != '"') ++s;
		if (s == reg_buf.end()) continue;
		f = ++s;
		while (f != reg_buf.end() && *f != '"') f += 1 + *f == '\\';
		if (f == reg_buf.end() || string(s, f) != key) continue;
		s = ++f;
		while (s != reg_buf.end() && *s != '"') ++s;
		if (s == reg_buf.end()) continue;
		f = ++s;
		while (f != reg_buf.end() && *f != '"') f += 1 + *f == '\\';
		e.mutate(string(s, f));
		fclose(fp);
		return RET_CONTINUE;
	    }
	}
    }
    fprintf(stderr, "Registry key %s\\%s not found.\n",
	    path.c_str(), key.c_str());
    fclose(fp);
    return RET_CONTINUE;
}


int PonscripterLabel::getmp3volCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(music_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getmouseposCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(current_button_state.x *
				  screen_ratio2 / screen_ratio1);
    script_h.readIntExpr().mutate(current_button_state.y *
				  screen_ratio2 / screen_ratio1);
    return RET_CONTINUE;
}


int PonscripterLabel::getlogCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    int page_no = script_h.readIntValue();
    TextBuffer* t_buf = current_text_buffer;
    while (t_buf != start_text_buffer && page_no > 0) {
        page_no--;
        t_buf = t_buf->previous;
    }
    e.mutate(page_no > 0 ? "" : t_buf->contents);
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
    if (!force_button_shortcut_flag) getenter_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getcursorposCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(int(floor(sentence_font.GetX())));
    script_h.readIntExpr().mutate(sentence_font.GetY());
    return RET_CONTINUE;
}


int PonscripterLabel::getcursorCommand(const string& cmd)
{
    if (!force_button_shortcut_flag) getcursor_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getcselstrCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    int csel_no = script_h.readIntValue();
    if (csel_no >= int(select_links.size()))
	errorAndExit("getcselstr: no select link");
    e.mutate(select_links[csel_no].text);
    return RET_CONTINUE;
}


int PonscripterLabel::getcselnumCommand(const string& cmd)
{
    script_h.readIntExpr().mutate(int(select_links.size()));
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
    Expression e = script_h.readIntExpr();
    e.mutate(ScriptHandler::cBR->getFileLength(script_h.readStrValue()) > 0);
    return RET_CONTINUE;
}


int PonscripterLabel::exec_dllCommand(const string& cmd)
{
    string dll_name = "[" + script_h.readStrValue().split("/", 2).at(0) + "]";

    FILE* fp;
    if ((fp = fopen(dll_file.c_str(), "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n", dll_file.c_str());
        return RET_CONTINUE;
    }

    string dll_buf;
    while (!feof(fp)) {
	fp >> dll_buf;
	dll_buf.trim();
	if (dll_buf == dll_name) {
	    getret_str.clear();
	    getret_int = 0;
	    while (!feof(fp)) {
		fp >> dll_buf;
		dll_buf.ltrim();
		if (dll_buf[0] == '[') break;
		string::vector parts = dll_buf.split("=", 2);
		parts[0].trim();
		parts[1].trim();
		if (parts[0] == "str") {
		    if (parts[1][0] == '"') parts[1].shift();
		    if (parts[1].back() == '"') parts[1].pop();
		    getret_str = parts[1];
		}
		else if (parts[0] == "ret") {
		    getret_int = atoi(parts[1].c_str());
		}
	    }
	    fclose(fp);
	    return RET_CONTINUE;
	}
    }
    fprintf(stderr, "The DLL is not found in %s.\n", dll_file.c_str());
    fclose(fp);
    return RET_CONTINUE;
}


int PonscripterLabel::exbtnCommand(const string& cmd)
{
    int sprite_no = -1, no = 0;
    ButtonElt* button = &exbtn_d_button;

    if (cmd != "exbtn_d") {
        bool cellcheck_flag = cmd == "cellcheckexbtn";
        sprite_no = script_h.readIntValue();
        no = script_h.readIntValue();
        if (cellcheck_flag && (sprite_info[sprite_no].num_of_cells < 2) ||
	    !cellcheck_flag && (sprite_info[sprite_no].num_of_cells == 0)) {
            script_h.readStrValue();
            return RET_CONTINUE;
        }
	button = &buttons[no];
    }

    button->button_type = ButtonElt::EX_SPRITE_BUTTON;
    button->sprite_no = sprite_no;
    button->exbtn_ctl = script_h.readStrValue();

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
    erase_text_window_mode = script_h.readIntValue();
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
    int ch = script_h.readIntValue();
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

    int ch = script_h.readIntValue();
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS - 1;

    if (play_mode == WAVE_PLAY_LOADED) {
        Mix_PlayChannel(ch, wave_sample[ch], loop_flag ? -1 : 0);
    }
    else {
        int fmt = SOUND_WAVE | SOUND_OGG;
        if (play_mode == WAVE_PRELOAD) fmt |= SOUND_PRELOAD;
        playSound(script_h.readStrValue(), fmt, loop_flag, ch);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::dvCommand(const string& cmd)
{
    string lcmd = cmd;
    lcmd.shift();
    lcmd.shift();
    playSound("voice" DELIMITER + lcmd, SOUND_WAVE | SOUND_OGG, false, 0);
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
    int sprite_no = script_h.readIntValue();
    int cell_no   = script_h.readIntValue();
    int alpha     = script_h.readIntValue();
    int x         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    // |mat[0][0] mat[0][1]|
    // |mat[1][0] mat[1][1]|
    int mat[2][2];
    mat[0][0] = script_h.readIntValue();
    mat[0][1] = script_h.readIntValue();
    mat[1][0] = script_h.readIntValue();
    mat[1][1] = script_h.readIntValue();

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    si.blendOnSurface2(accumulation_surface, x, y, alpha, mat);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawsp2Command(const string& cmd)
{
    int sprite_no = script_h.readIntValue();
    int cell_no   = script_h.readIntValue();
    int alpha     = script_h.readIntValue();
    int x         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int scale_x   = script_h.readIntValue();
    int scale_y   = script_h.readIntValue();
    int rot       = script_h.readIntValue();

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
    int sprite_no = script_h.readIntValue();
    int cell_no   = script_h.readIntValue();
    int alpha     = script_h.readIntValue();
    int x         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

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
    int r = script_h.readIntValue();
    int g = script_h.readIntValue();
    int b = script_h.readIntValue();
    SDL_FillRect(accumulation_surface, NULL,
		 SDL_MapRGBA(accumulation_surface->format, r, g, b, 0xff));
    return RET_CONTINUE;
}


int PonscripterLabel::drawclearCommand(const string& cmd)
{
    SDL_FillRect(accumulation_surface, NULL,
		 SDL_MapRGBA(accumulation_surface->format, 0, 0, 0, 0xff));
    return RET_CONTINUE;
}


int PonscripterLabel::drawbgCommand(const string& cmd)
{
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    bg_info.blendOnSurface(accumulation_surface, bg_info.pos.x, bg_info.pos.y,
			   clip);
    return RET_CONTINUE;
}


int PonscripterLabel::drawbg2Command(const string& cmd)
{
    int x       = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y       = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int scale_x = script_h.readIntValue();
    int scale_y = script_h.readIntValue();
    int rot     = script_h.readIntValue();

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
    int t = script_h.readIntValue();

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
    int no = script_h.readIntValue();

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
    int csel_no = script_h.readIntValue();
    if (csel_no >= int(select_links.size()))
	errorAndExit("cselgoto: no select link");

    setCurrentLabel(select_links[csel_no].label);
    select_links.clear();
    newPage(true);

    return RET_CONTINUE;
}


int PonscripterLabel::cselbtnCommand(const string& cmd)
{
    int csel_no   = script_h.readIntValue();
    int button_no = script_h.readIntValue();

    FontInfo csel_info = sentence_font;
    csel_info.top_x = script_h.readIntValue();
    csel_info.top_y = script_h.readIntValue();

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

    char loc = script_h.readBareword()[0];

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
    int ch  = script_h.readIntValue();
    int vol = script_h.readIntValue();
    if (wave_sample[ch]) Mix_Volume(ch, vol * 128 / 100);
    return RET_CONTINUE;
}


int PonscripterLabel::checkpageCommand(const string& cmd)
{
    Expression e = script_h.readIntExpr();
    int page_no = script_h.readIntValue();
    TextBuffer* t_buf = current_text_buffer;
    while (t_buf != start_text_buffer && page_no > 0) {
        page_no--;
        t_buf = t_buf->previous;
    }
    e.mutate(page_no <= 0);
    return RET_CONTINUE;
}


int PonscripterLabel::cellCommand(const string& cmd)
{
    int sprite_no = script_h.readIntValue();
    int no = script_h.readIntValue();

    sprite_info[sprite_no].setCell(no);
    dirty_rect.add(sprite_info[sprite_no].pos);

    return RET_CONTINUE;
}


int PonscripterLabel::captionCommand(const string& cmd)
{
    string buf = script_h.readStrValue();
    SDL_WM_SetCaption(buf.c_str(), buf.c_str());
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

    Expression e = script_h.readIntExpr();

    if (event_mode & WAIT_BUTTON_MODE
        || (textbtn_flag && (skip_flag || (draw_one_page_flag && clickstr_state == CLICK_WAIT) || ctrl_pressed_status))) {
        btnwait_time  = SDL_GetTicks() - internal_button_timer;
        btntime_value = 0;
        num_chars_in_sentence = 0;

        if (textbtn_flag && (skip_flag || (draw_one_page_flag && clickstr_state == CLICK_WAIT) || ctrl_pressed_status))
            current_button_state.button = 0;

	e.mutate(current_button_state.button);

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
    btntime_value = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::btndownCommand(const string& cmd)
{
    btndown_flag = script_h.readIntValue() == 1;
    return RET_CONTINUE;
}


int PonscripterLabel::btndefCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    if (!e.is_bareword("clear")) {
        btndef_info.remove();
        if (e.as_string()) {
            btndef_info.setImageName(e.as_string());
            parseTaggedString(&btndef_info);
            btndef_info.trans_mode = AnimationInfo::TRANS_COPY;
            setupAnimationInfo(&btndef_info);
	    if (btndef_info.image_surface)
		SDL_SetAlpha(btndef_info.image_surface, DEFAULT_BLIT_FLAG,
			     SDL_ALPHA_OPAQUE);
	    else
		fprintf(stderr, "Could not create button: %s not found\n",
			e.as_string().c_str());
        }
    }
    deleteButtons();
    disableGetButtonFlag();
    return RET_CONTINUE;
}


int PonscripterLabel::btnCommand(const string& cmd)
{
    SDL_Rect src_rect;

    const int no = script_h.readIntValue();

    ButtonElt* button = &buttons[no];
    
    button->image_rect.x = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    button->image_rect.y = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    button->image_rect.w = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    button->image_rect.h = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    button->select_rect = button->image_rect;

    src_rect.x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    src_rect.y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
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
    int delta = cmd == "br2" ? script_h.readIntValue()
	                     : (script_h.is_ponscripter ? 50 : 100);

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

    dx = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    dy = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    dw = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    dh = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    sx = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    sy = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    sw = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    sh = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

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

    Expression e = script_h.readStrExpr();
    if (!(e.is_bareword("white") || e.is_bareword("black")))
        bg_info.file_name = e.as_string();

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false), &bg_info, bg_effect_image);
    }
    else {
        for (int i = 0; i < 3; i++)
            tachi_info[i].remove();

        bg_info.remove();
        bg_info.file_name = e.as_string();

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
    int no = script_h.readIntValue();
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

    bar_info[no]->param = script_h.readIntValue();
    bar_info[no]->pos.x = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.y = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;

    bar_info[no]->max_width = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    bar_info[no]->pos.h = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    bar_info[no]->max_param = script_h.readIntValue();

    bar_info[no]->color = readColour(script_h.readStrValue());

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
    string name = script_h.readStrValue();
    stopBGM(false);
    playAVI(name, script_h.readIntValue() == 1);
    return RET_CONTINUE;
}


int PonscripterLabel::automode_timeCommand(const string& cmd)
{
    automode_time = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::autoclickCommand(const string& cmd)
{
    autoclick_time = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::amspCommand(const string& cmd)
{
    int no = script_h.readIntValue();
    dirty_rect.add(sprite_info[no].pos);
    sprite_info[no].pos.x = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;
    sprite_info[no].pos.y = script_h.readIntValue() *
	screen_ratio1 / screen_ratio2;

    if (script_h.hasMoreArgs())
        sprite_info[no].trans = script_h.readIntValue();

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
