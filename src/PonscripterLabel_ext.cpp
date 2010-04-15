/* -*- C++ -*-
 *
 *  PonscripterLabel_ext.cpp - Ponscripter extensions to the NScripter API
 *
 *  Copyright (c) 2006-9 Peter Jolly
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
    wind.w_right   = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
    wind.w_bottom  = script_h.hasMoreArgs() ? script_h.readIntValue() : 0;
    stored_windows[name] = wind;
    return RET_CONTINUE;
}
int PonscripterLabel::haeleth_usewindowCommand(const pstring& cmd)
{
    pstring name = script_h.readStrValue();
    DoSetwindow(stored_windows[name]);

    if ((cmd == "h_usewindow") || (cmd == "pusewindow")) {
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
    if (buf[0] == file_encoding->TextMarker()) buf.remove(0, 1);

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
	if (arg[0] == file_encoding->TextMarker()) arg.remove(0, 1);
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
	wchar ch = file_encoding->DecodeWithLigatures(it, f, l);
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
		else do { ++it; } while (script_h.isadigit(*it));
		continue;
	    }
	    else if (ch == '#') {
		it += 7;
		continue;
	    }

	    wchar next_ch = file_encoding->DecodeWithLigatures(it, f, l);
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
    if (buf[0] == file_encoding->TextMarker()) buf.remove(0, 1);

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
    bool is_indent = ((cmd == "h_indentstr") || (cmd == "pindentstr"));
    std::set<wchar>& char_set = is_indent ? indent_chars : break_chars;
    char_set.clear();

    Expression e = script_h.readStrExpr();
    pstring s = e.as_string();
    if (e.is_bareword() && (s == "basic")) {
        if (is_indent) {
            char_set.insert(0x0028); //left paren
            char_set.insert(0x2014); //em dash
            char_set.insert(0x2018); //left single curly quote
            char_set.insert(0x201c); //left double curly quote
            char_set.insert(0x300c); //left corner bracket
            char_set.insert(0x300e); //left white cornet bracket
            char_set.insert(0xff08); //fullwidth left paren
            char_set.insert(0xff5e); //fullwidth tilde
            char_set.insert(0xff62); //halfwidth left corner bracket
        } else {
            char_set.insert(0x0020); //space
            char_set.insert(0x002d); //hyphen-minus
            char_set.insert(0x2013); //en dash
            char_set.insert(0x2014); //em dash
        }
    } else {
        pstrIter it(s);
        if (it.get() == file_encoding->TextMarker()) it.next();
        while (it.get() >= 0) {
            char_set.insert(it.get());
            it.next();
        }
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
    pstring buf = script_h.readStrValue();
    if (buf[0] == file_encoding->TextMarker()) buf.remove(0, 1);
    while (buf[0] && (buf[0] != file_encoding->TextMarker()) && (buf[0] != '"')) {
        if (buf[0] == 'c') {
            if ((buf[1] >= '0') && (buf[1] <= '7'))
                Fontinfo::default_encoding = buf[1] - '0';
            buf.remove(0, 2);
        }
        else {
            file_encoding->SetStyle(Fontinfo::default_encoding, buf[0]);
            buf.remove(0, 1);
        }
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
	else if (s == "all")         DefaultLigatures(1 | 2 | 4 | 8);
	else if (s == "default")     DefaultLigatures(1 | 8);
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
	    AddLigature(s, file_encoding->DecodeChar(l.as_string()));
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

int PonscripterLabel::localestringCommand(const pstring& cmd)
{
    Expression e = script_h.readStrExpr();
    pstring tm = file_encoding->TextMarker();
    if (e.is_bareword()) {
        pstring msg = e.as_string();
        if (msg == "message_save_label") {
            locale.message_save_label = tm + script_h.readStrValue();
        } else if (msg == "message_save_exist") {
            locale.message_save_exist = tm + script_h.readStrValue();
        } else if (msg == "message_save_confirm") {
            locale.message_save_confirm = tm + script_h.readStrValue();
        } else if (msg == "message_load_confirm") {
            locale.message_load_confirm = tm + script_h.readStrValue();
        } else if (msg == "message_reset_confirm") {
            locale.message_reset_confirm = tm + script_h.readStrValue();
        } else if (msg == "message_end_confirm") {
            locale.message_end_confirm = tm + script_h.readStrValue();
        } else if (msg == "message_yes") {
            locale.message_yes = script_h.readStrValue();
        } else if (msg == "message_no") {
            locale.message_no = script_h.readStrValue();
        } else if (msg == "message_empty") {
            locale.message_empty = script_h.readStrValue();
        } else if (msg == "message_space") {
            locale.message_space = script_h.readStrValue();
        } else if (msg == "months") {
            for (int i=0; i<12; i++) {
                locale.months[i] = script_h.readStrValue();
            }
        } else if (msg == "days") {
            for (int i=0; i<7; i++) {
                locale.days[i] = script_h.readStrValue();
            }
        } else if (msg == "am_pm") {
            for (int i=0; i<2; i++) {
                locale.am_pm[i] = script_h.readStrValue();
            }
        } else if (msg == "digits") {
            for (int i=0; i<10; i++) {
                locale.digits[i] = script_h.readStrValue();
            }
        } else {
            errorAndCont("localestring: unrecognized var '" + e.as_string()
             + "'\n");
        }
    } else {
        errorAndCont("localestring: improper var " + e.as_string()
             + " (should be bareword)\n");
    }

    return RET_CONTINUE;
}

void PonscripterLabel::initLocale()
{
    pstring months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	pstring days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	pstring am_pm[2] = { "AM", "PM" };
    int i;

    pstring tm = file_encoding->TextMarker();
    if (script_h.is_ponscripter) {
        locale.message_save_exist = tm + "%b %d%i %k:%i%M";
    } else {
        locale.message_save_exist = tm + "Date %_m/%d    Time %_H:%M";
    }
    locale.message_save_label = tm + "%s%n";
    locale.message_save_confirm = tm + "Save in %s%n?";
    locale.message_load_confirm = tm + "Load from %s%n?";
    locale.message_reset_confirm = tm + "Return to Title Menu?";
    locale.message_end_confirm = tm + "Quit?";
    locale.message_yes = "Yes";
    locale.message_no = "No";
    locale.message_empty = "-";
    locale.message_space = " ";
    for (i=0; i<12; i++) {
        locale.months[i] = months[i];
    }
    for (i=0; i<7; i++) {
        locale.days[i] = days[i];
    }
    for (i=0; i<2; i++) {
        locale.am_pm[i] = am_pm[i];
    }
    for (i=0; i<10; i++) {
        locale.digits[i] = char(i + '0');
    }
}

//Mion: create integer strings using locale-defined digits
pstring PonscripterLabel::stringFromInteger(int no, int num_column,
                                            bool is_zero_inserted,
                                            bool use_locale_digits)
{
    pstring ns = script_h.stringFromInteger(no, num_column, is_zero_inserted);
    pstring ns2 = "";
    const char *ptr = (const char *) ns;

    while (*ptr) {
        if (*ptr == ' ') {
            ns2 += locale.message_space;
        } else if (use_locale_digits && (*ptr >= '0') && (*ptr <= '9')) {
            ns2 += locale.digits[*ptr - '0'];
        } else
            ns2 += *ptr;
        ++ptr;
    }

    return ns2;
}

#define MAX_INDENTS 5
float PonscripterLabel::processMessage(pstring &buffer, pstring message, SaveFileInfo &info, float **indents, int *num_ind, bool find_indents)
{
    const char *ptr = (const char *) message;

    bool parse_indents = (indents != NULL) && (num_ind != NULL);
    if (parse_indents && find_indents) {
        *num_ind = 0;
        if (*indents == NULL) {
            *indents = new float[MAX_INDENTS];
            for (int i=0; i<MAX_INDENTS; i++)
                (*indents)[i] = 0;
        }
    }

    enum { UNSET, NOPAD, SPACEPAD, ZEROPAD } field_pad;
    bool use_locale_digits;
    int field_width;
    buffer = "";
    pstring buf = "";
    int num = 0;
    float total_len = 0, last_ind = 0;

    while (*ptr) {
        while (*ptr && (*ptr != '%')) {
            if (*ptr == ' ')
                buf += locale.message_space;
            else
                buf += *ptr;
            ++ptr;
        }
        use_locale_digits = false;
        field_pad = UNSET;
        field_width = -1;
        if (*ptr && (*ptr == '%')) {
            ptr++;
            // optional padding
            if (*ptr == '-') {
                field_pad = NOPAD;
                ptr++;
            } else if (*ptr == '_') {
                field_pad = SPACEPAD;
                ptr++;
            } else if (*ptr == '0') {
                field_pad = ZEROPAD;
                ptr++;
            }
            // optional width
            if ((*ptr >= '0') && (*ptr <= '9')) {
                field_pad = UNSET;
                field_width = 0;
                while ((*ptr >= '0') && (*ptr <= '9')) {
                    field_width *= 10;
                    field_width += *ptr - '0';
                    ptr++;
                }
            }
            // optional use locale digits
            if (*ptr == 'O') {
                use_locale_digits = true;
                ptr++;
            }
            bool handled = false;
            pstring tmp = "";
            int val = -1;
            if (*ptr == '%') {
                buf += '%';
            } else if (*ptr == 't') {
                //tab (variable whitespace to allow indent lineup)
                total_len += current_font->StringAdvance(buf);
                buffer += buf;
                buf = "";
                handled = true;
            } else if (*ptr == 'i') {
                //indent (line-up position)
                // process later or else omit
                handled = true;
                float sz = current_font->StringAdvance(buf);
                total_len += sz;
                if (parse_indents) {
                    if (!find_indents && (num < *num_ind)) {
                        sz = (*indents)[num] - total_len + last_ind;
                        if (sz > 0) {
                            if (script_h.is_ponscripter)
                                tmp.format("~x+%d~", int(sz));
                            else {
                                float sp_sz = current_font->StringAdvance(locale.message_space);
                                int num_sp = ceil(sz/sp_sz);
                                sz = sp_sz * num_sp;
                                for (int j=0; j<num_sp; j++)
                                    tmp += locale.message_space;
                            }
                        }
                        total_len += sz;
                        buffer += tmp;
                        ++num;
                    } else if (find_indents && (num < MAX_INDENTS)) {
                        sz = total_len - last_ind;
                        if ((*indents)[num] < sz)
                            (*indents)[num] = sz;
                        ++num;
                    }
                }
                buffer += buf;
                buf = "";
                last_ind = total_len;
            } else if (*ptr == 's') {
                tmp = save_item_name;
                if (field_pad == UNSET) field_pad = SPACEPAD;
            } else if (*ptr == 'n') {
                val = info.no;
                if (field_pad == UNSET) field_pad = SPACEPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'b') {
                tmp = locale.months[info.month - 1];
                if (field_pad == UNSET) field_pad = SPACEPAD;
            } else if ((*ptr == 'a') && (info.wday >= 0)) {
                tmp = locale.days[info.wday];
                if (field_pad == UNSET) field_pad = SPACEPAD;
            } else if (*ptr == 'm') {
                val = info.month;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'd') {
                val = info.day;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'e') {
                val = info.day;
                if (field_pad == UNSET) field_pad = SPACEPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'y') {
                val = info.year % 100;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'Y') {
                val = info.year;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 4;
            } else if (*ptr == 'H') {
                // 0-23 hour
                val = info.hour;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'k') {
                // 0-23 hour
                val = info.hour;
                if (field_pad == UNSET) field_pad = SPACEPAD;
                if (field_width <= 0) field_width = 2;
            } else if ((*ptr == 'I') || (*ptr == 'l')) {
                // 1-12 hour
                val = info.hour;
                if (val == 0) {
                    val = 12;
                } else if (val > 12) {
                    val -= 12;
                }
                if (field_width <= 0) field_width = 2;
                if (*ptr == 'I') {
                    if (field_pad == UNSET) field_pad = ZEROPAD;
                }
                else {
                    if (field_pad == UNSET) field_pad = SPACEPAD;
                }
            } else if (*ptr == 'p') {
                // AM/PM
                if (info.hour > 12) {
                    val = 1;
                } else {
                    val = 0;
                }
                tmp = locale.am_pm[val];
                if (field_pad == UNSET) field_pad = SPACEPAD;
            } else if (*ptr == 'M') {
                val = info.minute;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            } else if (*ptr == 'S') {
                val = info.sec;
                if (field_pad == UNSET) field_pad = ZEROPAD;
                if (field_width <= 0) field_width = 2;
            }
            if (!handled) {
                if (field_pad == NOPAD) field_width = -1;
                if (tmp.length() > 0) {
                    if (field_width < 0) field_width = 0;
                    for (int j=0; j < (field_width - tmp.length()); j++)
                        buf += locale.message_space;
                    buf += tmp;
                } else if (val >= 0) {
                    buf += stringFromInteger(val, field_width,
                                             (field_pad == ZEROPAD),
                                             use_locale_digits);
                }
            }
            ptr++;
        }
    }
    total_len += current_font->StringAdvance(buf);
    buffer += buf;
    if (num_ind != NULL) *num_ind = num;
    if (debug_level > 0)
        printf("processMessage: made '%s'\n", (const char*)buffer);

    return total_len;
}

