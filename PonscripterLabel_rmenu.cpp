/* -*- C++ -*-
 *
 *  PonscripterLabel_rmenu.cpp - Right click menu handler of Ponscripter
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

#define MESSAGE_SAVE_CONFIRM "^Save in "
#define MESSAGE_LOAD_CONFIRM "^Load from "
#define MESSAGE_CONFIRM_TAIL "?"
#define MESSAGE_RESET_CONFIRM "^Return to Title Menu?"
#define MESSAGE_END_CONFIRM "^Quit?"
#define MESSAGE_YES "Yes"
#define MESSAGE_NO "No"

static const char* short_month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void PonscripterLabel::enterSystemCall()
{
    shelter_buttons.swap(buttons);
    buttons.clear();
    shelter_select_links.swap(select_links);
    select_links.clear();
    shelter_event_mode = event_mode;
    shelter_mouse_state.x = last_mouse_state.x;
    shelter_mouse_state.y = last_mouse_state.y;
    event_mode = IDLE_EVENT_MODE;
    system_menu_enter_flag = true;
    yesno_caller = SYSTEM_NULL;
    shelter_display_mode = display_mode;
    display_mode = TEXT_DISPLAY_MODE;
    shelter_draw_cursor_flag = draw_cursor_flag;
    draw_cursor_flag = false;
}


void PonscripterLabel::leaveSystemCall(bool restore_flag)
{
    current_font = &sentence_font;
    display_mode = shelter_display_mode;
    system_menu_mode = SYSTEM_NULL;
    system_menu_enter_flag = false;
    yesno_caller = SYSTEM_NULL;
    key_pressed_flag = false;

    if (restore_flag) {
        current_text_buffer = cached_text_buffer;
        restoreTextBuffer();
	buttons.swap(shelter_buttons);
	shelter_buttons.clear();
	select_links.swap(shelter_select_links);
	shelter_select_links.clear();

        event_mode = shelter_event_mode;
        draw_cursor_flag = shelter_draw_cursor_flag;
        if (event_mode & WAIT_BUTTON_MODE) {
            SDL_WarpMouse(shelter_mouse_state.x, shelter_mouse_state.y);
        }
    }

    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());

    //printf("leaveSystemCall %d %d\n",event_mode, clickstr_state);

    refreshMouseOverButton();
    advancePhase();
}


void PonscripterLabel::executeSystemCall()
{
    //printf("*****  executeSystemCall %d %d %d*****\n", system_menu_enter_flag, volatile_button_state.button, system_menu_mode );
    dirty_rect.fill(screen_width, screen_height);

    if (!system_menu_enter_flag) {
        enterSystemCall();
    }

    switch (system_menu_mode) {
    case SYSTEM_SKIP:
        executeSystemSkip();
        break;
    case SYSTEM_RESET:
        executeSystemReset();
        break;
    case SYSTEM_SAVE:
        executeSystemSave();
        break;
    case SYSTEM_YESNO:
        executeSystemYesNo();
        break;
    case SYSTEM_LOAD:
        executeSystemLoad();
        break;
    case SYSTEM_LOOKBACK:
        executeSystemLookback();
        break;
    case SYSTEM_WINDOWERASE:
        executeWindowErase();
        break;
    case SYSTEM_MENU:
        executeSystemMenu();
        break;
    case SYSTEM_AUTOMODE:
        executeSystemAutomode();
        break;
    case SYSTEM_END:
        executeSystemEnd();
        break;
    default:
        leaveSystemCall();
    }
}


void PonscripterLabel::executeSystemMenu()
{
    int counter = 1;

    current_font = &menu_font;
    if (event_mode & WAIT_BUTTON_MODE) {
        if (current_button_state.button == 0) return;

        event_mode = IDLE_EVENT_MODE;

        deleteButtons();

        if (current_button_state.button == -1) {
	    playSound(menuselectvoice_file_name[MENUSELECTVOICE_CANCEL],
		      SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

            leaveSystemCall();
            return;
        }

	playSound(menuselectvoice_file_name[MENUSELECTVOICE_CLICK],
		  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

	for (RMenuElt::iterator it = rmenu.begin(); it != rmenu.end(); ++it) {
	    if (current_button_state.button == counter++) {
                system_menu_mode = it->system_call_no;
                break;
            }
        }

        advancePhase();
    }
    else {
	playSound(menuselectvoice_file_name[MENUSELECTVOICE_OPEN],
		  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

        system_menu_mode = SYSTEM_MENU;
        yesno_caller = SYSTEM_MENU;

        text_info.fill(0, 0, 0, 0);
        flush(refreshMode());

        menu_font.area_x = screen_width * screen_ratio2 / screen_ratio1;
        menu_font.area_y = menu_font.line_top(rmenu.size());
        menu_font.top_x  = 0;
        menu_font.top_y  = (screen_height * screen_ratio2 / screen_ratio1 - menu_font.area_y) / 2;
        menu_font.SetXY(0, 0);

	for (RMenuElt::iterator it = rmenu.begin(); it != rmenu.end(); ++it) {
            const float sw = float (screen_width * screen_ratio2)
		           / float (screen_ratio1);
            menu_font.SetXY((sw - menu_font.StringAdvance(it->label)) / 2);
	    buttons[counter++] = getSelectableSentence(it->label, &menu_font,
						       false);
            flush(refreshMode());
        }

        flushEvent();
        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}


void PonscripterLabel::executeSystemSkip()
{
    skip_flag = true;
    if (!(shelter_event_mode & WAIT_BUTTON_MODE))
        shelter_event_mode &= ~WAIT_TIMER_MODE;

    leaveSystemCall();
}


void PonscripterLabel::executeSystemAutomode()
{
    automode_flag = true;
    skip_flag = false;
    printf("systemcall_automode: change to automode\n");
    leaveSystemCall();
}


void PonscripterLabel::executeSystemReset()
{
    if (yesno_caller == SYSTEM_RESET) {
        leaveSystemCall();
    }
    else {
        yesno_caller = SYSTEM_RESET;
        system_menu_mode = SYSTEM_YESNO;
        advancePhase();
    }
}


void PonscripterLabel::executeSystemEnd()
{
    if (yesno_caller == SYSTEM_END) {
        leaveSystemCall();
    }
    else {
        yesno_caller = SYSTEM_END;
        system_menu_mode = SYSTEM_YESNO;
        advancePhase();
    }
}


void PonscripterLabel::executeWindowErase()
{
    if (event_mode & WAIT_BUTTON_MODE) {
        event_mode = IDLE_EVENT_MODE;

        leaveSystemCall();
    }
    else {
        display_mode = NORMAL_DISPLAY_MODE;
        flush(mode_saya_flag ? REFRESH_SAYA_MODE : REFRESH_NORMAL_MODE);

        event_mode = WAIT_BUTTON_MODE;
        system_menu_mode = SYSTEM_WINDOWERASE;
    }
}


void PonscripterLabel::createSaveLoadMenu(bool is_save)
{
    SaveFileInfo save_file_info;
    text_info.fill(0, 0, 0, 0);

    // Set up formatting details for saved games.
    const float sw = float (screen_width * screen_ratio2)
	           / float (screen_ratio1);
    const int buf_sz = 1024;
    char  buf[buf_sz], no_save_line[buf_sz];
    float lw, entry_offs_x, entry_date_x, entry_time_x;
    {
        float max_ew = 0, max_dw = 0, max_hw = 0, max_mw = 0;
        for (unsigned int i = 1; i <= num_save_file; i++) {
            snprintf(buf, buf_sz, "^%s %-2d", save_item_name.c_str(), i);
            lw = menu_font.StringAdvance(buf);
            if (lw > max_ew) max_ew = lw;

            searchSaveFile(save_file_info, i);
            if (save_file_info.valid) {
                snprintf(buf, buf_sz, "^%s %2d",
			 short_month[save_file_info.month - 1],
			 save_file_info.day);
                lw = menu_font.StringAdvance(buf);
                if (lw > max_dw) max_dw = lw;

                snprintf(buf, buf_sz, "^%2d:", save_file_info.hour);
                lw = menu_font.StringAdvance(buf);
                if (lw > max_hw) max_hw = lw;

                snprintf(buf, buf_sz, "^%02d", save_file_info.minute);
                lw = menu_font.StringAdvance(buf);
                if (lw > max_mw) max_mw = lw;
            }
        }

        if (max_dw < 1) {
            strncpy(no_save_line, "------------------------", buf_sz);
            lw = ceil(max_ew + 24 + menu_font.StringAdvance(no_save_line) + 1);
            entry_offs_x = (sw - lw) / 2;
            entry_date_x = max_ew + 24;
            entry_time_x = 0;
        }
        else {
            lw = ceil(max_ew + 24 + max_dw + 16 + max_hw + max_mw);
            entry_offs_x = (sw - lw) / 2;
            entry_date_x = max_ew + 24;
            entry_time_x = lw - max_mw;
            int nslw = int((max_dw + 16 + max_hw + max_mw)
			   / menu_font.StringAdvance("-"));
	    // Avoid ugliness with ligatures
	    while (nslw % 3 || nslw % 2) ++nslw;
            no_save_line[0] = '-';
            for (int i = 1; i < nslw; ++i) no_save_line[i] = '-';

            no_save_line[nslw] = 0;
        }
    }

    // Set up the menu.
    menu_font.area_x = int(lw);
    menu_font.area_y = menu_font.line_top(num_save_file + 2);
    menu_font.top_x  = int(entry_offs_x);
    menu_font.top_y  = (screen_height * screen_ratio2 / screen_ratio1
			- menu_font.area_y) / 2;
    string& menu_name = is_save ? save_menu_name : load_menu_name;
    menu_font.SetXY((lw - menu_font.StringAdvance(menu_name)) / 2, 0);
    buttons[0] = getSelectableSentence(menu_name, &menu_font, false); // TODO: check that this isn't selectable!

    menu_font.newLine();

    flush(refreshMode());
    bool disable = false;

    for (unsigned int i = 1; i <= num_save_file; i++) {
        searchSaveFile(save_file_info, i);
        menu_font.SetXY(0);

        if (save_file_info.valid) {
            snprintf(buf, buf_sz, "^%2d:", save_file_info.hour);
            float hw = menu_font.StringAdvance(buf);
            snprintf(buf, buf_sz, "^%s %2d~x%d~%s %-2d~x%d~%2d:%02d",
		     save_item_name.c_str(), i, int(entry_date_x),
		     short_month[save_file_info.month - 1], save_file_info.day,
		     int(entry_time_x - hw), save_file_info.hour,
		     save_file_info.minute);
            disable = false;
        }
        else {
            snprintf(buf, buf_sz, "^%s %2d~x%d~%s", save_item_name.c_str(), i,
		     int(entry_date_x), no_save_line);
            disable = !is_save;
        }

	buttons[i] = getSelectableSentence(buf, &menu_font, false, disable);
        flush(refreshMode());
    }

    event_mode = WAIT_BUTTON_MODE;
    refreshMouseOverButton();
}


void PonscripterLabel::executeSystemLoad()
{
    SaveFileInfo save_file_info;

    current_font = &menu_font;
    if (event_mode & WAIT_BUTTON_MODE) {
        if (current_button_state.button == 0) return;

        event_mode = IDLE_EVENT_MODE;

        if (current_button_state.button > 0) {
            searchSaveFile(save_file_info, current_button_state.button);
            if (!save_file_info.valid) {
                event_mode = WAIT_BUTTON_MODE;
                refreshMouseOverButton();
                return;
            }

            deleteButtons();
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_LOAD;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
        }
        else {
            deleteButtons();
            leaveSystemCall();
        }
    }
    else {
        system_menu_mode = SYSTEM_LOAD;
        createSaveLoadMenu(false);
    }
}


void PonscripterLabel::executeSystemSave()
{
    current_font = &menu_font;
    if (event_mode & WAIT_BUTTON_MODE) {
        if (current_button_state.button == 0) return;

        event_mode = IDLE_EVENT_MODE;

        deleteButtons();

        if (current_button_state.button > 0) {
            yesno_selected_file_no = current_button_state.button;
            yesno_caller = SYSTEM_SAVE;
            system_menu_mode = SYSTEM_YESNO;
            advancePhase();
            return;
        }

        leaveSystemCall();
    }
    else {
        system_menu_mode = SYSTEM_SAVE;
        createSaveLoadMenu(true);
    }
}


void PonscripterLabel::executeSystemYesNo()
{
    current_font = &menu_font;
    if (event_mode & WAIT_BUTTON_MODE) {
        if (current_button_state.button == 0) return;

        event_mode = IDLE_EVENT_MODE;

        deleteButtons();

        if (current_button_state.button == 1) { // yes is selected
	    playSound(menuselectvoice_file_name[MENUSELECTVOICE_YES],
		      SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

            if (yesno_caller == SYSTEM_SAVE) {
                saveSaveFile(yesno_selected_file_no);
                leaveSystemCall();
            }
            else if (yesno_caller == SYSTEM_LOAD) {
                current_font = &sentence_font;
                if (loadSaveFile(yesno_selected_file_no)) {
                    system_menu_mode = yesno_caller;
                    advancePhase();
                    return;
                }

                leaveSystemCall(false);
                saveon_flag = true;
                internal_saveon_flag = true;
                text_on_flag  = false;
                indent_offset = 0;
                line_enter_status    = 0;
                string_buffer_offset = 0;
		break_flag = false;

                if (loadgosub_label)
                    gosubReal(loadgosub_label, script_h.getCurrent());

                readToken();
            }
            else if (yesno_caller == SYSTEM_RESET) {
                resetCommand("reset");
                readToken();
                event_mode = IDLE_EVENT_MODE;
                leaveSystemCall(false);
            }
            else if (yesno_caller == SYSTEM_END) {
                endCommand("end");
            }
        }
        else {
	    playSound(menuselectvoice_file_name[MENUSELECTVOICE_NO],
		      SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

            system_menu_mode = yesno_caller & 0xf;
            if (yesno_caller == SYSTEM_RESET)
                leaveSystemCall();

            advancePhase();
        }
    }
    else {
        text_info.fill(0, 0, 0, 0);
	string name;

        if (yesno_caller == SYSTEM_SAVE) {
            SaveFileInfo save_file_info;
            searchSaveFile(save_file_info, yesno_selected_file_no);
	    name = MESSAGE_SAVE_CONFIRM
		 + save_item_name + save_file_info.num_str
		 + MESSAGE_CONFIRM_TAIL;
        }
        else if (yesno_caller == SYSTEM_LOAD) {
            SaveFileInfo save_file_info;
            searchSaveFile(save_file_info, yesno_selected_file_no);
	    name = MESSAGE_LOAD_CONFIRM
		 + save_item_name + save_file_info.num_str
		 + MESSAGE_CONFIRM_TAIL;		
        }
        else if (yesno_caller == SYSTEM_RESET)
            name = MESSAGE_RESET_CONFIRM;
        else if (yesno_caller == SYSTEM_END)
            name = MESSAGE_END_CONFIRM;

        menu_font.area_x = int (ceil(menu_font.StringAdvance(name)));
        menu_font.area_y = menu_font.line_top(4);
        menu_font.top_x  = (screen_width * screen_ratio2 / screen_ratio1 - menu_font.area_x) / 2;
        menu_font.top_y  = (screen_height * screen_ratio2 / screen_ratio1 - menu_font.area_y) / 2;
        menu_font.SetXY(0, 0);

	buttons[0] = getSelectableSentence(name, &menu_font, false); // TODO: check that this is not selectable!

        flush(refreshMode());

        float yes_len = menu_font.StringAdvance(MESSAGE_YES),
              no_len  = menu_font.StringAdvance(MESSAGE_NO);

        name = MESSAGE_YES;
        menu_font.SetXY(float (menu_font.area_x) / 4 - yes_len / 2,
			menu_font.line_top(2));
	buttons[1] = getSelectableSentence(name, &menu_font, false);

        name = MESSAGE_NO;
        menu_font.SetXY(float (menu_font.area_x) * 3 / 4 - no_len / 2,
			menu_font.line_top(2));
        buttons[2] = getSelectableSentence(name, &menu_font, false);

        flush(refreshMode());

        event_mode = WAIT_BUTTON_MODE;
        refreshMouseOverButton();
    }
}


void PonscripterLabel::setupLookbackButton()
{
    deleteButtons();

    /* ---------------------------------------- */
    /* Previous button check */
    if (current_text_buffer->previous
        && current_text_buffer != start_text_buffer) {
	ButtonElt* button = &buttons[1];

        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h / 3;

        if (lookback_sp[0] >= 0) {
            button->button_type = ButtonElt::SPRITE_BUTTON;
            button->sprite_no = lookback_sp[0];
            sprite_info[button->sprite_no].visible = true;
            button->image_rect = sprite_info[button->sprite_no].pos;
        }
        else {
            button->button_type = ButtonElt::LOOKBACK_BUTTON;
            button->show_flag = 2;
            button->anim[0] = &lookback_info[0];
            button->anim[1] = &lookback_info[1];
            button->image_rect.x = sentence_font_info.pos.x
		                 + sentence_font_info.pos.w
		                 - button->anim[0]->pos.w;
            button->image_rect.y   = sentence_font_info.pos.y;
            button->image_rect.w   = button->anim[0]->pos.w;
            button->image_rect.h   = button->anim[0]->pos.h;
            button->anim[0]->pos.x = button->anim[1]->pos.x = button->image_rect.x;
            button->anim[0]->pos.y = button->anim[1]->pos.y = button->image_rect.y;
        }
    }
    else if (lookback_sp[0] >= 0) {
        sprite_info[lookback_sp[0]].visible = false;
    }

    /* ---------------------------------------- */
    /* Next button check */
    if (current_text_buffer->next != cached_text_buffer) {
	ButtonElt* button = &buttons[2];

        button->select_rect.x = sentence_font_info.pos.x;
        button->select_rect.y = sentence_font_info.pos.y
	                      + sentence_font_info.pos.h * 2 / 3;
        button->select_rect.w = sentence_font_info.pos.w;
        button->select_rect.h = sentence_font_info.pos.h / 3;

        if (lookback_sp[1] >= 0) {
            button->button_type = ButtonElt::SPRITE_BUTTON;
            button->sprite_no = lookback_sp[1];
            sprite_info[button->sprite_no].visible = true;
            button->image_rect = sprite_info[button->sprite_no].pos;
        }
        else {
            button->button_type = ButtonElt::LOOKBACK_BUTTON;
            button->show_flag = 2;
            button->anim[0] = &lookback_info[2];
            button->anim[1] = &lookback_info[3];
            button->image_rect.x = sentence_font_info.pos.x
		                 + sentence_font_info.pos.w
		                 - button->anim[0]->pos.w;
            button->image_rect.y = sentence_font_info.pos.y
 		                 + sentence_font_info.pos.h
	 	                 - button->anim[0]->pos.h;
            button->image_rect.w = button->anim[0]->pos.w;
            button->image_rect.h = button->anim[0]->pos.h;
            button->anim[0]->pos.x = button->anim[1]->pos.x = button->image_rect.x;
            button->anim[0]->pos.y = button->anim[1]->pos.y = button->image_rect.y;
        }
    }
    else if (lookback_sp[1] >= 0) {
        sprite_info[lookback_sp[1]].visible = false;
    }
}


void PonscripterLabel::executeSystemLookback()
{
    current_font = &sentence_font;
    if (event_mode & WAIT_BUTTON_MODE) {
        if (current_button_state.button == 0
            || (current_text_buffer == start_text_buffer
                && current_button_state.button == -2))
            return;

        if (current_button_state.button == -1
            || (current_button_state.button == -3
                && current_text_buffer->next == cached_text_buffer)
            || current_button_state.button <= -4) {
            event_mode = IDLE_EVENT_MODE;
            deleteButtons();
            if (lookback_sp[0] >= 0)
                sprite_info[lookback_sp[0]].visible = false;

            if (lookback_sp[1] >= 0)
                sprite_info[lookback_sp[1]].visible = false;

            leaveSystemCall();
            return;
        }

        if (current_button_state.button == 1
            || current_button_state.button == -2) {
            current_text_buffer = current_text_buffer->previous;
        }
        else
            current_text_buffer = current_text_buffer->next;
    }
    else {
        current_text_buffer = current_text_buffer->previous;
        if (current_text_buffer->empty()) {
            if (lookback_sp[0] >= 0)
                sprite_info[lookback_sp[0]].visible = false;

            if (lookback_sp[1] >= 0)
                sprite_info[lookback_sp[1]].visible = false;

            leaveSystemCall();
            return;
        }

        event_mode = WAIT_BUTTON_MODE;
        system_menu_mode = SYSTEM_LOOKBACK;
    }

    setupLookbackButton();
    refreshMouseOverButton();

    rgb_t color = sentence_font.color;
    sentence_font.color = lookback_color;
    restoreTextBuffer();
    sentence_font.color = color;

    dirty_rect.fill(screen_width, screen_height);
    flush(refreshMode());
}
