/* -*- C++ -*-
 *
 *  PonscripterLabel_ext.cpp - Ponscripter extensions to the NScripter API
 *
 *  Copyright (c) 2006 Peter Jolly
 *
 *  haeleth@haeleth.net
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

/* h_textextent <ivar>,<string>,[size_x],[size_y],[pitch_x]
 *
 * Sets <ivar> to the width, in pixels, of <string> as rendered in the
 * current sentence font.
 */
int PonscripterLabel::haeleth_text_extentCommand(const string& cmd)
{
    Expression ivar = script_h.readIntExpr();

    string buf = script_h.readStrValue();
    if (buf[0] == encoding->TextMarker()) buf.shift();

    Fontinfo f = sentence_font;
    if (script_h.hasMoreArgs()) {
        int s1 = script_h.readIntValue();
	int s2 = script_h.readIntValue();
        f.set_size(s1 > s2 ? s1 : s2);
        f.set_mod_size(0);
        f.pitch_x = script_h.readIntValue();
    }

    ivar.mutate(int(ceil(f.StringAdvance(buf))));
    return RET_CONTINUE;
}


/* h_centreline <string>
 *
 * For now, just sets the current x position to the expected location
 * required to centre the given <string> on screen (NOT in the window,
 * which must be large enough and appropriately positioned!) in the
 * current sentence font.
 */
int PonscripterLabel::haeleth_centre_lineCommand(const string& cmd)
{
    string buf = script_h.readStrValue();
    if (buf[0] == encoding->TextMarker()) buf.shift();

    sentence_font.SetXY(float(screen_width) / 2.0 -
			sentence_font.StringAdvance(buf) / 2.0 -
			sentence_font.top_x, -1);
    return RET_CONTINUE;
}


/* h_indentstr <string>
 *
 * Characters in the given <string> will set indents if they occur at
 * the start of a screen.  If the first character of a screen is not
 * in the given string, any set indent will be cleared.
 */
int PonscripterLabel::haeleth_char_setCommand(const string& cmd)
{
    std::set<wchar>& char_set = cmd == "h_indentstr"
	                      ? indent_chars
	                      : break_chars;
    char_set.clear();

    string buf = script_h.readStrValue();
    string::witerator it = buf.wbegin();
    if (*it == encoding->TextMarker()) ++it;
    while (it != buf.wend()) {
	char_set.insert(*it++);
    }
    return RET_CONTINUE;
}


/* h_fontstyle <string>
 *
 * Sets default font styling.  Equivalent to inserting ~d<string>~ at
 * the start of every subsequent text display command.  Note that this
 * has no effect on text sprites.
 */
int PonscripterLabel::haeleth_font_styleCommand(const string& cmd)
{
    string s = script_h.readStrValue();
    if (s[0] == encoding->TextMarker()) s.shift();

    Fontinfo::default_encoding = 0;
    const char* buf = s.c_str();
    while (*buf && *buf != encoding->TextMarker() && *buf != '"') {
        if (*buf == 'c') {
            ++buf;
            if (*buf >= '0' && *buf <= '7')
                Fontinfo::default_encoding = *buf++ - '0';
        }
        else encoding->SetStyle(Fontinfo::default_encoding, *buf++);
    }
    sentence_font.style = Fontinfo::default_encoding;
    return RET_CONTINUE;
}


/* h_mapfont <int>, <string>, [metrics file]
 *
 * Assigns a font file to be associated with the given style number.
 */
int PonscripterLabel::haeleth_map_fontCommand(const string& cmd)
{
    int id = script_h.readIntValue();
    MapFont(id, script_h.readStrValue());
    if (script_h.hasMoreArgs()) MapMetrics(id, script_h.readStrValue());
    return RET_CONTINUE;
}


/* h_rendering <hinting>, <positioning>, [rendermode]
 *
 * Selects a rendering mode.
 * Hinting is one of none, light, full.
 * Positioning is integer or float.
 * Rendermode is light or normal; if not specified, it will be light
 * when hinting is light, otherwise normal.
 */
int PonscripterLabel::haeleth_hinting_modeCommand(const string& cmd)
{
    string l = script_h.readStrValue();
    if (l == "light") hinting = LightHinting;
    else if (l == "full") hinting = FullHinting;
    else if (l == "none") hinting = NoHinting;
    else fprintf(stderr, "Unknown hinting mode `%s'\n", l.c_str());
    l = script_h.readStrValue();
    if (l == "integer") subpixel = false;
    else if (l == "float") subpixel = true;
    else fprintf(stderr, "Unknown positioning mode `%s'\n", l.c_str());

    if (script_h.hasMoreArgs()) {
	l = script_h.readStrValue();
        lightrender = l == "light";
    }
    else {
        lightrender = hinting == LightHinting;
    }
    return RET_CONTINUE;
}


/* h_ligate default
 * h_ligate none
 * h_ligate <input>, <unicode>
 * h_ligate <input>, "unicode"
 * h_ligate <input>, remove
 *
 * Set default ligatures, no ligatures, or add/remove a ligature
 * to/from the list.
 * e.g. 'h_ligate "ffi", 0xFB03' to map "ffi" onto an ffi ligature.
 * Ligature definitions are LIFO, so e.g. you must define "ff" before
 * "ffi", or the latter will never be seen.
 */
int PonscripterLabel::haeleth_ligate_controlCommand(const string& cmd)
{
    Expression e = script_h.readStrExpr();
    string s = e.as_string();
    if (e.is_bareword()) {
	if (s == "none")             ClearLigatures();
	else if (s == "default")     DefaultLigatures(1 | 2 | 4);
	else if (s == "basic")       DefaultLigatures(1);
	else if (s == "punctuation") DefaultLigatures(2);
	else if (s == "f_ligatures") DefaultLigatures(4);
	else fprintf(stderr, "Unknown ligature set `%s'\n", s.c_str());    
    }
    else {
	Expression l = script_h.readExpr();
	if (l.is_bareword("remove"))
            DeleteLigature(s);
        else if (l.is_numeric())
            AddLigature(s, l.as_int());
	else if (l.type() == Expression::String)
	    AddLigature(s, *l.as_string().wbegin());
	else
	    fprintf(stderr, "Unknown character `%s'\n",
		    l.debug_string().c_str());
    }

    return RET_CONTINUE;
}

int PonscripterLabel::haeleth_sayCommand(const string& cmd)
{
    while (1) {
	printf(script_h.readExpr().as_string().c_str());
	if (script_h.hasMoreArgs()) printf(", "); else break;   
    }
    printf("\n");
    return RET_CONTINUE;
}
