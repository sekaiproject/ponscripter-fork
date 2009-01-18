/* -*- C++ -*-
 *
 *  PonscripterLabel_ext.cpp - Ponscripter extensions to the NScripter API
 *
 *  Copyright (c) 2006-7 Peter Jolly
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

/* h_speedpercent <percent>
 *
 * Globally adjust all text speeds by <percent>; this conceptually
 * operates on _output speed_, while text speeds are given as _delay
 * times_, so the effect of this is that
 *
 *   h_speedpercent 50
 *   !s10
 *
 * means
 *
 *   !s20
 */
int PonscripterLabel::haeleth_speedpercentCommand(const pstring& cmd)
{
    global_speed_modifier = script_h.readIntValue();
    return RET_CONTINUE;
}
    
/* h_defwindow <name>, <left>,<top>,<width>,<height>,<fontsize>,
 *             <pitch_x>,<pitch_y>,<wait_time>,<bold>,<shadow>,
 *             <background>,<bg_x>,<bg_y>[,<bg_w>,<bg_h>]
 * h_usewindow <name>
 * h_usewindow3 <name>
 *
 * h_defwindow defines <name> as a stored window style; the remaining
 * parameters are the same as for the setwindow command, except that
 * only one font size is given.
 *
 * h_usewindow* selects the stored style <name>, as though a setwindow*
 * command with the given parameters had been issued.
 */
int PonscripterLabel::haeleth_defwindowCommand(const pstring& cmd)
{
    WindowDef wind;
    pstring name   = script_h.readStrValue();
    wind.left      = script_h.readIntValue();
    wind.top       = script_h.readIntValue();
    wind.width     = script_h.readIntValue();
    wind.height    = script_h.readIntValue();
    wind.font_size = script_h.readIntValue();
    wind.pitch_x   = script_h.readIntValue();
    wind.pitch_y   = script_h.readIntValue();
    wind.speed     = script_h.readIntValue();
    wind.bold      = script_h.readIntValue();
    wind.shadow    = script_h.readIntValue();
    wind.backdrop  = script_h.readStrValue();
    wind.w_left    = script_h.readIntValue();
    wind.w_top     = script_h.readIntValue(); 
    wind.w_width   = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
    wind.w_height  = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
    stored_windows[name] = wind;
    return RET_CONTINUE;
}
int PonscripterLabel::haeleth_usewindowCommand(const pstring& cmd)
{
    pstring name = script_h.readStrValue();
    DoSetwindow(stored_windows[name]);

    if (cmd == "h_usewindow") {
	lookbackflushCommand("lookbackflush");
    }
    else /* h_usewindow3 */ {
	clearCurrentTextBuffer();
    }
    indent_offset = 0;
    line_enter_status = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    flush(refreshMode(), &sentence_font_info.pos);

    return RET_CONTINUE;
}

/* h_textextent <ivar>,<string>,[size_x],[size_y],[pitch_x]
 *
 * Sets <ivar> to the width, in pixels, of <string> as rendered in the
 * current sentence font.
 */
int PonscripterLabel::haeleth_text_extentCommand(const pstring& cmd)
{
    Expression ivar = script_h.readIntExpr();

    pstring buf = script_h.readStrValue();
    if (buf[0] == encoding->TextMarker()) buf.remove(0, 1);

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

/* h_textheight <ivar>,<string...>
 *
 * Sets <ivar> to the height, in pixels, of the area that would be
 * taken up by rendering each <string> as a separate line in the
 * current text window.
 */
int PonscripterLabel::haeleth_text_heightCommand(const pstring& cmd)
{
    Expression ivar = script_h.readIntExpr();

    pstring buf;
    while (script_h.hasMoreArgs()) {
	pstring arg = script_h.readStrValue();
	if (arg[0] == encoding->TextMarker()) arg.remove(0, 1);
	buf += arg;
	buf += '\n';
    }

    if (!buf) {
	errorAndCont("h_textheight: no strings");
	ivar.mutate(0);
	return RET_CONTINUE;
    }
    
    Fontinfo f = sentence_font;
    f.area_y = 0xffffff;
    f.SetXY(0, 0);

    // Bad factoring: we already have a line breaking routine in
    // PonscripterLabel.cpp, but we had to write our own here to
    // handle breaking a complete string as opposed to just finding
    // the next breakpoint...
    const char* first = buf;
    const char* it = first;
    const char* lastbreak = first;
    const char* end = first + buf.length();
    while (it < end) {
	int l;
	wchar ch = encoding->DecodeWithLigatures(it, f, l);
    cont:
	const char* start = it;
	it += l;
	if (ch >= 0x10 && ch < 0x20) {
	    f.processCode(start);
	}
	else {
	    if (ch == '\\')
		errorAndExit("h_textheight: ditch the \\");

	    if (ch == '\n') {
		f.newLine();
		lastbreak = it;
		continue;
	    }
	    
	    if (ch < 0x20 || ch == '@' || is_break_char(ch))
		lastbreak = start;
	    
	    if (ch == '!' && (*it == 's' || *it == 'd' || *it == 'w')) {
		if (it[0] == 's' && it[1] == 'd') it += 2;
		else do { ++it; } while (isdigit(*it));
		continue;
	    }
	    else if (ch == '#') {
		it += 7;
		continue;
	    }

	    wchar next_ch = encoding->DecodeWithLigatures(it, f, l);
	    float adv = f.GlyphAdvance(ch, next_ch);
	    if (f.isNoRoomFor(adv)) {
		f.newLine();
		if (lastbreak != first) it = lastbreak;
		lastbreak = it;
		continue;
	    }
	    f.advanceBy(adv);
	    ch = next_ch;
	    goto cont;
	}
    }

    ivar.mutate(f.GetYOffset() + f.line_space());
        
    return RET_CONTINUE;
}

/* h_centreline <string>
 *
 * For now, just sets the current x position to the expected location
 * required to centre the given <string> on screen (NOT in the window,
 * which must be large enough and appropriately positioned!) in the
 * current sentence font.
 */
int PonscripterLabel::haeleth_centre_lineCommand(const pstring& cmd)
{
    pstring buf = script_h.readStrValue();
    if (buf[0] == encoding->TextMarker()) buf.remove(0, 1);

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
int PonscripterLabel::haeleth_char_setCommand(const pstring& cmd)
{
    std::set<wchar>& char_set = cmd == "h_indentstr"
	                      ? indent_chars
	                      : break_chars;
    char_set.clear();

    pstrIter it(script_h.readStrValue());
    if (it.get() == encoding->TextMarker()) it.next();
    while (it.get() >= 0) {
	char_set.insert(it.get());
	it.next();
    }
    return RET_CONTINUE;
}


/* h_fontstyle <string>
 *
 * Sets default font styling.  Equivalent to inserting ~d<string>~ at
 * the start of every subsequent text display command.  Note that this
 * has no effect on text sprites.
 */
int PonscripterLabel::haeleth_font_styleCommand(const pstring& cmd)
{
    Fontinfo::default_encoding = 0;
    const char* buf = script_h.readStrValue();
    if (*buf == encoding->TextMarker()) ++buf;
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
int PonscripterLabel::haeleth_map_fontCommand(const pstring& cmd)
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
int PonscripterLabel::haeleth_hinting_modeCommand(const pstring& cmd)
{
    pstring l = script_h.readStrValue();
    if (l == "light") hinting = LightHinting;
    else if (l == "full") hinting = FullHinting;
    else if (l == "none") hinting = NoHinting;
    else fprintf(stderr, "Unknown hinting mode `%s'\n", (const char*) l);
    l = script_h.readStrValue();
    if (l == "integer") subpixel = false;
    else if (l == "float") subpixel = true;
    else fprintf(stderr, "Unknown positioning mode `%s'\n", (const char*) l);

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
int PonscripterLabel::haeleth_ligate_controlCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();
    pstring s = e.as_string();
    if (e.is_bareword()) {
	if (s == "none")             ClearLigatures();
	else if (s == "default")     DefaultLigatures(1 | 2 | 4 | 8);
	else if (s == "basic")       DefaultLigatures(1);
	else if (s == "punctuation") DefaultLigatures(2);
	else if (s == "f_ligatures") DefaultLigatures(4);
	else if (s == "specials")    DefaultLigatures(8);
	else fprintf(stderr, "Unknown ligature set `%s'\n", (const char*) s);
    }
    else {
	Expression l = script_h.readExpr();
	if (l.is_bareword("remove"))
            DeleteLigature(s);
        else if (l.is_numeric())
            AddLigature(s, l.as_int());
	else if (l.type() == Expression::String)
	    AddLigature(s, encoding->DecodeChar(l.as_string()));
	else
	    fprintf(stderr, "Unknown character `%s'\n",
		    (const char*) l.debug_string());
    }

    return RET_CONTINUE;
}

int PonscripterLabel::haeleth_sayCommand(const pstring& cmd)
{
    while (1) {
	pstring s = script_h.readExpr().as_string();
	print_escaped(s, stdout);
	if (script_h.hasMoreArgs()) fputs(", ", stdout); else break;   
    }
    putchar('\n');
    fflush(stdout);
    return RET_CONTINUE;
}

/* vsp[2]_when auto,   <int>[, <int>]
 * vsp[2]_when manual, <int>[, <int>]
 * vsp[2]_when always, <int>[, <int>]
 *
 * Control sprite visibility automatically.  "auto" means the named
 * sprite or sprite range is displayed in auto mode and hidden
 * otherwise; "manual" does the opposite; "always" disables automatic
 * visibility controls.
 */
int PonscripterLabel::vsp_whenCommand(const pstring& cmd)
{
    int mode;
    pstring s = script_h.readStrExpr().as_string();
    if (s == "auto") {
        mode = 1;
    }
    else if (s == "manual") {
        mode = 2;
    }
    else if (s == "always") {
        mode = 0;
    }
    else {
        s.format("Expected `auto', `manual', or `always' after %s, found `%s'",
                 (const char*) cmd, (const char*) s);
        errorAndCont(s);
        return RET_CONTINUE;
    }
        
    AnimationInfo* arr;
    int max;
    if (cmd == "vsp_when") {
        arr = sprite_info;
        max = MAX_SPRITE_NUM;
    }
    else {
        arr = sprite2_info;
        max = MAX_SPRITE2_NUM;
    }
    int from = script_h.readIntValue();
    int to = script_h.hasMoreArgs() ? script_h.readIntValue() : from;
    if (from > to) { int tmp = to; to = from; from = tmp; }
    if (from < 0) {
        s.format("index for %s out of range: expected 0..%d, got %d",
                 (const char*) cmd, max - 1, from);
        errorAndCont(s);
    }
    else if (to >= max) {
        s.format("index for %s out of range: expected 0..%d, got %d",
                 (const char*) cmd, max - 1, to);
        errorAndCont(s);
    }
    else for (int i = from; i <= to; ++i) {
        arr[i].enablemode = mode;
        if (arr[i].enabled(mode == 1 ?  automode_flag :
                           mode == 2 ? !automode_flag :
                                       true))
            dirty_rect.add(arr == sprite_info ? arr[i].pos :
                                                arr[i].bounding_rect);
    }

    return RET_CONTINUE;
}
