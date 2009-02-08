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

Glyph
PonscripterLabel::renderGlyph(Font* font, Uint16 text, int size,
			      float x_fractional_part)
{
    font->set_size(size);
    static SDL_Color fcol = { 0xff, 0xff, 0xff }, bcol = { 0, 0, 0 };
    current_glyph = font->render_glyph(text, fcol, bcol, x_fractional_part);
    return current_glyph;
}


void
PonscripterLabel::drawGlyph(SDL_Surface* dst_surface, Fontinfo* info,
        SDL_Color &color, unsigned short unicode, float x, int y,
	bool shadow_flag, AnimationInfo* cache_info, SDL_Rect* clip,
	SDL_Rect &dst_rect)
{
    float minx, maxy;

    int sz = info->doSize();

    info->font()->get_metrics(unicode, &minx, NULL, NULL, &maxy);

    Glyph g = renderGlyph(info->font(), unicode, sz,
			  x + minx - floor(x + minx));
    bool rotate_flag = false;

    if (g.bitmap) {
	minx = g.left;
	maxy = g.top;
    }
    
    dst_rect.x = int(floor(x + minx));
    dst_rect.y = y + info->font()->ascent() - int(ceil(maxy));

    if (shadow_flag) {
        dst_rect.x += shade_distance[0];
        dst_rect.y += shade_distance[1];
    }

    if (g.bitmap) {
        dst_rect.w = g.bitmap->w;
        dst_rect.h = g.bitmap->h;

        if (cache_info == &text_info) {
            // When rendering text
            cache_info->blendBySurface(g.bitmap, dst_rect.x, dst_rect.y,
				       color, clip);
            cache_info->blendOnSurface(dst_surface, 0, 0, dst_rect);
        }
        else {
            if (cache_info)
                cache_info->blendBySurface(g.bitmap, dst_rect.x, dst_rect.y,
					   color, clip);

            if (dst_surface)
                alphaBlend32(dst_surface, dst_rect, g.bitmap, color, clip,
			     rotate_flag);
        }
    }
}


// Returns character bytes.
// This is where we process ligatures for display text!
int
PonscripterLabel::drawChar(const char* text, Fontinfo* info, bool flush_flag,
        bool lookback_flag, SDL_Surface* surface, AnimationInfo* cache_info,
	SDL_Rect* clip)
{
    int bytes;
    wchar unicode = encoding->DecodeWithLigatures(text, *info, bytes);

    bool code = info->processCode(text);
    if (!code) {
        // info->doSize() called in GlyphAdvance
	wchar next = encoding->DecodeWithLigatures(text + bytes, *info);
        float adv = info->GlyphAdvance(unicode, next);

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
	    if (surface == accumulation_surface)
		flush(refreshMode()); // hack to fix skip refresh bug
            flushDirect(dst_rect, REFRESH_NONE_MODE);
        }

        /* ---------------------------------------- */
        /* Update text buffer */
        info->advanceBy(adv);
    }

    if (lookback_flag) {
	current_text_buffer->addBytes(text, bytes);
        if (text[0] == '~') current_text_buffer->addBytes(text, bytes);
//TextBuffer_dumpstate(1);
    }
    return bytes;
}


void
PonscripterLabel::drawString(const char* str, rgb_t color, Fontinfo* info,
			     bool flush_flag, SDL_Surface* surface,
			     SDL_Rect* rect, AnimationInfo* cache_info)
{
    float start_x = info->GetXOffset();
    int   start_y = info->GetYOffset();

/**/float max_x = start_x;
/**/int   max_y = start_y;
    
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

        if (*str == 0x0a || (*str == '\\' && info->is_newline_accepted)) {
            info->newLine();
            str++;
        }
        else {
            str += drawChar(str, info, false, false, surface, cache_info);
/**/	    if (info->GetXOffset() > max_x) max_x = info->GetXOffset();
/**/	    if (info->GetYOffset() + info->line_space() > max_y)
/**/		max_y = info->GetYOffset() + info->line_space();
        }
    }
    info->color = org_color;

    /* ---------------------------------------- */
    /* Calculate the area of selection */
//  SDL_Rect clipped_rect = info->calcUpdatedArea(start_x, start_y,
//						  screen_ratio1, screen_ratio2);
/**/SDL_Rect clipped_rect = { int(start_x), start_y,
			      int(max_x - start_x), max_y - start_y };
    info->addShadeArea(clipped_rect, shade_distance);

    if (flush_flag)
        flush(refresh_shadow_text_mode, &clipped_rect);

    if (rect) *rect = clipped_rect;
}


void PonscripterLabel::restoreTextBuffer()
{
    text_info.fill(0, 0, 0, 0);

    Fontinfo f_info = sentence_font;
    f_info.clear();
    const char* buffer = current_text_buffer->contents;
    int buffer_count = current_text_buffer->contents.length();

    const wchar first_ch = encoding->DecodeWithLigatures(buffer, f_info);
    if (is_indent_char(first_ch)) f_info.SetIndent(first_ch);

    int i = 0;
    while (i < buffer_count) {
        if (buffer[i] == 0x0a) {
            f_info.newLine();
	    ++i;
        }
        else {
            i += drawChar(buffer + i, &f_info, false, false, NULL, &text_info);
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
            dirty_rect.clear();
            dirty_rect.add(sentence_font_info.pos);
	    SDL_BlitSurface(accumulation_comp_surface, NULL,
			    effect_dst_surface, NULL);
            SDL_BlitSurface(accumulation_surface, NULL,
			    accumulation_comp_surface, NULL);

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
    const char* c = script_h.getStrBuf(string_buffer_offset);

    skip_to_wait = 0;

    if ((skip_flag || draw_one_page_flag || ctrl_pressed_status) &&
	!textgosub_label) {
        clickstr_state = CLICK_NONE;
	int bytes;
	if (display_char)
	    bytes = drawChar(c, &sentence_font, false, true,
			     accumulation_surface, &text_info);
	else {
	    bytes = 1; // @, \, etc...?
	    flush(refreshMode());
	}
	string_buffer_offset += bytes;
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
            // Haeleth: fix @ bug, 20081214.
            //
            // The problem Agilis observed was caused by a buggy regex
            // in the Narci2 build system, but it seemed better to fix
            // the underlying issue that required the source to be
            // massaged with a regex in the first place.
            //
            // The issue was that when a textgosub routine was
            // defined, the first @ on a line would cause a gosub with
            // the return point being defined as the next command
            // (i.e. the next line).
            //
            // The fix is to extend the gosub system to allow saving a
            // point within the string buffer, return to the _current_
            // command after the gosub, and then jump to that point.
            
//            const char* next_text = script_h.getCurrent()
//                                  + string_buffer_offset;
            const char* next_text = c + 1;
            
            saveoffCommand("saveoff");
            textgosub_clickstr_state =
		(next_text[0] == 0x0a)
                ? CLICK_WAITEOL : CLICK_WAIT;

//            gosubReal(textgosub_label, next_text);
            gosubDoTextgosub();
            
            indent_offset = 0;        // Do we want to reset all these?
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
    const char* c = script_h.getStrBuf(string_buffer_offset);

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
                && (script_h.readStrBuf(string_buffer_offset) == '['
                    || (zenkakko_flag && encoding->DecodeChar(script_h.getStrBuf(string_buffer_offset)) == 0x3010 /* left lenticular bracket */))))) {
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
    if (string_buffer_restore > 0) {
        string_buffer_offset = string_buffer_restore;
        string_buffer_restore = -1;
    }

    if (event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE)) {
        draw_cursor_flag = false;
        if (clickstr_state == CLICK_WAIT) {
	    string_buffer_offset += encoding->NextCharSizeWithLigatures
		(script_h.getStrBuf(string_buffer_offset), &sentence_font);
            clickstr_state = CLICK_NONE;
        }
        else if (clickstr_state == CLICK_NEWPAGE) {
            event_mode = IDLE_EVENT_MODE;
	    string_buffer_offset += encoding->NextCharSizeWithLigatures
		(script_h.getStrBuf(string_buffer_offset), &sentence_font);
            newPage(true);
            clickstr_state = CLICK_NONE;
            return RET_CONTINUE | RET_NOREAD;
        }
        else if (script_h.readStrBuf(string_buffer_offset) == '!') {
            string_buffer_offset++;
            if (script_h.readStrBuf(string_buffer_offset) == 'w' ||
		script_h.readStrBuf(string_buffer_offset) == 'd') {
                ++string_buffer_offset;
                while (script_h.readStrBuf(string_buffer_offset) >= '0'
                       && script_h.readStrBuf(string_buffer_offset) <= '9')
                    ++string_buffer_offset;
                while (script_h.readStrBuf(string_buffer_offset) == ' ' ||
                       script_h.readStrBuf(string_buffer_offset) == '\t')
		    ++string_buffer_offset;
            }
        }
        else
            string_buffer_offset +=
		encoding->NextCharSizeWithLigatures
		(script_h.getStrBuf(string_buffer_offset), &sentence_font);
        event_mode = IDLE_EVENT_MODE;
    }

    if (script_h.readStrBuf(string_buffer_offset) == 0x0a ||
	script_h.readStrBuf(string_buffer_offset) == 0x00) {
        indent_offset = 0; // redundant
        return RET_CONTINUE;
    }

    new_line_skip_flag = false;

    char ch = script_h.readStrBuf(string_buffer_offset);

    if (ch == '@') { // wait for click
        return clickWait(false);
    }
    else if (ch == '\\') { // new page
        return clickNewPage(false);
    }
    else if (ch == '_') { // Ignore following forced return
        clickstr_state = CLICK_IGNORE;
        ++string_buffer_offset;
        return RET_CONTINUE | RET_NOREAD;
    }
    else if (ch == '!') {
        ++string_buffer_offset;
        if (script_h.readStrBuf(string_buffer_offset) == 's') {
            string_buffer_offset++;
            if (script_h.readStrBuf(string_buffer_offset) == 'd') {
                sentence_font.wait_time = -1;
                string_buffer_offset++;
            }
            else {
                int t = 0;
                while (script_h.readStrBuf(string_buffer_offset) >= '0'
                       && script_h.readStrBuf(string_buffer_offset) <= '9') {
                    t = t * 10 +
                        script_h.readStrBuf(string_buffer_offset) -'0';
                    string_buffer_offset++;
                }
                sentence_font.wait_time = t;
                while (script_h.readStrBuf(string_buffer_offset) == ' ' ||
                       script_h.readStrBuf(string_buffer_offset) == '\t')
		    string_buffer_offset++;
            }
        }
        else if (script_h.readStrBuf(string_buffer_offset) == 'w'
                 || script_h.readStrBuf(string_buffer_offset) == 'd') {
            bool flag = false;
            if (script_h.readStrBuf(string_buffer_offset) == 'd')
		flag = true;

            string_buffer_offset++;
            int tmp_string_buffer_offset = string_buffer_offset;
            int t = 0;
            while (script_h.readStrBuf(string_buffer_offset) >= '0'
                   && script_h.readStrBuf(string_buffer_offset) <= '9') {
                t = t * 10
		    + script_h.readStrBuf(string_buffer_offset) - '0';
                string_buffer_offset++;
            }
            while (script_h.readStrBuf(string_buffer_offset) == ' '
                   || script_h.readStrBuf(string_buffer_offset) == '\t')
		string_buffer_offset++;
            if (skip_flag || draw_one_page_flag || ctrl_pressed_status ||
		skip_to_wait) {
		skip_to_wait = 0;
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
        for (int i = 0; i <= 5; ++i) {
            hexchecker = script_h.readStrBuf(string_buffer_offset + i + 1);
            if (!((hexchecker >= '0' && hexchecker <= '9') ||
		  (hexchecker >= 'a' && hexchecker <= 'f') ||
		  (hexchecker >= 'A' && hexchecker <= 'F')))
		goto notacommand;
        }

        sentence_font.color
	    = readColour(script_h.getStrBuf(string_buffer_offset));
        string_buffer_offset += 7;

        return RET_CONTINUE | RET_NOREAD;
    }
    else if (ch == '/') {
        if (script_h.readStrBuf(string_buffer_offset + 1) != 0x0a)
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
	    const char* c = script_h.getStrBuf(string_buffer_offset);
	    if (script_h.checkClickstr(c)) {
		if (sentence_font.isNoRoomForLines(clickstr_line))
		    return clickNewPage(true);
		else
		    return clickWait(true);
	    }
	}

        bool flush_flag = !(skip_flag || draw_one_page_flag ||
			    ctrl_pressed_status);

	// Or possibly just:
//	flush_flag = true;
	
#ifdef BROKEN_SKIP_WRAPPING
        int bytes =
#endif	    
	drawChar(script_h.getStrBuf(string_buffer_offset), &sentence_font,
		 flush_flag, true, accumulation_surface, &text_info);
        ++num_chars_in_sentence;

        if (skip_flag || draw_one_page_flag || ctrl_pressed_status) {
#ifdef BROKEN_SKIP_WRAPPING
            string_buffer_offset += bytes;
            return RET_CONTINUE | RET_NOREAD;
#else
//#ifdef SKIP_TO_WAIT
            skip_to_wait = 1;
//#endif	    
            event_mode = WAIT_SLEEP_MODE;
            advancePhase(0);
            return RET_WAIT | RET_NOREAD;
#endif
        }
        else {
            event_mode = WAIT_SLEEP_MODE;
	    int wait_time;
            if (skip_to_wait == 1)
                wait_time = 0;
            else if (sentence_font.wait_time == -1)
                wait_time = default_text_speed[text_speed_no];
            else
                wait_time = sentence_font.wait_time;
	    advancePhase(wait_time * 100 / global_speed_modifier);

            return RET_WAIT | RET_NOREAD;
        }
    }

    return RET_NOMATCH;
}
