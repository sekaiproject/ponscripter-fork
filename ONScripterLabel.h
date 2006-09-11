/* -*- C++ -*-
 *
 *  ONScripterLabel.h - Execution block parser of ONScripter
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
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

#ifndef __ONSCRIPTER_LABEL_H__
#define __ONSCRIPTER_LABEL_H__

#include "ScriptParser.h"
#include "DirtyRect.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#if defined(MP3_MAD)
#include "MadWrapper.h"
#else
#include <smpeg.h>
#endif

#if defined(USE_OGG_VORBIS)
#if defined(INTEGER_OGG_VORBIS)
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif
#endif

#define DEFAULT_VIDEO_SURFACE_FLAG (SDL_SWSURFACE)

#define DEFAULT_BLIT_FLAG (0)
//#define DEFAULT_BLIT_FLAG (SDL_RLEACCEL)

#define MAX_SPRITE_NUM 1000
#define MAX_PARAM_NUM 100
#define CUSTOM_EFFECT_NO 100

#define ONS_MIX_CHANNELS 50
#define ONS_MIX_EXTRA_CHANNELS 4
#define MIX_WAVE_CHANNEL (ONS_MIX_CHANNELS+0)
#define MIX_BGM_CHANNEL (ONS_MIX_CHANNELS+1)
#define MIX_LOOPBGM_CHANNEL0 (ONS_MIX_CHANNELS+2)
#define MIX_LOOPBGM_CHANNEL1 (ONS_MIX_CHANNELS+3)

#define DEFAULT_WM_TITLE "ONScripter"
#define DEFAULT_WM_ICON  "ONScripter"

#define NUM_GLYPH_CACHE 30

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

class ONScripterLabel : public ScriptParser
{
public:
    typedef AnimationInfo::ONSBuf ONSBuf;

    ONScripterLabel();
    ~ONScripterLabel();

    // ----------------------------------------
    // start-up options
    void enableCDAudio();
    void setCDNumber(int cdrom_drive_number);
    void setFontFile(const char *filename);
    void setRegistryFile(const char *filename);
    void setDLLFile(const char *filename);
    void setArchivePath(const char *path);
    void setFullscreenMode();
    void setWindowMode();
    void enableButtonShortCut();
    void enableWheelDownAdvance();
    void disableRescale();
    void enableEdit();
    void setKeyEXE(const char *path);

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
    int wavestopCommand();
    int waveCommand();
    int waittimerCommand();
    int waitCommand();
    int vspCommand();
    int voicevolCommand();
    int vCommand();
    int trapCommand();
    int textspeedCommand();
    int textshowCommand();
    int textonCommand();
    int textoffCommand();
    int texthideCommand();
    int textclearCommand();
    int texecCommand();
    int tateyokoCommand();
    int talCommand();
    int tablegotoCommand();
    int systemcallCommand();
    int strspCommand();
    int stopCommand();
    int sp_rgb_gradationCommand();
    int spstrCommand();
    int spreloadCommand();
    int splitCommand();
    int spclclkCommand();
    int spbtnCommand();
    int skipoffCommand();
    int sevolCommand();
    int setwindow3Command();
    int setwindow2Command();
    int setwindowCommand();
    int setcursorCommand();
    int selectCommand();
    int savetimeCommand();
    int saveonCommand();
    int saveoffCommand();
    int savegameCommand();
    int savefileexistCommand();
    int savescreenshotCommand();
    int resettimerCommand();
    int resetCommand();
    int repaintCommand();
    int rndCommand();
    int rmodeCommand();
    int quakeCommand();
    int puttextCommand();
    int prnumclearCommand();
    int prnumCommand();
    int printCommand();
    int playstopCommand();
    int playonceCommand();
    int playCommand();
    int ofscopyCommand();
    int negaCommand();
    int mspCommand();
    int mpegplayCommand();
    int mp3volCommand();
#if defined(INSANI)
    int mp3fadeoutCommand();
#endif
    int mp3Command();
    int movemousecursorCommand();
    int monocroCommand();
    int menu_windowCommand();
    int menu_fullCommand();
    int menu_automodeCommand();
    int lspCommand();
    int loopbgmstopCommand();
    int loopbgmCommand();
    int lookbackflushCommand();
    int lookbackbuttonCommand();
    int logspCommand();
    int locateCommand();
    int loadgameCommand();
    int ldCommand();
    int jumpfCommand();
    int jumpbCommand();
    int ispageCommand();
    int isfullCommand();
    int isskipCommand();
    int isdownCommand();
    int inputCommand();
    int indentCommand();
    int humanorderCommand();
    int getzxcCommand();
    int getvoicevolCommand();
    int getversionCommand();
    int gettimerCommand();
    int gettextCommand();
    int gettagCommand();
    int gettabCommand();
    int getspsizeCommand();
    int getspmodeCommand();
    int getsevolCommand();
    int getscreenshotCommand();
    int getretCommand();
    int getregCommand();
    int getpageupCommand();
    int getpageCommand();
    int getmp3volCommand();
    int getmouseposCommand();
    int getlogCommand();
    int getinsertCommand();
    int getfunctionCommand();
    int getenterCommand();
    int getcursorposCommand();
    int getcursorCommand();
    int getcselstrCommand();
    int getcselnumCommand();
    int gameCommand();
    int fileexistCommand();
    int exec_dllCommand();
    int exbtnCommand();
    int erasetextwindowCommand();
    int endCommand();
    int dwavestopCommand();
    int dwaveCommand();
    int dvCommand();
    int drawtextCommand();
    int drawsp3Command();
    int drawsp2Command();
    int drawspCommand();
    int drawfillCommand();
    int drawclearCommand();
    int drawbg2Command();
    int drawbgCommand();
    int drawCommand();
    int delayCommand();
    int defineresetCommand();
    int cspCommand();
    int cselgotoCommand();
    int cselbtnCommand();
    int clickCommand();
    int clCommand();
    int chvolCommand();
    int checkpageCommand();
    int cellCommand();
    int captionCommand();
    int btnwait2Command();
    int btnwaitCommand();
    int btntime2Command();
    int btntimeCommand();
    int btndownCommand();
    int btndefCommand();
    int btnCommand();
    int brCommand();
    int bltCommand();
    int bgcopyCommand();
    int bgCommand();
    int barclearCommand();
    int barCommand();
    int aviCommand();
    int automode_timeCommand();
    int autoclickCommand();
    int allspresumeCommand();
    int allsphideCommand();
    int amspCommand();

protected:
    /* ---------------------------------------- */
    /* Event related variables */
    enum { NOT_EDIT_MODE            = 0,
           EDIT_SELECT_MODE         = 1,
           EDIT_VARIABLE_INDEX_MODE = 2,
           EDIT_VARIABLE_NUM_MODE   = 3,
           EDIT_MP3_VOLUME_MODE     = 4,
           EDIT_VOICE_VOLUME_MODE   = 5,
           EDIT_SE_VOLUME_MODE      = 6
    };

    int variable_edit_mode;
    int variable_edit_index;
    int variable_edit_num;
    int variable_edit_sign;

#if defined(INSANI)
	int skip_to_wait;
#endif

    void variableEditMode( SDL_KeyboardEvent *event );
    void keyDownEvent( SDL_KeyboardEvent *event );
    void keyUpEvent( SDL_KeyboardEvent *event );
    void keyPressEvent( SDL_KeyboardEvent *event );
    void mousePressEvent( SDL_MouseButtonEvent *event );
    void mouseMoveEvent( SDL_MouseMotionEvent *event );
    void timerEvent();
    void flushEventSub( SDL_Event &event );
    void flushEvent();
    void startTimer( int count );
    void advancePhase( int count=0 );
    void trapHandler();
    void initSDL();
    void openAudio();

private:
    enum { NORMAL_DISPLAY_MODE = 0, TEXT_DISPLAY_MODE = 1 };
    enum { IDLE_EVENT_MODE      = 0,
           EFFECT_EVENT_MODE    = 1,
           WAIT_BUTTON_MODE     = 2, // For select, btnwait and rmenu.
           WAIT_INPUT_MODE      = (4|8),  // can be skipped by a click
           WAIT_SLEEP_MODE      = 16, // cannot be skipped by a click
           WAIT_TIMER_MODE      = 32,
           WAIT_TEXTBTN_MODE    = 64,
           WAIT_VOICE_MODE      = 128,
           WAIT_TEXT_MODE       = 256 // clickwait, newpage, select
    };
    typedef enum { COLOR_EFFECT_IMAGE  = 0,
                   DIRECT_EFFECT_IMAGE = 1,
                   BG_EFFECT_IMAGE     = 2,
                   TACHI_EFFECT_IMAGE  = 3
    } EFFECT_IMAGE;
    enum { ALPHA_BLEND_CONST          = 1,
           ALPHA_BLEND_MULTIPLE       = 2,
           ALPHA_BLEND_FADE_MASK      = 3,
           ALPHA_BLEND_CROSSFADE_MASK = 4
    };

    // ----------------------------------------
    // start-up options
    bool cdaudio_flag;
    char *default_font;
    char *registry_file;
    char *dll_file;
    char *getret_str;
    int  getret_int;
    bool enable_wheeldown_advance_flag;
    bool disable_rescale_flag;
    bool edit_flag;
    char *key_exe_file;

    // ----------------------------------------
    // Global definitions
    long internal_timer;
    bool automode_flag;
    long automode_time;
    long autoclick_time;
    long remaining_time;

    bool saveon_flag;
    bool internal_saveon_flag; // to saveoff at the head of text
    int yesno_caller;
    int yesno_selected_file_no;

    bool monocro_flag;
    uchar3 monocro_color;
    uchar3 monocro_color_lut[256];
    int  nega_mode;

    enum { TRAP_NONE        = 0,
           TRAP_LEFT_CLICK  = 1,
           TRAP_RIGHT_CLICK = 2,
           TRAP_NEXT_SELECT = 4,
           TRAP_STOP        = 8
    };
    int  trap_mode;
    char *trap_dist;
    char *wm_title_string;
    char *wm_icon_string;
    char wm_edit_string[256];
    bool fullscreen_mode;
    bool window_mode;

    bool btntime2_flag;
    long btntime_value;
    long internal_button_timer;
    long btnwait_time;
    bool btndown_flag;

    void quit();

    /* ---------------------------------------- */
    /* Script related variables */
    enum { REFRESH_NONE_MODE        = 0,
           REFRESH_NORMAL_MODE      = 1,
           REFRESH_SAYA_MODE        = 2,
           REFRESH_SHADOW_MODE      = 4,
           REFRESH_TEXT_MODE        = 8,
           REFRESH_CURSOR_MODE      = 16
    };

    int refresh_shadow_text_mode;
    int current_refresh_mode;
    int display_mode, next_display_mode;
    int event_mode;
    SDL_Surface *accumulation_surface; // Final image, i.e. picture_surface (+ shadow + text_surface)
    SDL_Surface *screen_surface; // Text + Select_image + Tachi image + background
    SDL_Surface *effect_dst_surface; // Intermediate source buffer for effect
    SDL_Surface *effect_src_surface; // Intermediate destnation buffer for effect
    SDL_Surface *screenshot_surface; // Screenshot
    SDL_Surface *image_surface; // Reference for loadImage()

    /* ---------------------------------------- */
    /* Button related variables */
    AnimationInfo btndef_info;

    struct ButtonState{
        int x, y, button;
        bool down_flag;
    } current_button_state, volatile_button_state, last_mouse_state, shelter_mouse_state;

    struct ButtonLink{
        typedef enum { NORMAL_BUTTON        = 0,
                       SPRITE_BUTTON        = 1,
                       EX_SPRITE_BUTTON     = 2,
                       LOOKBACK_BUTTON      = 3,
                       TMP_SPRITE_BUTTON    = 4
        } BUTTON_TYPE;

        struct ButtonLink *next;
        BUTTON_TYPE button_type;
        int no;
        int sprite_no;
        char *exbtn_ctl;
        SDL_Rect select_rect;
        SDL_Rect image_rect;
        AnimationInfo *anim[2];
        int show_flag; // 0...show nothing, 1... show anim[0], 2 ... show anim[1]

        ButtonLink(){
            button_type = NORMAL_BUTTON;
            next = NULL;
            exbtn_ctl = NULL;
            anim[0] = anim[1] = NULL;
            show_flag = 0;
        };
        ~ButtonLink(){
            if ((button_type == NORMAL_BUTTON ||
                 button_type == TMP_SPRITE_BUTTON) && anim[0]) delete anim[0];
            if ( exbtn_ctl ) delete[] exbtn_ctl;
        };
        void insert( ButtonLink *button ){
            button->next = this->next;
            this->next = button;
        };
        void removeSprite( int no ){
            ButtonLink *p = this;
            while(p->next){
                if ( p->next->sprite_no == no &&
                     ( p->next->sprite_no == SPRITE_BUTTON ||
                       p->next->sprite_no == EX_SPRITE_BUTTON ) ){
                    ButtonLink *p2 = p->next;
                    p->next = p->next->next;
                    delete p2;
                }
                else{
                    p = p->next;
                }
            }
        };
    } root_button_link, *current_button_link, *shelter_button_link, exbtn_d_button_link;

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
    void deleteButtonLink();
    void refreshMouseOverButton();
    void refreshSprite( SDL_Surface *surface, int sprite_no, bool active_flag, int cell_no, SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect );

    void decodeExbtnControl( SDL_Surface *surface, const char *ctl_str, SDL_Rect *check_src_rect=NULL, SDL_Rect *check_dst_rect=NULL );

    void disableGetButtonFlag();
    int getNumberFromBuffer( const char **buf );

    /* ---------------------------------------- */
    /* Background related variables */
    AnimationInfo bg_info;
    EFFECT_IMAGE bg_effect_image; // This is no longer used. Remove it later.

    /* ---------------------------------------- */
    /* Tachi-e related variables */
    /* 0 ... left, 1 ... center, 2 ... right */
    AnimationInfo tachi_info[3];
    int human_order[3];

    /* ---------------------------------------- */
    /* Sprite related variables */
    AnimationInfo sprite_info[MAX_SPRITE_NUM];
    bool all_sprite_hide_flag;

    /* ---------------------------------------- */
    /* Parameter related variables */
    AnimationInfo *bar_info[MAX_PARAM_NUM], *prnum_info[MAX_PARAM_NUM];

    /* ---------------------------------------- */
    /* Cursor related variables */
    enum { CURSOR_WAIT_NO    = 0,
           CURSOR_NEWPAGE_NO = 1
    };
    AnimationInfo cursor_info[2];

    void loadCursor( int no, const char *str, int x, int y, bool abs_flag = false );
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
    char *font_file;
    int erase_text_window_mode;
    bool text_on_flag; // suppress the effect of erase_text_window_mode
    bool draw_cursor_flag;
    int  textgosub_clickstr_state;
    int  indent_offset;
    int  line_enter_status; // 0 ... no enter, 1 ... pretext, 2 ... body
    struct GlyphCache{
        GlyphCache *next;
        Uint16 text;
        TTF_Font *font;
        SDL_Surface *surface;
    } *root_glyph_cache, glyph_cache[NUM_GLYPH_CACHE];

    int  refreshMode();
    void setwindowCore();

    SDL_Surface *renderGlyph(TTF_Font *font, Uint16 text);
    void drawGlyph( SDL_Surface *dst_surface, FontInfo *info, SDL_Color &color, char *text, int xy[2], bool shadow_flag, AnimationInfo *cache_info, SDL_Rect *clip, SDL_Rect &dst_rect );
    void drawChar( char* text, FontInfo *info, bool flush_flag, bool lookback_flag, SDL_Surface *surface, AnimationInfo *cache_info, SDL_Rect *clip=NULL );
    void drawDoubleChars( char* text, FontInfo *info, bool flush_flag, bool lookback_flag, SDL_Surface *surface, AnimationInfo *cache_info, SDL_Rect *clip=NULL );
    void drawString( const char *str, uchar3 color, FontInfo *info, bool flush_flag, SDL_Surface *surface, SDL_Rect *rect = NULL, AnimationInfo *cache_info=NULL );
    void restoreTextBuffer();
    int  enterTextDisplayMode(bool text_flag = true);
    int  leaveTextDisplayMode();
    void doClickEnd();
    int  clickWait( char *out_text );
    int  clickNewPage( char *out_text );
    void startRuby(char *buf, FontInfo &info);
    void endRuby(bool flush_flag, bool lookback_flag, SDL_Surface *surface);
    int  textCommand();
    int  processText();

    /* ---------------------------------------- */
    /* Effect related variables */
    DirtyRect dirty_rect, dirty_rect_tmp; // only this region is updated
    int effect_counter; // counter in each effect
    int effect_timer_resolution;
    int effect_start_time;
    int effect_start_time_old;

    int  setEffect( EffectLink *effect );
    int  doEffect( EffectLink *effect, AnimationInfo *anim, int effect_image );
    void drawEffect( SDL_Rect *dst_rect, SDL_Rect *src_rect, SDL_Surface *surface );
    void generateMosaic( SDL_Surface *src_surface, int level );

    /* ---------------------------------------- */
    /* Select related variables */
    enum { SELECT_GOTO_MODE=0, SELECT_GOSUB_MODE=1, SELECT_NUM_MODE=2, SELECT_CSEL_MODE=3 };
    struct SelectLink{
        struct SelectLink *next;
        char *text;
        char *label;

        SelectLink(){
            next = NULL;
            text = label = NULL;
        };
        ~SelectLink(){
            if ( text )  delete[] text;
            if ( label ) delete[] label;
        };
    } root_select_link, *shelter_select_link;
    struct NestInfo select_label_info;
    int shortcut_mouse_line;

    void deleteSelectLink();
    struct ButtonLink *getSelectableSentence( char *buffer, FontInfo *info, bool flush_flag = true, bool nofile_flag = false );

    /* ---------------------------------------- */
    /* Sound related variables */
    enum{
        SOUND_NONE          =  0,
        SOUND_PRELOAD       =  1,
        SOUND_WAVE          =  2,
        SOUND_OGG           =  4,
        SOUND_OGG_STREAMING =  8,
        SOUND_MP3           = 16,
        SOUND_MIDI          = 32,
        SOUND_OTHER         = 64
    };
    int  cdrom_drive_number;
    char *default_cdrom_drive;
    bool cdaudio_on_flag; // false if mute
    bool volume_on_flag; // false if mute
    SDL_AudioSpec audio_format;
    bool audio_open_flag;

    bool wave_play_loop_flag;
    char *wave_file_name;

    bool midi_play_loop_flag;
    char *midi_file_name;
    Mix_Music *midi_info;

    SDL_CD *cdrom_info;
    int current_cd_track;
    bool cd_play_loop_flag;
    bool music_play_loop_flag;
    bool mp3save_flag;
    char *music_file_name;
    unsigned char *mp3_buffer;
    SMPEG *mp3_sample;
#if defined(INSANI)
    Uint32 mp3fadeout_start;
    Uint32 mp3fadeout_duration;
#endif
    OVInfo *music_ovi;
    Mix_Music *music_info;
    char *loop_bgm_name[2];

    Mix_Chunk *wave_sample[ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS];

    char *music_cmd;
    char *midi_cmd;

    int playSound(const char *filename, int format, bool loop_flag, int channel=0);
    void playCDAudio();
    int playWave(Mix_Chunk *chunk, int format, bool loop_flag, int channel);
    int playMP3();
    int playOGG(int format, unsigned char *buffer, long length, bool loop_flag, int channel);
    int playExternalMusic(bool loop_flag);
    int playMIDI(bool loop_flag);

    int playMPEG( const char *filename, bool click_flag );
    void playAVI( const char *filename, bool click_flag );
    enum { WAVE_PLAY        = 0,
           WAVE_PRELOAD     = 1,
           WAVE_PLAY_LOADED = 2
    };
    void stopBGM( bool continue_flag );
    void playClickVoice();
    void setupWaveHeader( unsigned char *buffer, int channels, int rate, unsigned long data_length );
    OVInfo *openOggVorbis(unsigned char *buf, long len, int &channels, int &rate);
    int  closeOggVorbis(OVInfo *ovi);

    /* ---------------------------------------- */
    /* Text event related variables */
    TTF_Font *text_font;
    bool new_line_skip_flag;
    int text_speed_no;

    void shadowTextDisplay( SDL_Surface *surface, SDL_Rect &clip );
    void clearCurrentTextBuffer();
    void newPage( bool next_flag );

    void flush( int refresh_mode, SDL_Rect *rect=NULL, bool clear_dirty_flag=true, bool direct_flag=false );
    void flushDirect( SDL_Rect &rect, int refresh_mode );
    void executeLabel();
    SDL_Surface *loadImage( char *file_name );
    int parseLine();

    void mouseOverCheck( int x, int y );

    /* ---------------------------------------- */
    /* Animation */
    int  proceedAnimation();
    int  estimateNextDuration( AnimationInfo *anim, SDL_Rect &rect, int minimum );
    void resetRemainingTime( int t );
    void setupAnimationInfo( AnimationInfo *anim, FontInfo *info=NULL );
    void parseTaggedString( AnimationInfo *anim );
    void drawTaggedSurface( SDL_Surface *dst_surface, AnimationInfo *anim, SDL_Rect &clip );
    void stopAnimation( int click );

    /* ---------------------------------------- */
    /* File I/O */
    void searchSaveFile( SaveFileInfo &info, int no );
    int  loadSaveFile( int no );
    void saveMagicNumber( bool output_flag );
    int  saveSaveFile( int no );

    int  loadSaveFile2( int file_version );
    void saveSaveFile2( bool output_flag );

    /* ---------------------------------------- */
    /* Image processing */
    unsigned char *resize_buffer;
    size_t resize_buffer_size;

    int  resizeSurface( SDL_Surface *src, SDL_Surface *dst );
    void shiftCursorOnButton( int diff );
    void alphaBlend( SDL_Surface *mask_surface,
                     int trans_mode, Uint32 mask_value = 255, SDL_Rect *clip=NULL );
    void alphaBlend32( SDL_Surface *dst_surface, SDL_Rect dst_rect,
                       SDL_Surface *src_surface, SDL_Color &color, SDL_Rect *clip, bool rotate_flag );
    void makeNegaSurface( SDL_Surface *surface, SDL_Rect &clip );
    void makeMonochromeSurface( SDL_Surface *surface, SDL_Rect &clip );
    void refreshSurface( SDL_Surface *surface, SDL_Rect *clip_src, int refresh_mode = REFRESH_NORMAL_MODE );
    void createBackground();

    /* ---------------------------------------- */
    /* rmenu and system call */
    bool system_menu_enter_flag;
    int  system_menu_mode;

    int  shelter_event_mode;
    bool shelter_draw_cursor_flag;
    struct TextBuffer *cached_text_buffer;

    void enterSystemCall();
    void leaveSystemCall( bool restore_flag = true );
    void executeSystemCall();

    void executeSystemMenu();
    void executeSystemSkip();
    void executeSystemAutomode();
    void executeSystemReset();
    void executeSystemEnd();
    void executeWindowErase();
    void executeSystemLoad();
    void executeSystemSave();
    void executeSystemYesNo();
    void setupLookbackButton();
    void executeSystemLookback();
};

#endif // __ONSCRIPTER_LABEL_H__
