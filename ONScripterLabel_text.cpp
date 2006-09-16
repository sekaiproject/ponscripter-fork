/* -*- C++ -*-
 *
 *  ONScripterLabel_text.cpp - Text parser of PONScripter
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ONScripterLabel.h"
#include "utf8_util.h"

SDL_Surface *ONScripterLabel::renderGlyph(TTF_Font *font, Uint16 text)
{
	GlyphCache *gc = root_glyph_cache;
	GlyphCache *pre_gc = gc;
	while(1){
		if (gc->text == text &&
			gc->font == font){
			if (gc != pre_gc){
				pre_gc->next = gc->next;
				gc->next = root_glyph_cache;
				root_glyph_cache = gc;
			}
			return gc->surface;
		}
		if (gc->next == NULL) break;
		pre_gc = gc;
		gc = gc->next;
	}

	pre_gc->next = NULL;
	gc->next = root_glyph_cache;
	root_glyph_cache = gc;

	gc->text = text;
	gc->font = font;
	if (gc->surface) SDL_FreeSurface(gc->surface);

	static SDL_Color fcol={0xff, 0xff, 0xff}, bcol={0, 0, 0};
	gc->surface = TTF_RenderGlyph_Shaded( font, text, fcol, bcol );

	return gc->surface;
}

void ONScripterLabel::drawGlyph( SDL_Surface *dst_surface, FontInfo *info, SDL_Color &color, unsigned short unicode, int xy[2], bool shadow_flag, AnimationInfo *cache_info, SDL_Rect *clip, SDL_Rect &dst_rect )
{
	int minx, maxx, miny, maxy, advanced;
#if 0
	if (TTF_GetFontStyle( (TTF_Font*)info->ttf_font ) !=
		(info->is_bold?TTF_STYLE_BOLD:TTF_STYLE_NORMAL) )
		TTF_SetFontStyle( (TTF_Font*)info->ttf_font, (info->is_bold?TTF_STYLE_BOLD:TTF_STYLE_NORMAL));
#endif
	TTF_GlyphMetrics( (TTF_Font*)info->ttf_font, unicode,
					  &minx, &maxx, &miny, &maxy, &advanced );
	//printf("min %d %d %d %d %d %d\n", minx, maxx, miny, maxy, advanced,TTF_FontAscent((TTF_Font*)info->ttf_font)  );

	SDL_Surface *tmp_surface = renderGlyph( (TTF_Font*)info->ttf_font, unicode );

	bool rotate_flag = false;

	dst_rect.x = xy[0] + minx;
	dst_rect.y = xy[1] + TTF_FontAscent((TTF_Font*)info->ttf_font) - maxy;
	if ( rotate_flag ) dst_rect.x += miny - minx;

	if ( shadow_flag ){
		dst_rect.x += shade_distance[0];
		dst_rect.y += shade_distance[1];
	}

	if ( tmp_surface ){
		if (rotate_flag){
			dst_rect.w = tmp_surface->h;
			dst_rect.h = tmp_surface->w;
		}
		else{
			dst_rect.w = tmp_surface->w;
			dst_rect.h = tmp_surface->h;
		}

		if (cache_info)
			cache_info->blendBySurface( tmp_surface, dst_rect.x, dst_rect.y, color, clip, rotate_flag );

		if (dst_surface)
			alphaBlend32( dst_surface, dst_rect, tmp_surface, color, clip, rotate_flag );
	}
}

void ONScripterLabel::drawChar( const char* text, FontInfo *info, bool flush_flag, bool lookback_flag, SDL_Surface *surface, AnimationInfo *cache_info, SDL_Rect *clip )
{
	if ( info->ttf_font == NULL ){
		if ( info->openFont() == NULL ){
			fprintf( stderr, "can't open font file: %s\n", font_file );
			quit();
			exit(-1);
		}
	}

	int bytes = CharacterBytes(text);
	unsigned short unicode = UnicodeOfUTF8(text);
	int advance = info->GlyphAdvance(unicode, UnicodeOfUTF8(text + bytes));

	if ( info->isNoRoomFor(advance) ){
		info->newLine();
		if ( lookback_flag ){
			current_text_buffer->addBuffer( 0x0a );
		}
	}

	int xy[2];
	xy[0] = info->GetX() * screen_ratio1 / screen_ratio2;
	xy[1] = info->GetY() * screen_ratio1 / screen_ratio2;

	SDL_Color color;
	SDL_Rect dst_rect;
	if ( info->is_shadow ){
		color.r = color.g = color.b = 0;
		drawGlyph(surface, info, color, unicode, xy, true, cache_info, clip, dst_rect);
	}
	color.r = info->color[0];
	color.g = info->color[1];
	color.b = info->color[2];
	drawGlyph( surface, info, color, unicode, xy, false, cache_info, clip, dst_rect );

	if ( surface == accumulation_surface &&
		 !flush_flag &&
		 (!clip || AnimationInfo::doClipping( &dst_rect, clip ) == 0) ){
		dirty_rect.add( dst_rect );
	}
	else if ( flush_flag ){
		info->addShadeArea(dst_rect, shade_distance);
		flushDirect( dst_rect, REFRESH_NONE_MODE );
	}

	/* ---------------------------------------- */
	/* Update text buffer */
	info->advanceBy(advance);
	if ( lookback_flag ){
		while ( bytes-- ) 
			current_text_buffer->addBuffer( *text++ );
	}
}

void ONScripterLabel::drawString( const char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect, AnimationInfo *cache_info )
{
	int i;

	int start_xy[2];
	start_xy[0] = info->GetXOffset();
	start_xy[1] = info->GetYOffset();

	/* ---------------------------------------- */
	/* Draw selected characters */
	uchar3 org_color;
	for ( i=0 ; i<3 ; i++ ) org_color[i] = info->color[i];
	for ( i=0 ; i<3 ; i++ ) info->color[i] = color[i];

	bool skip_whitespace_flag = true;
	while( *str ){
		while (*str == ' ' && skip_whitespace_flag) str++;

		if ( *str == '^' ){
			str++;
			skip_whitespace_flag = false;
			continue;
		}
		if (*str == 0x0a || *str == '\\' && info->is_newline_accepted){
			info->newLine();
			str++;
		}
		else{
			drawChar( str, info, false, false, surface, cache_info );
			str += CharacterBytes(str);
		}
	}
	for ( i=0 ; i<3 ; i++ ) info->color[i] = org_color[i];

	/* ---------------------------------------- */
	/* Calculate the area of selection */
	SDL_Rect clipped_rect = info->calcUpdatedArea(start_xy, screen_ratio1, screen_ratio2);
	info->addShadeArea(clipped_rect, shade_distance);

	if ( flush_flag )
		flush( refresh_shadow_text_mode, &clipped_rect );

	if ( rect ) *rect = clipped_rect;
}

void ONScripterLabel::restoreTextBuffer()
{
	text_info.fill( 0, 0, 0, 0 );

	FontInfo f_info = sentence_font;
	f_info.clear();
	const char *buffer = current_text_buffer->contents.c_str();
	int buffer_count = current_text_buffer->contents.size();
	
	const unsigned short first_ch = UnicodeOfUTF8(buffer);
	if (is_indent_char(first_ch)) f_info.SetIndent(first_ch);
	
	for ( int i=0 ; i<buffer_count ; ++i ){
		if ( buffer[i] == 0x0a ){
			f_info.newLine();
		}
		else{
			drawChar( buffer + i, &f_info, false, false, NULL, &text_info );
			i += CharacterBytes(buffer + i) - 1;
		}
	}
}

int ONScripterLabel::enterTextDisplayMode(bool text_flag)
{
	if (line_enter_status <= 1 && saveon_flag && internal_saveon_flag && text_flag){
		saveSaveFile( -1 );
		internal_saveon_flag = false;
	}

	if ( !(display_mode & TEXT_DISPLAY_MODE) ){
		if ( event_mode & EFFECT_EVENT_MODE ){
			if ( doEffect( &window_effect, NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
				display_mode = TEXT_DISPLAY_MODE;
				text_on_flag = true;
				return RET_CONTINUE | RET_NOREAD;
			}
			return RET_WAIT | RET_REREAD;
		}
		else{
			next_display_mode = TEXT_DISPLAY_MODE;
			refreshSurface( effect_dst_surface, NULL, refreshMode() );
			dirty_rect.clear();
			dirty_rect.add( sentence_font_info.pos );

			return setEffect( &window_effect );
		}
	}

	return RET_NOMATCH;
}

int ONScripterLabel::leaveTextDisplayMode()
{
	if ( display_mode & TEXT_DISPLAY_MODE &&
		 erase_text_window_mode != 0 ){

		if ( event_mode & EFFECT_EVENT_MODE ){
			if ( doEffect( &window_effect,  NULL, DIRECT_EFFECT_IMAGE ) == RET_CONTINUE ){
				display_mode = NORMAL_DISPLAY_MODE;
				return RET_CONTINUE | RET_NOREAD;
			}
			return RET_WAIT | RET_REREAD;
		}
		else{
			next_display_mode = NORMAL_DISPLAY_MODE;
			refreshSurface( effect_dst_surface, NULL, refreshMode() );
			dirty_rect.add( sentence_font_info.pos );

			return setEffect( &window_effect );
		}
	}

	return RET_NOMATCH;
}

void ONScripterLabel::doClickEnd()
{
	skip_to_wait = 0;

	if ( automode_flag ){
		event_mode =  WAIT_TEXT_MODE | WAIT_INPUT_MODE | WAIT_VOICE_MODE;
		if ( automode_time < 0 )
			startTimer( -automode_time * num_chars_in_sentence );
		else
			startTimer( automode_time );
	}
	else if ( autoclick_time > 0 ){
		event_mode = WAIT_SLEEP_MODE;
		startTimer( autoclick_time );
	}
	else{
		event_mode = WAIT_TEXT_MODE | WAIT_INPUT_MODE | WAIT_TIMER_MODE;
		advancePhase();
	}
	draw_cursor_flag = true;
	num_chars_in_sentence = 0;
}

int ONScripterLabel::clickWait()
{
	skip_to_wait = 0;

	if ( (skip_flag || draw_one_page_flag || ctrl_pressed_status) && !textgosub_label ){
		clickstr_state = CLICK_NONE;
		flush(refreshMode());
		string_buffer_offset++;
		num_chars_in_sentence = 0;

		return RET_CONTINUE | RET_NOREAD;
	}
	else{
		clickstr_state = CLICK_WAIT;
		key_pressed_flag = false;
		if ( textgosub_label ){
			saveoffCommand();

			textgosub_clickstr_state = CLICK_WAIT;
			if (script_h.getNext()[0] == 0x0a)
				textgosub_clickstr_state |= CLICK_EOL;
			gosubReal( textgosub_label, script_h.getNext() );
			indent_offset = 0;
			line_enter_status = 0;
			string_buffer_offset = 0;
			return RET_CONTINUE;
		}

		doClickEnd();

		return RET_WAIT | RET_NOREAD;
	}
}

int ONScripterLabel::clickNewPage()
{
	skip_to_wait = 0;

	clickstr_state = CLICK_NEWPAGE;
	if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ) flush( refreshMode() );

	if ( (skip_flag || ctrl_pressed_status) && !textgosub_label  ){
		event_mode = WAIT_SLEEP_MODE;
		advancePhase();
		num_chars_in_sentence = 0;
	}
	else{
		key_pressed_flag = false;
		if ( textgosub_label ){
			saveoffCommand();

			textgosub_clickstr_state = CLICK_NEWPAGE;
			gosubReal( textgosub_label, script_h.getNext() );
			indent_offset = 0;
			line_enter_status = 0;
			string_buffer_offset = 0;
			return RET_CONTINUE;
		}

		doClickEnd();
	}
	return RET_WAIT | RET_NOREAD;
}

int ONScripterLabel::textCommand()
{
	if (pretextgosub_label &&
		(line_enter_status == 0 ||
		 (line_enter_status == 1 &&
		  (script_h.getStringBuffer()[string_buffer_offset] == '[' ||
		   zenkakko_flag && UnicodeOfUTF8(script_h.getStringBuffer() + string_buffer_offset) == 0x3010 /*y */))) ){
		gosubReal( pretextgosub_label, script_h.getCurrent() );
		line_enter_status = 1;
		return RET_CONTINUE;
	}

	int ret = enterTextDisplayMode();
	if ( ret != RET_NOMATCH ) return ret;

	line_enter_status = 2;
	ret = processText();
	if (ret == RET_CONTINUE){
		indent_offset = 0;
	}

	return ret;
}

int ONScripterLabel::processText()
{
	if ( event_mode & (WAIT_INPUT_MODE | WAIT_SLEEP_MODE) ){
		draw_cursor_flag = false;
		if ( script_h.getStringBuffer()[ string_buffer_offset ] == '!' ){
			string_buffer_offset++;
			if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'w' || script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
				string_buffer_offset++;
				while ( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
						script_h.getStringBuffer()[ string_buffer_offset ] <= '9' )
					string_buffer_offset++;
				while (script_h.getStringBuffer()[ string_buffer_offset ] == ' ' ||
					   script_h.getStringBuffer()[ string_buffer_offset ] == '\t') string_buffer_offset++;
			}
		}
		else 
			string_buffer_offset += CharacterBytes(script_h.getStringBuffer() + string_buffer_offset);

		event_mode = IDLE_EVENT_MODE;
	}

	if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a){
		indent_offset = 0; // redundant
		return RET_CONTINUE;
	}

	new_line_skip_flag = false;

	while( (!(script_h.getEndStatus() & ScriptHandler::END_1BYTE_CHAR) &&
			script_h.getStringBuffer()[ string_buffer_offset ] == ' ') ||
		   script_h.getStringBuffer()[ string_buffer_offset ] == '\t' ) string_buffer_offset ++;

	char ch = script_h.getStringBuffer()[string_buffer_offset];

	if ( ch == '@' ){ // wait for click
		return clickWait();
	}
	else if ( ch == '\\' ){ // new page
		return clickNewPage();
	}
	else if ( ch == '_' ){ // Ignore following forced return
		clickstr_state = CLICK_IGNORE;
		string_buffer_offset++;
		return RET_CONTINUE | RET_NOREAD;
	}
	else if ( ch == '!' ){
		string_buffer_offset++;
		if ( script_h.getStringBuffer()[ string_buffer_offset ] == 's' ){
			string_buffer_offset++;
			if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
				sentence_font.wait_time = -1;
				string_buffer_offset++;
			}
			else{
				int t = 0;
				while( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
					   script_h.getStringBuffer()[ string_buffer_offset ] <= '9' ){
					t = t*10 + script_h.getStringBuffer()[ string_buffer_offset ] - '0';
					string_buffer_offset++;
				}
				sentence_font.wait_time = t;
				while (script_h.getStringBuffer()[ string_buffer_offset ] == ' ' ||
					   script_h.getStringBuffer()[ string_buffer_offset ] == '\t') string_buffer_offset++;
			}
		}
		else if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'w' ||
				  script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ){
			bool flag = false;
			if ( script_h.getStringBuffer()[ string_buffer_offset ] == 'd' ) flag = true;
			string_buffer_offset++;
			int tmp_string_buffer_offset = string_buffer_offset;
			int t = 0;
			while( script_h.getStringBuffer()[ string_buffer_offset ] >= '0' &&
				   script_h.getStringBuffer()[ string_buffer_offset ] <= '9' ){
				t = t*10 + script_h.getStringBuffer()[ string_buffer_offset ] - '0';
				string_buffer_offset++;
			}
			while (script_h.getStringBuffer()[ string_buffer_offset ] == ' ' ||
				   script_h.getStringBuffer()[ string_buffer_offset ] == '\t') string_buffer_offset++;
			if ( skip_flag || draw_one_page_flag || ctrl_pressed_status || skip_to_wait ){
				return RET_CONTINUE | RET_NOREAD;
			}
			else{
				event_mode = WAIT_SLEEP_MODE;
				if ( flag ) event_mode |= WAIT_INPUT_MODE;
				key_pressed_flag = false;
				startTimer( t );
				string_buffer_offset = tmp_string_buffer_offset - 2;
				return RET_WAIT | RET_NOREAD;
			}
		}
		else{
			string_buffer_offset--;
			goto notacommand;
		}
		return RET_CONTINUE | RET_NOREAD;
	}
	else if ( ch == '#' ){
		char hexchecker;
		for ( int tmpctr = 0; tmpctr <= 5; tmpctr++)
		{
			hexchecker = script_h.getStringBuffer()[ string_buffer_offset+tmpctr+1 ];
			if(!((hexchecker >= '0' && hexchecker <= '9') || (hexchecker >= 'a' && hexchecker <= 'f') || (hexchecker >= 'A' && hexchecker <= 'F'))) goto notacommand;
		}
		readColor( &sentence_font.color, script_h.getStringBuffer() + string_buffer_offset );
		string_buffer_offset += 7;

		return RET_CONTINUE | RET_NOREAD;
	}
	else if ( ch == '/'){
		if (script_h.getStringBuffer()[string_buffer_offset+1] != 0x0a) goto notacommand;
		else{ // skip new line
			new_line_skip_flag = true;
			string_buffer_offset++;
			return RET_CONTINUE; // skip the following eol
		}
	}
	else{
		notacommand:


		clickstr_state = CLICK_NONE;

		bool flush_flag = !(skip_flag || draw_one_page_flag || ctrl_pressed_status);
				
		drawChar(script_h.getStringBuffer() + string_buffer_offset, &sentence_font, flush_flag, true, accumulation_surface, &text_info);
		++num_chars_in_sentence;
		
		if ( skip_flag || draw_one_page_flag || ctrl_pressed_status ){
			string_buffer_offset += CharacterBytes(script_h.getStringBuffer() + string_buffer_offset);
			return RET_CONTINUE | RET_NOREAD;
		}
		else{
			event_mode = WAIT_SLEEP_MODE;
			if ( skip_to_wait == 1 )
				advancePhase( 0 );
			else if ( sentence_font.wait_time == -1 )
				advancePhase( default_text_speed[text_speed_no] );
			else
				advancePhase( sentence_font.wait_time );
			return RET_WAIT | RET_NOREAD;
		}
	}

	return RET_NOMATCH;
}

