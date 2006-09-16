/* -*- C++ -*-
 *
 *  ONScripterLabel_haeleth.cpp - New commands for Haeleth's version
 *
 *  Copyright (c) 2006 Peter Jolly
 *
 *  haeleth@haeleth.net
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

/* h_textextent <ivar>,<string>
 *
 * Sets <ivar> to the width, in pixels, of <string> as rendered in the current sentence font.
 */
int ONScripterLabel::haeleth_text_extentCommand()
{
	script_h.readInt();
	script_h.pushVariable();
	const char *buf = script_h.readStr();
	if (*buf == '^') ++buf;
	char localbuf[1024];
	strcpy(localbuf, buf);
	FontInfo f = sentence_font;
	if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
		f.font_size_x = script_h.readInt();
		f.font_size_y = script_h.readInt();
		f.pitch_x = script_h.readInt();
		f.ttf_font = NULL;
	}
    script_h.setInt(&script_h.pushed_variable, f.StringAdvance(localbuf));
	return RET_CONTINUE;
}

/* h_centreline <string>
 *
 * For now, just sets the current x position to the expected location required to centre the
 * given <string> on screen (NOT in the window, which must be large enough and appropriately
 * positioned!) in the current sentence font.
 */
int ONScripterLabel::haeleth_centre_lineCommand()
{
	const char *buf = script_h.readStr();
	if (*buf == '^') ++buf;
	sentence_font.SetXY(screen_width / 2 - sentence_font.StringAdvance(buf) / 2 - sentence_font.top_x, -1);
	return RET_CONTINUE;
}

/* h_indentstr <string>
 *
 * Characters in the given <string> will set indents if they occur at the start of a screen.
 * If the first character of a screen is not in the given string, any set indent will be cleared.
 */
int ONScripterLabel::haeleth_char_setCommand()
{
	unsigned short*& char_set = script_h.isName("h_indentstr") ? indent_chars : break_chars;
	if (indent_chars) { delete[] char_set; char_set = NULL; }
	const char* buf = script_h.readStr();
	if (*buf == '^') ++buf;
	char_set = new unsigned short[UTF8Length(buf) + 1];
	int idx = 0;
	while (*buf) {
		char_set[idx++] = UnicodeOfUTF8(buf);
		buf += CharacterBytes(buf);
	}
	char_set[idx] = 0;
	return RET_CONTINUE;
}

/* h_fontstyle <string>
 *
 * Sets default font styling.  Equivalent to inserting ~d<string>~ at the start of every subsequent
 * text display command.  Note that this has no effect on text sprites.
 */
int ONScripterLabel::haeleth_font_styleCommand()
{
	const char *buf = script_h.readStr();
	if (*buf == '^') ++buf;
	script_h.default_encoding = 0;
	while (*buf && *buf != '^' && *buf != '"') SetEncoding(script_h.default_encoding, *buf++);
	return RET_CONTINUE;
}
