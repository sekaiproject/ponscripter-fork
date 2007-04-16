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

typedef int (ScriptParser::* FuncList)();
static struct FuncLUT {
    char command[40];
    FuncList method;
} func_lut[] =
{
    { "zenkakko", &ScriptParser::zenkakkoCommand },
    { "windoweffect", &ScriptParser::effectCommand },
    { "windowback", &ScriptParser::windowbackCommand },
    { "versionstr", &ScriptParser::versionstrCommand },
    { "usewheel", &ScriptParser::usewheelCommand },
    { "useescspc", &ScriptParser::useescspcCommand },
    { "underline", &ScriptParser::underlineCommand },
    { "transmode", &ScriptParser::transmodeCommand },
    { "time", &ScriptParser::timeCommand },
    { "textgosub", &ScriptParser::textgosubCommand },
    { "tan", &ScriptParser::tanCommand },
    { "sub", &ScriptParser::subCommand },
    { "stralias", &ScriptParser::straliasCommand },
    { "spi", &ScriptParser::soundpressplginCommand },
    { "soundpressplgin", &ScriptParser::soundpressplginCommand },
    { "skip", &ScriptParser::skipCommand },
    { "sin", &ScriptParser::sinCommand },
    { "shadedistance", &ScriptParser::shadedistanceCommand },
    { "selectvoice", &ScriptParser::selectvoiceCommand },
    { "selectcolor", &ScriptParser::selectcolorCommand },
    { "savenumber", &ScriptParser::savenumberCommand },
    { "savename", &ScriptParser::savenameCommand },
    { "sar", &ScriptParser::nsaCommand },
    { "rubyon", &ScriptParser::rubyonCommand },
    { "rubyoff", &ScriptParser::rubyoffCommand },
    { "roff", &ScriptParser::roffCommand },
    { "rmenu", &ScriptParser::rmenuCommand },
    { "return", &ScriptParser::returnCommand },
    { "pretextgosub", &ScriptParser::pretextgosubCommand },
    { "numalias", &ScriptParser::numaliasCommand },
    { "nsadir", &ScriptParser::nsadirCommand },
    { "nsa", &ScriptParser::nsaCommand },
    { "notif", &ScriptParser::ifCommand },
    { "next", &ScriptParser::nextCommand },
    { "nsa", &ScriptParser::arcCommand },
    { "ns3", &ScriptParser::nsaCommand },
    { "ns2", &ScriptParser::nsaCommand },
    { "mul", &ScriptParser::mulCommand },
    { "movl", &ScriptParser::movCommand },
    { "mov10", &ScriptParser::movCommand },
    { "mov9", &ScriptParser::movCommand },
    { "mov8", &ScriptParser::movCommand },
    { "mov7", &ScriptParser::movCommand },
    { "mov6", &ScriptParser::movCommand },
    { "mov5", &ScriptParser::movCommand },
    { "mov4", &ScriptParser::movCommand },
    { "mov3", &ScriptParser::movCommand },
    { "mov", &ScriptParser::movCommand },
    { "mode_saya", &ScriptParser::mode_sayaCommand },
    { "mode_ext", &ScriptParser::mode_extCommand },
    { "mod", &ScriptParser::modCommand },
    { "mid", &ScriptParser::midCommand },
    { "menusetwindow", &ScriptParser::menusetwindowCommand },
    { "menuselectvoice", &ScriptParser::menuselectvoiceCommand },
    { "menuselectcolor", &ScriptParser::menuselectcolorCommand },
    { "maxkaisoupage", &ScriptParser::maxkaisoupageCommand },
    { "lookbacksp", &ScriptParser::lookbackspCommand },
    { "lookbackcolor", &ScriptParser::lookbackcolorCommand },
    //{"lookbackbutton",      &ScriptParser::lookbackbuttonCommand},
    { "loadgosub", &ScriptParser::loadgosubCommand },
    { "linepage2", &ScriptParser::linepageCommand },
    { "linepage", &ScriptParser::linepageCommand },
    { "len", &ScriptParser::lenCommand },
    { "labellog", &ScriptParser::labellogCommand },
    { "kidokuskip", &ScriptParser::kidokuskipCommand },
    { "kidokumode", &ScriptParser::kidokumodeCommand },
    { "itoa2", &ScriptParser::itoaCommand },
    { "itoa", &ScriptParser::itoaCommand },
    { "intlimit", &ScriptParser::intlimitCommand },
    { "inc", &ScriptParser::incCommand },
    { "if", &ScriptParser::ifCommand },
    { "humanz", &ScriptParser::humanzCommand },
    { "goto", &ScriptParser::gotoCommand },
    { "gosub", &ScriptParser::gosubCommand },
    { "globalon", &ScriptParser::globalonCommand },
    { "getparam", &ScriptParser::getparamCommand },
    //{"game",    &ScriptParser::gameCommand},
    { "for", &ScriptParser::forCommand },
    { "filelog", &ScriptParser::filelogCommand },
    { "effectcut", &ScriptParser::effectcutCommand },
    { "effectblank", &ScriptParser::effectblankCommand },
    { "effect", &ScriptParser::effectCommand },
    { "div", &ScriptParser::divCommand },
    { "dim", &ScriptParser::dimCommand },
    { "defvoicevol", &ScriptParser::defvoicevolCommand },
    { "defsub", &ScriptParser::defsubCommand },
    { "defsevol", &ScriptParser::defsevolCommand },
    { "defmp3vol", &ScriptParser::defmp3volCommand },
    { "defaultspeed", &ScriptParser::defaultspeedCommand },
    { "defaultfont", &ScriptParser::defaultfontCommand },
    { "dec", &ScriptParser::decCommand },
    { "date", &ScriptParser::dateCommand },
    { "cos", &ScriptParser::cosCommand },
    { "cmp", &ScriptParser::cmpCommand },
    { "clickvoice", &ScriptParser::clickvoiceCommand },
    { "clickstr", &ScriptParser::clickstrCommand },
    { "break", &ScriptParser::breakCommand },
    { "automode", &ScriptParser::mode_extCommand },
    { "atoi", &ScriptParser::atoiCommand },
    { "arc", &ScriptParser::arcCommand },
    { "add", &ScriptParser::addCommand },
    { "", NULL }
};

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
    UserFuncLUT* func = root_user_func.next;
    while (func) {
        UserFuncLUT* tmp = func;
        func = func->next;
        delete tmp;
    }
    root_user_func.next = NULL;
    last_user_func = &root_user_func;

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
    lookback_color[0] = 0xff;
    lookback_color[1] = 0xff;
    lookback_color[2] = 0x00;

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
    menu_font.window_color[0] = menu_font.window_color[1] = menu_font.window_color[2] = 0xcc;

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


void ScriptParser::readColor(uchar3* color, const char* buf)
{
    if (buf[0] != '#') errorAndExit("readColor: no preceeding #.");

    (*color)[0] = convHexToDec(buf[1]) << 4 | convHexToDec(buf[2]);
    (*color)[1] = convHexToDec(buf[3]) << 4 | convHexToDec(buf[4]);
    (*color)[2] = convHexToDec(buf[5]) << 4 | convHexToDec(buf[6]);
}


int ScriptParser::parseLine()
{
    if (debug_level > 0) printf("ScriptParser::Parseline %s\n", script_h.getStringBuffer());

    if (script_h.getStringBuffer()[0] == ';') return RET_CONTINUE;
    else if (script_h.getStringBuffer()[0] == '*') return RET_CONTINUE;
    else if (script_h.getStringBuffer()[0] == ':') return RET_CONTINUE;
    else if (script_h.isText()) return RET_NOMATCH;

    const char* cmd = script_h.getStringBuffer();
    if (cmd[0] != '_') {
        UserFuncLUT* uf = root_user_func.next;
        while (uf) {
            if (!strcmp(uf->command, cmd)) {
                gosubReal(cmd, script_h.getNext());
                return RET_CONTINUE;
            }

            uf = uf->next;
        }
    }
    else {
        cmd++;
    }

    int lut_counter = 0;
    while (func_lut[lut_counter].method) {
        if (!strcmp(func_lut[lut_counter].command, cmd)) {
            return (this->*func_lut[lut_counter].method)();
        }

        lut_counter++;
    }

    return RET_NOMATCH;
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


void ScriptParser::writeStr(char* s, bool output_flag)
{
    if (s && s[0]) {
        if (output_flag)
            memcpy(file_io_buf + file_io_buf_ptr,
                s,
                strlen(s));

        file_io_buf_ptr += strlen(s);
    }

    writeChar(0, output_flag);
}


void ScriptParser::readStr(char** s)
{
    int counter = 0;

    while (file_io_buf_ptr + counter < file_io_buf_len) {
        if (file_io_buf[file_io_buf_ptr + counter++] == 0) break;
    }

    if (*s) delete[] * s;

    *s = NULL;

    if (counter > 1) {
        *s = new char[counter];
        memcpy(*s, file_io_buf + file_io_buf_ptr, counter);
    }

    file_io_buf_ptr += counter;
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
        readStr(&script_h.variable_data[i].str);
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
        unsigned int len = strlen(script_h.getStringBuffer());
        if (script_h.getStringBuffer()[len - 1] == 0x0a) {
            script_h.getStringBuffer()[len - 1] = ch;
            script_h.addStringBuffer(0x0a);
        }
        else {
            script_h.addStringBuffer(ch);
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
