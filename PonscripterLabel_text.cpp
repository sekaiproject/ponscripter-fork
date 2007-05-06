/* -*- C++ -*-
 *
 *  PonscripterLabel_text.cpp - Text parser of Ponscripter
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

SDL_Surface*
PonscripterLabel::renderGlyph(Font* font, Uint16 text, int size,
			      float x_fractional_part)
{
    if (glyph_surface) {
        SDL_FreeSurface(glyph_surface);
        glyph_surface = NULL;
    }

    font->set_size(size);
    static SDL_Color fcol = { 0xff, 0xff, 0xff }, bcol = { 0, 0, 0 };
    glyph_surface = font->render_glyph(text, fcol, bcol, x_fractional_part);

    return glyph_surface;
}


void
PonscripterLabel::drawGlyph(SDL_Surface* dst_surface, FontInfo* info,
			    SDL_Color &color, unsigned short unicode,
			    float x, int y, bool shadow_flag,
			    AnimationInfo* cache_info, SDL_Rect* clip,
			    SDL_Rect &dst_rect)
{
    float minx, maxy;

    int sz = info->doSize();

    info->font()->get_metrics(unicode, &minx, NULL, NULL, &maxy);

    SDL_Surface* tmp_surface = renderGlyph(info->font(), unicode, sz,
					   x + minx - floor(x + minx));
    bool rotate_flag = false;

    dst_rect.x = int (floor(x + minx));
    dst_rect.y = y + info->font()->ascent() - int (floor(maxy));

    if (shadow_flag) {
        dst_rect.x += shade_distance[0];
        dst_rect.y += shade_distance[1];
    }

    if (tmp_surface) {
        dst_rect.w = tmp_surface->w;
        dst_rect.h = tmp_surface->h;

        if (cache_info == &text_info) {
            // When rendering text
            cache_info->blendBySurface(tmp_surface, dst_rect.x, dst_rect.y,
				       color, clip);
            cache_info->blendOnSurface(dst_surface, 0, 0, dst_rect);
        }
        else {
            if (cache_info)
                cache_info->blendBySurface(tmp_surface, dst_rect.x, dst_rect.y,
					   color, clip);

            if (dst_surface)
                alphaBlend32(dst_surface, dst_rect, tmp_surface, color, clip,
			     rotate_flag);
        }
    }
}


void PonscripterLabel::drawChar(const char* text, FontInfo* info, bool flush_flag, bool lookback_flag, SDL_Surface* surface, AnimationInfo* cache_info, SDL_Rect* clip)
{
    int bytes = encoding->CharacterBytes(text);

    if (!info->processCode(text)) {
        // info->doSize() called in GlyphAdvance

        wchar unicode = encoding->Decode(text);
        float adv = info->GlyphAdvance(unicode, encoding->Decode(text + bytes));

        if (info->isNoRoomFor(adv)) info->newLine();

        float x = info->GetX() * screen_ratio1 / screen_ratio2;
        int   y = info->GetY() * screen_ratio1 / screen_ratio2;

        SDL_Color color;
        SDL_Rect  dst_rect;
        if (info->is_shadow) {
            color.r = color.g = color.b = 0;
            drawGlyph(surface, info, color, unicode, x, y, true, cache_info,
		      clip, dst_rect);
        }

        color.r = info->color.r;
        color.g = info->color.g;
        color.b = info->color.b;	
        drawGlyph(surface, info, color, unicode, x, y, false, cache_info,
		  clip, dst_rect);

	info->addShadeArea(dst_rect, shade_distance);
        if (surface == accumulation_surface && !flush_flag
            && (!clip || AnimationInfo::doClipping(&dst_rect, clip) == 0)) {
            dirty_rect.add(dst_rect);
        }
        else if (flush_flag) {
            flushDirect(dst_rect, REFRESH_NONE_MODE);
        }

        /* ---------------------------------------- */
        /* Update text buffer */
        info->advanceBy(adv);
    }

    if (lookback_flag)
        while (bytes--)
            current_text_buffer->addBuffer(*text++);
}


void
PonscripterLabel::drawString(const char* str, rgb_t color, FontInfo* info,
			     bool flush_flag, SDL_Surface* surface,
			     SDL_Rect* rect, AnimationInfo* cache_info)
{
    float start_x = info->GetXOffset();
    int   start_y = info->GetYOffset();

    /* ---------------------------------------- */
    /* Draw selected characters */
    rgb_t org_color = info->color;
    info->color = color;

    bool skip_whitespace_flag = true;
    while (*str) {
        while (*str == ' ' && skip_whitespace_flag) str++;

        if (*str == encoding->TextMarker()) {
            str++;
            skip_whitespace_flag = false;
            continue;
        }

        if (*str == 0x0a || *str == '\\' && info->is_newline_accepted) {
            info->newLine();
            str++;
        }
        else {
            drawChar(str, info, false, false, surface, cache_info);
            str += encoding->CharacterBytes(str);
        }
    }
    info->color = org_color;

    /* ---------------------------------------- */
    /* Calculate the area of selection */
    SDL_Rect clipped_rect = info->calcUpdatedArea(start_x, start_y,
						  screen_ratio1, screen_ratio2);
    info->addShadeArea(clipped_rect, shade_distance);

    if (flush_flag)
        flush(refresh_shadow_text_mode, &clipped_rect);

    if (rect) *rect = clipped_rect;
}


void PonscripterLabel::restoreTextBuffer()
{
    text_info.fill(0, 0, 0, 0);

    FontInfo f_info = sentence_font;
    f_info.clear();
    const char* buffer = current_text_buffer->contents.c_str();
    int buffer_count = current_text_buffer->contents.size();

    const wchar first_ch = encoding->Decode(buffer);
    if (is_indent_char(first_ch)) f_info.SetIndent(first_ch);

    for (int i = 0; i < buffer_count; ++i) {
        if (buffer[i] == 0x0a) {
            f_info.newLine();
        }
        else {
            drawChar(buffer + i, &f_info, false, false, NULL, &text_info);
            i += encoding->CharacterBytes(buffer + i) - 1;
        }
    }
}


int PonscripterLabel::enterTextDisplayMode(bool text_flag)
{
    if (line_enter_status <= 1 && saveon_flag && internal_saveon_flag &&
	text_flag) {
        saveSaveFile(-1);
        internal_saveon_flag = false;
    }

    if (!(display_mode & TEXT_DISPLAY_MODE)) {
        if (event_mode & EFFECT_EVENT_MODE) {
            if (doEffect(window_effect, NULL, DIRECT_EFFECT_IMAGE, false) ==
		RET_CONTINUE)
	    {
                display_mode = TEXT_DISPLAY_MODE;
                text_on_flag = true;
                return RET_CONTINUE | RET_NOREAD;
            }
            return RET_WAIT | RET_REREAD;
        }
        else {
	    SDL_BlitSurface(accumulation_comp_surface, NULL,
			    effect_dst_surface, NULL);
            SDL_BlitSurface(accumulation_surface, NULL,
			    accumulation_comp_surface, NULL);
            dirty_rect.clear();
            dirty_rect.add(sentence_font_info.pos);

            return setEffect(window_effect);
        }
    }

    return RET_NOMATCH;
}


int PonscripterLabel::leaveTextDisplayMode()
{
    if (display_mode & TEXT_DISPLAY_MODE
        && erase_text_window_mode != 0) {
        if (event_mode & EFFECT_EVENT_MODE) {
            if (doEffect(window_effect, NULL, DIRECT_EFFECT_IMAGE, false) ==
		RET_CONTINUE)
	    {
                display_mode = NORMAL_DISPLAY_MODE;
                return RET_CONTINUE | RET_NOREAD;
            }

            return RET_WAIT | RET_REREAD;
        }
        else {
	    SDL_BlitSurface(accumulation_comp_surface, NULL,
			    effect_dst_surface, NULL);
            SDL_BlitSurface(accumulation_surface, NULL,
			    accumulation_comp_surface, NULL);
            dirty_rect.add(sentence_font_info.pos);
            return setEffect(window_effect);
        }
    }

    return RET_NOMATCH;
}


void PonscripterLabel::doClickEnd()
{
    skip_to_wait = 0;

    if (automode_flag) {
        event_mode = WAIT_TEXT_MODE | WAIT_INPUT_MODE | WAIT_VOICE_MODE;
        if (automode_time < 0)
            startTimer(-automode_time * num_chars_in_sentence);
        else
            startTimer(automode_time);
    }
    else if (autoclick_time > 0) {
        event_mode = WAIT_SLEEP_MODE;
        startTimer(autoclick_time);
    }
    else {
        event_mode = WAIT_TEXT_MODE | WAIT_INPUT_MODE | WAIT_TIMER_MODE;
        advancePhase();
    }

    draw_cursor_flag = true;
    num_chars_in_sentence = 0;
}


int PonscripterLabel::clickWait(bool display_char)
{
    const char* c = script_h.getStringBuffer().c_str() + string_buffer_offset;

    skip_to_wait = 0;

    if ((skip_flag || draw_one_page_flag || ctrl_pressed_status) &&
	!textgosub_label) {
        clickstr_state = CLICK_NONE;
	if (display_char) {
	    drawChar(c, &sentence_font, false, true, accumulation_surface,
		     &text_info);
	}
	else flush(refreshMode());
	string_buffer_offset += encoding->CharacterBytes(c);
        num_chars_in_sentence = 0;
        return RET_CONTINUE | RET_NOREAD;
    }
    else {
        clickstr_state   = CLICK_WAIT;
        key_pressed_flag = false;
	if (display_char) {
	    drawChar(c, &sentence_font, false, true, accumulation_surface,
		     &text_info);
	    ++num_chars_in_sentence;
	}
        if (textgosub_label) {
            saveoffCommand("saveoff");

            textgosub_clickstr_state = CLICK_WAIT;
            if (script_h.getNext()[0] == 0x0a)
                textgosub_clickstr_state |= CLICK_EOL;

            gosubReal(textgosub_label, script_h.getNext());
            indent_offset = 0;
            line_enter_status = 0;
            string_buffer_offset = 0;
            return RET_CONTINUE;
        }

        doClickEnd();

        return RET_WAIT | RET_NOREAD;
    }
}


int PonscripterLabel::clickNewPage(bool display_char)
{
    const char* c = script_h.getStringBuffer().c_str() + string_buffer_offset;

    skip_to_wait = 0;

    clickstr_state = CLICK_NEWPAGE;

    if (display_char) {
        drawChar(c, &sentence_font, true, true, accumulation_surface,
		 &text_info);
        ++num_chars_in_sentence;
    }
    
    if (skip_flag || draw_one_page_flag || ctrl_pressed_status)
	flush(refreshMode());

    if ((skip_flag || ctrl_pressed_status) && !textgosub_label) {
        event_mode = WAIT_SLEEP_MODE;
        advancePhase();
        num_chars_in_sentence = 0;
    }
    else {
        key_pressed_flag = false;
        if (textgosub_label) {
            saveoffCommand("saveoff");

            textgosub_clickstr_state = CLICK_NEWPAGE;
            gosubReal(textgosub_label, script_h.getNext());
            indent_offset = 0;
            line_enter_status = 0;
            string_buffer_offset = 0;
            return RET_CONTINUE;
        }

        doClickEnd();
    }

    return RET_WAIT | RET_NOREAD;
}


int PonscripterLabel::textCommand()
{
    if (pretextgosub_label
        && (line_enter_status == 0
            || (line_enter_status == 1
                && (script_h.getStringBuffer()[string_buffer_offset] == '['
                    || zenkakko_flag && encoding->Decode(script_h.getStringBuffer().c_str() + string_buffer_offset) == 0x3010 /*y */)))) {
        gosubReal(pretextgosub_label, script_h.getCurrent());
        line_enter_status = 1;
        return RET_CONTINUE;
    }

    int ret = enterTextDisplayMode();
    if (ret != RET_NOMATCH) return ret;

    line_enter_status = 2;
    ret = processText();
    if (ret == RET_CONTINUE) {
        indent_offset = 0;
    }

    return ret;
}


int PonscripterLabel::processText()
{
    if (event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE)) {
        draw_cursor_flag = false;
        if (clickstr_state == CLICK_WAIT) {
	    string_buffer_offset += encoding->CharacterBytes(script_h.getStringBuffer().c_str() + string_buffer_offset);
            clickstr_state = CLICK_NONE;
        }
        else if (clickstr_state == CLICK_NEWPAGE) {
            event_mode = IDLE_EVENT_MODE;
	    string_buffer_offset += encoding->CharacterBytes(script_h.getStringBuffer().c_str() + string_buffer_offset);
            newPage(true);
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE | RET_NOREAD;
        }
        else if (script_h.getStringBuffer()[string_buffer_offset] == '!') {
            string_buffer_offset++;
            if (script_h.getStringBuffer()[string_buffer_offset] == 'w' || script_h.getStringBuffer()[string_buffer_offset] == 'd') {
                string_buffer_offset++;
                while (script_h.getStringBuffer()[string_buffer_offset] >= '0'
                       && script_h.getStringBuffer()[string_buffer_offset] <= '9')
                    string_buffer_offset++;
                while (script_h.getStringBuffer()[string_buffer_offset] == ' '
                       || script_h.getStringBuffer()[string_buffer_offset] == '\t') string_buffer_offset++;
            }
        }
        else
            string_buffer_offset +=
		encoding->CharacterBytes(script_h.getStringBuffer().c_str() +
					 string_buffer_offset);
        event_mode = IDLE_EVENT_MODE;
    }

    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a ||
	script_h.getStringBuffer()[string_buffer_offset] == 0x00) {
        indent_offset = 0; // redundant
        return RET_CONTINUE;
    }

    new_line_skip_flag = false;

    while ((!script_h.end1ByteChar()
            && script_h.getStringBuffer()[string_buffer_offset] == ' ')
           || script_h.getStringBuffer()[string_buffer_offset] == '\t')
	string_buffer_offset++;

    char ch = script_h.getStringBuffer()[string_buffer_offset];

    if (ch == '@') { // wait for click
        return clickWait(false);
    }
    else if (ch == '\\') { // new page
        return clickNewPage(false);
    }
    else if (ch == '_') { // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        string_buffer_offset++;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if (ch == '!') {
        string_buffer_offset++;
        if (script_h.getStringBuffer()[string_buffer_offset] == 's') {
            string_buffer_offset++;
            if (script_h.getStringBuffer()[string_buffer_offset] == 'd') {
                sentence_font.wait_time = -1;
                string_buffer_offset++;
            }
            else {
                int t = 0;
                while (script_h.getStringBuffer()[string_buffer_offset] >= '0'
                       && script_h.getStringBuffer()[string_buffer_offset] <= '9') {
                    t = t * 10 + script_h.getStringBuffer()[string_buffer_offset] - '0';
                    string_buffer_offset++;
                }
                sentence_font.wait_time = t;
                while (script_h.getStringBuffer()[string_buffer_offset] == ' '
                       || script_h.getStringBuffer()[string_buffer_offset] == '\t') string_buffer_offset++;
            }
        }
        else if (script_h.getStringBuffer()[string_buffer_offset] == 'w'
                 || script_h.getStringBuffer()[string_buffer_offset] == 'd') {
            bool flag = false;
            if (script_h.getStringBuffer()[string_buffer_offset] == 'd')
		flag = true;

            string_buffer_offset++;
            int tmp_string_buffer_offset = string_buffer_offset;
            int t = 0;
            while (script_h.getStringBuffer()[string_buffer_offset] >= '0'
                   && script_h.getStringBuffer()[string_buffer_offset] <= '9') {
                t = t * 10
		    + script_h.getStringBuffer()[string_buffer_offset] - '0';
                string_buffer_offset++;
            }
            while (script_h.getStringBuffer()[string_buffer_offset] == ' '
                   || script_h.getStringBuffer()[string_buffer_offset] == '\t')
		string_buffer_offset++;
            if (skip_flag || draw_one_page_flag ||
		ctrl_pressed_status || skip_to_wait) {
                return RET_CONTINUE | RET_NOREAD;
            }
            else {
                event_mode = WAIT_SLEEP_MODE;
                if (flag) event_mode |= WAIT_INPUT_MODE;

                key_pressed_flag = false;
                startTimer(t);
                string_buffer_offset = tmp_string_buffer_offset - 2;
                return RET_WAIT | RET_NOREAD;
            }
        }
        else {
            string_buffer_offset--;
            goto notacommand;
        }

        return RET_CONTINUE | RET_NOREAD;
    }
    else if (ch == '#') {
        char hexchecker;
        for (int tmpctr = 0; tmpctr <= 5; tmpctr++) {
            hexchecker = script_h.getStringBuffer()[string_buffer_offset + tmpctr + 1];
            if (!((hexchecker >= '0' && hexchecker <= '9') || (hexchecker >= 'a' && hexchecker <= 'f') || (hexchecker >= 'A' && hexchecker <= 'F'))) goto notacommand;
        }

        sentence_font.color = readColour(script_h.getStringBuffer().c_str() + string_buffer_offset);
        string_buffer_offset += 7;

        return RET_CONTINUE | RET_NOREAD;
    }
    else if (ch == '/') {
        if (script_h.getStringBuffer()[string_buffer_offset + 1] != 0x0a)
	    goto notacommand;
        else { // skip new line
            new_line_skip_flag = true;
            string_buffer_offset++;
            return RET_CONTINUE; // skip the following eol
        }
    }
    else {
        notacommand:

	if (clickstr_state == CLICK_IGNORE) {
	    clickstr_state = CLICK_NONE;
	}
	else {
	    const char* c = script_h.getStringBuffer().c_str() +
		            string_buffer_offset;
	    if (script_h.checkClickstr(c)) {
		if (sentence_font.isNoRoomForLines(clickstr_line))
		    return clickNewPage(true);
		else
		    return clickWait(true);
	    }
	}

        bool flush_flag = !(skip_flag || draw_one_page_flag ||
			    ctrl_pressed_status);

        drawChar(script_h.getStringBuffer().c_str() + string_buffer_offset,
		 &sentence_font, flush_flag, true, accumulation_surface,
		 &text_info);
        ++num_chars_in_sentence;

        if (skip_flag || draw_one_page_flag || ctrl_pressed_status) {
#ifdef BROKEN_SKIP_WRAPPING
            string_buffer_offset += CharacterBytes(script_h.getStringBuffer() +
						   string_buffer_offset);
            return RET_CONTINUE | RET_NOREAD;
#else
            skip_to_wait = 1;
            event_mode = WAIT_SLEEP_MODE;
            advancePhase(0);
            return RET_WAIT | RET_NOREAD;
#endif
        }
        else {
            event_mode = WAIT_SLEEP_MODE;
            if (skip_to_wait == 1)
                advancePhase(0);
            else if (sentence_font.wait_time == -1)
                advancePhase(default_text_speed[text_speed_no]);
            else
                advancePhase(sentence_font.wait_time);

            return RET_WAIT | RET_NOREAD;
        }
    }

    return RET_NOMATCH;
}
