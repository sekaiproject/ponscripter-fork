/* -*- C++ -*-
 *
 *  ScriptParser.h - Define block parser of Ponscripter
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

#ifndef __SCRIPT_PARSER_H__
#define __SCRIPT_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "ScriptHandler.h"
#include "NsaReader.h"
#include "DirectReader.h"
#include "AnimationInfo.h"
#include "FontInfo.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEFAULT_FONT_SIZE 26

#define DEFAULT_LOOKBACK_NAME0 "uoncur.bmp"
#define DEFAULT_LOOKBACK_NAME1 "uoffcur.bmp"
#define DEFAULT_LOOKBACK_NAME2 "doncur.bmp"
#define DEFAULT_LOOKBACK_NAME3 "doffcur.bmp"
#define DEFAULT_CURSOR0 "cursor0.bmp"
#define DEFAULT_CURSOR1 "cursor1.bmp"
#define DEFAULT_CURSOR_WAIT ":l/3,160,2;cursor0.bmp"
#define DEFAULT_CURSOR_NEWPAGE ":l/3,160,2;cursor1.bmp"

class ScriptParser {
public:
    ScriptParser();
    virtual ~ScriptParser();

    void reset();
    int open();
    int parseLine();

    FILE* fopen(const char* path, const char* mode, const bool save = false);
    FILE* fopen(const string& path, const char* mode, const bool save = false)
	{ return fopen(path.c_str(), mode, save); }
    void saveGlobalData();

    /* Command */
    int zenkakkoCommand(const string& cmd);
    int windowbackCommand(const string& cmd);
    int versionstrCommand(const string& cmd);
    int usewheelCommand(const string& cmd);
    int useescspcCommand(const string& cmd);
    int underlineCommand(const string& cmd);
    int transmodeCommand(const string& cmd);
    int timeCommand(const string& cmd);
    int textgosubCommand(const string& cmd);
    int tanCommand(const string& cmd);
    int subCommand(const string& cmd);
    int straliasCommand(const string& cmd);
    int soundpressplginCommand(const string& cmd);
    int skipCommand(const string& cmd);
    int sinCommand(const string& cmd);
    int shadedistanceCommand(const string& cmd);
    int selectvoiceCommand(const string& cmd);
    int selectcolorCommand(const string& cmd);
    int savenumberCommand(const string& cmd);
    int savenameCommand(const string& cmd);
    int rubyonCommand(const string& cmd);
    int rubyoffCommand(const string& cmd);
    int roffCommand(const string& cmd);
    int rmenuCommand(const string& cmd);
    int returnCommand(const string& cmd);
    int pretextgosubCommand(const string& cmd);
    int numaliasCommand(const string& cmd);
    int nsadirCommand(const string& cmd);
    int nsaCommand(const string& cmd);
    int nextCommand(const string& cmd);
    int mulCommand(const string& cmd);
    int movCommand(const string& cmd);
    int mode_sayaCommand(const string& cmd);
    int mode_extCommand(const string& cmd);
    int modCommand(const string& cmd);
    int midCommand(const string& cmd);
    int menusetwindowCommand(const string& cmd);
    int menuselectvoiceCommand(const string& cmd);
    int menuselectcolorCommand(const string& cmd);
    int maxkaisoupageCommand(const string& cmd);
    int lookbackspCommand(const string& cmd);
    int lookbackcolorCommand(const string& cmd);

    //int lookbackbuttonCommand(const string& cmd);
    int loadgosubCommand(const string& cmd);
    int linepageCommand(const string& cmd);
    int lenCommand(const string& cmd);
    int labellogCommand(const string& cmd);
    int kidokuskipCommand(const string& cmd);
    int kidokumodeCommand(const string& cmd);
    int itoaCommand(const string& cmd);
    int intlimitCommand(const string& cmd);
    int incCommand(const string& cmd);
    int ifCommand(const string& cmd);
    int humanzCommand(const string& cmd);
    int gotoCommand(const string& cmd);
    int gosubCommand(const string& cmd);
    int globalonCommand(const string& cmd);
    int getparamCommand(const string& cmd);

    //int gameCommand(const string& cmd);
    int forCommand(const string& cmd);
    int filelogCommand(const string& cmd);
    int effectcutCommand(const string& cmd);
    int effectblankCommand(const string& cmd);
    int effectCommand(const string& cmd);
    int divCommand(const string& cmd);
    int dimCommand(const string& cmd);
    int defvoicevolCommand(const string& cmd);
    int defsubCommand(const string& cmd);
    int defsevolCommand(const string& cmd);
    int defmp3volCommand(const string& cmd);
    int defaultspeedCommand(const string& cmd);
    int defaultfontCommand(const string& cmd);
    int decCommand(const string& cmd);
    int dateCommand(const string& cmd);
    int cosCommand(const string& cmd);
    int cmpCommand(const string& cmd);
    int clickvoiceCommand(const string& cmd);
    int clickstrCommand(const string& cmd);
    int breakCommand(const string& cmd);
    int atoiCommand(const string& cmd);
    int arcCommand(const string& cmd);
    int addCommand(const string& cmd);

protected:
    set<string>::t user_func_lut;

    struct NestInfo {
	typedef std::vector<NestInfo> vector;
	typedef vector::iterator iterator;
	
        enum { LABEL = 0, FOR = 1 } nest_mode;
        char* next_script; // used in gosub and for
        int var_no, to, step; // used in for

        NestInfo(char* ns = 0) : nest_mode(LABEL), next_script(ns) {}
    };
    NestInfo last_tilde; // CHECK: should this be NestInfo::iterator?
    NestInfo::vector nest_infos;
    void deleteNestInfo() { nest_infos.clear(); }

    enum { SYSTEM_NULL        = 0,
           SYSTEM_SKIP        = 1,
           SYSTEM_RESET       = 2,
           SYSTEM_SAVE        = 3,
           SYSTEM_LOAD        = 4,
           SYSTEM_LOOKBACK    = 5,
           SYSTEM_WINDOWERASE = 6,
           SYSTEM_MENU     = 7,
           SYSTEM_YESNO    = 8,
           SYSTEM_AUTOMODE = 9,
           SYSTEM_END = 10 };
    enum { RET_NOMATCH   = 0,
           RET_SKIP_LINE = 1,
           RET_CONTINUE  = 2,
           RET_WAIT   = 4,
           RET_NOREAD = 8,
           RET_REREAD = 16 };
    enum { CLICK_NONE    = 0,
           CLICK_WAIT    = 1,
           CLICK_NEWPAGE = 2,
           CLICK_IGNORE  = 3,
           CLICK_EOL = 4 };
    enum { NORMAL_MODE, DEFINE_MODE };
    int current_mode;
    int debug_level;

    string archive_path;
    string nsa_path;

    bool globalon_flag;
    bool labellog_flag;
    bool filelog_flag;
    bool kidokuskip_flag;
    bool kidokumode_flag;

    int  z_order;
    bool rmode_flag;
    bool windowback_flag;
    bool usewheel_flag;
    bool useescspc_flag;
    bool mode_saya_flag;
    bool mode_ext_flag;
    bool force_button_shortcut_flag;
    bool zenkakko_flag;

    int string_buffer_offset;

    ScriptHandler::LabelInfo current_label_info;
    int current_line;

    /* ---------------------------------------- */
    /* Global definitions */
    int    screen_width, screen_height;
    int    screen_texture_width, screen_texture_height;
    int    screen_bpp;
    string version_str;
    int    underline_value;

    void gosubReal(const string& label, char* next_script);
    void setCurrentLabel(const string& label);
    void readToken();

    /* ---------------------------------------- */
    /* Effect related variables */
    struct Effect {
        typedef std::vector<Effect> vector;
	typedef vector::iterator iterator;
        int no;
        int effect;
        int duration;
        AnimationInfo anim;

        Effect() : effect(10), duration(0) {}
    } window_effect, tmp_effect;
    Effect::vector effects;

    int  effect_blank;
    bool effect_cut_flag;

    int readEffect(Effect& effect);
    Effect& parseEffect(bool init_flag);

    /* ---------------------------------------- */
    /* Lookback related variables */
    //char *lookback_image_name[4];
    int lookback_sp[2];
    rgb_t lookback_color;

    /* ---------------------------------------- */
    /* For loop related variables */
    bool break_flag;

    /* ---------------------------------------- */
    /* Transmode related variables */
    int trans_mode;

    /* ---------------------------------------- */
    /* Save/Load related variables */
    struct SaveFileInfo {
        bool valid;
        int month, day, hour, minute;
        char sjis_no[5];
        char sjis_month[5];
        char sjis_day[5];
        char sjis_hour[5];
        char sjis_minute[5];
    };
    unsigned int num_save_file;
    string save_menu_name;
    string load_menu_name;
    string save_item_name;

    unsigned char* save_data_buf;
    unsigned char* file_io_buf;
    size_t file_io_buf_ptr;
    size_t file_io_buf_len;
    size_t save_data_len;

    /* ---------------------------------------- */
    /* Text related variables */
    string default_env_font;
    int default_text_speed[3];
    struct TextBuffer {
        struct TextBuffer* next, * previous;
        string contents;
        int addBuffer(char ch)
        {
            contents += ch;
            return 0;
        }


        void clear() { contents.clear(); }
        bool empty() { return contents.empty(); }
    } *text_buffer, *start_text_buffer, *current_text_buffer; // ring buffer
    int max_text_buffer;
    int clickstr_line;
    int clickstr_state;
    int linepage_mode;
    int num_chars_in_sentence;

    /* ---------------------------------------- */
    /* Sound related variables */
    int music_volume;
    int voice_volume;
    int se_volume;

    enum { CLICKVOICE_NORMAL  = 0,
           CLICKVOICE_NEWPAGE = 1,
           CLICKVOICE_NUM     = 2 };
    string clickvoice_file_name[CLICKVOICE_NUM];

    enum { SELECTVOICE_OPEN   = 0,
           SELECTVOICE_OVER   = 1,
           SELECTVOICE_SELECT = 2,
           SELECTVOICE_NUM    = 3 };
    string selectvoice_file_name[SELECTVOICE_NUM];

    enum { MENUSELECTVOICE_OPEN   = 0,
           MENUSELECTVOICE_CANCEL = 1,
           MENUSELECTVOICE_OVER   = 2,
           MENUSELECTVOICE_CLICK  = 3,
           MENUSELECTVOICE_WARN   = 4,
           MENUSELECTVOICE_YES    = 5,
           MENUSELECTVOICE_NO     = 6,
           MENUSELECTVOICE_NUM    = 7 };
    string menuselectvoice_file_name[MENUSELECTVOICE_NUM];

    /* ---------------------------------------- */
    /* Font related variables */
    FontInfo* current_font, sentence_font, menu_font;
    int shade_distance[2];

    /* ---------------------------------------- */
    /* RMenu related variables */
    struct RMenuLink {
        RMenuLink* next;
        string label;
        int system_call_no;
        RMenuLink() : next(0) {}
    } root_rmenu_link;
    unsigned int rmenu_link_num, rmenu_link_width;

    void deleteRMenuLink();
    int getSystemCallNo(const char* buffer);
    unsigned char convHexToDec(char ch);
    rgb_t readColour(const char* buf);
    rgb_t readColour(const string& buf)
	{ return readColour(buf.c_str()); }

    void errorAndExit(const char* str, const char* reason = NULL);
    void errorAndExit(const string& str, const char* reason = NULL)
	{ errorAndExit(str.c_str(), reason); }

    void allocFileIOBuf();
    int saveFileIOBuf(const char* filename, int offset = 0);
    int loadFileIOBuf(const char* filename);

    void writeChar(char c, bool output_flag);
    char readChar();
    void writeInt(int i, bool output_flag);
    int readInt();
    void writeStr(const string& s, bool output_flag);
    void readStr(char** s);
    string readStr();
    void writeVariables(int from, int to, bool output_flag);
    void readVariables(int from, int to);
    void writeArrayVariable(bool output_flag);
    void readArrayVariable();

    /* ---------------------------------------- */
    /* System customize related variables */
    string textgosub_label;
    string pretextgosub_label;
    string loadgosub_label;

    ScriptHandler script_h;

    unsigned char* key_table;

    void createKeyTable(const string& key_exe);
};

#endif // __SCRIPT_PARSER_H__
