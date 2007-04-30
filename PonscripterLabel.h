/* -*- C++ -*-
 *
 *  PonscripterLabel.h - Execution block parser of Ponscripter
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

#ifndef __PONSCRIPTER_LABEL_H__
#define __PONSCRIPTER_LABEL_H__

#include "ScriptParser.h"
#include "DirtyRect.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

#ifdef MP3_MAD
#include "MadWrapper.h"
#else
#include <smpeg.h>
#endif

#ifdef USE_OGG_VORBIS
#ifdef INTEGER_OGG_VORBIS
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif
#endif

#define DEFAULT_VIDEO_SURFACE_FLAG (SDL_SWSURFACE)

#define DEFAULT_BLIT_FLAG (0)

#define MAX_SPRITE_NUM 1000
#define MAX_SPRITE2_NUM 256
#define MAX_PARAM_NUM 100
#define CUSTOM_EFFECT_NO 100

#define ONS_MIX_CHANNELS 50
#define ONS_MIX_EXTRA_CHANNELS 4
#define MIX_WAVE_CHANNEL (ONS_MIX_CHANNELS + 0)
#define MIX_BGM_CHANNEL (ONS_MIX_CHANNELS + 1)
#define MIX_LOOPBGM_CHANNEL0 (ONS_MIX_CHANNELS + 2)
#define MIX_LOOPBGM_CHANNEL1 (ONS_MIX_CHANNELS + 3)

#ifndef DEFAULT_WM_TITLE
#define DEFAULT_WM_TITLE "Ponscripter"
#endif
#define DEFAULT_WM_ICON "Ponscripter"

#define NUM_GLYPH_CACHE 30

struct OVInfo {
    SDL_AudioCVT cvt;
    int cvt_len;
    int mult1;
    int mult2;
    unsigned char* buf;
    long decoded_length;
#ifdef USE_OGG_VORBIS
    ogg_int64_t length;
    ogg_int64_t pos;
    OggVorbis_File ovf;
#endif
};

class PonscripterLabel : public ScriptParser {
public:
    typedef AnimationInfo::ONSBuf ONSBuf;

    PonscripterLabel();
    ~PonscripterLabel();

    // ----------------------------------------
    // start-up options
    void enableCDAudio();
    void setCDNumber(int cdrom_drive_number);
    void setRegistryFile(const char* filename);
    void setDLLFile(const char* filename);
    void setSavePath(const char* path);
    void setArchivePath(const char* path);

    bool hasArchivePath() const { return archive_path; }
    void setFullscreenMode();
    void setWindowMode();
    void enableButtonShortCut();
    void enableWheelDownAdvance();
    void disableRescale();
    void enableEdit();
    void setKeyEXE(const char* path);

    int  init();
    int  eventLoop();

    void reset(); // used if definereset
    void resetSub(); // used if reset

    bool skip_flag;
    bool draw_one_page_flag;
    bool key_pressed_flag;
    int  shift_pressed_status;
    int  ctrl_pressed_status;

    /* ---------------------------------------- */
    /* Commands */
    // my extensions
    int haeleth_text_extentCommand(const string& cmd);
    int haeleth_centre_lineCommand(const string& cmd);
    int haeleth_char_setCommand(const string& cmd);
    int haeleth_font_styleCommand(const string& cmd);
    int haeleth_map_fontCommand(const string& cmd);
    int haeleth_hinting_modeCommand(const string& cmd);
    int haeleth_ligate_controlCommand(const string& cmd);
    int haeleth_sayCommand(const string& cmd);

    // regular NScripter stuff
    int wavestopCommand(const string& cmd);
    int waveCommand(const string& cmd);
    int waittimerCommand(const string& cmd);
    int waitCommand(const string& cmd);
    int vspCommand(const string& cmd);
    int voicevolCommand(const string& cmd);
    int vCommand(const string& cmd);
    int trapCommand(const string& cmd);
    int textspeedCommand(const string& cmd);
    int textshowCommand(const string& cmd);
    int textonCommand(const string& cmd);
    int textoffCommand(const string& cmd);
    int texthideCommand(const string& cmd);
    int textclearCommand(const string& cmd);
    int texecCommand(const string& cmd);
    int tateyokoCommand(const string& cmd);
    int talCommand(const string& cmd);
    int tablegotoCommand(const string& cmd);
    int systemcallCommand(const string& cmd);
    int strspCommand(const string& cmd);
    int stopCommand(const string& cmd);
    int sp_rgb_gradationCommand(const string& cmd);
    int spstrCommand(const string& cmd);
    int spreloadCommand(const string& cmd);
    int splitCommand(const string& cmd);
    int spclclkCommand(const string& cmd);
    int spbtnCommand(const string& cmd);
    int skipoffCommand(const string& cmd);
    int sevolCommand(const string& cmd);
    int setwindow3Command(const string& cmd);
    int setwindow2Command(const string& cmd);
    int setwindowCommand(const string& cmd);
    int setcursorCommand(const string& cmd);
    int selectCommand(const string& cmd);
    int savetimeCommand(const string& cmd);
    int saveonCommand(const string& cmd);
    int saveoffCommand(const string& cmd);
    int savegameCommand(const string& cmd);
    int savefileexistCommand(const string& cmd);
    int savescreenshotCommand(const string& cmd);
    int resettimerCommand(const string& cmd);
    int resetCommand(const string& cmd);
    int repaintCommand(const string& cmd);
    int rndCommand(const string& cmd);
    int rmodeCommand(const string& cmd);
    int quakeCommand(const string& cmd);
    int puttextCommand(const string& cmd);
    int prnumclearCommand(const string& cmd);
    int prnumCommand(const string& cmd);
    int printCommand(const string& cmd);
    int playstopCommand(const string& cmd);
    int playonceCommand(const string& cmd);
    int playCommand(const string& cmd);
    int ofscopyCommand(const string& cmd);
    int negaCommand(const string& cmd);
    int mspCommand(const string& cmd);
    int mpegplayCommand(const string& cmd);
    int mp3volCommand(const string& cmd);
    int mp3fadeoutCommand(const string& cmd);
    int mp3Command(const string& cmd);
    int movemousecursorCommand(const string& cmd);
    int monocroCommand(const string& cmd);
    int menu_windowCommand(const string& cmd);
    int menu_fullCommand(const string& cmd);
    int menu_automodeCommand(const string& cmd);
    int lspCommand(const string& cmd);
    int loopbgmstopCommand(const string& cmd);
    int loopbgmCommand(const string& cmd);
    int lookbackflushCommand(const string& cmd);
    int lookbackbuttonCommand(const string& cmd);
    int logspCommand(const string& cmd);
    int locateCommand(const string& cmd);
    int loadgameCommand(const string& cmd);
    int ldCommand(const string& cmd);
    int jumpfCommand(const string& cmd);
    int jumpbCommand(const string& cmd);
    int ispageCommand(const string& cmd);
    int isfullCommand(const string& cmd);
    int isskipCommand(const string& cmd);
    int isdownCommand(const string& cmd);
    int inputCommand(const string& cmd);
    int indentCommand(const string& cmd);
    int humanorderCommand(const string& cmd);
    int getzxcCommand(const string& cmd);
    int getvoicevolCommand(const string& cmd);
    int getversionCommand(const string& cmd);
    int gettimerCommand(const string& cmd);
    int gettextCommand(const string& cmd);
    int gettagCommand(const string& cmd);
    int gettabCommand(const string& cmd);
    int getspsizeCommand(const string& cmd);
    int getspmodeCommand(const string& cmd);
    int getsevolCommand(const string& cmd);
    int getscreenshotCommand(const string& cmd);
    int getretCommand(const string& cmd);
    int getregCommand(const string& cmd);
    int getpageupCommand(const string& cmd);
    int getpageCommand(const string& cmd);
    int getmp3volCommand(const string& cmd);
    int getmouseposCommand(const string& cmd);
    int getlogCommand(const string& cmd);
    int getinsertCommand(const string& cmd);
    int getfunctionCommand(const string& cmd);
    int getenterCommand(const string& cmd);
    int getcursorposCommand(const string& cmd);
    int getcursorCommand(const string& cmd);
    int getcselstrCommand(const string& cmd);
    int getcselnumCommand(const string& cmd);
    int gameCommand(const string& cmd);
    int fileexistCommand(const string& cmd);
    int exec_dllCommand(const string& cmd);
    int exbtnCommand(const string& cmd);
    int erasetextwindowCommand(const string& cmd);
    int endCommand(const string& cmd);
    int dwavestopCommand(const string& cmd);
    int dwaveCommand(const string& cmd);
    int dvCommand(const string& cmd);
    int drawtextCommand(const string& cmd);
    int drawsp3Command(const string& cmd);
    int drawsp2Command(const string& cmd);
    int drawspCommand(const string& cmd);
    int drawfillCommand(const string& cmd);
    int drawclearCommand(const string& cmd);
    int drawbg2Command(const string& cmd);
    int drawbgCommand(const string& cmd);
    int drawCommand(const string& cmd);
    int delayCommand(const string& cmd);
    int defineresetCommand(const string& cmd);
    int cspCommand(const string& cmd);
    int cselgotoCommand(const string& cmd);
    int cselbtnCommand(const string& cmd);
    int clickCommand(const string& cmd);
    int clCommand(const string& cmd);
    int chvolCommand(const string& cmd);
    int checkpageCommand(const string& cmd);
    int cellCommand(const string& cmd);
    int captionCommand(const string& cmd);
    int btnwait2Command(const string& cmd);
    int btnwaitCommand(const string& cmd);
    int btntime2Command(const string& cmd);
    int btntimeCommand(const string& cmd);
    int btndownCommand(const string& cmd);
    int btndefCommand(const string& cmd);
    int btnCommand(const string& cmd);
    int brCommand(const string& cmd);
    int bltCommand(const string& cmd);
    int bgcopyCommand(const string& cmd);
    int bgCommand(const string& cmd);
    int barclearCommand(const string& cmd);
    int barCommand(const string& cmd);
    int aviCommand(const string& cmd);
    int automode_timeCommand(const string& cmd);
    int autoclickCommand(const string& cmd);
    int allspresumeCommand(const string& cmd);
    int allsphideCommand(const string& cmd);
    int amspCommand(const string& cmd);

protected:
    /* ---------------------------------------- */
    /* Event related variables */
    enum { NOT_EDIT_MODE            = 0,
           EDIT_SELECT_MODE         = 1,
           EDIT_VARIABLE_INDEX_MODE = 2,
           EDIT_VARIABLE_NUM_MODE   = 3,
           EDIT_MP3_VOLUME_MODE     = 4,
           EDIT_VOICE_VOLUME_MODE   = 5,
           EDIT_SE_VOLUME_MODE      = 6 };

    int variable_edit_mode;
    int variable_edit_index;
    int variable_edit_num;
    int variable_edit_sign;

    int skip_to_wait;

    void variableEditMode(SDL_KeyboardEvent* event);
    void keyDownEvent(SDL_KeyboardEvent* event);
    void keyUpEvent(SDL_KeyboardEvent* event);
    void keyPressEvent(SDL_KeyboardEvent* event);
    void mousePressEvent(SDL_MouseButtonEvent* event);
    void mouseMoveEvent(SDL_MouseMotionEvent* event);
    void timerEvent();
    void flushEventSub(SDL_Event &event);
    void flushEvent();
    void startTimer(int count);
    void advancePhase(int count = 0);
    void trapHandler();
    void initSDL();
    void openAudio();

private:
    enum { NORMAL_DISPLAY_MODE = 0, TEXT_DISPLAY_MODE = 1 };
    enum { IDLE_EVENT_MODE   = 0,
           EFFECT_EVENT_MODE = 1,
           WAIT_BUTTON_MODE  = 2, // For select, btnwait and rmenu.
           WAIT_INPUT_MODE   = (4 | 8), // Can be skipped by a click.
           WAIT_SLEEP_MODE   = 16,      // Cannot be skipped by a click.
           WAIT_TIMER_MODE   = 32,
           WAIT_TEXTBTN_MODE = 64,
           WAIT_VOICE_MODE   = 128,
           WAIT_TEXT_MODE    = 256 // clickwait, newpage, select
    };
    typedef enum { COLOR_EFFECT_IMAGE  = 0,
                   DIRECT_EFFECT_IMAGE = 1,
                   BG_EFFECT_IMAGE     = 2,
                   TACHI_EFFECT_IMAGE  = 3 } EFFECT_IMAGE;
    enum { ALPHA_BLEND_CONST          = 1,
           ALPHA_BLEND_MULTIPLE       = 2,
           ALPHA_BLEND_FADE_MASK      = 3,
           ALPHA_BLEND_CROSSFADE_MASK = 4 };

    // ----------------------------------------
    // start-up options
    bool   cdaudio_flag;
    string registry_file;
    string dll_file;
    string getret_str;
    int    getret_int;
    bool   enable_wheeldown_advance_flag;
    bool   disable_rescale_flag;
    bool   edit_flag;
    string key_exe_file;

    // ----------------------------------------
    // Global definitions
    long internal_timer;
    bool automode_flag;
    long automode_time;
    long autoclick_time;
    long remaining_time;

    bool saveon_flag;
    bool internal_saveon_flag; // to saveoff at the head of text
    int  yesno_caller;
    int  yesno_selected_file_no;

    bool  monocro_flag;
    rgb_t monocro_color;
    rgb_t monocro_color_lut[256];
    int nega_mode;

    enum { TRAP_NONE = 0,
           TRAP_LEFT_CLICK  = 1,
           TRAP_RIGHT_CLICK = 2,
           TRAP_NEXT_SELECT = 4,
           TRAP_STOP = 8 };
    int    trap_mode;
    string trap_dist;
    string wm_title_string;
    string wm_icon_string;
    string wm_edit_string;
    bool   fullscreen_mode;
    bool   window_mode;

    bool btntime2_flag;
    long btntime_value;
    long internal_button_timer;
    long btnwait_time;
    bool btndown_flag;

    void quit();

    /* ---------------------------------------- */
    /* Script related variables */
    enum { REFRESH_NONE_MODE   = 0,
           REFRESH_NORMAL_MODE = 1,
           REFRESH_SAYA_MODE   = 2,
           REFRESH_SHADOW_MODE = 4,
           REFRESH_TEXT_MODE   = 8,
           REFRESH_CURSOR_MODE = 16,
	   REFRESH_COMP_MODE   = 32 };

    int refresh_shadow_text_mode;
    int current_refresh_mode;
    int display_mode;
    int event_mode;
    // Final image, i.e. picture_surface (+ shadow + text_surface):
    SDL_Surface* accumulation_surface;
    // Complementary final image, i.e. final image xor (shadow + text_surface):
    SDL_Surface* accumulation_comp_surface;
    // Text + Select_image + Tachi image + background:
    SDL_Surface* screen_surface;
    SDL_Surface* effect_dst_surface; // Intermediate source buffer for effect
    SDL_Surface* effect_src_surface; // Intermediate dest buffer for effect
    SDL_Surface* screenshot_surface; // Screenshot
    SDL_Surface* image_surface; // Reference for loadImage()

    /* ---------------------------------------- */
    /* Button related variables */
    AnimationInfo btndef_info;

    struct ButtonState {
        int x, y, button;
        bool down_flag;
    } current_button_state, volatile_button_state,
      last_mouse_state, shelter_mouse_state;

    struct ButtonElt {
	typedef std::map<int, ButtonElt> collection;
	typedef collection::iterator iterator;
	
        enum BUTTON_TYPE {
	    NORMAL_BUTTON     = 0,
	    SPRITE_BUTTON     = 1,
	    EX_SPRITE_BUTTON  = 2,
	    LOOKBACK_BUTTON   = 3,
	    TMP_SPRITE_BUTTON = 4
	};
        BUTTON_TYPE button_type;
        int sprite_no;
        string exbtn_ctl;
        SDL_Rect select_rect;
        SDL_Rect image_rect;
        AnimationInfo* anim[2];
        int show_flag; // 0: show nothing, 1: show anim[0], 2: show anim[1]

	bool isSprite() { return button_type == SPRITE_BUTTON
		              || button_type == EX_SPRITE_BUTTON; }
	bool isTmpSprite() { return button_type == TMP_SPRITE_BUTTON; }
	
        ButtonElt() {
            button_type = NORMAL_BUTTON;
	    anim[0] = anim[1] = 0;
	    show_flag = 0;
        }
        void destroy() {
            if ((button_type == NORMAL_BUTTON || isTmpSprite()) && anim[0]) {
		delete anim[0];
		anim[0] = 0;
	    }
        }
    };
    ButtonElt::collection buttons, shelter_buttons;
    ButtonElt exbtn_d_button;

    void buttonsRemoveSprite(int no) {
	ButtonElt::iterator it = buttons.begin();
	while (it != buttons.end())
	    if (it->second.sprite_no == no && it->second.isSprite()) {
		it->second.destroy();
		buttons.erase(it++);
	    }
	    else
		++it;
    }
    
    void deleteButtons() {
	for (ButtonElt::iterator it = buttons.begin(); it != buttons.end();
	     ++it)
	    it->second.destroy();
	buttons.clear();
	exbtn_d_button.exbtn_ctl.clear();
    }
    
    int current_over_button;

    bool getzxc_flag;
    bool gettab_flag;
    bool getpageup_flag;
    bool getpagedown_flag;
    bool getinsert_flag;
    bool getfunction_flag;
    bool getenter_flag;
    bool getcursor_flag;
    bool spclclk_flag;

    void resetSentenceFont();
    void refreshMouseOverButton();
    void refreshSprite(int sprite_no, bool active_flag, int cell_no,
		       SDL_Rect* check_src_rect, SDL_Rect* check_dst_rect);

    void decodeExbtnControl(const string& ctl_string,
			    SDL_Rect* check_src_rect = 0,
			    SDL_Rect* check_dst_rect = 0);

    void disableGetButtonFlag();
    int getNumberFromBuffer(const char** buf);

    /* ---------------------------------------- */
    /* Background related variables */
    AnimationInfo bg_info;
    EFFECT_IMAGE  bg_effect_image; // This is no longer used. Remove it later.

    /* ---------------------------------------- */
    /* Tachi-e related variables */
    /* 0 ... left, 1 ... center, 2 ... right */
    AnimationInfo tachi_info[3];
    int human_order[3];

    /* ---------------------------------------- */
    /* Sprite related variables */
    AnimationInfo sprite_info[MAX_SPRITE_NUM];
    AnimationInfo sprite2_info[MAX_SPRITE2_NUM];    
    bool all_sprite_hide_flag;

    /* ---------------------------------------- */
    /* Parameter related variables */
    AnimationInfo* bar_info[MAX_PARAM_NUM];
    AnimationInfo* prnum_info[MAX_PARAM_NUM];

    /* ---------------------------------------- */
    /* Cursor related variables */
    enum { CURSOR_WAIT_NO    = 0,
           CURSOR_NEWPAGE_NO = 1 };
    AnimationInfo cursor_info[2];

    void loadCursor(int no, const char* str, int x, int y, bool abs_flag = 0);
    void saveAll();
    void loadEnvData();
    void saveEnvData();

    /* ---------------------------------------- */
    /* Lookback related variables */
    AnimationInfo lookback_info[4];

    /* ---------------------------------------- */
    /* Text related variables */
    AnimationInfo text_info;
    AnimationInfo sentence_font_info;
    int  erase_text_window_mode;
    bool text_on_flag; // suppress the effect of erase_text_window_mode
    bool draw_cursor_flag;
    int  textgosub_clickstr_state;
    int  indent_offset;
    int  line_enter_status; // 0 ... no enter, 1 ... pretext, 2 ... body
    SDL_Surface* glyph_surface;

    int  refreshMode();
    void setwindowCore();

    SDL_Surface* renderGlyph(Font* font, Uint16 text, int size,
			     float x_fractional_part);
    void drawGlyph(SDL_Surface* dst_surface, FontInfo* info, SDL_Color &color,
		   wchar unicode, float x, int y, bool shadow_flag,
		   AnimationInfo* cache_info, SDL_Rect* clip,
		   SDL_Rect &dst_rect);
    void drawChar(const char* text, FontInfo* info, bool flush_flag,
		  bool lookback_flag, SDL_Surface* surface,
		  AnimationInfo* cache_info, SDL_Rect* clip = 0);
    void drawString(const char* str, rgb_t color, FontInfo* info,
		    bool flush_flag, SDL_Surface* surface, SDL_Rect* rect = 0,
		    AnimationInfo* cache_info = 0);

    void drawString(const string& str, rgb_t color, FontInfo* info,
		    bool flush_flag, SDL_Surface* surface, SDL_Rect* rect = 0,
		    AnimationInfo* cache_info = 0)
	{ /* 取り敢えず */ drawString(str.c_str(), color, info, flush_flag,
				      surface, rect, cache_info); }
    
    void restoreTextBuffer();
    int  enterTextDisplayMode(bool text_flag = true);
    int  leaveTextDisplayMode();
    void doClickEnd();
    int  clickWait(bool display_char);
    int  clickNewPage(bool display_char);
    int  textCommand();
    int  processText();

    std::set<wchar> indent_chars;
    std::set<wchar> break_chars;
    bool is_indent_char(const wchar c) const
	{ return indent_chars.find(c) != indent_chars.end(); }
    bool is_break_char(const wchar c) const
	{ return break_chars.find(c) != break_chars.end(); }
    bool check_orphan_control();

    /* ---------------------------------------- */
    /* Effect related variables */
    DirtyRect dirty_rect, dirty_rect_tmp; // only this region is updated
    int effect_counter; // counter in each effect
    int effect_timer_resolution;
    int effect_start_time;
    int effect_start_time_old;

    int setEffect(const Effect& effect);
    int doEffect(Effect& effect, AnimationInfo* anim, int effect_image,
		  bool clear_dirty_region = true);
    void drawEffect(SDL_Rect* dst_rect, SDL_Rect* src_rect,
		    SDL_Surface* surface);
    void generateMosaic(SDL_Surface* src_surface, int level);

    /* ---------------------------------------- */
    /* Select related variables */
    enum {
	SELECT_GOTO_MODE  = 0,
	SELECT_GOSUB_MODE = 1,
	SELECT_NUM_MODE   = 2,
	SELECT_CSEL_MODE  = 3
    };
    struct SelectElt {
	typedef std::vector<SelectElt> vector;
	typedef vector::iterator iterator;
	string text, label;
	SelectElt(const string& t, const string& l) : text(t), label(l) {}
    };
    SelectElt::vector select_links, shelter_select_links;
    NestInfo select_label_info;
    ButtonElt::iterator shortcut_mouse_line;

    ButtonElt getSelectableSentence(const string& buffer, FontInfo* info,
				    bool flush_flag = true,
				    bool nofile_flag = false);

    /* ---------------------------------------- */
    /* Sound related variables */
    enum {
        SOUND_NONE    = 0,
        SOUND_PRELOAD = 1,
        SOUND_WAVE    = 2,
        SOUND_OGG     = 4,
        SOUND_OGG_STREAMING = 8,
        SOUND_MP3     = 16,
        SOUND_MIDI    = 32,
        SOUND_OTHER   = 64
    };
    int cdrom_drive_number;
    string default_cdrom_drive;
    bool cdaudio_on_flag; // false if mute
    bool volume_on_flag; // false if mute
    SDL_AudioSpec audio_format;
    bool audio_open_flag;

    bool   wave_play_loop_flag;
    string wave_file_name;

    bool   midi_play_loop_flag;
    string midi_file_name;
    Mix_Music* midi_info;

    SDL_CD* cdrom_info;
    int    current_cd_track;
    bool   cd_play_loop_flag;
    bool   music_play_loop_flag;
    bool   mp3save_flag;
    string music_file_name;
    unsigned char* mp3_buffer;
    SMPEG*  mp3_sample;
    Uint32  mp3fadeout_start;
    Uint32  mp3fadeout_duration;
    OVInfo* music_ovi;
    Mix_Music* music_info;
    string loop_bgm_name[2];

    Mix_Chunk* wave_sample[ONS_MIX_CHANNELS + ONS_MIX_EXTRA_CHANNELS];

    string music_cmd;
    string midi_cmd;

    int playSound(const string& filename, int format, bool loop_flag,
		  int channel = 0);
    
    void playCDAudio();
    int playWave(Mix_Chunk* chunk, int format, bool loop_flag, int channel);
    int playMP3();
    int playOGG(int format, unsigned char* buffer, long length, bool loop_flag,
		int channel);
    int playExternalMusic(bool loop_flag);
    int playMIDI(bool loop_flag);

    int playMPEG(const char* filename, bool click_flag);
    void playAVI(const char* filename, bool click_flag);

    enum { WAVE_PLAY        = 0,
           WAVE_PRELOAD     = 1,
           WAVE_PLAY_LOADED = 2 };
    void stopBGM(bool continue_flag);
    void playClickVoice();
    void setupWaveHeader(unsigned char* buffer, int channels, int rate,
			 unsigned long data_length);
    OVInfo* openOggVorbis(unsigned char* buf, long len, int &channels,
			  int &rate);
    int  closeOggVorbis(OVInfo* ovi);

    /* ---------------------------------------- */
    /* Text event related variables */
    bool new_line_skip_flag;
    int  text_speed_no;

    void shadowTextDisplay(SDL_Surface* surface, SDL_Rect &clip);
    void clearCurrentTextBuffer();
    void newPage(bool next_flag);

    void flush(int refresh_mode, SDL_Rect* rect = 0,
	       bool clear_dirty_flag = true, bool direct_flag = false);
    void flushDirect(SDL_Rect &rect, int refresh_mode, bool updaterect = true);
    void executeLabel();
    SDL_Surface* loadImage(const char* file_name, bool* has_alpha = NULL);
    SDL_Surface* loadImage(const string& file_name, bool* has_alpha = NULL)
	{ return loadImage(file_name.c_str(), has_alpha); }
    int parseLine();

    void mouseOverCheck(int x, int y);

    /* ---------------------------------------- */
    /* Animation */
    int  proceedAnimation();
    int  estimateNextDuration(AnimationInfo* anim, SDL_Rect &rect, int minimum);
    void resetRemainingTime(int t);
    void setupAnimationInfo(AnimationInfo* anim, FontInfo* info = NULL);
    void parseTaggedString(AnimationInfo* anim);
    void drawTaggedSurface(SDL_Surface* dst_surface, AnimationInfo* anim,
			   SDL_Rect &clip);
    void stopAnimation(int click);

    /* ---------------------------------------- */
    /* File I/O */
    void searchSaveFile(SaveFileInfo &info, int no);
    int  loadSaveFile(int no);
    void saveMagicNumber(bool output_flag);
    int  saveSaveFile(int no);

    int  loadSaveFile2(int file_version);
    void saveSaveFile2(bool output_flag);

    /* ---------------------------------------- */
    /* Image processing */
    unsigned char* resize_buffer;
    size_t resize_buffer_size;

    int  resizeSurface(SDL_Surface* src, SDL_Surface* dst);
    void shiftCursorOnButton(int diff);
    void alphaBlend(SDL_Surface* mask_surface, int trans_mode,
	     Uint32 mask_value = 255, SDL_Rect* clip = 0);
    void alphaBlend32(SDL_Surface* dst_surface, SDL_Rect dst_rect,
	     SDL_Surface* src_surface, SDL_Color &color, SDL_Rect* clip,
	     bool rotate_flag);
    void makeNegaSurface(SDL_Surface* surface, SDL_Rect &clip);
    void makeMonochromeSurface(SDL_Surface* surface, SDL_Rect &clip);
    void refreshSurface(SDL_Surface* surface, SDL_Rect* clip_src,
	     int refresh_mode = REFRESH_NORMAL_MODE);
    void createBackground();

    /* ---------------------------------------- */
    /* rmenu and system call */
    bool system_menu_enter_flag;
    int  system_menu_mode;

    int  shelter_event_mode;
    int  shelter_display_mode;
    bool shelter_draw_cursor_flag;
    TextBuffer* cached_text_buffer;

    void enterSystemCall();
    void leaveSystemCall(bool restore_flag = true);
    void executeSystemCall();

    void executeSystemMenu();
    void executeSystemSkip();
    void executeSystemAutomode();
    void executeSystemReset();
    void executeSystemEnd();
    void executeWindowErase();
    void createSaveLoadMenu(bool is_save);
    void executeSystemLoad();
    void executeSystemSave();
    void executeSystemYesNo();
    void setupLookbackButton();
    void executeSystemLookback();
};

#endif // __PONSCRIPTER_LABEL_H__
