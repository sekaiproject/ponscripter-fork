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

#include <SDL_mixer.h>
#include "DirPaths.h"
#include "ScriptHandler.h"
#include "NsaReader.h"
#include "DirectReader.h"
#include "AnimationInfo.h"
#include "Fontinfo.h"

#if defined(USE_OGG_VORBIS)
#if defined(INTEGER_OGG_VORBIS)
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif
#endif

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

struct OVInfo{
    SDL_AudioCVT cvt;
    int cvt_len;
    int mult1;
    int mult2;
    unsigned char *buf;
    long decoded_length;
#if defined(USE_OGG_VORBIS)
    ogg_int64_t length;
    ogg_int64_t pos;
    OggVorbis_File ovf;
#endif
};

class ScriptParser {
public:
    typedef struct{
        OVInfo *ovi;
        int volume;
        bool is_mute;
        Mix_Chunk **voice_sample; //Mion: for bgmdownmode
    } MusicStruct;

    ScriptParser();

    virtual ~ScriptParser();

    void reset();
    int open(const char* preferred_script);
    int parseLine();

    void saveGlobalData();

    /* Command */
    int zenkakkoCommand(const pstring& cmd);
    int windowbackCommand(const pstring& cmd);
    int watch_varCommand(const pstring& cmd);
    int versionstrCommand(const pstring& cmd);
    int usewheelCommand(const pstring& cmd);
    int useescspcCommand(const pstring& cmd);
    int underlineCommand(const pstring& cmd);
    int transmodeCommand(const pstring& cmd);
    int timeCommand(const pstring& cmd);
    int textgosubCommand(const pstring& cmd);
    int tanCommand(const pstring& cmd);
    int subCommand(const pstring& cmd);
    int straliasCommand(const pstring& cmd);
    int soundpressplginCommand(const pstring& cmd);
    int skipCommand(const pstring& cmd);
    int sinCommand(const pstring& cmd);
    int shadedistanceCommand(const pstring& cmd);
    int selectvoiceCommand(const pstring& cmd);
    int selectcolorCommand(const pstring& cmd);
    int savenumberCommand(const pstring& cmd);
    int savenameCommand(const pstring& cmd);
    int rubyonCommand(const pstring& cmd);
    int rubyoffCommand(const pstring& cmd);
    int roffCommand(const pstring& cmd);
    int rmenuCommand(const pstring& cmd);
    int returnCommand(const pstring& cmd);
    int pretextgosubCommand(const pstring& cmd);
    int numaliasCommand(const pstring& cmd);
    int nsadirCommand(const pstring& cmd);
    int nsaCommand(const pstring& cmd);
    int nextCommand(const pstring& cmd);
    int mulCommand(const pstring& cmd);
    int movCommand(const pstring& cmd);
    int mode_wave_demoCommand(const pstring& cmd);
    int mode_sayaCommand(const pstring& cmd);
    int mode_extCommand(const pstring& cmd);
    int modCommand(const pstring& cmd);
    int midCommand(const pstring& cmd);
    int menusetwindowCommand(const pstring& cmd);
    int menuselectvoiceCommand(const pstring& cmd);
    int menuselectcolorCommand(const pstring& cmd);
    int maxkaisoupageCommand(const pstring& cmd);
    int lookbackspCommand(const pstring& cmd);
    int lookbackcolorCommand(const pstring& cmd);

    //int lookbackbuttonCommand(const pstring& cmd);
    int loadgosubCommand(const pstring& cmd);
    int linepageCommand(const pstring& cmd);
    int lenCommand(const pstring& cmd);
    int labellogCommand(const pstring& cmd);
    int kidokuskipCommand(const pstring& cmd);
    int kidokumodeCommand(const pstring& cmd);
    int itoaCommand(const pstring& cmd);
    int intlimitCommand(const pstring& cmd);
    int incCommand(const pstring& cmd);
    int ifCommand(const pstring& cmd);
    int humanzCommand(const pstring& cmd);
    int gotoCommand(const pstring& cmd);
    int gosubCommand(const pstring& cmd);
    int globalonCommand(const pstring& cmd);
    int getparamCommand(const pstring& cmd);

    //int gameCommand(const pstring& cmd);
    int forCommand(const pstring& cmd);
    int filelogCommand(const pstring& cmd);
    int effectcutCommand(const pstring& cmd);
    int effectblankCommand(const pstring& cmd);
    int effectCommand(const pstring& cmd);
    int divCommand(const pstring& cmd);
    int dimCommand(const pstring& cmd);
    int defvoicevolCommand(const pstring& cmd);
    int defsubCommand(const pstring& cmd);
    int defsevolCommand(const pstring& cmd);
    int defmp3volCommand(const pstring& cmd);
    int defaultspeedCommand(const pstring& cmd);
    int defaultfontCommand(const pstring& cmd);
    int decCommand(const pstring& cmd);
    int dateCommand(const pstring& cmd);
    int cosCommand(const pstring& cmd);
    int cmpCommand(const pstring& cmd);
    int clickvoiceCommand(const pstring& cmd);
    int clickstrCommand(const pstring& cmd);
    int breakCommand(const pstring& cmd);
    int atoiCommand(const pstring& cmd);
    int arcCommand(const pstring& cmd);
    int addCommand(const pstring& cmd);

protected:
    set<pstring>::t user_func_lut;

    struct NestInfo {
	typedef std::vector<NestInfo> vector;
	typedef vector::iterator iterator;
	
        enum { LABEL = 0, FOR = 1, TEXTGOSUB = 2 } nest_mode;
        const char* next_script; // used in gosub and for
	Expression var; // used in for
        int to, step; // used in for

        NestInfo(ScriptHandler& h, const char* ns = 0)
	    : nest_mode(LABEL), next_script(ns), var(h) {}
	NestInfo(Expression e, const char* ns = 0)
	    : nest_mode(FOR), next_script(ns), var(e) {}

        NestInfo(ScriptHandler& h, const char* cs, int string_buffer_offset)
            : nest_mode(TEXTGOSUB), next_script(cs), var(h),
              to(string_buffer_offset) {}
    };
    const char* last_tilde;
    NestInfo::vector nest_infos;
    void deleteNestInfo() { nest_infos.clear(); }

    enum syscall { SYSTEM_NULL        = 0,
		   SYSTEM_SKIP        = 1,
		   SYSTEM_RESET       = 2,
		   SYSTEM_SAVE        = 3,
		   SYSTEM_LOAD        = 4,
		   SYSTEM_LOOKBACK    = 5,
		   SYSTEM_WINDOWERASE = 6,
		   SYSTEM_MENU        = 7,
		   SYSTEM_YESNO       = 8,
		   SYSTEM_AUTOMODE    = 9,
		   SYSTEM_END         = 10 };
    typedef dictionary<pstring, syscall>::t syscall_dict_t;
    syscall_dict_t syscall_dict;
    
    enum { RET_NOMATCH   = 0,
           RET_SKIP_LINE = 1,
           RET_CONTINUE  = 2,
           RET_WAIT      = 4,
           RET_NOREAD    = 8,
           RET_REREAD    = 16 };
    enum { CLICK_NONE    = 0,
           CLICK_WAIT    = 1,
           CLICK_NEWPAGE = 2,
           CLICK_IGNORE  = 3,
           CLICK_WAITEOL = 4 };
    enum { NORMAL_MODE, DEFINE_MODE };
    int current_mode;
    int debug_level;

    DirPaths archive_path;
    pstring nsa_path;

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
    bool mode_wave_demo_flag;
    bool mode_saya_flag;
    bool mode_ext_flag;
    bool force_button_shortcut_flag;
    bool zenkakko_flag;

    int string_buffer_offset;
    int string_buffer_restore;

    ScriptHandler::LabelInfo current_label_info;
    int current_line;

    /* ---------------------------------------- */
    /* Global definitions */
    int    screen_width, screen_height;
    int    screen_texture_width, screen_texture_height;
    int    screen_bpp;
    pstring version_str;
    int    underline_value;

    void gosubReal(const pstring& label, const char* next_script);
    void gosubDoTextgosub();
    void setCurrentLabel(const pstring& label);
    void readToken();

    /* ---------------------------------------- */
    /* Effect related variables */
    struct Effect {
        typedef std::vector<Effect*> vector;
        typedef vector::iterator iterator;
        int no;
        int effect;
        int duration;
        AnimationInfo anim;

        Effect() : effect(10), duration(0) {}
        Effect(const Effect &e) :
            no(e.no), effect(e.effect), duration(e.duration), anim(e.anim) {
                anim.deepcopy(e.anim);
            }
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
        int no;
        int month, day, wday, year, hour, minute, sec;
    };
    unsigned int num_save_file;
    pstring save_menu_name;
    pstring load_menu_name;
    pstring save_item_name;

    unsigned char* save_data_buf;
    unsigned char* file_io_buf;
    size_t file_io_buf_ptr;
    size_t file_io_buf_len;
    size_t save_data_len;

    /* ---------------------------------------- */
    /* Text related variables */
    pstring default_env_font;
    int default_text_speed[3];
    struct TextBuffer {
        TextBuffer *next, *previous;
        pstring contents;
        void addBuffer(char ch) { contents += ch; }
	void addBuffer(const pstring& s) { contents += s; }
	void addBytes(const char* c, int num) { contents.add(c, num);}
        void clear() { contents.trunc(0); }
        bool empty() { return !contents; }
	void dumpstate(int = -1);
    } *text_buffer, *start_text_buffer, *current_text_buffer; // ring buffer
    void TextBuffer_dumpstate(int = 0);
    int max_text_buffer;
    int clickstr_line;
    int clickstr_state;
    int linepage_mode;
    int num_chars_in_sentence;

    /* ---------------------------------------- */
    /* Sound related variables */
    MusicStruct music_struct;
    int music_volume;
    int voice_volume;
    int se_volume;
    bool use_default_volume;

    enum { CLICKVOICE_NORMAL  = 0,
           CLICKVOICE_NEWPAGE = 1,
           CLICKVOICE_NUM     = 2 };
    pstring clickvoice_file_name[CLICKVOICE_NUM];

    enum { SELECTVOICE_OPEN   = 0,
           SELECTVOICE_OVER   = 1,
           SELECTVOICE_SELECT = 2,
           SELECTVOICE_NUM    = 3 };
    pstring selectvoice_file_name[SELECTVOICE_NUM];

    enum { MENUSELECTVOICE_OPEN   = 0,
           MENUSELECTVOICE_CANCEL = 1,
           MENUSELECTVOICE_OVER   = 2,
           MENUSELECTVOICE_CLICK  = 3,
           MENUSELECTVOICE_WARN   = 4,
           MENUSELECTVOICE_YES    = 5,
           MENUSELECTVOICE_NO     = 6,
           MENUSELECTVOICE_NUM    = 7 };
    pstring menuselectvoice_file_name[MENUSELECTVOICE_NUM];

    /* ---------------------------------------- */
    /* Font related variables */
    Fontinfo* current_font, sentence_font, menu_font;
    int shade_distance[2];

    /* ---------------------------------------- */
    /* RMenu related variables */
    struct RMenuElt {
	typedef std::vector<RMenuElt> vec;
	typedef vec::iterator iterator;
        pstring label;
        int system_call_no;
	RMenuElt(pstring& l, int s) : label(l), system_call_no(s) {}
    };
    RMenuElt::vec rmenu;
    unsigned int rmenu_link_width;

    int getSystemCallNo(const pstring& buffer);
    unsigned char convHexToDec(char ch);
    rgb_t readColour(const char* buf);

    void errorAndExit(const char* why, const char* reason = NULL);
    void errorAndCont(const char* why, const char* reason = NULL);

    void allocFileIOBuf();
    int saveFileIOBuf(const pstring& filename, int offset = 0,
                      const char* savestr = NULL);
    int loadFileIOBuf(const pstring& filename);

    void writeChar(char c, bool output_flag);
    char readChar();
    void writeInt(int i, bool output_flag);
    int readInt();
    void writeStr(const pstring& s, bool output_flag);
    pstring readStr();
    void writeVariables(int from, int to, bool output_flag);
    void readVariables(int from, int to);
    void writeArrayVariable(bool output_flag);
    void readArrayVariable();

    /* ---------------------------------------- */
    /* System customize related variables */
    pstring textgosub_label;
    pstring pretextgosub_label;
    pstring loadgosub_label;

    ScriptHandler script_h;

    unsigned char* key_table;

    void createKeyTable(const pstring& key_exe);
};

#endif // __SCRIPT_PARSER_H__
