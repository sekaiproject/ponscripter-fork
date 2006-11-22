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

/* h_textextent <ivar>,<string>,[size_x],[size_y],[pitch_x]
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
		int s1 = script_h.readInt(), s2 = script_h.readInt();
		f.set_size(s1 > s2 ? s1 : s2);
		f.set_mod_size(0);
		f.pitch_x = script_h.readInt();
	}
    script_h.setInt(&script_h.pushed_variable, int(ceil(f.StringAdvance(localbuf))));
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
	sentence_font.SetXY(float(screen_width) / 2.0 - sentence_font.StringAdvance(buf) / 2.0 - sentence_font.top_x, -1);
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
	FontInfo::default_encoding = 0;
	while (*buf && *buf != '^' && *buf != '"') {
		if (*buf == 'c') {
			++buf;
			if (*buf >= '0' && *buf <= '7') FontInfo::default_encoding = *buf++ - '0';
		}
		else SetEncoding(FontInfo::default_encoding, *buf++);
	}
	sentence_font.style = FontInfo::default_encoding;
	return RET_CONTINUE;
}

/* h_mapfont <int>, <string>, [metrics file]
 *
 * Assigns a font file to be associated with the given style number.
 */
int ONScripterLabel::haeleth_map_fontCommand()
{
	int id = script_h.readInt();
	MapFont(id, script_h.readStr());
	if (script_h.getEndStatus() & ScriptHandler::END_COMMA) MapMetrics(id, script_h.readStr());
	return RET_CONTINUE;
}

/* h_rendering <hinting>, <positioning>, [rendermode]
 *
 * Selects a rendering mode.
 * Hinting is one of none, light, full.
 * Positioning is integer or float.
 * Rendermode is light or normal; if not specified, it will be light when hinting is light, otherwise normal.
 */
int ONScripterLabel::haeleth_hinting_modeCommand()
{
	if (script_h.compareString("light")) hinting = LightHinting;
	else if (script_h.compareString("full")) hinting = FullHinting;
	else if (script_h.compareString("none")) hinting = NoHinting;
	script_h.readLabel();
	if (script_h.compareString("integer")) subpixel = false;
	else if (script_h.compareString("float")) subpixel = true;
	script_h.readLabel();
	if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
		lightrender = script_h.compareString("light");
		script_h.readLabel();
	}
	else {
		lightrender = hinting == LightHinting;
	}
	return RET_CONTINUE;
}

/* h_ligate default
 * h_ligate none
 * h_ligate <input>, <unicode>
 * h_ligate <input>, remove
 *
 * Set default ligatures, no ligatures, or add/remove a ligature to/from the list.
 * e.g. 'h_ligate "ffi", "U+FB03"' to map "ffi" onto an ffi ligature.
 * Ligature definitions are LIFO, so e.g. you must define "ff" before "ffi", or the latter will never be seen.
 */
int ONScripterLabel::haeleth_ligature_controlCommand()
{
 	if (script_h.compareString("none")) {
 		script_h.readLabel();
 		ClearLigatures();
 	}
 	else if (script_h.compareString("default")) {
 		script_h.readLabel();
 		DefaultLigatures(1|2|4);
 	}
 	else if (script_h.compareString("basic")) {
 		script_h.readLabel();
 		DefaultLigatures(1);
 	}
 	else if (script_h.compareString("punctuation")) {
 		script_h.readLabel();
 		DefaultLigatures(2);
 	}
 	else if (script_h.compareString("f_ligatures")) {
 		script_h.readLabel();
 		DefaultLigatures(4);
 	}
 	else {
 		script_h.readStr();
 		const char* in = script_h.saveStringBuffer();
 		if (script_h.compareString("remove")) {
 			script_h.readLabel();
 			DeleteLigature(in);
 		}
 		else {
 			AddLigature(in, script_h.readInt());
 		}
 	}

//DumpLigatures();

 	return RET_CONTINUE;
}
