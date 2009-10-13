/* -*- C++ -*-
 *
 *  PonscripterLabel_command.cpp - Command executer of Ponscripter
 *
 *  Copyright (c) 2001-2008 Ogapee (original ONScripter, of which this
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

int PonscripterLabel::waveCommand(const pstring& cmd)
{
    wavestopCommand("wavestop");

    wave_file_name = script_h.readStrValue();
    playSound(wave_file_name, SOUND_WAVE | SOUND_OGG, cmd == "waveloop",
	      MIX_WAVE_CHANNEL);

    return RET_CONTINUE;
}


int PonscripterLabel::wavestopCommand(const pstring& cmd)
{
    if (wave_sample[MIX_WAVE_CHANNEL]) {
        Mix_Pause(MIX_WAVE_CHANNEL);
        Mix_FreeChunk(wave_sample[MIX_WAVE_CHANNEL]);
        wave_sample[MIX_WAVE_CHANNEL] = NULL;
    }

    wave_file_name = "";

    return RET_CONTINUE;
}


int PonscripterLabel::waittimerCommand(const pstring& cmd)
{
    startTimer(script_h.readIntValue() + internal_timer - SDL_GetTicks());
    return RET_WAIT;
}


int PonscripterLabel::waitCommand(const pstring& cmd)
{
    int count = script_h.readIntValue();
    if (skip_flag || draw_one_page_flag || ctrl_pressed_status)
	return RET_CONTINUE;
    if (skip_to_wait) {
	skip_to_wait = 0;
	return RET_CONTINUE;
    }	
    startTimer(count);
    return RET_WAIT;
}

int PonscripterLabel::vspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    // Haeleth extension: allow vsp <sprite1>,<sprite2>,<value> like csp
    int no1 = script_h.readIntValue();
    int no2 = no1;
    int vis = script_h.readIntValue();
    if (script_h.hasMoreArgs()) { no2 = vis; vis = script_h.readIntValue(); }
    if (no2 < no1) { int swap = no2; no2 = no1; no1 = swap; }
    if (no2 >= MAX_SPRITE_NUM) no2 = MAX_SPRITE_NUM - 1;
    if (cmd == "vsp2") {
        for (int no = no1; no <= no2; ++no){
            sprite2_info[no].visible(vis);
            dirty_rect.add(sprite2_info[no].bounding_rect);
        }
    }
    else {
        for (int no = no1; no <= no2; ++no){
            sprite_info[no].visible(vis);
            dirty_rect.add(sprite_info[no].pos);
        }
    }
    return RET_CONTINUE;
}

int PonscripterLabel::voicevolCommand(const pstring& cmd)
{
    voice_volume = script_h.readIntValue();
    if (wave_sample[0]) Mix_Volume(0, se_volume * 128 / 100);
    return RET_CONTINUE;
}


int PonscripterLabel::vCommand(const pstring& cmd)
{
    playSound("wav" DELIMITER + cmd.midstr(1, cmd.length()),
	      SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
    return RET_CONTINUE;
}


int PonscripterLabel::trapCommand(const pstring& cmd)
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
        printf("%s: [%s] is not supported\n",
	       (const char*) cmd,
	       (const char*) e.debug_string());

    return RET_CONTINUE;
}


int PonscripterLabel::textspeedCommand(const pstring& cmd)
{
    sentence_font.wait_time = script_h.readIntValue();
    return RET_CONTINUE;
}

int PonscripterLabel::gettextspeedCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(sentence_font.wait_time);
    return RET_CONTINUE;
}


int PonscripterLabel::textshowCommand(const pstring& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE
	                     | REFRESH_SHADOW_MODE
	                     | REFRESH_TEXT_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::textonCommand(const pstring& cmd)
{
    int ret = enterTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    text_on_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::textoffCommand(const pstring& cmd)
{
    refreshSurface(backup_surface, NULL, REFRESH_NORMAL_MODE);

    int ret = leaveTextDisplayMode(true);
    if (ret != RET_NOMATCH) return ret;

    text_on_flag = false;

    return RET_CONTINUE;
}


int PonscripterLabel::texthideCommand(const pstring& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE;
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::textclearCommand(const pstring& cmd)
{
    newPage(false);
    return RET_CONTINUE;
}


int PonscripterLabel::texecCommand(const pstring& cmd)
{
    if (textgosub_clickstr_state == CLICK_NEWPAGE) {
        newPage(true);
        clickstr_state = CLICK_NONE;
    }
    else if (textgosub_clickstr_state == CLICK_WAITEOL) {
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            indent_offset = 0;
            line_enter_status = 0;
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::tateyokoCommand(const pstring& cmd)
{
    sentence_font.setTateYoko(script_h.readIntValue()!=0);
    return RET_CONTINUE;
}


int PonscripterLabel::talCommand(const pstring& cmd)
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
        return doEffect(parseEffect(false));
    }
    else {
        if (no >= 0) {
            tachi_info[no].trans = trans;
            dirty_rect.add(tachi_info[no].pos);
        }

        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::tablegotoCommand(const pstring& cmd)
{
    // Haeleth extension: tablegoto1 uses 1-based indexing
    int count = cmd == "tablegoto1";
    int no = script_h.readIntValue();
    while (script_h.hasMoreArgs()) {
        pstring buf = script_h.readStrValue();
        if (count++ == no) {
	    if (cmd == "debugtablegoto")
		fprintf(stderr, "tablegoto %d -> %s\n", no, (const char*) buf);
	    setCurrentLabel(buf);
	    count = -1;
	    break;
	}
    }
    if (count > 0 && cmd == "debugtablegoto")
	fprintf(stderr, "tablegoto %d -> FAIL\n", no);
    return RET_CONTINUE;
}


int PonscripterLabel::systemcallCommand(const pstring& cmd)
{
    system_menu_mode = getSystemCallNo(script_h.readStrValue());
    enterSystemCall();
    advancePhase();
    return RET_WAIT;
}


int PonscripterLabel::strspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int sprite_no = script_h.readIntValue();
    AnimationInfo* ai = &sprite_info[sprite_no];
    ai->removeTag();
    ai->file_name = script_h.readStrValue();

    Fontinfo fi;
    fi.is_newline_accepted = true;
    ai->pos.x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    ai->pos.y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
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
    ai->visible(true);
    ai->is_single_line  = false;
    ai->is_tight_region = false;
    setupAnimationInfo(ai, &fi);
    return RET_CONTINUE;
}


int PonscripterLabel::stopCommand(const pstring& cmd)
{
    stopBGM(false);
    wavestopCommand("wavestop");

    return RET_CONTINUE;
}


int PonscripterLabel::sp_rgb_gradationCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int no       = script_h.readIntValue();
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

    if (si->showing())
        dirty_rect.add(si->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::spstrCommand(const pstring& cmd)
{
    decodeExbtnControl(script_h.readStrValue());
    return RET_CONTINUE;
}


int PonscripterLabel::spreloadCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int no = script_h.readIntValue();
    AnimationInfo* si;
    if (no == -1) si = &sentence_font_info;
    else si = &sprite_info[no];

    parseTaggedString(si);
    setupAnimationInfo(si);

    if (si->showing())
        dirty_rect.add(si->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::splitCommand(const pstring& cmd)
{
    pstring buf = script_h.readStrValue();
    pstring delimiter = script_h.readStrValue();
    delimiter.trunc(file_encoding->NextCharSize(delimiter));
    CBStringList parts = buf.splitstr(delimiter);
   
    CBStringList::const_iterator it = parts.begin();
    while (script_h.hasMoreArgs()) {
	Expression e = script_h.readExpr();
	if (e.is_numeric())
	    e.mutate(it == parts.end() ? 0 : atoi(*it));
	else
	    e.mutate(it == parts.end() ? "" : *it);
	++it;
    }
    return RET_CONTINUE;
}


int PonscripterLabel::spclclkCommand(const pstring& cmd)
{
    if (!force_button_shortcut_flag)
        spclclk_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::spbtnCommand(const pstring& cmd)
{
    // Haeleth extension: spbtn SPRITE1,SPRITE2,BTN assigns buttons
    // counting up from BTN to sprites in the range SPRITE1..SPRITE2.
    bool cellcheck_flag = cmd == "cellcheckspbtn";

    int sprite_n1 = script_h.readIntValue();
    int sprite_n2 = sprite_n1;
    int no = script_h.readIntValue();
    if (script_h.hasMoreArgs()) {
	sprite_n2 = no;
	no = script_h.readIntValue();
    }

    for (int sprite_no = sprite_n1; sprite_no <= sprite_n2; ++sprite_no, ++no) {
    
	if (cellcheck_flag) {
	    if (sprite_info[sprite_no].num_of_cells < 2) continue;
	}
	else {
	    if (sprite_info[sprite_no].num_of_cells == 0) continue;
	}

	ButtonElt button;
	button.button_type = ButtonElt::SPRITE_BUTTON;
	button.sprite_no = sprite_no;

	if (sprite_info[sprite_no].image_surface
	    || sprite_info[sprite_no].trans_mode ==
                  AnimationInfo::TRANS_STRING)
	{
	    button.image_rect = button.select_rect =
                sprite_info[sprite_no].pos;
	    sprite_info[sprite_no].visible(true);
	}

	buttons[no] = button;
    }
    
    return RET_CONTINUE;
}


int PonscripterLabel::skipoffCommand(const pstring& cmd)
{
    setSkipMode(false);

    return RET_CONTINUE;
}


int PonscripterLabel::sevolCommand(const pstring& cmd)
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


void PonscripterLabel::DoSetwindow(PonscripterLabel::WindowDef& def)
{
    sentence_font.top_x  = def.left;
    sentence_font.top_y  = def.top;
    sentence_font.area_x = def.width;
    sentence_font.area_y = def.height;
    sentence_font.set_size(def.font_size);
    sentence_font.set_mod_size(0);
    sentence_font.pitch_x   = def.pitch_x;
    sentence_font.pitch_y   = def.pitch_y;
    sentence_font.wait_time = def.speed;
    sentence_font.is_bold   = def.bold;
    sentence_font.is_shadow = def.shadow;

    dirty_rect.add(sentence_font_info.pos);
    float r = float(screen_ratio1) / float(screen_ratio2);
    if (def.backdrop[0] == '#') {
        sentence_font.is_transparent = true;
        sentence_font.window_color = readColour(def.backdrop);
        sentence_font_info.pos.x = int(def.w_left   * r);
        sentence_font_info.pos.y = int(def.w_top    * r);
        sentence_font_info.pos.w = int((def.w_right - def.w_left + 1) * r);
        sentence_font_info.pos.h = int((def.w_bottom - def.w_top + 1) * r);
    }
    else {
        sentence_font.is_transparent = false;
        sentence_font_info.setImageName(def.backdrop);
        parseTaggedString(&sentence_font_info);
        setupAnimationInfo(&sentence_font_info);
        sentence_font_info.pos.x = int(def.w_left * r);
        sentence_font_info.pos.y = int(def.w_top  * r);
#if 0
        if (sentence_font_info.image_surface) {
            sentence_font_info.pos.w = int(sentence_font_info.image_surface->w * r);
            sentence_font_info.pos.h = int(sentence_font_info.image_surface->h * r);
        }

#endif
        sentence_font.window_color.set(0xff);
    }
}


void PonscripterLabel::setwindowCore()
{
    WindowDef wind;
    wind.left      = script_h.readIntValue();
    wind.top       = script_h.readIntValue();
    wind.width     = script_h.readIntValue();
    wind.height    = script_h.readIntValue();
    int size_1     = script_h.readIntValue();
    int size_2     = script_h.readIntValue();
    wind.font_size = size_1 > size_2 ? size_1 : size_2;
    wind.pitch_x   = script_h.readIntValue();
    wind.pitch_y   = script_h.readIntValue();
    wind.speed     = script_h.readIntValue();
    wind.bold      = script_h.readIntValue();
    wind.shadow    = script_h.readIntValue();
    wind.backdrop  = script_h.readStrValue();
    wind.w_left    = script_h.readIntValue();
    wind.w_top     = script_h.readIntValue(); 
    wind.w_right   = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
    wind.w_bottom  = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
  
    // Window size is defined in characters
    // (this used to be just for non-Ponscripter games, but as of
    // 20080122 we revert to the NScripter behaviour; new code should
    // really be using h_defwindow and h_usewindow anyway!)    
    // if (!script_h.is_ponscripter) {
    wind.width  *= size_1 + wind.pitch_x;
    wind.height *= size_2 + wind.pitch_y;
    //}

    DoSetwindow(wind);
}


int PonscripterLabel::setwindow3Command(const pstring& cmd)
{
    setwindowCore();

    display_mode = NORMAL_DISPLAY_MODE;
    flush(refreshMode(), &sentence_font_info.pos);
    
    return RET_CONTINUE;
}


int PonscripterLabel::setwindow2Command(const pstring& cmd)
{
    pstring back = script_h.readStrValue();
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


int PonscripterLabel::setwindowCommand(const pstring& cmd)
{
    setwindowCore();

    lookbackflushCommand("lookbackflush");
    indent_offset = 0;
    line_enter_status = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    flush(refreshMode(), &sentence_font_info.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::setcursorCommand(const pstring& cmd)
{
    int no    = script_h.readIntValue();
    pstring c = script_h.readStrValue();
    int x     = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y     = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    loadCursor(no, c, x, y, cmd == "abssetcursor");

    return RET_CONTINUE;
}


int PonscripterLabel::selectCommand(const pstring& cmd)
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
                pstring text = script_h.readStrValue();
                comma_flag = script_h.hasMoreArgs();
                if (select_mode != SELECT_NUM_MODE && !comma_flag)
		    errorAndExit(cmd + ": comma is needed here.");

		pstring label;
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

        setSkipMode(false);
        setAutoMode(false);
        sentence_font.SetXY(old_x, old_y);

        flush(refreshMode());

        event_mode = WAIT_TEXT_MODE | WAIT_BUTTON_MODE | WAIT_TIMER_MODE;
        advancePhase();
        refreshMouseOverButton();

        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::savetimeCommand(const pstring& cmd)
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


int PonscripterLabel::savescreenshotCommand(const pstring& cmd)
{
    pstring filename = script_h.readStrValue();
    pstring ext = file_extension(filename);
    ext.toupper();
    if (ext == "BMP") {
	filename = script_h.save_path + filename;
	replace_ascii(filename, '/', DELIMITER[0]);
	replace_ascii(filename, '\\', DELIMITER[0]);
        SDL_SaveBMP(screenshot_surface, filename);
    }
    else
        printf("%s: %s files are not supported.\n",
	       (const char*) cmd, (const char*) ext);

    return RET_CONTINUE;
}


int PonscripterLabel::saveonCommand(const pstring& cmd)
{
    saveon_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::saveoffCommand(const pstring& cmd)
{
    saveon_flag = false;

    return RET_CONTINUE;
}


int PonscripterLabel::savegameCommand(const pstring& cmd)
{
    bool savegame2_flag = false;
    if (cmd == "savegame2")
        savegame2_flag = true;
    
    int no = script_h.readIntValue();
    
    pstring savestr = "";
    if (savegame2_flag)
        savestr = script_h.readStrValue();

    if (no < 0)
        errorAndExit("savegame: the specified number is less than 0.");
    else {
        shelter_event_mode = event_mode;
        saveSaveFile(no, savestr);
    }
    return RET_CONTINUE;
}


int PonscripterLabel::savefileexistCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    SaveFileInfo info;
    searchSaveFile(info, script_h.readIntValue());
    e.mutate(info.valid);
    return RET_CONTINUE;
}


int PonscripterLabel::rndCommand(const pstring& cmd)
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
    e.mutate(get_rnd(lower, upper));
    return RET_CONTINUE;
}


int PonscripterLabel::rmodeCommand(const pstring& cmd)
{
    rmode_flag = script_h.readIntValue() == 1;
    return RET_CONTINUE;
}


int PonscripterLabel::resettimerCommand(const pstring& cmd)
{
    internal_timer = SDL_GetTicks();
    return RET_CONTINUE;
}


int PonscripterLabel::resetCommand(const pstring& cmd)
{
    resetSub();
    clearCurrentTextBuffer();

    setCurrentLabel("start");
    saveSaveFile(-1);

    return RET_CONTINUE;
}


int PonscripterLabel::repaintCommand(const pstring& cmd)
{
    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());

    return RET_CONTINUE;
}


int PonscripterLabel::quakeCommand(const pstring& cmd)
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

    tmp_effect.effect = CUSTOM_EFFECT_NO + quake_type;

    if (ctrl_pressed_status || skip_to_wait) {
        dirty_rect.fill(screen_width, screen_height);
        SDL_BlitSurface(accumulation_surface, NULL, effect_dst_surface, NULL);
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }

    if (tmp_effect.duration < tmp_effect.no * 4)
        tmp_effect.duration = tmp_effect.no * 4;
    tmp_effect.effect = CUSTOM_EFFECT_NO + quake_type;

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(tmp_effect);
    }
    else {
        dirty_rect.fill(screen_width, screen_height);
        SDL_BlitSurface(accumulation_surface, NULL, effect_dst_surface, NULL);

        return setEffect(tmp_effect, false, true);
    }
}


int PonscripterLabel::puttextCommand(const pstring& cmd)
{
    int ret = enterTextDisplayMode(false);
    if (ret != RET_NOMATCH) return ret;

    pstring s = script_h.readStrValue() + "\n";
    if (s[0] == file_encoding->TextMarker()) s.remove(0, 1);

    script_h.getStrBuf() = s;
    ret = processText();
    if (script_h.readStrBuf(string_buffer_offset) == 0x0a) {
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


int PonscripterLabel::prnumclearCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    for (int i = 0; i < MAX_PARAM_NUM; i++) {
        if (prnum_info[i]) {
            dirty_rect.add(prnum_info[i]->pos);
            delete prnum_info[i];
            prnum_info[i] = NULL;
        }
    }

    return RET_CONTINUE;
}


int PonscripterLabel::prnumCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

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
//    // NScr uses fullwidth digits; somewhat fake it by shifting position
//    prnum_info[no]->pos.x += prnum_info[no]->font_size_x;

    prnum_info[no]->color_list[0] = readColour(script_h.readStrValue());

    prnum_info[no]->file_name = stringFromInteger(prnum_info[no]->param, 3);

    setupAnimationInfo(prnum_info[no]);
    dirty_rect.add(prnum_info[no]->pos);

    return RET_CONTINUE;
}


int PonscripterLabel::printCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false));
    }
    else {
        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::playstopCommand(const pstring& cmd)
{
    stopBGM(false);
    return RET_CONTINUE;
}


int PonscripterLabel::playCommand(const pstring& cmd)
{
    pstring buf = script_h.readStrValue();
    if (buf[0] == '*') {
	cd_play_loop_flag = cmd != "playonce";
	buf.remove(0, 1);
        int new_cd_track = buf;
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
		    (const char*) midi_file_name);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::ofscopyCommand(const pstring& cmd)
{
    SDL_BlitSurface(screen_surface, NULL, accumulation_surface, NULL);

    return RET_CONTINUE;
}


int PonscripterLabel::negaCommand(const pstring& cmd)
{
    nega_mode = script_h.readIntValue();
    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());
    return RET_CONTINUE;
}


int PonscripterLabel::mspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    // Haeleth extension: the form `msp NUM,NUM,NUM[,NUM]' is augmented
    // by the form `msp NUM,NUM' which modifies only the transparency.
    // Likewise `msp2 NUM,NUM,NUM,NUM,NUM,NUM[,NUM]' is augmented by
    // `msp2 NUM,NUM'.
    bool sprite2 = cmd == "msp2" || cmd == "amsp2";
    bool absolute = cmd == "amsp" || cmd == "amsp2";
    int no = script_h.readIntValue();
    int val = script_h.readIntValue();
    bool modsp2 = false;
    int x, y, a, sx, sy, r;
    AnimationInfo& si = sprite2 ? sprite2_info[no] : sprite_info[no];
    if (script_h.hasMoreArgs()) {
	dirty_rect.add(sprite2 ? si.bounding_rect : si.pos);
	x = val * screen_ratio1 / screen_ratio2;
	y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
	if (sprite2) {
	    modsp2 = true;
	    sx = script_h.readIntValue();
	    sy = script_h.readIntValue();
	    r  = script_h.readIntValue();
	}
	a = script_h.hasMoreArgs() ? script_h.readIntValue()
	                           : (absolute ? si.trans : 0);
    }
    else {
	x = absolute ? si.pos.x : 0;
	y = absolute ? si.pos.y : 0;
	a = val;
    }

    if (absolute) {
    	si.pos.x = x;
	si.pos.y = y;
	si.trans = a;
    }
    else {
	si.pos.x += x;
	si.pos.y += y;
	si.trans += a;
    }
    if (modsp2) {
	if (absolute) {
	    si.scale_x = sx;
	    si.scale_y = sy;
	    si.rot     = r;
	}
	else {
	    si.scale_x += sx;
	    si.scale_y += sy;
	    si.rot     += r;
	}
	si.calcAffineMatrix();
    }
    dirty_rect.add(sprite2 ? si.bounding_rect : si.pos);

    if (si.trans > 256) si.trans = 256;
    else if (si.trans < 0) si.trans = 0;

    return RET_CONTINUE;
}

void SubtitleDefs::add(int n, float t, pstring x)
{
    sorted = false;
    Subtitle sub;
    sub.number = n;
    sub.time = t;
    sub.text = x;
    text.push_back(sub);
}

void SubtitleDefs::define(int n, rgb_t colour, int pos, int alpha)
{
    if (subs.size() < (unsigned) n + 1) subs.resize(n + 1);
    subs[n].colour = colour;
    subs[n].pos = pos;
    subs[n].alpha = alpha;
}

float SubtitleDefs::next()
{
    if (!sorted) sort();
    if (text.empty()) return -1;
    return text.front().time;
}

Subtitle SubtitleDefs::pop()
{
    if (!sorted) sort();
    Subtitle rv;
    if (!text.empty()) {
	rv = text.front();
	text.pop_front();
    }
    return rv;
}

bool sublessthan(const Subtitle& a, const Subtitle& b)
{
    return a.time < b.time;
}

void SubtitleDefs::sort()
{
    sorted = true;
    std::sort(text.begin(), text.end(), sublessthan);
}

SubtitleDefs PonscripterLabel::parseSubtitles(pstring file)
{
    SubtitleDefs defs;
    CBStringList lines = ScriptHandler::cBR->getFile(file).split(0x0a);

    for (CBStringList::iterator it = lines.begin(); it != lines.end(); ++it) {
	it->trim();
	if (!*it || (*it)[0] == ';') continue;
	if (it->midstr(0, 7) == "define ") {
	    CBStringList e = it->midstr(7, it->length()).split(',');
	    if (e.size() > 2) {
		int alpha = e.size() > 3 ? (int) e[3] : 255;
		rgb_t col = readColour(e.size() > 2 ? e[2].trim() : "#FFFFFF");
		defs.define(e[0], col, e[1], alpha);
	    }
	    else fprintf(stderr, "Bad line in subtitle file: %s\n",
			 (const char*) *it);
	}
	else {
	    CBStringList e = it->split(',', 4);
	    if (e.size() == 4) {
		defs.add(e[2], e[0], e[3].ltrim());
		defs.add(e[2], e[1], "");
	    }
	    else fprintf(stderr, "Bad line in subtitle file: %s\n",
			 (const char*) *it);
	}
    }
    return defs;
}

int PonscripterLabel::mpegplayCommand(const pstring& cmd)
{
    pstring name = script_h.readStrValue();
    bool cancel  = script_h.readIntValue() == 1;
    SubtitleDefs subtitles;
    if (script_h.hasMoreArgs())
	subtitles = parseSubtitles(script_h.readStrValue());
    stopBGM(false);
    if (playMPEG(name, cancel, subtitles))
	endCommand("end");
    return RET_CONTINUE;
}


int PonscripterLabel::mp3volCommand(const pstring& cmd)
{
    music_volume = script_h.readIntValue();

    if (mp3_sample)
	SMPEG_setvolume(mp3_sample, music_volume);

    return RET_CONTINUE;
}


int PonscripterLabel::mp3fadeoutCommand(const pstring& cmd)
{
    mp3fadeout_start    = SDL_GetTicks();
    mp3fadeout_duration = script_h.readIntValue();

    timer_mp3fadeout_id = SDL_AddTimer(20, mp3fadeoutCallback, NULL);

    event_mode |= WAIT_TIMER_MODE;
    return RET_WAIT;
}


int PonscripterLabel::mp3Command(const pstring& cmd)
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


int PonscripterLabel::movemousecursorCommand(const pstring& cmd)
{
    int x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    SDL_WarpMouse(x, y);

    return RET_CONTINUE;
}


int PonscripterLabel::monocroCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

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


int PonscripterLabel::menu_windowCommand(const pstring& cmd)
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


int PonscripterLabel::menu_fullCommand(const pstring& cmd)
{
    if (!fullscreen_mode) {
#if !defined (PSP)
        if (!SDL_WM_ToggleFullScreen(screen_surface)) {
            SDL_FreeSurface(screen_surface);
            screen_surface = SDL_SetVideoMode(screen_width, screen_height,
	            screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG | fullscreen_flags);
            SDL_Rect rect = { 0, 0, screen_width, screen_height };
            flushDirect(rect, refreshMode());
        }
#endif
        fullscreen_mode = true;
    }

    return RET_CONTINUE;
}


int PonscripterLabel::menu_automodeCommand(const pstring& cmd)
{
    setAutoMode(true);
    printf("menu_automode: change to automode\n");

    return RET_CONTINUE;
}


int PonscripterLabel::lspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    bool sprite2 = cmd == "lsp2" || cmd == "lsph2";
    bool hidden = cmd == "lsph" || cmd == "lsph2";
    
    int no = script_h.readIntValue();
    AnimationInfo& si = sprite2 ? sprite2_info[no] : sprite_info[no];
    
    if (si.showing()) dirty_rect.add(sprite2 ? si.bounding_rect : si.pos);

    si.visible(!hidden);
    si.setImageName(script_h.readStrValue());
    si.pos.x = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    si.pos.y = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    if (sprite2) {
	si.scale_x = script_h.readIntValue();
	si.scale_y = script_h.readIntValue();
	si.rot     = script_h.readIntValue();
    }
    si.trans = script_h.hasMoreArgs() ? script_h.readIntValue() : 256;

    parseTaggedString(&si);
    setupAnimationInfo(&si);

    if (sprite2) {
	si.calcAffineMatrix();
    }
    
    if (si.showing()) dirty_rect.add(sprite2 ? si.bounding_rect : si.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::loopbgmstopCommand(const pstring& cmd)
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

    loop_bgm_name[0] = "";

    return RET_CONTINUE;
}


int PonscripterLabel::loopbgmCommand(const pstring& cmd)
{
    loop_bgm_name[0] = script_h.readStrValue();
    loop_bgm_name[1] = script_h.readStrValue();    

    playSound(loop_bgm_name[1],
        SOUND_PRELOAD | SOUND_WAVE | SOUND_OGG, false, MIX_LOOPBGM_CHANNEL1);
    playSound(loop_bgm_name[0],
        SOUND_WAVE | SOUND_OGG, false, MIX_LOOPBGM_CHANNEL0);

    return RET_CONTINUE;
}


int PonscripterLabel::lookbackflushCommand(const pstring& cmd)
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


int PonscripterLabel::lookbackbuttonCommand(const pstring& cmd)
{
    for (int i = 0; i < 4; i++) {
        lookback_info[i].image_name = script_h.readStrValue();
        parseTaggedString(&lookback_info[i]);
        setupAnimationInfo(&lookback_info[i]);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::logspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    int sprite_no = script_h.readIntValue();

    AnimationInfo &si = sprite_info[sprite_no];
    if (si.showing()) dirty_rect.add(si.pos);

    si.remove();
    si.file_name = cmd == "logsp2utf" ? "^" : "";
    si.file_name += script_h.readStrValue();

    si.pos.x = script_h.readIntValue();
    si.pos.y = script_h.readIntValue();

    si.trans_mode = AnimationInfo::TRANS_STRING;
    if (cmd == "logsp2") {
        si.font_size_x = script_h.readIntValue();
        si.font_size_y = script_h.readIntValue();
        si.font_pitch  = script_h.readIntValue() + si.font_size_x;
        script_h.readIntValue(); // dummy read for y pitch
    }
    else if (cmd == "logsp2utf") {
        si.font_size_x = script_h.readIntValue();
        si.font_size_y = script_h.readIntValue();
	si.font_pitch = script_h.readIntValue();
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
    si.visible(true);
    dirty_rect.add(si.pos);

    return RET_CONTINUE;
}


int PonscripterLabel::locateCommand(const pstring& cmd)
{
    int x = script_h.readIntValue();
    int y = script_h.readIntValue();
    // As of 20080122, pixel-oriented behaviour is provided with the
    // command name "h_locate", not with a UTF-8 script.
    //if (!script_h.is_ponscripter) {
    if (cmd == "locate") {
	x *= sentence_font.size() + sentence_font.pitch_x;
	y *= sentence_font.line_space() + sentence_font.pitch_y;
    }
    else {
	// For h_locate only, store new location in backlog.
	pstring tag;
	int phony;
	tag.format("x%d", x);
	current_text_buffer->addBuffer(file_encoding->TranslateTag(tag, phony));
	tag.format("y%d", y);
	current_text_buffer->addBuffer(file_encoding->TranslateTag(tag, phony));
    }
    sentence_font.SetXY(x, y);
    return RET_CONTINUE;
}


int PonscripterLabel::loadgameCommand(const pstring& cmd)
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
        setSkipMode(false);
        setAutoMode(false);
        deleteButtons();
        select_links.clear();
        key_pressed_flag = false;
        text_on_flag  = false;
        indent_offset = 0;
        line_enter_status = 0;
        string_buffer_offset = 0;
        string_buffer_restore = 0;

        refreshMouseOverButton();

        if (loadgosub_label)
            gosubReal(loadgosub_label, script_h.getCurrent());

        readToken();

        if (event_mode & WAIT_INPUT_MODE) return RET_WAIT | RET_NOREAD;

        return RET_CONTINUE | RET_NOREAD;
    }
}


int PonscripterLabel::ldCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    Expression loc = script_h.readStrExpr();
    int no = -1;
    if (loc.is_bareword("l")) no = 0;
    else if (loc.is_bareword("c")) no = 1;
    else if (loc.is_bareword("r")) no = 2;

    pstring buf;
    if (no >= 0) buf = script_h.readStrValue();

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false));
    }
    else {
        if (no >= 0) {
            dirty_rect.add(tachi_info[no].pos);
            tachi_info[no].setImageName(buf);
            parseTaggedString(&tachi_info[no]);
            setupAnimationInfo(&tachi_info[no]);
            if (tachi_info[no].image_surface) {
                tachi_info[no].pos.x = screen_width * (no + 1) / 4 -
                                       tachi_info[no].pos.w / 2;
                tachi_info[no].pos.y = underline_value -
                                       tachi_info[no].image_surface->h + 1;
                tachi_info[no].visible(true);
                dirty_rect.add(tachi_info[no].pos);
            }
        }

        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::jumpfCommand(const pstring& cmd)
{
    const char* buf = script_h.getNext();
    while (*buf != '\0' && *buf != '~') buf++;
    if (*buf == '~') buf++;

    script_h.setCurrent(buf);
    current_label_info = script_h.getLabelByAddress(buf);
    current_line = script_h.getLineByAddress(buf);

    return RET_CONTINUE;
}


int PonscripterLabel::jumpbCommand(const pstring& cmd)
{
    script_h.setCurrent(last_tilde);
    current_label_info = script_h.getLabelByAddress(last_tilde);
    current_line = script_h.getLineByAddress(last_tilde);

    return RET_CONTINUE;
}


int PonscripterLabel::ispageCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(textgosub_clickstr_state == CLICK_NEWPAGE);
    return RET_CONTINUE;
}


int PonscripterLabel::isfullCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(fullscreen_mode);
    return RET_CONTINUE;
}


int PonscripterLabel::isskipCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(automode_flag         ? 2 :
				  (skip_flag            ? 1 :
				   (ctrl_pressed_status ? 3 : 
				                          0)));
    return RET_CONTINUE;
}


int PonscripterLabel::isdownCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(current_button_state.down_flag);
    return RET_CONTINUE;
}


int PonscripterLabel::inputCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();

    script_h.readStrValue(); // description

    e.mutate(script_h.readStrValue());
    printf("%s: %s is set to the default value, %s\n", (const char*) cmd,
	   (const char*) e.debug_string(), (const char*) e.as_string());

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


int PonscripterLabel::indentCommand(const pstring& cmd)
{
    indent_offset = script_h.readIntValue();
    fprintf(stderr, " warning: [indent] command is broken\n");
    return RET_CONTINUE;
}


int PonscripterLabel::humanorderCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    pstring buf = script_h.readStrValue() + "   ";
    int i;
    for (i = 0; i < 3; i++) {
        if (buf[i] == 'l') human_order[i] = 0;
        else if (buf[i] == 'c') human_order[i] = 1;
        else if (buf[i] == 'r') human_order[i] = 2;
        else human_order[i] = -1;
    }

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false));
    }
    else {
        for (i = 0; i < 3; i++)
            dirty_rect.add(tachi_info[i].pos);

        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::getzxcCommand(const pstring& cmd)
{
    getzxc_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getvoicevolCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(voice_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getversionCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(NSC_VERSION);
    return RET_CONTINUE;
}


int PonscripterLabel::gettimerCommand(const pstring& cmd)
{
    if (cmd == "gettimer")
	script_h.readIntExpr().mutate(SDL_GetTicks() - internal_timer);
    else
	script_h.readIntExpr().mutate(btnwait_time);	

    return RET_CONTINUE;
}


int PonscripterLabel::gettextCommand(const pstring& cmd)
{
    pstring buf = current_text_buffer->contents;
    buf.findreplace("\x0a", "");
    script_h.readStrExpr(true).mutate(buf);
    return RET_CONTINUE;
}


int PonscripterLabel::gettagCommand(const pstring& cmd)
{
    if (nest_infos.empty() ||
        nest_infos.back().nest_mode != NestInfo::TEXTGOSUB)
        errorAndExit("gettag: not in a subroutine, e.g. pretextgosub");

    bool end_flag = false;
    const char* buf = nest_infos.back().next_script;
    while (*buf == ' ' || *buf == '\t') buf++;
    int bytes;
    if (zenkakko_flag && file_encoding->DecodeChar(buf, bytes) == 0x3010 /*y */)
        buf += bytes;
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
	    int bytes;
	    while (*buf != '/' &&
		   (!zenkakko_flag ||
		    file_encoding->DecodeChar(buf, bytes) != 0x3011 /* z*/) &&
		   *buf != ']')
		buf += bytes;
	    e.mutate(pstring(buf_start, buf - buf_start));
	}
        if (*buf == '/')
            buf++;
        else
            end_flag = true;
    }
    while (more_args);

    if (zenkakko_flag && file_encoding->DecodeChar(buf, bytes) == 0x3010 /*y */)
	buf += bytes;
    else if (*buf == ']') buf++;

    while (*buf == ' ' || *buf == '\t') buf++;
    nest_infos.back().next_script = buf;

    return RET_CONTINUE;
}


int PonscripterLabel::gettabCommand(const pstring& cmd)
{
    gettab_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getspsizeCommand(const pstring& cmd)
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


int PonscripterLabel::getspmodeCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(sprite_info[script_h.readIntValue()].showing());
    return RET_CONTINUE;
}


int PonscripterLabel::getsevolCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(se_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getscreenshotCommand(const pstring& cmd)
{
    int w = script_h.readIntValue();
    int h = script_h.readIntValue();
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    if (screenshot_surface &&
	(screenshot_surface->w != w || screenshot_surface->h != h))
    {
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


int PonscripterLabel::getpageupCommand(const pstring& cmd)
{
    getpageup_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getpageCommand(const pstring& cmd)
{
    getpageup_flag   = true;
    getpagedown_flag = true;

    return RET_CONTINUE;
}


int PonscripterLabel::getretCommand(const pstring& cmd)
{
    Expression e = script_h.readExpr();
    if (e.is_numeric()) e.mutate(getret_int); else e.mutate(getret_str);
    return RET_CONTINUE;
}


int PonscripterLabel::getregCommand(const pstring& cmd)
{
    Expression ex = script_h.readStrExpr();

    pstring path = "[" + script_h.readStrValue() + "]";
    pstring key = script_h.readStrValue();

    FILE* fp;
    if ((fp = fopen(registry_file, "r")) == NULL &&
	(fp = fopen(archive_path.get_path(0) + registry_file, "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n",
		(const char*) registry_file);
        return RET_CONTINUE;
    }
    pstring reg_buf;
    while (!feof(fp)) {
	reg_buf = "";
	int c;
	while ((c = fgetc(fp)) != EOF && c != '\n')
	    reg_buf += (unsigned char) c;
	if (reg_buf == path) {
	    while (!feof(fp)) {
		reg_buf = "";
		while ((c = fgetc(fp)) != EOF && c != '\n')
		    reg_buf += (unsigned char) c;
		const char *s, *e, *f;
		s = reg_buf;
		e = s + reg_buf.length();
		while (s != e && *s != '"') ++s;
		if (s >= e) continue;
		f = ++s;
		while (f != e && *f != '"') f += 1 + *f == '\\';
		if (f >= e || pstring(s, f - s) != key) continue;
		s = ++f;
		while (s != e && *s != '"') ++s;
		if (s >= e) continue;
		f = ++s;
		while (f != e && *f != '"') f += 1 + *f == '\\';
		ex.mutate(pstring(s, f - s));
		fclose(fp);
		return RET_CONTINUE;
	    }
	}
    }
    fprintf(stderr, "Registry key %s\\%s not found.\n",
	    (const char*) path, (const char*) key);
    fclose(fp);
    return RET_CONTINUE;
}


int PonscripterLabel::getmp3volCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(music_volume);
    return RET_CONTINUE;
}


int PonscripterLabel::getmouseposCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(current_button_state.x *
				  screen_ratio2 / screen_ratio1);
    script_h.readIntExpr().mutate(current_button_state.y *
				  screen_ratio2 / screen_ratio1);
    return RET_CONTINUE;
}


int PonscripterLabel::getlogCommand(const pstring& cmd)
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


int PonscripterLabel::getinsertCommand(const pstring& cmd)
{
    getinsert_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getfunctionCommand(const pstring& cmd)
{
    getfunction_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getenterCommand(const pstring& cmd)
{
    if (!force_button_shortcut_flag) getenter_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getcursorposCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(int(floor(sentence_font.GetX())));
    script_h.readIntExpr().mutate(sentence_font.GetY());
    return RET_CONTINUE;
}


int PonscripterLabel::getcursorCommand(const pstring& cmd)
{
    if (!force_button_shortcut_flag) getcursor_flag = true;
    return RET_CONTINUE;
}


int PonscripterLabel::getcselstrCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();
    int csel_no = script_h.readIntValue();
    if (csel_no >= int(select_links.size()))
	errorAndExit("getcselstr: no select link");
    e.mutate(select_links[csel_no].text);
    return RET_CONTINUE;
}


int PonscripterLabel::getcselnumCommand(const pstring& cmd)
{
    script_h.readIntExpr().mutate(int(select_links.size()));
    return RET_CONTINUE;
}


int PonscripterLabel::gameCommand(const pstring& cmd)
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


int PonscripterLabel::fileexistCommand(const pstring& cmd)
{
    Expression e = script_h.readIntExpr();
    e.mutate(ScriptHandler::cBR->getFileLength(script_h.readStrValue()) > 0);
    return RET_CONTINUE;
}


int PonscripterLabel::exec_dllCommand(const pstring& cmd)
{
    pstring dll_name = "[" + script_h.readStrValue().split("/", 2).at(0) + "]";

    FILE* fp;
    if ((fp = fopen(dll_file, "r")) == NULL &&
	(fp = fopen(archive_path.get_path(0) + dll_file, "r")) == NULL) {
        fprintf(stderr, "Cannot open file [%s]\n", (const char*) dll_file);
        return RET_CONTINUE;
    }

    pstring dll_buf;
    while (!feof(fp)) {
	dll_buf = "";
	int c;
	while ((c = fgetc(fp)) != EOF && c != '\n')
	    dll_buf += (unsigned char) c;
	dll_buf.trim();
	if (dll_buf == dll_name) {
	    getret_str = "";
	    getret_int = 0;
	    while (!feof(fp)) {
		dll_buf = "";
		while ((c = fgetc(fp)) != EOF && c != '\n')
		    dll_buf += (unsigned char) c;
		dll_buf.ltrim();
		if (dll_buf[0] == '[') break;
		CBStringList parts = dll_buf.split("=", 2);
		parts[0].trim();
		parts[1].trim();
		if (parts[0] == "str") {
		    if (parts[1][0] == '"')
			parts[1].remove(0, 1);
		    if (parts[1][parts[1].length()] == '"')
			parts[1].trunc(parts[1].length() - 1);
		    getret_str = parts[1];
		}
		else if (parts[0] == "ret") {
		    getret_int = parts[1];
		}
	    }
	    fclose(fp);
	    return RET_CONTINUE;
	}
    }
    fprintf(stderr, "The DLL is not found in %s.\n", (const char*) dll_file);
    fclose(fp);
    return RET_CONTINUE;
}


int PonscripterLabel::exbtnCommand(const pstring& cmd)
{
    int sprite_no = -1, no = 0;
    ButtonElt* button = &exbtn_d_button;

    if (cmd != "exbtn_d") {
        bool cellcheck_flag = cmd == "cellcheckexbtn";
        sprite_no = script_h.readIntValue();
        no = script_h.readIntValue();
        if ((cellcheck_flag && (sprite_info[sprite_no].num_of_cells < 2)) ||
	    (!cellcheck_flag && (sprite_info[sprite_no].num_of_cells == 0))) {
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
        sprite_info[sprite_no].visible(true);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::erasetextwindowCommand(const pstring& cmd)
{
    erase_text_window_mode = script_h.readIntValue();
    dirty_rect.add(sentence_font_info.pos);
    return RET_CONTINUE;
}


int PonscripterLabel::endCommand(const pstring& cmd)
{
    quit();
    exit(0);
    return RET_CONTINUE; // dummy
}


int PonscripterLabel::dwavestopCommand(const pstring& cmd)
{
    int ch = script_h.readIntValue();
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS - 1;
    if (wave_sample[ch]) {
        Mix_Pause(ch);
        Mix_FreeChunk(wave_sample[ch]);
        wave_sample[ch] = NULL;
    }
    return RET_CONTINUE;
}


int PonscripterLabel::dwaveCommand(const pstring& cmd)
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


int PonscripterLabel::dvCommand(const pstring& cmd)
{
    playSound("voice" DELIMITER + cmd.midstr(2, cmd.length()),
	      SOUND_WAVE | SOUND_OGG, false, 0);
    return RET_CONTINUE;
}


int PonscripterLabel::drawtextCommand(const pstring& cmd)
{
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    text_info.blendOnSurface(accumulation_surface, 0, 0, clip);

    return RET_CONTINUE;
}


int PonscripterLabel::drawsp3Command(const pstring& cmd)
{
    int sprite_no = script_h.readIntValue();
    int cell_no   = script_h.readIntValue();
    int alpha     = script_h.readIntValue();
    int x         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y         = script_h.readIntValue() * screen_ratio1 / screen_ratio2;

    AnimationInfo &si = sprite_info[sprite_no];
    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    si.mat[0][0] = script_h.readIntValue();
    si.mat[0][1] = script_h.readIntValue();
    si.mat[1][0] = script_h.readIntValue();
    si.mat[1][1] = script_h.readIntValue();

    int denom = (si.mat[0][0] * si.mat[1][1] -
		 si.mat[0][1] * si.mat[1][0])
	      / 1000;

    if (denom) {
        si.inv_mat[0][0] =  si.mat[1][1] * 1000 / denom;
        si.inv_mat[0][1] = -si.mat[0][1] * 1000 / denom;
        si.inv_mat[1][0] = -si.mat[1][0] * 1000 / denom;
        si.inv_mat[1][1] =  si.mat[0][0] * 1000 / denom;
    }

    SDL_Rect clip = { 0, 0, screen_surface->w, screen_surface->h };
    si.blendOnSurface2(accumulation_surface, x, y, clip, alpha);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawsp2Command(const pstring& cmd)
{
    int sprite_no = script_h.readIntValue();
    int cell_no   = script_h.readIntValue();
    int alpha     = script_h.readIntValue();

    AnimationInfo &si = sprite_info[sprite_no];
    si.pos.x   = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    si.pos.y   = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    si.scale_x = script_h.readIntValue();
    si.scale_y = script_h.readIntValue();
    si.rot     = script_h.readIntValue();
    si.calcAffineMatrix();

    int old_cell_no = si.current_cell;
    si.setCell(cell_no);

    SDL_Rect clip = { 0, 0, screen_surface->w, screen_surface->h };
    si.blendOnSurface2(accumulation_surface, si.pos.x, si.pos.y, clip, alpha);
    si.setCell(old_cell_no);

    return RET_CONTINUE;
}


int PonscripterLabel::drawspCommand(const pstring& cmd)
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


int PonscripterLabel::drawfillCommand(const pstring& cmd)
{
    int r = script_h.readIntValue();
    int g = script_h.readIntValue();
    int b = script_h.readIntValue();
    SDL_FillRect(accumulation_surface, NULL,
		 SDL_MapRGBA(accumulation_surface->format, r, g, b, 0xff));
    return RET_CONTINUE;
}


int PonscripterLabel::drawclearCommand(const pstring& cmd)
{
    SDL_FillRect(accumulation_surface, NULL,
		 SDL_MapRGBA(accumulation_surface->format, 0, 0, 0, 0xff));
    return RET_CONTINUE;
}


int PonscripterLabel::drawbgCommand(const pstring& cmd)
{
    SDL_Rect clip = { 0, 0, accumulation_surface->w, accumulation_surface->h };
    bg_info.blendOnSurface(accumulation_surface, bg_info.pos.x, bg_info.pos.y,
			   clip);
    return RET_CONTINUE;
}


int PonscripterLabel::drawbg2Command(const pstring& cmd)
{
    int x       = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    int y       = script_h.readIntValue() * screen_ratio1 / screen_ratio2;
    bg_info.scale_x = script_h.readIntValue();
    bg_info.scale_y = script_h.readIntValue();
    bg_info.rot     = script_h.readIntValue();
    bg_info.calcAffineMatrix();

    SDL_Rect clip = { 0, 0, screen_surface->w, screen_surface->h };
    bg_info.blendOnSurface2(accumulation_surface, x, y, clip, 256);

    return RET_CONTINUE;
}


int PonscripterLabel::drawCommand(const pstring& cmd)
{
    SDL_Rect rect = { 0, 0, screen_width, screen_height };
    flushDirect(rect, REFRESH_NONE_MODE);
    dirty_rect.clear();

    return RET_CONTINUE;
}

int PonscripterLabel::deletescreenshotCommand(const pstring& cmd)
{
    if (screenshot_surface) {
        SDL_FreeSurface(screenshot_surface);
        screenshot_surface = NULL;
    }
    return RET_CONTINUE;
}

int PonscripterLabel::delayCommand(const pstring& cmd)
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


int PonscripterLabel::defineresetCommand(const pstring& cmd)
{
    script_h.reset();
    ScriptParser::reset();
    reset();

    setCurrentLabel("define");

    return RET_CONTINUE;
}


int PonscripterLabel::cspCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    // Haeleth extension: csp <sprite>, <sprite2> clears all sprites
    // between those numbers, inclusive.  Ditto for csp2.
    bool csp2         = cmd == "csp2";
    const int max     = csp2 ? MAX_SPRITE2_NUM : MAX_SPRITE_NUM;
    AnimationInfo* si = csp2 ? sprite2_info    : sprite_info;
    
    int no1 = script_h.readIntValue();
    int no2 = script_h.hasMoreArgs() ? script_h.readIntValue() : no1;
    if (no2 < no1) { int swap = no2; no2 = no1; no1 = swap; }
    if (no2 >= max) no2 = max - 1;
    
    if (no1 == -1)
        for (int i = 0; i < max; i++) {
            if (si[i].showing())
                dirty_rect.add(csp2 ? si[i].bounding_rect : si[i].pos);

            if (si[i].image_name) {
                si[i].pos.x = -1000 * screen_ratio1 / screen_ratio2;
                si[i].pos.y = -1000 * screen_ratio1 / screen_ratio2;
            }

            if (!csp2) buttonsRemoveSprite(i);
            si[i].remove();
        }
    else for (int no = no1; no <= no2; ++no) if (no >= 0 && no < max) {
        if (si[no].showing())
            dirty_rect.add(csp2 ? si[no].bounding_rect : si[no].pos);

        if (!csp2) buttonsRemoveSprite(no);
        si[no].remove();
    }

    return RET_CONTINUE;
}


int PonscripterLabel::cselgotoCommand(const pstring& cmd)
{
    int csel_no = script_h.readIntValue();
    if (csel_no >= int(select_links.size()))
	errorAndExit("cselgoto: no select link");

    setCurrentLabel(select_links[csel_no].label);
    select_links.clear();
    newPage(true);

    return RET_CONTINUE;
}


int PonscripterLabel::cselbtnCommand(const pstring& cmd)
{
    int csel_no   = script_h.readIntValue();
    int button_no = script_h.readIntValue();

    Fontinfo csel_info = sentence_font;
    csel_info.top_x = script_h.readIntValue();
    csel_info.top_y = script_h.readIntValue();

    if (csel_no >= (int)select_links.size()) return RET_CONTINUE;
    const pstring& text = select_links[csel_no].text;
    if (!text) return RET_CONTINUE;

    csel_info.setLineArea(int(ceil(csel_info.StringAdvance(text))));
    csel_info.clear();
    buttons[button_no] = getSelectableSentence(text, &csel_info);
    buttons[button_no].sprite_no = csel_no;

    return RET_CONTINUE;
}


int PonscripterLabel::clickCommand(const pstring& cmd)
{
    if (event_mode & WAIT_INPUT_MODE) {
        event_mode = IDLE_EVENT_MODE;
        return RET_CONTINUE;
    }
    else {
        setSkipMode(false);
        event_mode = WAIT_INPUT_MODE;
        key_pressed_flag = false;
        return RET_WAIT | RET_REREAD;
    }
}


int PonscripterLabel::clCommand(const pstring& cmd)
{
    int ret = leaveTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    char loc = script_h.readBareword()[0];

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false));
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

        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::chvolCommand(const pstring& cmd)
{
    int ch  = script_h.readIntValue();
    int vol = script_h.readIntValue();
    if (ch < 0) ch = 0;
    else if (ch >= ONS_MIX_CHANNELS) ch = ONS_MIX_CHANNELS - 1;
    if (wave_sample[ch]) Mix_Volume(ch, vol * 128 / 100);
    return RET_CONTINUE;
}


int PonscripterLabel::checkpageCommand(const pstring& cmd)
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


int PonscripterLabel::cellCommand(const pstring& cmd)
{
    // Haeleth extension: allow cell <sprite1>,<sprite2>,<set1>,[val1],[val2].
    // In this form, all sprites between sprite1 and sprite2 are changed:
    // sprite set1 is set to val1, and the rest to val2.
    // val1 and val2 default to 1 and 0 respectively.
    int no1   = script_h.readIntValue();
    int cell1 = script_h.readIntValue();
    if (script_h.hasMoreArgs()) {
	int no2   = cell1;
	int set   = script_h.readIntValue();
	cell1     = script_h.hasMoreArgs() ? script_h.readIntValue() : 1;
	int cell2 = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
	if (no2 < no1) { int swap = no2; no2 = no1; no1 = swap; }
	if (no2 >= MAX_SPRITE_NUM) no2 = MAX_SPRITE_NUM - 1;
	for (int no = no1; no <= no2; ++no) {
	    sprite_info[no].setCell(no == set ? cell1 : cell2);
	    dirty_rect.add(sprite_info[no].pos);
	}
    }
    else {
	sprite_info[no1].setCell(cell1);
	dirty_rect.add(sprite_info[no1].pos);
    }
    return RET_CONTINUE;
}


int PonscripterLabel::captionCommand(const pstring& cmd)
{
    pstring buf = script_h.readStrValue();
    pstring cap = buf;
    if (script_h.utf_encoding != file_encoding) {
        cap = "";
        const char *bufp = buf;
        while (*bufp) {
            int bytes = 0;
            cap += script_h.utf_encoding->Encode(file_encoding->DecodeChar(bufp, bytes));
            bufp += bytes;
        }
    }
    SDL_WM_SetCaption(cap, cap);
    return RET_CONTINUE;
}


int PonscripterLabel::btnwaitCommand(const pstring& cmd)
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

    bool skipping = skip_flag
                 || (draw_one_page_flag && clickstr_state == CLICK_WAIT)
                 || ctrl_pressed_status;
    if (event_mode & WAIT_BUTTON_MODE || (textbtn_flag && skipping)) {
        btnwait_time  = SDL_GetTicks() - internal_button_timer;
        btntime_value = 0;
        num_chars_in_sentence = 0;

        if (textbtn_flag && skipping)
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
        setSkipMode(false);

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


int PonscripterLabel::btntimeCommand(const pstring& cmd)
{
    btntime2_flag = cmd == "btntime2";
    btntime_value = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::btndownCommand(const pstring& cmd)
{
    btndown_flag = script_h.readIntValue() == 1;
    return RET_CONTINUE;
}


int PonscripterLabel::btndefCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();
    if (!e.is_bareword("clear")) {
        btndef_info.remove();
        if (e.as_string()) {
            btndef_info.setImageName(e.as_string());
            parseTaggedString(&btndef_info);
            btndef_info.trans_mode = AnimationInfo::TRANS_COPY;
            setupAnimationInfo(&btndef_info);
	    if (btndef_info.image_surface) {
		SDL_SetAlpha(btndef_info.image_surface, DEFAULT_BLIT_FLAG,
			     SDL_ALPHA_OPAQUE);
	    }
	    else {
		btntime_value = 0; //Mion - clear the btn wait time
		fprintf(stderr, "Could not create button: %s not found\n",
			(const char*) e.as_string());
	    }
        }
    }
    deleteButtons();
    current_button_state.button = 0;
    disableGetButtonFlag();
    return RET_CONTINUE;
}


int PonscripterLabel::btnCommand(const pstring& cmd)
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


int PonscripterLabel::brCommand(const pstring& cmd)
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

    int ignored;
    pstring tag;
    tag.format("=%d", ns);
    current_text_buffer->addBuffer(file_encoding->TranslateTag(tag, ignored));
    current_text_buffer->addBuffer(0x0a);
    tag.format("=%d", cs);
    current_text_buffer->addBuffer(file_encoding->TranslateTag(tag, ignored));

    return RET_CONTINUE;
}


int PonscripterLabel::bltCommand(const pstring& cmd)
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


int PonscripterLabel::bidirectCommand(const pstring& cmd)
{
    sentence_font.setRTL(script_h.readIntValue()!=0);
    return RET_CONTINUE;
}


int PonscripterLabel::bgcopyCommand(const pstring& cmd)
{
    SDL_BlitSurface(screen_surface, NULL, accumulation_surface, NULL);

    bg_info.num_of_cells = 1;
    bg_info.trans_mode = AnimationInfo::TRANS_COPY;
    bg_info.pos.x = 0;
    bg_info.pos.y = 0;
    bg_info.copySurface(accumulation_surface, NULL);

    return RET_CONTINUE;
}


int PonscripterLabel::bgCommand(const pstring& cmd)
{
    //Mion: prefer removing textwindow for bg change effects even during skip;
    //but don't remove text window if erasetextwindow == 0
    int ret = leaveTextDisplayMode((erase_text_window_mode != 0));
    if (ret != RET_NOMATCH) return ret;

    Expression e = script_h.readStrExpr();
    if (!(e.is_bareword("white") || e.is_bareword("black")))
        bg_info.file_name = e.as_string();

    if (event_mode & EFFECT_EVENT_MODE) {
        return doEffect(parseEffect(false));
    }
    else {
        for (int i = 0; i < 3; i++)
            tachi_info[i].remove();

        bg_info.remove();
        bg_info.file_name = e.as_string();

        createBackground();
        dirty_rect.fill(screen_width, screen_height);

        return setEffect(parseEffect(true), true, true);
    }
}


int PonscripterLabel::barclearCommand(const pstring& cmd)
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


int PonscripterLabel::barCommand(const pstring& cmd)
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


int PonscripterLabel::aviCommand(const pstring& cmd)
{
    pstring name = script_h.readStrValue();
    stopBGM(false);
    playAVI(name, script_h.readIntValue() == 1);
    return RET_CONTINUE;
}


int PonscripterLabel::automode_timeCommand(const pstring& cmd)
{
    automode_time = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::autoclickCommand(const pstring& cmd)
{
    autoclick_time = script_h.readIntValue();
    return RET_CONTINUE;
}


int PonscripterLabel::allspresumeCommand(const pstring& cmd)
{
    all_sprite_hide_flag = false;
    for (int i = 0; i < MAX_SPRITE_NUM; i++) {
        if (sprite_info[i].showing())
            dirty_rect.add(sprite_info[i].pos);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::allsphideCommand(const pstring& cmd)
{
    all_sprite_hide_flag = true;
    for (int i = 0; i < MAX_SPRITE_NUM; i++) {
        if (sprite_info[i].showing())
            dirty_rect.add(sprite_info[i].pos);
    }

    return RET_CONTINUE;
}

int PonscripterLabel::allsp2resumeCommand(const pstring& cmd)
{
    all_sprite2_hide_flag = false;
    for (int i = 0; i < MAX_SPRITE2_NUM; i++) {
        if (sprite2_info[i].showing())
            dirty_rect.add(sprite2_info[i].bounding_rect);
    }

    return RET_CONTINUE;
}


int PonscripterLabel::allsp2hideCommand(const pstring& cmd)
{
    all_sprite2_hide_flag = true;
    for (int i = 0; i < MAX_SPRITE2_NUM; i++) {
        if (sprite2_info[i].showing())
            dirty_rect.add(sprite2_info[i].bounding_rect);
    }

    return RET_CONTINUE;
}
