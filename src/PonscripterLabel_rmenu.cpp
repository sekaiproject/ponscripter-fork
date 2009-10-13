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

        current_font->area_x = screen_width * screen_ratio2 / screen_ratio1;
        current_font->area_y = current_font->line_top(rmenu.size());
        current_font->top_x  = 0;
        current_font->top_y  = (screen_height * screen_ratio2 / screen_ratio1 -
                                current_font->area_y) / 2;
        current_font->SetXY(0, 0);

	for (RMenuElt::iterator it = rmenu.begin(); it != rmenu.end(); ++it) {
            const float sw = float (screen_width * screen_ratio2)
		           / float (screen_ratio1);
            current_font->SetXY((sw - current_font->StringAdvance(it->label)) / 2);
	    buttons[counter++] = getSelectableSentence(it->label, current_font,
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
    setSkipMode(true);
    if (!(shelter_event_mode & WAIT_BUTTON_MODE))
        shelter_event_mode &= ~WAIT_TIMER_MODE;

    leaveSystemCall();
}


void PonscripterLabel::executeSystemAutomode()
{
    setAutoMode(true);
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
    const int spacing = 16;

    pstring buffer, saveless_line;
    float linew, lw, ew, line_offs_x, item_x;
    float *label_inds = NULL, *save_inds = NULL;
    int num_label_ind = 0, num_save_ind = 0;
    {
        float max_lw = 0, max_ew = 0;
        for (unsigned int i = 1; i <= num_save_file; i++) {
            searchSaveFile(save_file_info, i);
            lw = processMessage(buffer, locale.message_save_label,
                                save_file_info, &label_inds, &num_label_ind);
            if (max_lw < lw) max_lw = lw;
            if (save_file_info.valid) {
                ew = processMessage(buffer, locale.message_save_exist,
                                    save_file_info, &save_inds, &num_save_ind);
                if (max_ew < ew) max_ew = ew;
            }
        }

        pstring tm = file_encoding->TextMarker();
        if (save_inds == NULL) {
            saveless_line = tm;
            for (int j=0; j<24; j++)
                saveless_line += locale.message_empty;
            max_ew = current_font->StringAdvance(saveless_line);
        }
        else {
            // Avoid possible ugliness of ligatures
            pstring long_empty;
            for (int j=0; j<6; j++)
                long_empty += locale.message_empty;
            saveless_line = tm;
            while (current_font->StringAdvance(saveless_line) < max_ew)
                saveless_line += long_empty;
            if (max_ew < current_font->StringAdvance(saveless_line))
                max_ew = current_font->StringAdvance(saveless_line);
        }
        item_x = max_lw + spacing;
        linew = ceil(item_x + max_ew + spacing);
        line_offs_x = (sw - linew) / 2;
    }

    // Set up the menu.
    current_font->area_x = int(linew);
    current_font->area_y = current_font->line_top(num_save_file + 2);
    current_font->top_x  = int(line_offs_x);
    current_font->top_y  = (screen_height * screen_ratio2 / screen_ratio1
			- current_font->area_y) / 2;
    pstring& menu_name = is_save ? save_menu_name : load_menu_name;
    current_font->SetXY((linew - current_font->StringAdvance(menu_name)) / 2, 0);
    buttons[0] = getSelectableSentence(menu_name, current_font, false); 

    current_font->newLine();

    flush(refreshMode());
    bool disable = false;

    for (unsigned int i = 1; i <= num_save_file; i++) {
        searchSaveFile(save_file_info, i);
        lw = processMessage(buffer, locale.message_save_label, 
                       save_file_info, &label_inds,
                       &num_label_ind, false);
        current_font->SetXY(0);

        pstring tmp = "";
        if (script_h.is_ponscripter)
            tmp.format("~x%d~", int(item_x));
        else {
            int num_sp = ceil((spacing + 0.0) / 
                              current_font->StringAdvance(locale.message_space));
            for (int j=0; j<num_sp; j++)
                tmp += locale.message_space;
        }
        buffer += tmp;
        if (save_file_info.valid) {
            processMessage(tmp, locale.message_save_exist,
                           save_file_info, &save_inds,
                           &num_save_ind, false);
            disable = false;
        }
        else {
            tmp = saveless_line;
            disable = !is_save;
        }
        buffer += tmp;

	buttons[i] = getSelectableSentence(buffer, current_font, false, disable);
        flush(refreshMode());
    }
    if (label_inds) delete[] label_inds;
    if (save_inds) delete[] save_inds;

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
                string_buffer_restore = -1;
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
	pstring name;

        if (yesno_caller == SYSTEM_SAVE) {
            SaveFileInfo save_file_info;
            searchSaveFile(save_file_info, yesno_selected_file_no);
            processMessage(name, locale.message_save_confirm, save_file_info);
        }
        else if (yesno_caller == SYSTEM_LOAD) {
            SaveFileInfo save_file_info;
            searchSaveFile(save_file_info, yesno_selected_file_no);
            processMessage(name, locale.message_load_confirm, save_file_info);
        }
        else if (yesno_caller == SYSTEM_RESET)
            name = locale.message_reset_confirm;
        else if (yesno_caller == SYSTEM_END)
            name = locale.message_end_confirm;

        current_font->area_x = int (ceil(current_font->StringAdvance(name)));
        current_font->area_y = current_font->line_top(4);
        current_font->top_x  = (screen_width * screen_ratio2 / screen_ratio1 - current_font->area_x) / 2;
        current_font->top_y  = (screen_height * screen_ratio2 / screen_ratio1 - current_font->area_y) / 2;
        current_font->SetXY(0, 0);

	buttons[0] = getSelectableSentence(name, current_font, false);

        flush(refreshMode());

        float yes_len = current_font->StringAdvance(locale.message_yes),
              no_len  = current_font->StringAdvance(locale.message_no);

        name = locale.message_yes;
        current_font->SetXY(float (current_font->area_x) / 4 - yes_len / 2,
			current_font->line_top(2));
	buttons[1] = getSelectableSentence(name, current_font, false);

        name = locale.message_no;
        current_font->SetXY(float (current_font->area_x) * 3 / 4 - no_len / 2,
			current_font->line_top(2));
        buttons[2] = getSelectableSentence(name, current_font, false);

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
            sprite_info[button->sprite_no].visible(true);
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
        sprite_info[lookback_sp[0]].visible(false);
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
            sprite_info[button->sprite_no].visible(true);
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
        sprite_info[lookback_sp[1]].visible(false);
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
                sprite_info[lookback_sp[0]].visible(false);

            if (lookback_sp[1] >= 0)
                sprite_info[lookback_sp[1]].visible(false);

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
                sprite_info[lookback_sp[0]].visible(false);

            if (lookback_sp[1] >= 0)
                sprite_info[lookback_sp[1]].visible(false);

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
