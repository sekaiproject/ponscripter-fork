/* -*- C++ -*-
 *
 *  ScriptParser.cpp - Define block parser of Ponscripter
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

#include "ScriptParser.h"

#define VERSION_STR1 "Ponscripter"
#define VERSION_STR2 "Copyright (C) 2001-2006 Studio O.G.A., 2006-2007 Haeleth."

#define DEFAULT_SAVE_MENU_NAME "Save"
#define DEFAULT_LOAD_MENU_NAME "Load"
#define DEFAULT_SAVE_ITEM_NAME "Slot"

#define DEFAULT_TEXT_SPEED_LOW 40
#define DEFAULT_TEXT_SPEED_MIDDLE 20
#define DEFAULT_TEXT_SPEED_HIGHT 10

#define MAX_TEXT_BUFFER 17

typedef int (ScriptParser::*ParserFun)(const pstring&);
static class func_lut_t {
    typedef dictionary<pstring, ParserFun>::t dic_t;
    dic_t dict;
public:
    func_lut_t();
    ParserFun get(pstring what) const {
	dic_t::const_iterator it = dict.find(what);
	if (it == dict.end()) return 0;
	return it->second;
    }
} func_lut;
func_lut_t::func_lut_t() {
    dict["add"]             = &ScriptParser::addCommand;
    dict["arc"]             = &ScriptParser::arcCommand;
    dict["atoi"]            = &ScriptParser::atoiCommand;
    dict["automode"]        = &ScriptParser::mode_extCommand;
    dict["break"]           = &ScriptParser::breakCommand;
    dict["clickstr"]        = &ScriptParser::clickstrCommand;
    dict["clickvoice"]      = &ScriptParser::clickvoiceCommand;
    dict["cmp"]             = &ScriptParser::cmpCommand;
    dict["cos"]             = &ScriptParser::cosCommand;
    dict["date"]            = &ScriptParser::dateCommand;
    dict["dec"]             = &ScriptParser::decCommand;
    dict["defaultfont"]     = &ScriptParser::defaultfontCommand;
    dict["defaultspeed"]    = &ScriptParser::defaultspeedCommand;
    dict["defmp3vol"]       = &ScriptParser::defmp3volCommand;
    dict["defsevol"]        = &ScriptParser::defsevolCommand;
    dict["defsub"]          = &ScriptParser::defsubCommand;
    dict["defvoicevol"]     = &ScriptParser::defvoicevolCommand;
    dict["dim"]             = &ScriptParser::dimCommand;
    dict["div"]             = &ScriptParser::divCommand;
    dict["effect"]          = &ScriptParser::effectCommand;
    dict["effectblank"]     = &ScriptParser::effectblankCommand;
    dict["effectcut"]       = &ScriptParser::effectcutCommand;
    dict["filelog"]         = &ScriptParser::filelogCommand;
    dict["for"]             = &ScriptParser::forCommand;
    dict["getparam"]        = &ScriptParser::getparamCommand;
    dict["globalon"]        = &ScriptParser::globalonCommand;
    dict["gosub"]           = &ScriptParser::gosubCommand;
    dict["goto"]            = &ScriptParser::gotoCommand;
    dict["humanz"]          = &ScriptParser::humanzCommand;
    dict["if"]              = &ScriptParser::ifCommand;
    dict["inc"]             = &ScriptParser::incCommand;
    dict["intlimit"]        = &ScriptParser::intlimitCommand;
    dict["itoa"]            = &ScriptParser::itoaCommand;
    dict["itoa2"]           = &ScriptParser::itoaCommand;
    dict["kidokumode"]      = &ScriptParser::kidokumodeCommand;
    dict["kidokuskip"]      = &ScriptParser::kidokuskipCommand;
    dict["labellog"]        = &ScriptParser::labellogCommand;
    dict["len"]             = &ScriptParser::lenCommand;
    dict["linepage"]        = &ScriptParser::linepageCommand;
    dict["linepage2"]       = &ScriptParser::linepageCommand;
    dict["loadgosub"]       = &ScriptParser::loadgosubCommand;
    dict["lookbackcolor"]   = &ScriptParser::lookbackcolorCommand;
    dict["lookbacksp"]      = &ScriptParser::lookbackspCommand;
    dict["maxkaisoupage"]   = &ScriptParser::maxkaisoupageCommand;
    dict["menuselectcolor"] = &ScriptParser::menuselectcolorCommand;
    dict["menuselectvoice"] = &ScriptParser::menuselectvoiceCommand;
    dict["menusetwindow"]   = &ScriptParser::menusetwindowCommand;
    dict["mid"]             = &ScriptParser::midCommand;
    dict["mod"]             = &ScriptParser::modCommand;
    dict["mode_ext"]        = &ScriptParser::mode_extCommand;
    dict["mode_saya"]       = &ScriptParser::mode_sayaCommand;
    dict["mov"]             = &ScriptParser::movCommand;
    dict["mov10"]           = &ScriptParser::movCommand;
    dict["mov3"]            = &ScriptParser::movCommand;
    dict["mov4"]            = &ScriptParser::movCommand;
    dict["mov5"]            = &ScriptParser::movCommand;
    dict["mov6"]            = &ScriptParser::movCommand;
    dict["mov7"]            = &ScriptParser::movCommand;
    dict["mov8"]            = &ScriptParser::movCommand;
    dict["mov9"]            = &ScriptParser::movCommand;
    dict["movl"]            = &ScriptParser::movCommand;
    dict["movz"]            = &ScriptParser::movCommand;
    dict["mul"]             = &ScriptParser::mulCommand;
    dict["next"]            = &ScriptParser::nextCommand;
    dict["notif"]           = &ScriptParser::ifCommand;
    dict["ns2"]             = &ScriptParser::nsaCommand;
    dict["ns3"]             = &ScriptParser::nsaCommand;
    dict["nsa"]             = &ScriptParser::arcCommand;
    dict["nsa"]             = &ScriptParser::nsaCommand;
    dict["nsadir"]          = &ScriptParser::nsadirCommand;
    dict["numalias"]        = &ScriptParser::numaliasCommand;
    dict["pretextgosub"]    = &ScriptParser::pretextgosubCommand;
    dict["return"]          = &ScriptParser::returnCommand;
    dict["rmenu"]           = &ScriptParser::rmenuCommand;
    dict["roff"]            = &ScriptParser::roffCommand;
    dict["rubyoff"]         = &ScriptParser::rubyoffCommand;
    dict["rubyon"]          = &ScriptParser::rubyonCommand;
    dict["sar"]             = &ScriptParser::nsaCommand;
    dict["savename"]        = &ScriptParser::savenameCommand;
    dict["savenumber"]      = &ScriptParser::savenumberCommand;
    dict["selectcolor"]     = &ScriptParser::selectcolorCommand;
    dict["selectvoice"]     = &ScriptParser::selectvoiceCommand;
    dict["shadedistance"]   = &ScriptParser::shadedistanceCommand;
    dict["sin"]             = &ScriptParser::sinCommand;
    dict["skip"]            = &ScriptParser::skipCommand;
    dict["soundpressplgin"] = &ScriptParser::soundpressplginCommand;
    dict["spi"]             = &ScriptParser::soundpressplginCommand;
    dict["stralias"]        = &ScriptParser::straliasCommand;
    dict["sub"]             = &ScriptParser::subCommand;
    dict["tan"]             = &ScriptParser::tanCommand;
    dict["textgosub"]       = &ScriptParser::textgosubCommand;
    dict["time"]            = &ScriptParser::timeCommand;
    dict["transmode"]       = &ScriptParser::transmodeCommand;
    dict["underline"]       = &ScriptParser::underlineCommand;
    dict["useescspc"]       = &ScriptParser::useescspcCommand;
    dict["usewheel"]        = &ScriptParser::usewheelCommand;
    dict["versionstr"]      = &ScriptParser::versionstrCommand;
    dict["watch_var"]       = &ScriptParser::watch_varCommand;
    dict["windowback"]      = &ScriptParser::windowbackCommand;
    dict["windoweffect"]    = &ScriptParser::effectCommand;
    dict["zenkakko"]        = &ScriptParser::zenkakkoCommand;
}


ScriptParser::ScriptParser()
{
    debug_level = 0;
    init_rnd();

    key_table = NULL;
    force_button_shortcut_flag = false;

    file_io_buf     = NULL;
    save_data_buf   = NULL;
    file_io_buf_ptr = 0;
    file_io_buf_len = 0;
    save_data_len   = 0;

    text_buffer = NULL;

    syscall_dict["skip"]        = SYSTEM_SKIP;
    syscall_dict["reset"]       = SYSTEM_RESET;
    syscall_dict["save"]        = SYSTEM_SAVE;
    syscall_dict["load"]        = SYSTEM_LOAD;
    syscall_dict["lookback"]    = SYSTEM_LOOKBACK;
    syscall_dict["windowerase"] = SYSTEM_WINDOWERASE;
    syscall_dict["rmenu"]       = SYSTEM_MENU;
    syscall_dict["automode"]    = SYSTEM_AUTOMODE;
    syscall_dict["end"]         = SYSTEM_END;
}


ScriptParser::~ScriptParser()
{
    reset();
    if (file_io_buf) delete[] file_io_buf;
    if (save_data_buf) delete[] save_data_buf;
}


void ScriptParser::reset()
{
    user_func_lut.clear();

    // reset misc variables
    nsa_path.trunc(0);

    globalon_flag   = false;
    labellog_flag   = false;
    filelog_flag    = false;
    kidokuskip_flag = false;

    rmode_flag = true;
    windowback_flag = false;
    usewheel_flag  = false;
    useescspc_flag = false;
    mode_saya_flag = false;
    mode_ext_flag  = false;
    zenkakko_flag  = false;
    string_buffer_offset = 0;
    string_buffer_restore = -1;

    break_flag = false;
    trans_mode = AnimationInfo::TRANS_TOPLEFT;

    version_str = VERSION_STR1 "\n" VERSION_STR2 "\n";
    z_order = 499;

    textgosub_label.trunc(0);
    pretextgosub_label.trunc(0);
    loadgosub_label.trunc(0);

    /* ---------------------------------------- */
    /* Lookback related variables */
    lookback_sp[0]    = lookback_sp[1] = -1;
    lookback_color.set(0xff, 0xff, 0x00);

    /* ---------------------------------------- */
    /* Save/Load related variables */
    save_menu_name = DEFAULT_SAVE_MENU_NAME;
    load_menu_name = DEFAULT_LOAD_MENU_NAME;
    save_item_name = DEFAULT_SAVE_ITEM_NAME;
    num_save_file = 9;

    /* ---------------------------------------- */
    /* Text related variables */
    sentence_font.reset();
    menu_font.reset();

    current_font = &sentence_font;
    shade_distance[0] = 1;
    shade_distance[1] = 1;

    default_text_speed[0] = DEFAULT_TEXT_SPEED_LOW;
    default_text_speed[1] = DEFAULT_TEXT_SPEED_MIDDLE;
    default_text_speed[2] = DEFAULT_TEXT_SPEED_HIGHT;
    max_text_buffer = MAX_TEXT_BUFFER;
    num_chars_in_sentence = 0;
    if (text_buffer) {
        delete[] text_buffer;
        text_buffer = NULL;
    }

    current_text_buffer = start_text_buffer = NULL;

    clickstr_line  = 0;
    clickstr_state = CLICK_NONE;
    linepage_mode  = 0;

    /* ---------------------------------------- */
    /* Sound related variables */
    int i;
    for (i = 0; i < CLICKVOICE_NUM; i++)
	clickvoice_file_name[i].trunc(0);
    for (i = 0; i < SELECTVOICE_NUM; i++)
	selectvoice_file_name[i].trunc(0);
    for (i = 0; i < MENUSELECTVOICE_NUM; i++)
	menuselectvoice_file_name[i].trunc(0);

    /* ---------------------------------------- */
    /* Menu related variables */
    menu_font.set_size(DEFAULT_FONT_SIZE);
    menu_font.set_mod_size(0);
    menu_font.top_x   = 0;
    menu_font.top_y   = 16;
    menu_font.area_x  = 32 * DEFAULT_FONT_SIZE;
    menu_font.area_y  = 23;
    menu_font.pitch_x = 0;
    menu_font.pitch_y = 0;
    menu_font.window_color.set(0xcc);
    rmenu.clear();
    rmenu_link_width = 0;

    /* ---------------------------------------- */
    /* Effect related variables */
    effect_blank    = 10;
    effect_cut_flag = false;

    window_effect.effect   = 1;
    window_effect.duration = 0;
    effects.clear();

    script_h.label_log.read(script_h);

    current_mode = DEFINE_MODE;
}


int ScriptParser::open(const char* preferred_script)
{
    ScriptHandler::cBR = new DirectReader(&archive_path, key_table);
    ScriptHandler::cBR->open();

    if (script_h.readScript(&archive_path, preferred_script)) return -1;

    switch (script_h.screen_size) {
    case ScriptHandler::SCREEN_SIZE_800x600:
#ifdef PDA
        screen_ratio1 = 2;
        screen_ratio2 = 5;
#else
        screen_ratio1 = 1;
        screen_ratio2 = 1;
#endif
        screen_width  = 800 * screen_ratio1 / screen_ratio2;
        screen_height = 600 * screen_ratio1 / screen_ratio2;
        break;
    case ScriptHandler::SCREEN_SIZE_400x300:
#ifdef PDA
        screen_ratio1 = 4;
        screen_ratio2 = 5;
#else
        screen_ratio1 = 1;
        screen_ratio2 = 1;
#endif
        screen_width  = 400 * screen_ratio1 / screen_ratio2;
        screen_height = 300 * screen_ratio1 / screen_ratio2;
        break;
    case ScriptHandler::SCREEN_SIZE_320x240:
        screen_ratio1 = 1;
        screen_ratio2 = 1;
        screen_width  = 320 * screen_ratio1 / screen_ratio2;
        screen_height = 240 * screen_ratio1 / screen_ratio2;
        break;
    case ScriptHandler::SCREEN_SIZE_640x480:
    default:
#ifdef PDA
        screen_ratio1 = 1;
        screen_ratio2 = 2;
#else
        screen_ratio1 = 1;
        screen_ratio2 = 1;
#endif
        screen_width  = 640 * screen_ratio1 / screen_ratio2;
        screen_height = 480 * screen_ratio1 / screen_ratio2;
        break;
    }

    return 0;
}


unsigned char ScriptParser::convHexToDec(char ch)
{
    if ('0' <= ch && ch <= '9') return ch - '0';
    else if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    else if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
        else errorAndExit("convHexToDec: not valid character for color.");

    return 0;
}


rgb_t ScriptParser::readColour(const char* buf)
{
    if (buf[0] != '#') errorAndExit("readColour: no preceeding #.");
    return rgb_t(convHexToDec(buf[1]) << 4 | convHexToDec(buf[2]),
		  convHexToDec(buf[3]) << 4 | convHexToDec(buf[4]),
		  convHexToDec(buf[5]) << 4 | convHexToDec(buf[6]));
}


int ScriptParser::parseLine()
{
    pstring cmd = script_h.getStrBuf();
    if (debug_level > 1) {
        printf("ScriptParser::Parseline %s\n", (const char*) cmd);
        fflush(stdout);
    }

    if (script_h.isText()) return RET_NOMATCH;

    if (cmd[0] == ';' || cmd[0] == '*' || cmd[0] == ':' || cmd[0] == 0x0a)
	return RET_CONTINUE;

    if (cmd[0] != '_') {
	if (user_func_lut.find(cmd) != user_func_lut.end()) {
	    gosubReal(cmd, script_h.getNext());
	    return RET_CONTINUE;
	}
    }
    else {
	cmd.remove(0, 1);
    }
    ParserFun f = func_lut.get(cmd);
    return f ? (this->*f)(cmd) : RET_NOMATCH;
}


int ScriptParser::getSystemCallNo(const pstring& buffer)
{
    syscall_dict_t::iterator e = syscall_dict.find(buffer);
    if (e != syscall_dict.end()) {
	return e->second;
    }
    else {
        printf("Unsupported system call %s\n", (const char*) buffer);
        return -1;
    }
}


void ScriptParser::saveGlobalData()
{
    if (!globalon_flag) return;

    file_io_buf_ptr = 0;
    writeVariables(script_h.global_variable_border, VARIABLE_RANGE, false);
    allocFileIOBuf();
    writeVariables(script_h.global_variable_border, VARIABLE_RANGE, true);

    if (saveFileIOBuf("global.sav")) {
        fprintf(stderr, "can't open global.sav for writing\n");
        exit(-1);
    }
}


void ScriptParser::allocFileIOBuf()
{
    if (file_io_buf_ptr > file_io_buf_len) {
        file_io_buf_len = file_io_buf_ptr;
        if (file_io_buf) delete[] file_io_buf;

        file_io_buf = new unsigned char[file_io_buf_len];

        if (save_data_buf) {
            memcpy(file_io_buf, save_data_buf, save_data_len);
            delete[] save_data_buf;
        }

        save_data_buf = new unsigned char[file_io_buf_len];
        memcpy(save_data_buf, file_io_buf, save_data_len);
    }

    file_io_buf_ptr = 0;
}


int ScriptParser::saveFileIOBuf(const pstring& filename, int offset,
                                const char* savestr)
{
    FILE* fp;
    if ((fp = fopen(script_h.save_path + filename, "wb")) == NULL)
	return -1;

    size_t ret = fwrite(file_io_buf + offset, 1, file_io_buf_ptr - offset, fp);

    if (savestr){
        size_t savelen = strlen(savestr);
        if ((fputc('"', fp) == EOF)
            || (fwrite(savestr, 1, savelen, fp) != savelen)
            || (fputs("\"*", fp) == EOF))
            fprintf(stderr, "Warning: error writing to %s\n", (const char*)filename);
    }

    fclose(fp);

    if (ret != file_io_buf_ptr - offset) return -2;

    return 0;
}


int ScriptParser::loadFileIOBuf(const pstring& filename)
{
    FILE* fp;
    if ((fp = fopen(script_h.save_path + filename, "rb")) == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    file_io_buf_ptr = len;
    allocFileIOBuf();

    fseek(fp, 0, SEEK_SET);
    size_t ret = fread(file_io_buf, 1, len, fp);
    fclose(fp);

    if (ret != len) return -2;

    return 0;
}


void ScriptParser::writeChar(char c, bool output_flag)
{
    if (output_flag)
        file_io_buf[file_io_buf_ptr] = (unsigned char) c;

    file_io_buf_ptr++;
}


char ScriptParser::readChar()
{
    if (file_io_buf_ptr >= file_io_buf_len) return 0;

    return (char) file_io_buf[file_io_buf_ptr++];
}


void ScriptParser::writeInt(int i, bool output_flag)
{
    if (output_flag) {
        file_io_buf[file_io_buf_ptr++] = i & 0xff;
        file_io_buf[file_io_buf_ptr++] = (i >> 8) & 0xff;
        file_io_buf[file_io_buf_ptr++] = (i >> 16) & 0xff;
        file_io_buf[file_io_buf_ptr++] = (i >> 24) & 0xff;
    }
    else {
        file_io_buf_ptr += 4;
    }
}


int ScriptParser::readInt()
{
    if (file_io_buf_ptr + 3 >= file_io_buf_len) return 0;

    int i =
        (unsigned int) file_io_buf[file_io_buf_ptr + 3] << 24 |
        (unsigned int) file_io_buf[file_io_buf_ptr + 2] << 16 |
        (unsigned int) file_io_buf[file_io_buf_ptr + 1] << 8 |
        (unsigned int) file_io_buf[file_io_buf_ptr];
    file_io_buf_ptr += 4;

    return i;
}

void ScriptParser::writeStr(const pstring& s, bool output_flag)
{
    if (s) {
	if (output_flag)
	    memcpy(file_io_buf + file_io_buf_ptr, (const char*) s, s.length());

	file_io_buf_ptr += s.length();
    }

    writeChar(0, output_flag);
}


pstring ScriptParser::readStr()
{
    pstring rv((const char*) file_io_buf + file_io_buf_ptr);
    file_io_buf_ptr += rv.length() + 1;
    return rv;
}


void ScriptParser::writeVariables(int from, int to, bool output_flag)
{
    for (int i = from; i < to; i++) {
        writeInt(script_h.variable_data[i].get_num(), output_flag);
        writeStr(script_h.variable_data[i].str, output_flag);
    }
}


void ScriptParser::readVariables(int from, int to)
{
    for (int i = from; i < to; i++) {
        script_h.variable_data[i].set_num(readInt());
        script_h.variable_data[i].str = readStr();
    }
}


void ScriptParser::writeArrayVariable(bool output_flag)
{
    ScriptHandler::ArrayVariable::iterator it = script_h.arrays.begin();
    while (it != script_h.arrays.end()) {
        for (h_index_t::iterator d = it->second.begin();
	     d != it->second.end(); ++d) {
            unsigned long ch = *d;
            if (output_flag) {
                file_io_buf[file_io_buf_ptr + 3] = (unsigned char) ((ch >> 24) & 0xff);
                file_io_buf[file_io_buf_ptr + 2] = (unsigned char) ((ch >> 16) & 0xff);
                file_io_buf[file_io_buf_ptr + 1] = (unsigned char) ((ch >> 8) & 0xff);
                file_io_buf[file_io_buf_ptr] = (unsigned char) (ch & 0xff);
            }

            file_io_buf_ptr += 4;
        }
        ++it;
    }
}


void ScriptParser::readArrayVariable()
{
    ScriptHandler::ArrayVariable::iterator it = script_h.arrays.begin();
    while (it != script_h.arrays.end()) {
        for (h_index_t::iterator d = it->second.begin();
	     d != it->second.end(); ++d) {
            unsigned long ret;
            if (file_io_buf_ptr + 3 >= file_io_buf_len) return;

            ret = file_io_buf[file_io_buf_ptr + 3];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr + 2];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr + 1];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr];
            file_io_buf_ptr += 4;
            *d = ret;
        }
        ++it;
    }
}


void ScriptParser::errorAndCont(const char* why, const char* reason)
{
    fprintf(stderr, "Error at line %d: %s",
	    script_h.getLineByAddress(script_h.getCurrent(), true), why);
    if (reason) fprintf(stderr, "; %s", reason);
    fprintf(stderr, "\n(*%s line %d)\n",
	    (const char*) current_label_info.name, current_line);
}

void ScriptParser::errorAndExit(const char* why, const char* reason)
{
    errorAndCont(why, reason);
    exit(-1);
}

void ScriptParser::setCurrentLabel(const pstring& label)
{
    current_label_info = script_h.lookupLabel(label);
    current_line = script_h.getLineByAddress(current_label_info.start_address);
//cerr << "Jump to *" + label + " ("
//  + nstr(script_h.getOffset(current_label_info.start_address), 8, 1, 16)
//  + ")" << eol;
    script_h.setCurrent(current_label_info.start_address);
}


void ScriptParser::readToken()
{
    script_h.readToken();
    if (script_h.isText()) string_buffer_offset = 0;

    if (script_h.isText() && linepage_mode > 0) {
        char ch = (linepage_mode == 1) ? '\\' : '@';

        // ugly work around
	pstring& s = script_h.getStrBuf();
	if (s && s[s.length() - 1] == '\n') {
	    s.insertchrs(s.length() - 1, 1, ch);
	}
	else {
	    s += ch;
	}
    }
}


int ScriptParser::readEffect(Effect& effect)
{
    int num = 1;

    effect.effect = script_h.readIntValue();
    if (script_h.hasMoreArgs()) {
        ++num;
        effect.duration = script_h.readIntValue();
        if (script_h.hasMoreArgs()) {
            ++num;
            effect.anim.setImageName(script_h.readStrValue());
        }
        else
            effect.anim.remove();
    }
    else if (effect.effect < 0 || effect.effect > 255) {
        fprintf(stderr, "Effect %d out of range: using 0.\n", effect.effect);
        effect.effect = 0; // to suppress error
    }
    return num;
}


ScriptParser::Effect& ScriptParser::parseEffect(bool init_flag)
{
    if (init_flag) tmp_effect.anim.remove();
    
    int num = readEffect(tmp_effect);

    if (num > 1) return tmp_effect;

    if (tmp_effect.effect == 0 || tmp_effect.effect == 1) return tmp_effect;

    for (Effect::iterator it = effects.begin(); it != effects.end(); ++it)
        if (*it && ((*it)->no == tmp_effect.effect)) return **it;

    fprintf(stderr, "Effect No. %d is not found.\n", tmp_effect.effect);
    exit(-1);
}


void ScriptParser::createKeyTable(const pstring& key_exe)
{
    if (!key_exe) return;

    FILE* fp = NULL;
    int n=0;
    while ((fp == NULL) && (n<archive_path.get_num_paths())) {
        pstring path = archive_path.get_path(n++) + key_exe;
        fp = fopen(path, "rb");
    }
    if (fp == NULL) {
        fprintf(stderr, "createKeyTable: can't open EXE file %s\n",
		(const char*) key_exe);
        return;
    }

    key_table = new unsigned char[256];

    int i;
    for (i = 0; i < 256; i++) key_table[i] = i;

    unsigned char ring_buffer[256];
    int ring_start = 0, ring_last = 0;

    int ch, count;
    while ((ch = fgetc(fp)) != EOF) {
        i = ring_start;
        count = 0;
        while (i != ring_last
               && ring_buffer[i] != ch) {
            count++;
            i = (i + 1) % 256;
        }
        if (i == ring_last && count == 255) break;

        if (i != ring_last)
            ring_start = (i + 1) % 256;

        ring_buffer[ring_last] = ch;
        ring_last = (ring_last + 1) % 256;
    }
    fclose(fp);

    if (ch == EOF)
        errorAndExit("createKeyTable: can't find a key table.");

    // Key table creation
    ring_buffer[ring_last] = ch;
    for (i = 0; i < 256; i++)
        key_table[ring_buffer[(ring_start + i) % 256]] = i;
}

void ScriptParser::TextBuffer_dumpstate(int num)
{
    if (num == 0) num = MAX_INT;
    TextBuffer* t_buf = current_text_buffer;
    int n = 0;
    while (t_buf != start_text_buffer) {
        t_buf = t_buf->previous;
	++n;
    }
    puts("Text buffer status:");
    t_buf = 0;
    while (t_buf != start_text_buffer && num--) {
        t_buf = t_buf ? t_buf->previous : current_text_buffer;
	t_buf->dumpstate(n--);
    }
}

void ScriptParser::TextBuffer::dumpstate(int n)
{
    if (n >= 0) printf(" %d:", n);
    printf("%3d [", contents.length());
    print_escaped(contents);
    puts("]");
}

