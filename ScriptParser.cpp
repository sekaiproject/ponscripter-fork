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

typedef int (ScriptParser::*ParserFun)(const string&);
static class func_lut_t {
    typedef dictionary<string, ParserFun>::t dic_t;
    dic_t dict;
public:
    func_lut_t();
    ParserFun get(string what) const {
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
  //dict["game"]            = &ScriptParser::gameCommand;
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
  //dict["lookbackbutton]   = &ScriptParser::lookbackbuttonCommand;
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
    dict["windowback"]      = &ScriptParser::windowbackCommand;
    dict["windoweffect"]    = &ScriptParser::effectCommand;
    dict["zenkakko"]        = &ScriptParser::zenkakkoCommand;
}


ScriptParser::ScriptParser()
{
    debug_level = 0;
    srand(time(NULL));

    key_table = NULL;
    force_button_shortcut_flag = false;

    file_io_buf     = NULL;
    save_data_buf   = NULL;
    file_io_buf_ptr = 0;
    file_io_buf_len = 0;
    save_data_len   = 0;

    text_buffer = NULL;
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
    nsa_path.clear();

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

    break_flag = false;
    trans_mode = AnimationInfo::TRANS_TOPLEFT;

    version_str = VERSION_STR1 "\n" VERSION_STR2 "\n";
    z_order = 499;

    textgosub_label.clear();
    pretextgosub_label.clear();
    loadgosub_label.clear();

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
	clickvoice_file_name[i].clear();
    for (i = 0; i < SELECTVOICE_NUM; i++)
	selectvoice_file_name[i].clear();
    for (i = 0; i < MENUSELECTVOICE_NUM; i++)
	menuselectvoice_file_name[i].clear();

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

    deleteRMenuLink();

    /* ---------------------------------------- */
    /* Effect related variables */
    effect_blank    = 10;
    effect_cut_flag = false;

    window_effect.effect   = 1;
    window_effect.duration = 0;
    root_effect_link.no = 0;
    root_effect_link.effect   = 0;
    root_effect_link.duration = 0;

    EffectLink* link = root_effect_link.next;
    while (link) {
        EffectLink* tmp = link;
        link = link->next;
        delete tmp;
    }
    last_effect_link = &root_effect_link;
    last_effect_link->next = NULL;

    readLog(script_h.log_info[ScriptHandler::LABEL_LOG]);

    current_mode = DEFINE_MODE;
}


int ScriptParser::open()
{
    ScriptHandler::cBR = new DirectReader(archive_path.c_str(), key_table);
    ScriptHandler::cBR->open();

    if (script_h.readScript(archive_path.c_str())) return -1;

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
    string cmd = script_h.getStringBuffer();
    if (debug_level > 0) printf("ScriptParser::Parseline %s\n", cmd.c_str());

    if (cmd[0] == ';' || cmd[0] == '*' || cmd[0] == ':') return RET_CONTINUE;

    if (script_h.isText()) return RET_NOMATCH;

    if (cmd[0] != '_') {
	if (user_func_lut.find(cmd) != user_func_lut.end()) {
	    gosubReal(cmd, script_h.getNext());
	    return RET_CONTINUE;
	}
    }
    else {
	cmd.shift();
    }
    ParserFun f = func_lut.get(cmd);
    return f ? (this->*f)(cmd) : RET_NOMATCH;
}


void ScriptParser::deleteRMenuLink()
{
    RMenuLink* link = root_rmenu_link.next;
    while (link) {
        RMenuLink* tmp = link;
        link = link->next;
        delete tmp;
    }
    root_rmenu_link.next = NULL;

    rmenu_link_num   = 0;
    rmenu_link_width = 0;
}


int ScriptParser::getSystemCallNo(const char* buffer)
{
    if (!strcmp(buffer, "skip")) return SYSTEM_SKIP;
    else if (!strcmp(buffer, "reset")) return SYSTEM_RESET;
    else if (!strcmp(buffer, "save")) return SYSTEM_SAVE;
    else if (!strcmp(buffer, "load")) return SYSTEM_LOAD;
    else if (!strcmp(buffer, "lookback")) return SYSTEM_LOOKBACK;
    else if (!strcmp(buffer, "windowerase")) return SYSTEM_WINDOWERASE;
    else if (!strcmp(buffer, "rmenu")) return SYSTEM_MENU;
    else if (!strcmp(buffer, "automode")) return SYSTEM_AUTOMODE;
    else if (!strcmp(buffer, "end")) return SYSTEM_END;
    else {
        printf("Unsupported system call %s\n", buffer);
        return -1;
    }
}


void ScriptParser::saveGlovalData()
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


int ScriptParser::saveFileIOBuf(const char* filename, int offset)
{
    FILE* fp;
    if ((fp = fopen(filename, "wb", true)) == NULL) return -1;

    size_t ret = fwrite(file_io_buf + offset, 1, file_io_buf_ptr - offset, fp);
    fclose(fp);

    if (ret != file_io_buf_ptr - offset) return -2;

    return 0;
}


int ScriptParser::loadFileIOBuf(const char* filename)
{
    FILE* fp;
    if ((fp = fopen(filename, "rb", true)) == NULL)
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

void ScriptParser::writeStr(const string& s, bool output_flag)
{
    if (s) {
	if (output_flag)
	    memcpy(file_io_buf + file_io_buf_ptr, s.c_str(), s.size());

	file_io_buf_ptr += s.size();
    }

    writeChar(0, output_flag);
}


void ScriptParser::readStr(char** s)
{
    int counter = 0;

    while (file_io_buf_ptr + counter < file_io_buf_len) {
        if (file_io_buf[file_io_buf_ptr + counter++] == 0) break;
    }

    if (*s) delete[] *s;

    *s = NULL;

    if (counter > 1) {
        *s = new char[counter];
        memcpy(*s, file_io_buf + file_io_buf_ptr, counter);
    }

    file_io_buf_ptr += counter;
}

string ScriptParser::readStr()
{
    const size_t start = file_io_buf_ptr;
    while (file_io_buf_ptr < file_io_buf_len) {
	if (file_io_buf[file_io_buf_ptr++] == 0) break;
    }
    return string((char*)file_io_buf + start, file_io_buf_ptr - start);
}


void ScriptParser::writeVariables(int from, int to, bool output_flag)
{
    for (int i = from; i < to; i++) {
        writeInt(script_h.variable_data[i].num, output_flag);
        writeStr(script_h.variable_data[i].str, output_flag);
    }
}


void ScriptParser::readVariables(int from, int to)
{
    for (int i = from; i < to; i++) {
        script_h.variable_data[i].num = readInt();
        script_h.variable_data[i].str = readStr();
    }
}


void ScriptParser::writeArrayVariable(bool output_flag)
{
    ScriptHandler::ArrayVariable* av = script_h.getRootArrayVariable();

    while (av) {
        int i, dim = 1;
        for (i = 0; i < av->num_dim; i++)
            dim *= av->dim[i];

        for (i = 0; i < dim; i++) {
            unsigned long ch = av->data[i];
            if (output_flag) {
                file_io_buf[file_io_buf_ptr + 3] = (unsigned char) ((ch >> 24) & 0xff);
                file_io_buf[file_io_buf_ptr + 2] = (unsigned char) ((ch >> 16) & 0xff);
                file_io_buf[file_io_buf_ptr + 1] = (unsigned char) ((ch >> 8) & 0xff);
                file_io_buf[file_io_buf_ptr] = (unsigned char) (ch & 0xff);
            }

            file_io_buf_ptr += 4;
        }

        av = av->next;
    }
}


void ScriptParser::readArrayVariable()
{
    ScriptHandler::ArrayVariable* av = script_h.getRootArrayVariable();

    while (av) {
        int i, dim = 1;
        for (i = 0; i < av->num_dim; i++)
            dim *= av->dim[i];

        for (i = 0; i < dim; i++) {
            unsigned long ret;
            if (file_io_buf_ptr + 3 >= file_io_buf_len) return;

            ret = file_io_buf[file_io_buf_ptr + 3];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr + 2];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr + 1];
            ret = ret << 8 | file_io_buf[file_io_buf_ptr];
            file_io_buf_ptr += 4;
            av->data[i] = ret;
        }

        av = av->next;
    }
}


void ScriptParser::writeLog(ScriptHandler::LogInfo &info)
{
    file_io_buf_ptr = 0;
    bool output_flag = false;
    for (int n = 0; n < 2; n++) {
        int  i, j;
        char buf[10];

        sprintf(buf, "%d", info.num_logs);
        for (i = 0; i < (int) strlen(buf); i++) writeChar(buf[i], output_flag);

        writeChar(0x0a, output_flag);

        ScriptHandler::LogLink* cur = info.root_log.next;
        for (i = 0; i < info.num_logs; i++) {
            writeChar('"', output_flag);
            for (j = 0; j < (int) strlen(cur->name); j++)
                writeChar(cur->name[j] ^ 0x84, output_flag);

            writeChar('"', output_flag);
            cur = cur->next;
        }

        if (n == 1) break;

        allocFileIOBuf();
        output_flag = true;
    }

    if (saveFileIOBuf(info.filename)) {
        fprintf(stderr, "can't write %s\n", info.filename);
        exit(-1);
    }
}


void ScriptParser::readLog(ScriptHandler::LogInfo &info)
{
    script_h.resetLog(info);

    if (loadFileIOBuf(info.filename) == 0) {
        int  i, j, ch, count = 0;
        char buf[100];

        while ((ch = readChar()) != 0x0a) {
            count = count * 10 + ch - '0';
        }

        for (i = 0; i < count; i++) {
            readChar();
            j = 0;
            while ((ch = readChar()) != '"') buf[j++] = ch ^ 0x84;
            buf[j] = '\0';

            script_h.findAndAddLog(info, buf, true);
        }
    }
}


void ScriptParser::errorAndExit(const char* str, const char* reason)
{
    if (reason)
        fprintf(stderr, " *** Parse error at %s:%d [%s]; %s ***\n",
		current_label_info.name.c_str(),
		current_line,
		str, reason);
    else
        fprintf(stderr, " *** Parse error at %s:%d [%s] ***\n",
		current_label_info.name.c_str(),
		current_line,
		str);

    exit(-1);
}


void ScriptParser::deleteNestInfo()
{
    NestInfo* info = root_nest_info.next;
    while (info) {
        NestInfo* tmp = info;
        info = info->next;
        delete tmp;
    }
    root_nest_info.next = NULL;
    last_nest_info = &root_nest_info;
}


void ScriptParser::SET_STR(char** dst, const char* src, int num)
{
    if (*dst) delete[] * dst;

    *dst = NULL;

    if (src) {
        if (num >= 0) {
            *dst = new char[num + 1];
            memcpy(*dst, src, num);
            (*dst)[num] = '\0';
        }
        else {
            *dst = new char[strlen(src) + 1];
            strcpy(*dst, src);
        }
    }
}


void ScriptParser::setCurrentLabel(const string& label)
{
    current_label_info = script_h.lookupLabel(label);
    current_line = script_h.getLineByAddress(current_label_info.start_address);
    script_h.setCurrent(current_label_info.start_address);
}


void ScriptParser::readToken()
{
    script_h.readToken();
    if (script_h.isText()) string_buffer_offset = 0;

    if (script_h.isText() && linepage_mode > 0) {
        char ch = (linepage_mode == 1) ? '\\' : '@';

        // ugly work around
	string& s = script_h.getStringBuffer();
	if (s && s.back() == 0x0a) {
	    s.back() = ch;
	    s.push_back(0x0a);
	}
	else {
	    s.push_back(ch);
	}
    }
}


int ScriptParser::readEffect(EffectLink* effect)
{
    int num = 1;

    effect->effect = script_h.readInt();
    if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
        num++;
        effect->duration = script_h.readInt();
        if (script_h.getEndStatus() & ScriptHandler::END_COMMA) {
            num++;
            const char* buf = script_h.readStr();
            effect->anim.setImageName(buf);
        }
        else
            effect->anim.remove();
    }
    else if (effect->effect < 0 || effect->effect > 255) {
        fprintf(stderr, "Effect %d is out of range and is switched to 0.\n", effect->effect);
        effect->effect = 0; // to suppress error
    }

    //printf("readEffect %d: %d %d %s\n", num, effect->effect, effect->duration, effect->anim.image_name );
    return num;
}


ScriptParser::EffectLink* ScriptParser::parseEffect(bool init_flag)
{
    if (init_flag) tmp_effect.anim.remove();
    
    int num = readEffect(&tmp_effect);

    if (num > 1) return &tmp_effect;

    if (tmp_effect.effect == 0 || tmp_effect.effect == 1) return &tmp_effect;

    EffectLink* link = &root_effect_link;
    while (link) {
        if (link->no == tmp_effect.effect) return link;

        link = link->next;
    }

    fprintf(stderr, "Effect No. %d is not found.\n", tmp_effect.effect);
    exit(-1);

    return NULL;
}


FILE* ScriptParser::fopen(const char* path, const char* mode, const bool save)
{
    string filename = save ? script_h.save_path : archive_path;
    filename += path;
    return ::fopen(filename.c_str(), mode);
}


void ScriptParser::createKeyTable(const char* key_exe)
{
    if (!key_exe) return;

    FILE* fp = ::fopen(key_exe, "rb");
    if (fp == NULL) {
        fprintf(stderr, "createKeyTable: can't open EXE file %s\n", key_exe);
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
