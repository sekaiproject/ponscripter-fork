/* -*- C++ -*-
 *
 *  ONScripterLabel.cpp - Execution block parser of ONScripter
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

#include "ONScripterLabel.h"

extern void initSJIS2UTF16();
extern "C" void waveCallback( int channel );

#define DEFAULT_AUDIOBUF  4096

#define FONT_FILE "default.ttf"
#define REGISTRY_FILE "registry.txt"
#define DLL_FILE "dll.txt"
#define DEFAULT_ENV_FONT "ÇlÇr ÉSÉVÉbÉN"
#define DEFAULT_VOLUME 100

typedef int (ONScripterLabel::*FuncList)();
static struct FuncLUT{
    char command[40];
    FuncList method;
} func_lut[] = {
    {"wavestop",   &ONScripterLabel::wavestopCommand},
    {"waveloop",   &ONScripterLabel::waveCommand},
    {"wave",   &ONScripterLabel::waveCommand},
    {"waittimer",   &ONScripterLabel::waittimerCommand},
    {"wait",   &ONScripterLabel::waitCommand},
    {"vsp",   &ONScripterLabel::vspCommand},
    {"voicevol",   &ONScripterLabel::voicevolCommand},
    {"trap",   &ONScripterLabel::trapCommand},
    {"textspeed",   &ONScripterLabel::textspeedCommand},
    {"textshow",   &ONScripterLabel::textshowCommand},
    {"texton",   &ONScripterLabel::textonCommand},
    {"textoff",   &ONScripterLabel::textoffCommand},
    {"texthide",   &ONScripterLabel::texthideCommand},
    {"textclear",   &ONScripterLabel::textclearCommand},
    {"textbtnwait",   &ONScripterLabel::btnwaitCommand},
    {"texec",   &ONScripterLabel::texecCommand},
    {"tateyoko",   &ONScripterLabel::tateyokoCommand},
    {"tal", &ONScripterLabel::talCommand},
    {"tablegoto",   &ONScripterLabel::tablegotoCommand},
    {"systemcall",   &ONScripterLabel::systemcallCommand},
    {"strsp",   &ONScripterLabel::strspCommand},
    {"stop",   &ONScripterLabel::stopCommand},
    {"sp_rgb_gradation",   &ONScripterLabel::sp_rgb_gradationCommand},
    {"spstr",   &ONScripterLabel::spstrCommand},
    {"spreload",   &ONScripterLabel::spreloadCommand},
    {"splitstring",   &ONScripterLabel::splitCommand},
    {"split",   &ONScripterLabel::splitCommand},
    {"spclclk",   &ONScripterLabel::spclclkCommand},
    {"spbtn",   &ONScripterLabel::spbtnCommand},
    {"skipoff",   &ONScripterLabel::skipoffCommand},
    {"sevol",   &ONScripterLabel::sevolCommand},
    {"setwindow3",   &ONScripterLabel::setwindow3Command},
    {"setwindow2",   &ONScripterLabel::setwindow2Command},
    {"setwindow",   &ONScripterLabel::setwindowCommand},
    {"setcursor",   &ONScripterLabel::setcursorCommand},
    {"selnum",   &ONScripterLabel::selectCommand},
    {"selgosub",   &ONScripterLabel::selectCommand},
    {"selectbtnwait", &ONScripterLabel::btnwaitCommand},
    {"select",   &ONScripterLabel::selectCommand},
    {"savetime",   &ONScripterLabel::savetimeCommand},
    {"savescreenshot2",   &ONScripterLabel::savescreenshotCommand},
    {"savescreenshot",   &ONScripterLabel::savescreenshotCommand},
    {"saveon",   &ONScripterLabel::saveonCommand},
    {"saveoff",   &ONScripterLabel::saveoffCommand},
    {"savegame",   &ONScripterLabel::savegameCommand},
    {"savefileexist",   &ONScripterLabel::savefileexistCommand},
    {"rnd",   &ONScripterLabel::rndCommand},
    {"rnd2",   &ONScripterLabel::rndCommand},
    {"rmode",   &ONScripterLabel::rmodeCommand},
    {"resettimer",   &ONScripterLabel::resettimerCommand},
    {"reset",   &ONScripterLabel::resetCommand},
    {"repaint",   &ONScripterLabel::repaintCommand},
    {"quakey",   &ONScripterLabel::quakeCommand},
    {"quakex",   &ONScripterLabel::quakeCommand},
    {"quake",   &ONScripterLabel::quakeCommand},
    {"puttext",   &ONScripterLabel::puttextCommand},
    {"prnumclear",   &ONScripterLabel::prnumclearCommand},
    {"prnum",   &ONScripterLabel::prnumCommand},
    {"print",   &ONScripterLabel::printCommand},
    {"playstop",   &ONScripterLabel::playstopCommand},
    {"playonce",   &ONScripterLabel::playCommand},
    {"play",   &ONScripterLabel::playCommand},
    {"ofscpy", &ONScripterLabel::ofscopyCommand},
    {"ofscopy", &ONScripterLabel::ofscopyCommand},
    {"nega", &ONScripterLabel::negaCommand},
    {"msp", &ONScripterLabel::mspCommand},
    {"mpegplay", &ONScripterLabel::mpegplayCommand},
    {"mp3vol", &ONScripterLabel::mp3volCommand},
    {"mp3stop", &ONScripterLabel::playstopCommand},
    {"mp3save", &ONScripterLabel::mp3Command},
    {"mp3loop", &ONScripterLabel::mp3Command},
#if defined(INSANI)
    {"mp3fadeout", &ONScripterLabel::mp3fadeoutCommand},
#endif
    {"mp3", &ONScripterLabel::mp3Command},
    {"movemousecursor", &ONScripterLabel::movemousecursorCommand},
    {"monocro", &ONScripterLabel::monocroCommand},
    {"menu_window", &ONScripterLabel::menu_windowCommand},
    {"menu_full", &ONScripterLabel::menu_fullCommand},
    {"menu_automode", &ONScripterLabel::menu_automodeCommand},
    {"lsph", &ONScripterLabel::lspCommand},
    {"lsp", &ONScripterLabel::lspCommand},
    {"lr_trap",   &ONScripterLabel::trapCommand},
    {"loopbgmstop", &ONScripterLabel::loopbgmstopCommand},
    {"loopbgm", &ONScripterLabel::loopbgmCommand},
    {"lookbackflush", &ONScripterLabel::lookbackflushCommand},
    {"lookbackbutton",      &ONScripterLabel::lookbackbuttonCommand},
    {"logsp2", &ONScripterLabel::logspCommand},
    {"logsp", &ONScripterLabel::logspCommand},
    {"locate", &ONScripterLabel::locateCommand},
    {"loadgame", &ONScripterLabel::loadgameCommand},
    {"ld", &ONScripterLabel::ldCommand},
    {"jumpf", &ONScripterLabel::jumpfCommand},
    {"jumpb", &ONScripterLabel::jumpbCommand},
    {"isfull", &ONScripterLabel::isfullCommand},
    {"isskip", &ONScripterLabel::isskipCommand},
    {"ispage", &ONScripterLabel::ispageCommand},
    {"isdown", &ONScripterLabel::isdownCommand},
    {"input", &ONScripterLabel::inputCommand},
    {"indent", &ONScripterLabel::indentCommand},
    {"humanorder", &ONScripterLabel::humanorderCommand},
    {"getzxc", &ONScripterLabel::getzxcCommand},
    {"getvoicevol", &ONScripterLabel::getvoicevolCommand},
    {"getversion", &ONScripterLabel::getversionCommand},
    {"gettimer", &ONScripterLabel::gettimerCommand},
    {"getspsize", &ONScripterLabel::getspsizeCommand},
    {"getspmode", &ONScripterLabel::getspmodeCommand},
    {"getsevol", &ONScripterLabel::getsevolCommand},
    {"getscreenshot", &ONScripterLabel::getscreenshotCommand},
    {"gettext", &ONScripterLabel::gettextCommand},
    {"gettag", &ONScripterLabel::gettagCommand},
    {"gettab", &ONScripterLabel::gettabCommand},
    {"getret", &ONScripterLabel::getretCommand},
    {"getreg", &ONScripterLabel::getregCommand},
    {"getpageup", &ONScripterLabel::getpageupCommand},
    {"getpage", &ONScripterLabel::getpageCommand},
    {"getmp3vol", &ONScripterLabel::getmp3volCommand},
    {"getmousepos", &ONScripterLabel::getmouseposCommand},
    {"getlog", &ONScripterLabel::getlogCommand},
    {"getinsert", &ONScripterLabel::getinsertCommand},
    {"getfunction", &ONScripterLabel::getfunctionCommand},
    {"getenter", &ONScripterLabel::getenterCommand},
    {"getcursorpos", &ONScripterLabel::getcursorposCommand},
    {"getcursor", &ONScripterLabel::getcursorCommand},
    {"getcselstr", &ONScripterLabel::getcselstrCommand},
    {"getcselnum", &ONScripterLabel::getcselnumCommand},
    {"getbtntimer", &ONScripterLabel::gettimerCommand},
    {"getbgmvol", &ONScripterLabel::getmp3volCommand},
    {"game", &ONScripterLabel::gameCommand},
    {"fileexist", &ONScripterLabel::fileexistCommand},
    {"existspbtn", &ONScripterLabel::spbtnCommand},
    {"exec_dll", &ONScripterLabel::exec_dllCommand},
    {"exbtn_d", &ONScripterLabel::exbtnCommand},
    {"exbtn", &ONScripterLabel::exbtnCommand},
    {"erasetextwindow", &ONScripterLabel::erasetextwindowCommand},
    {"end", &ONScripterLabel::endCommand},
    {"dwavestop", &ONScripterLabel::dwavestopCommand},
    {"dwaveplayloop", &ONScripterLabel::dwaveCommand},
    {"dwaveplay", &ONScripterLabel::dwaveCommand},
    {"dwaveloop", &ONScripterLabel::dwaveCommand},
    {"dwaveload", &ONScripterLabel::dwaveCommand},
    {"dwave", &ONScripterLabel::dwaveCommand},
    {"drawtext", &ONScripterLabel::drawtextCommand},
    {"drawsp3", &ONScripterLabel::drawsp3Command},
    {"drawsp2", &ONScripterLabel::drawsp2Command},
    {"drawsp", &ONScripterLabel::drawspCommand},
    {"drawfill", &ONScripterLabel::drawfillCommand},
    {"drawclear", &ONScripterLabel::drawclearCommand},
    {"drawbg2", &ONScripterLabel::drawbg2Command},
    {"drawbg", &ONScripterLabel::drawbgCommand},
    {"draw", &ONScripterLabel::drawCommand},
    {"delay", &ONScripterLabel::delayCommand},
    {"definereset", &ONScripterLabel::defineresetCommand},
    {"csp", &ONScripterLabel::cspCommand},
    {"cselgoto", &ONScripterLabel::cselgotoCommand},
    {"cselbtn", &ONScripterLabel::cselbtnCommand},
    {"csel", &ONScripterLabel::selectCommand},
    {"click", &ONScripterLabel::clickCommand},
    {"cl", &ONScripterLabel::clCommand},
    {"chvol", &ONScripterLabel::chvolCommand},
    {"checkpage", &ONScripterLabel::checkpageCommand},
    {"cellcheckspbtn", &ONScripterLabel::spbtnCommand},
    {"cellcheckexbtn", &ONScripterLabel::exbtnCommand},
    {"cell", &ONScripterLabel::cellCommand},
    {"caption", &ONScripterLabel::captionCommand},
    {"btnwait2", &ONScripterLabel::btnwaitCommand},
    {"btnwait", &ONScripterLabel::btnwaitCommand},
    {"btntime2", &ONScripterLabel::btntimeCommand},
    {"btntime", &ONScripterLabel::btntimeCommand},
    {"btndown",  &ONScripterLabel::btndownCommand},
    {"btndef",  &ONScripterLabel::btndefCommand},
    {"btn",     &ONScripterLabel::btnCommand},
    {"br",      &ONScripterLabel::brCommand},
    {"blt",      &ONScripterLabel::bltCommand},
    {"bgmvol", &ONScripterLabel::mp3volCommand},
    {"bgmstop", &ONScripterLabel::playstopCommand},
    {"bgmonce", &ONScripterLabel::mp3Command},
    {"bgm", &ONScripterLabel::mp3Command},
    {"bgcpy",      &ONScripterLabel::bgcopyCommand},
    {"bgcopy",      &ONScripterLabel::bgcopyCommand},
    {"bg",      &ONScripterLabel::bgCommand},
    {"barclear",      &ONScripterLabel::barclearCommand},
    {"bar",      &ONScripterLabel::barCommand},
    {"avi",      &ONScripterLabel::aviCommand},
    {"automode_time",      &ONScripterLabel::automode_timeCommand},
    {"autoclick",      &ONScripterLabel::autoclickCommand},
    {"amsp",      &ONScripterLabel::amspCommand},
    {"allspresume",      &ONScripterLabel::allspresumeCommand},
    {"allsphide",      &ONScripterLabel::allsphideCommand},
    {"abssetcursor", &ONScripterLabel::setcursorCommand},
    {"", NULL}
};

static void SDL_Quit_Wrapper()
{
    SDL_Quit();
}

void ONScripterLabel::initSDL()
{
    /* ---------------------------------------- */
    /* Initialize SDL */

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO ) < 0 ){
        fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
        exit(-1);
    }
    atexit(SDL_Quit_Wrapper); // work-around for OS/2

    if( cdaudio_flag && SDL_InitSubSystem( SDL_INIT_CDROM ) < 0 ){
        fprintf( stderr, "Couldn't initialize CD-ROM: %s\n", SDL_GetError() );
        exit(-1);
    }

    if(SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == 0 && SDL_JoystickOpen(0) != NULL)
        printf( "Initialize JOYSTICK\n");

#if defined(PSP) || defined(IPODLINUX)
    SDL_ShowCursor(SDL_DISABLE);
#endif

    /* ---------------------------------------- */
    /* Initialize SDL */
    if ( TTF_Init() < 0 ){
        fprintf( stderr, "can't initialize SDL TTF\n");
        exit(-1);
    }

#if defined(INSANI)
	SDL_WM_SetIcon(IMG_Load("icon.png"), NULL);
	fprintf(stderr, "Autodetect: insanity spirit detected!\n");
#endif

#if defined(BPP16)
    screen_bpp = 16;
#else
    screen_bpp = 32;
#endif

#if defined(PDA) && defined(PDA_WIDTH)
    screen_ratio1 *= PDA_WIDTH;
    screen_ratio2 *= 320;
    screen_width   = screen_width  * PDA_WIDTH / 320;
    screen_height  = screen_height * PDA_WIDTH / 320;
#endif

    screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG|(fullscreen_mode?SDL_FULLSCREEN:0) );

    /* ---------------------------------------- */
    /* Check if VGA screen is available. */
#if defined(PDA) && (PDA_WIDTH==640)
    if ( screen_surface == NULL ){
        screen_ratio1 /= 2;
        screen_width  /= 2;
        screen_height /= 2;
        screen_surface = SDL_SetVideoMode( screen_width, screen_height, screen_bpp, DEFAULT_VIDEO_SURFACE_FLAG|(fullscreen_mode?SDL_FULLSCREEN:0) );
    }
#endif
    underline_value = screen_height - 1;

    if ( screen_surface == NULL ) {
        fprintf( stderr, "Couldn't set %dx%dx%d video mode: %s\n",
                 screen_width, screen_height, screen_bpp, SDL_GetError() );
        exit(-1);
    }
    printf("Display: %d x %d (%d bpp)\n", screen_width, screen_height, screen_bpp);

    initSJIS2UTF16();

    wm_title_string = new char[ strlen(DEFAULT_WM_TITLE) + 1 ];
    memcpy( wm_title_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_TITLE) + 1 );
    wm_icon_string = new char[ strlen(DEFAULT_WM_ICON) + 1 ];
    memcpy( wm_icon_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_ICON) + 1 );
    SDL_WM_SetCaption( wm_title_string, wm_icon_string );

    openAudio();
}

void ONScripterLabel::openAudio()
{
#if defined(PDA) && !defined(PSP)
    if ( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF ) < 0 ){
#else
    if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF ) < 0 ){
#endif
        fprintf(stderr, "Couldn't open audio device!\n"
                "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else{
        int freq;
        Uint16 format;
        int channels;

        Mix_QuerySpec( &freq, &format, &channels);
        printf("Audio: %d Hz %d bit %s\n", freq,
               (format&0xFF),
               (channels > 1) ? "stereo" : "mono");
        audio_format.format = format;
        audio_format.freq = freq;
        audio_format.channels = channels;

        audio_open_flag = true;

        Mix_AllocateChannels( ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS );
        Mix_ChannelFinished( waveCallback );
    }
}

ONScripterLabel::ONScripterLabel()
{
    cdrom_drive_number = 0;
    cdaudio_flag = false;
    default_font = NULL;
    registry_file = NULL;
    setStr( &registry_file, REGISTRY_FILE );
    dll_file = NULL;
    setStr( &dll_file, DLL_FILE );
    getret_str = NULL;
    enable_wheeldown_advance_flag = false;
    disable_rescale_flag = false;
    edit_flag = false;
    key_exe_file = NULL;
    fullscreen_mode = false;
    window_mode = false;
#if defined(INSANI)
	skip_to_wait = 0;
#endif

    for (int i=0 ; i<NUM_GLYPH_CACHE ; i++){
        if (i != NUM_GLYPH_CACHE-1) glyph_cache[i].next = &glyph_cache[i+1];
        glyph_cache[i].font = NULL;
        glyph_cache[i].surface = NULL;
    }
    glyph_cache[NUM_GLYPH_CACHE-1].next = NULL;
    root_glyph_cache = &glyph_cache[0];

    // External Players
    music_cmd = getenv("PLAYER_CMD");
    midi_cmd  = getenv("MUSIC_CMD");
}

ONScripterLabel::~ONScripterLabel()
{
    reset();
}

void ONScripterLabel::enableCDAudio(){
    cdaudio_flag = true;
}

void ONScripterLabel::setCDNumber(int cdrom_drive_number)
{
    this->cdrom_drive_number = cdrom_drive_number;
}

void ONScripterLabel::setFontFile(const char *filename)
{
    setStr(&default_font, filename);
}

void ONScripterLabel::setRegistryFile(const char *filename)
{
    setStr(&registry_file, filename);
}

void ONScripterLabel::setDLLFile(const char *filename)
{
    setStr(&dll_file, filename);
}

void ONScripterLabel::setArchivePath(const char *path)
{
    if (archive_path) delete[] archive_path;
    archive_path = new char[ RELATIVEPATHLENGTH + strlen(path) + 2 ];
    sprintf( archive_path, RELATIVEPATH "%s%c", path, DELIMITER );
}

void ONScripterLabel::setFullscreenMode()
{
    fullscreen_mode = true;
}

void ONScripterLabel::setWindowMode()
{
    window_mode = true;
}

void ONScripterLabel::enableButtonShortCut()
{
    force_button_shortcut_flag = true;
}

void ONScripterLabel::enableWheelDownAdvance()
{
    enable_wheeldown_advance_flag = true;
}

void ONScripterLabel::disableRescale()
{
    disable_rescale_flag = true;
}

void ONScripterLabel::enableEdit()
{
    edit_flag = true;
}

void ONScripterLabel::setKeyEXE(const char *filename)
{
    setStr(&key_exe_file, filename);
}

int ONScripterLabel::init()
{
    if (archive_path == NULL) archive_path = "";

    if (key_exe_file){
        createKeyTable( key_exe_file );
        script_h.setKeyTable( key_table );
    }

    if ( open() ) return -1;


    initSDL();

    image_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, 1, 1, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );

    accumulation_surface = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_src_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_dst_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    SDL_SetAlpha( accumulation_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( effect_src_surface, 0, SDL_ALPHA_OPAQUE );
    SDL_SetAlpha( effect_dst_surface, 0, SDL_ALPHA_OPAQUE );
    screenshot_surface   = NULL;
    text_info.num_of_cells = 1;
    text_info.allocImage( screen_width, screen_height );
    text_info.fill(0, 0, 0, 0);

    // ----------------------------------------
    // Initialize font
    if ( default_font ){
        font_file = new char[ strlen(default_font) + 1 ];
        sprintf( font_file, "%s", default_font );
    }
    else{
        font_file = new char[ strlen(archive_path) + strlen(FONT_FILE) + 1 ];
        sprintf( font_file, "%s%s", archive_path, FONT_FILE );
    }

    // ----------------------------------------
    // Sound related variables
    this->cdaudio_flag = cdaudio_flag;
    cdrom_info = NULL;
    if ( cdaudio_flag ){
        if ( cdrom_drive_number >= 0 && cdrom_drive_number < SDL_CDNumDrives() )
            cdrom_info = SDL_CDOpen( cdrom_drive_number );
        if ( !cdrom_info ){
            fprintf(stderr, "Couldn't open default CD-ROM: %s\n", SDL_GetError());
        }
        else if ( cdrom_info && !CD_INDRIVE( SDL_CDStatus( cdrom_info ) ) ) {
            fprintf( stderr, "no CD-ROM in the drive\n" );
            SDL_CDClose( cdrom_info );
            cdrom_info = NULL;
        }
    }

    wave_file_name = NULL;
    midi_file_name = NULL;
    midi_info  = NULL;
    mp3_sample = NULL;
    music_file_name = NULL;
    mp3_buffer = NULL;
    music_info = NULL;
    music_ovi = NULL;

    loop_bgm_name[0] = NULL;
    loop_bgm_name[1] = NULL;

    int i;
    for (i=0 ; i<ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS ; i++) wave_sample[i] = NULL;

    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    trap_dist = NULL;
    resize_buffer = new unsigned char[16];
    resize_buffer_size = 16;

    for (i=0 ; i<MAX_PARAM_NUM ; i++) bar_info[i] = prnum_info[i] = NULL;

    defineresetCommand();
    readToken();

    if ( sentence_font.openFont( font_file, screen_ratio1, screen_ratio2 ) == NULL ){
        fprintf( stderr, "can't open font file: %s\n", font_file );
        return -1;
    }

    loadEnvData();

    return 0;
}

void ONScripterLabel::reset()
{
    automode_flag = false;
    automode_time = 3000;
    autoclick_time = 0;
    remaining_time = -1;
    btntime2_flag = false;
    btntime_value = 0;
    btnwait_time = 0;

    disableGetButtonFlag();

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    key_pressed_flag = false;
    shift_pressed_status = 0;
    ctrl_pressed_status = 0;
    display_mode = next_display_mode = NORMAL_DISPLAY_MODE;
    current_refresh_mode = REFRESH_NORMAL_MODE;
    event_mode = IDLE_EVENT_MODE;
    all_sprite_hide_flag = false;

    if (resize_buffer_size != 16){
        delete[] resize_buffer;
        resize_buffer = new unsigned char[16];
        resize_buffer_size = 16;
    }

    current_over_button = 0;
    variable_edit_mode = NOT_EDIT_MODE;

    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE | REFRESH_TEXT_MODE;
    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = false;

    resetSentenceFont();

    setStr(&getret_str, NULL);
    getret_int = 0;

    // ----------------------------------------
    // Sound related variables

    wave_play_loop_flag = false;
    midi_play_loop_flag = false;
    music_play_loop_flag = false;
    cd_play_loop_flag = false;
    mp3save_flag = false;
    current_cd_track = -1;

    resetSub();

    /* ---------------------------------------- */
    /* Load global variables if available */
    if ( loadFileIOBuf( "gloval.sav" ) == 0 ||
         loadFileIOBuf( "global.sav" ) == 0 )
        readVariables( script_h.global_variable_border, VARIABLE_RANGE );
}

void ONScripterLabel::resetSub()
{
    int i;

    for ( i=0 ; i<script_h.global_variable_border ; i++ )
        script_h.variable_data[i].reset(false);

    for ( i=0 ; i<3 ; i++ ) human_order[i] = 2-i; // "rcl"

    erase_text_window_mode = 1;
    skip_flag = false;
    monocro_flag = false;
    nega_mode = 0;
    clickstr_state = CLICK_NONE;
    trap_mode = TRAP_NONE;
    setStr(&trap_dist, NULL);

    saveon_flag = true;
    internal_saveon_flag = true;

    textgosub_clickstr_state = CLICK_NONE;
    indent_offset = 0;
    line_enter_status = 0;

    resetSentenceFont();

    deleteNestInfo();
    deleteButtonLink();
    deleteSelectLink();

    stopCommand();
    loopbgmstopCommand();
    setStr(&loop_bgm_name[1], NULL);

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    setStr( &bg_info.file_name, "black" );
    createBackground();
    for (i=0 ; i<3 ; i++) tachi_info[i].reset();
    for (i=0 ; i<MAX_SPRITE_NUM ; i++) sprite_info[i].reset();
    barclearCommand();
    prnumclearCommand();
    for (i=0 ; i<2 ; i++) cursor_info[i].reset();
    for (i=0 ; i<4 ; i++) lookback_info[i].reset();
    sentence_font_info.reset();

    dirty_rect.fill( screen_width, screen_height );
}

void ONScripterLabel::resetSentenceFont()
{
    sentence_font.reset();
    sentence_font.font_size_xy[0] = DEFAULT_FONT_SIZE;
    sentence_font.font_size_xy[1] = DEFAULT_FONT_SIZE;
    sentence_font.top_xy[0] = 21;
    sentence_font.top_xy[1] = 16;// + sentence_font.font_size;
    sentence_font.num_xy[0] = 23;
    sentence_font.num_xy[1] = 16;
    sentence_font.pitch_xy[0] = sentence_font.font_size_xy[0];
    sentence_font.pitch_xy[1] = 2 + sentence_font.font_size_xy[1];
    sentence_font.wait_time = 20;
    sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0x99;
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width+1;
    sentence_font_info.pos.h = screen_height+1;
}

void ONScripterLabel::flush( int refresh_mode, SDL_Rect *rect, bool clear_dirty_flag, bool direct_flag )
{

    if ( direct_flag ){
        flushDirect( *rect, refresh_mode );
    }
    else{
        if ( rect ) dirty_rect.add( *rect );

        if ( dirty_rect.area > 0 ){
            if ( dirty_rect.area >= dirty_rect.bounding_box.w * dirty_rect.bounding_box.h ){
                flushDirect( dirty_rect.bounding_box, refresh_mode );
            }
            else{
                for ( int i=0 ; i<dirty_rect.num_history ; i++ ){
                    //printf("%d: ", i );
                    flushDirect( dirty_rect.history[i], refresh_mode );
                }
            }
        }
    }

    if ( clear_dirty_flag ) dirty_rect.clear();
}

void ONScripterLabel::flushDirect( SDL_Rect &rect, int refresh_mode )
{
    //printf("flush %d: %d %d %d %d\n", refresh_mode, rect.x, rect.y, rect.w, rect.h );

    refreshSurface( accumulation_surface, &rect, refresh_mode );

    SDL_BlitSurface( accumulation_surface, &rect, screen_surface, &rect );
    SDL_UpdateRect( screen_surface, rect.x, rect.y, rect.w, rect.h );
}

void ONScripterLabel::mouseOverCheck( int x, int y )
{
    int c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    ButtonLink *p_button_link = root_button_link.next;
    while( p_button_link ){
        if ( x >= p_button_link->select_rect.x && x < p_button_link->select_rect.x + p_button_link->select_rect.w &&
             y >= p_button_link->select_rect.y && y < p_button_link->select_rect.y + p_button_link->select_rect.h ){
            button = p_button_link->no;
            break;
        }
        p_button_link = p_button_link->next;
        c++;
    }

    if ( current_over_button != button ){
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = {0, 0, 0, 0};
        SDL_Rect check_dst_rect = {0, 0, 0, 0};
        if ( current_over_button != 0 ){
            current_button_link->show_flag = 0;
            check_src_rect = current_button_link->image_rect;
            if ( current_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                 current_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                sprite_info[ current_button_link->sprite_no ].visible = true;
                sprite_info[ current_button_link->sprite_no ].setCell(0);
            }
            else if ( current_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON ){
                current_button_link->show_flag = 1;
                current_button_link->anim[0]->visible = true;
                current_button_link->anim[0]->setCell(0);
            }
            else if ( current_button_link->anim[1] != NULL ){
                current_button_link->show_flag = 2;
            }
            dirty_rect.add( current_button_link->image_rect );
        }

        if ( exbtn_d_button_link.exbtn_ctl ){
            decodeExbtnControl( accumulation_surface, exbtn_d_button_link.exbtn_ctl, &check_src_rect, &check_dst_rect );
        }

        if ( p_button_link ){
            if ( system_menu_mode != SYSTEM_NULL ){
                if ( menuselectvoice_file_name[MENUSELECTVOICE_OVER] )
                    playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            else{
                if ( selectvoice_file_name[SELECTVOICE_OVER] )
                    playSound(selectvoice_file_name[SELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            check_dst_rect = p_button_link->image_rect;
            if ( p_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                 p_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                sprite_info[ p_button_link->sprite_no ].setCell(1);
                sprite_info[ p_button_link->sprite_no ].visible = true;
                if ( p_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    decodeExbtnControl( accumulation_surface, p_button_link->exbtn_ctl, &check_src_rect, &check_dst_rect );
                }
            }
            else if ( p_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON ){
                p_button_link->show_flag = 1;
                p_button_link->anim[0]->visible = true;
                p_button_link->anim[0]->setCell(1);
            }
            else if ( p_button_link->button_type == ButtonLink::NORMAL_BUTTON ||
                      p_button_link->button_type == ButtonLink::LOOKBACK_BUTTON ){
                p_button_link->show_flag = 1;
            }
            dirty_rect.add( p_button_link->image_rect );
            current_button_link = p_button_link;
            shortcut_mouse_line = c;
        }

        flush( refreshMode() );
        dirty_rect = dirty;
    }
    current_over_button = button;
}

void ONScripterLabel::executeLabel()
{
  executeLabelTop:

    while ( current_line<current_label_info.num_of_lines ){
        if ( debug_level > 0 )
            printf("*****  executeLabel %s:%d/%d:%d:%d *****\n",
                   current_label_info.name,
                   current_line,
                   current_label_info.num_of_lines,
                   string_buffer_offset, display_mode );

        if ( script_h.getStringBuffer()[0] == '~' ){
            last_tilde.next_script = script_h.getNext();
            readToken();
            continue;
        }
        if ( break_flag && !script_h.isName("next") ){
            if ( script_h.getStringBuffer()[string_buffer_offset] == 0x0a )
                current_line++;

            if ( script_h.getStringBuffer()[string_buffer_offset] != ':' &&
                 script_h.getStringBuffer()[string_buffer_offset] != ';' &&
                 script_h.getStringBuffer()[string_buffer_offset] != 0x0a )
                script_h.skipToken();

            readToken();
            continue;
        }

        if ( kidokuskip_flag && skip_flag && kidokumode_flag && !script_h.isKidoku() ) skip_flag = false;

        char *current = script_h.getCurrent();
        int ret = ScriptParser::parseLine();
        if ( ret == RET_NOMATCH ) ret = this->parseLine();

        if ( ret & RET_SKIP_LINE ){
            script_h.skipLine();
            if (++current_line >= current_label_info.num_of_lines) break;
        }

        if ( ret & RET_REREAD ) script_h.setCurrent( current );

        if (!(ret & RET_NOREAD)){
            if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a){
                string_buffer_offset = 0;
                if (++current_line >= current_label_info.num_of_lines) break;
            }
            readToken();
        }

        if ( ret & RET_WAIT ) return;
    }

    current_label_info = script_h.lookupLabelNext( current_label_info.name );
    current_line = 0;

    if ( current_label_info.start_address != NULL ){
        script_h.setCurrent( current_label_info.label_header );
        readToken();
        goto executeLabelTop;
    }

    fprintf( stderr, " ***** End *****\n");
    endCommand();
}

int ONScripterLabel::parseLine( )
{
    int ret, lut_counter = 0;
    const char *s_buf = script_h.getStringBuffer();
    const char *cmd = script_h.getStringBuffer();
    if (cmd[0] == '_') cmd++;

    if ( !script_h.isText() ){
        while( func_lut[ lut_counter ].method ){
            if ( !strcmp( func_lut[ lut_counter ].command, cmd ) ){
                return (this->*func_lut[ lut_counter ].method)();
            }
            lut_counter++;
        }

        if ( s_buf[0] == 0x0a )
            return RET_CONTINUE;
        else if ( s_buf[0] == 'v' && s_buf[1] >= '0' && s_buf[1] <= '9' )
            return vCommand();
        else if ( s_buf[0] == 'd' && s_buf[1] == 'v' && s_buf[2] >= '0' && s_buf[2] <= '9' )
            return dvCommand();

        fprintf( stderr, " command [%s] is not supported yet!!\n", s_buf );

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text */
    if ( current_mode == DEFINE_MODE ) errorAndExit( "text cannot be displayed in define section." );
    ret = textCommand();

#if defined(ENABLE_1BYTE_CHAR) && defined(FORCE_1BYTE_CHAR)
    if (script_h.getStringBuffer()[string_buffer_offset] == ' '){
        char *tmp = strchr(script_h.getStringBuffer()+string_buffer_offset+1, ' ');
        if (!tmp)
            tmp = script_h.getStringBuffer() + strlen(script_h.getStringBuffer());
        int len = tmp - script_h.getStringBuffer() - string_buffer_offset - 1;
#if defined(INSANI)
		/*
		 * This block takes the current word being considered for line-wrapping and checks
		 * it to see if it contains an inline command of forms:
		 *   !s<int>
		 *   !d<int>
		 *   !w<int>
		 *   !sd
		 *   #rrggbb
		 * If it does, then we subtract the length of the command from the length of the
		 * word, then compare it to see if it is at the end of a line or not.
		 *
		 *
		 */
		char tocheck[255];
		char *tocheckiterator;
        if(len > 0){
        	strncpy (tocheck, script_h.getStringBuffer()+string_buffer_offset+1, len);
        	tocheck[len] = '\0';
		}

        // In the case of !s ...
        if(strstr(tocheck, "!s"))
        {
			tocheckiterator = strstr(tocheck, "!s");

			// ... we loop for every !s we find ...
			while(strstr(tocheckiterator, "!s"))
			{
				tocheckiterator = strstr(tocheckiterator, "!s");

				// There are two possible cases here -- either we're seeing !sd -- in
				// which case we subtract 3 from the length -- or !s<number> -- in which
				// case we subtract some variable number from the length.

				// case: !sd
				if(tocheckiterator[2] == 'd')
				{
					strcpy(tocheckiterator, tocheckiterator+3);
					len = len - 3;
				}
				// case: !s<number>
				else
				{
					int bang_s_flag = 1;
					int bang_s_num = 0;
					char bang_s_iterator;

					// Here, we walk through the characters that fall after the !s -- for instance,
					// if tocheckiterator starts out as !s3000oogabooga, then bang_s_iterator will
					// be 3, then 0, then 0, then 0, and finally o -- at which point it detects that
					// we're no longer in a number and exits out.
					while(bang_s_flag == 1)
					{
						bang_s_iterator = tocheckiterator[2+bang_s_num];

						if((bang_s_iterator >= '0') && (bang_s_iterator <= '9'))
						{
							bang_s_num++;
						}
						else bang_s_flag = 0;
					}
					// Then, so long as it wasn't a solitary !s with no numbers after it (in which
					// case it should be printed), we subtract the requisite number from the length --
					// in the case of !s1 it would be 3, and in the case of !s20 it would be 4, etc.
					if(bang_s_num > 0)
					{
						len = len - bang_s_num - 2;
						strcpy(tocheckiterator, tocheckiterator+bang_s_num+2);
					}
					else strcpy(tocheckiterator, tocheckiterator+1);
				}
			}
		}

		// In the case of !d<int> ...
		if(strstr(tocheck, "!d"))
        {
			tocheckiterator = strstr(tocheck, "!d");

			// ... we loop for every !d we find ...
			while(strstr(tocheckiterator, "!d"))
			{
				tocheckiterator = strstr(tocheckiterator, "!d");

				// Unlike in the case of !s, there's only one possible case here.

				// case: !d<number>
				int bang_d_flag = 1;
				int bang_d_num = 0;
				char bang_d_iterator;

				// Follows the pattern of !s<num> as above.
				while(bang_d_flag == 1)
				{
					bang_d_iterator = tocheckiterator[2+bang_d_num];

					if((bang_d_iterator >= '0') && (bang_d_iterator <= '9'))
					{
						bang_d_num++;
					}
					else bang_d_flag = 0;
				}
				// Follows the pattern of !s<num> as above.
				if(bang_d_num > 0)
				{
					len = len - bang_d_num - 2;
					strcpy(tocheckiterator, tocheckiterator+bang_d_num+2);
				}
				else strcpy(tocheckiterator, tocheckiterator+1);
			}
		}

		// In the case of !w<int> ...
		if(strstr(tocheck, "!w"))
        {
			tocheckiterator = strstr(tocheck, "!w");

			// ... we loop for every !w we find ...
			while(strstr(tocheckiterator, "!w"))
			{
				tocheckiterator = strstr(tocheckiterator, "!w");

				// Unlike in the case of !s, there's only one possible case here.

				// case: !w<number>
				int bang_w_flag = 1;
				int bang_w_num = 0;
				char bang_w_iterator;

				// Follows the pattern of !s<num> as above.
				while(bang_w_flag == 1)
				{
					bang_w_iterator = tocheckiterator[2+bang_w_num];

					if((bang_w_iterator >= '0') && (bang_w_iterator <= '9'))
					{
						bang_w_num++;
					}
					else bang_w_flag = 0;
				}
				// Follows the pattern of !s<num> as above.
				if(bang_w_num > 0)
				{
					len = len - bang_w_num - 2;
					strcpy(tocheckiterator, tocheckiterator+bang_w_num+2);
				}
				else strcpy(tocheckiterator, tocheckiterator+1);
			}
		}

		// In the case of #rrggbb ...
		if(strstr(tocheck, "#"))
		{
			tocheckiterator = strstr(tocheck, "#");

			while(strstr(tocheckiterator, "#"))
			{
				tocheckiterator = strstr(tocheckiterator, "#");

				int pound_color_flag = 1;
				int pound_color_num = 0;
				char pound_color_iterator;

				while(pound_color_flag == 1)
				{
					pound_color_iterator = tocheckiterator[1+pound_color_num];

					if( ((pound_color_iterator >= '0') && (pound_color_iterator <= '9')) ||
					    ((pound_color_iterator >= 'a') && (pound_color_iterator <= 'f')) ||
					    ((pound_color_iterator >= 'A') && (pound_color_iterator <= 'F')) )
					{
						pound_color_num++;
					}
					else pound_color_flag = 0;
				}
				if(pound_color_num >= 6)
				{
					len = len - 7;
					strcpy(tocheckiterator, tocheckiterator+7);
				}
				else strcpy(tocheckiterator, tocheckiterator+1);
			}
		}
#endif
        if (len > 0 && sentence_font.isEndOfLine(len)){
            current_text_buffer->addBuffer( 0x0a );
            sentence_font.newLine();
        }
    }
#endif
    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a){
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag){
            current_text_buffer->addBuffer( 0x0a );
            sentence_font.newLine();
            for (int i=0 ; i<indent_offset ; i++){
                current_text_buffer->addBuffer(((char*)"Å@")[0]);
                current_text_buffer->addBuffer(((char*)"Å@")[1]);
                sentence_font.advanceCharInHankaku(2);
            }
        }
        //event_mode = IDLE_EVENT_MODE;
        line_enter_status = 0;
    }

    return ret;
}

SDL_Surface *ONScripterLabel::loadImage( char *file_name )
{
    if ( !file_name ) return NULL;
    unsigned long length = script_h.cBR->getFileLength( file_name );
    if ( length == 0 ){
        fprintf( stderr, " *** can't find file [%s] ***\n", file_name );
        return NULL;
    }
    if ( filelog_flag )
        script_h.findAndAddLog( script_h.log_info[ScriptHandler::FILE_LOG], file_name, true );
    //printf(" ... loading %s length %ld\n", file_name, length );
    unsigned char *buffer = new unsigned char[length];
    int location;
    script_h.cBR->getFile( file_name, buffer, &location );
    SDL_Surface *tmp = IMG_Load_RW(SDL_RWFromMem( buffer, length ), 1);

    char *ext = strrchr(file_name, '.');
    if ( !tmp && ext && (!strcmp( ext+1, "JPG" ) || !strcmp( ext+1, "jpg" ) ) ){
        fprintf( stderr, " *** force-loading a JPG image [%s]\n", file_name );
        SDL_RWops *src = SDL_RWFromMem( buffer, length );
        tmp = IMG_LoadJPG_RW(src);
        SDL_RWclose(src);
    }

    delete[] buffer;
    if ( !tmp ){
        fprintf( stderr, " *** can't load file [%s] ***\n", file_name );
        return NULL;
    }

    SDL_Surface *ret = SDL_ConvertSurface( tmp, image_surface->format, SDL_SWSURFACE );
    if ( ret &&
         screen_ratio2 != screen_ratio1 &&
         (!disable_rescale_flag || location == BaseReader::ARCHIVE_TYPE_NONE) )
    {
        SDL_Surface *src_s = ret;

        int w, h;
        if ( (w = src_s->w * screen_ratio1 / screen_ratio2) == 0 ) w = 1;
        if ( (h = src_s->h * screen_ratio1 / screen_ratio2) == 0 ) h = 1;
        SDL_PixelFormat *fmt = image_surface->format;
        ret = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h,
                                    fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );

        resizeSurface( src_s, ret );
        SDL_FreeSurface( src_s );
    }
    SDL_FreeSurface( tmp );

    return ret;
}

/* ---------------------------------------- */
void ONScripterLabel::deleteButtonLink()
{
    ButtonLink *b1 = root_button_link.next;

    while( b1 ){
        ButtonLink *b2 = b1;
        b1 = b1->next;
        delete b2;
    }
    root_button_link.next = NULL;

    if ( exbtn_d_button_link.exbtn_ctl ) delete[] exbtn_d_button_link.exbtn_ctl;
    exbtn_d_button_link.exbtn_ctl = NULL;
}

void ONScripterLabel::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = 0;
    current_button_link = root_button_link.next;
    SDL_GetMouseState( &mx, &my );
    mouseOverCheck( mx, my );
}

/* ---------------------------------------- */
/* Delete select link */
void ONScripterLabel::deleteSelectLink()
{
    SelectLink *link, *last_select_link = root_select_link.next;

    while ( last_select_link ){
        link = last_select_link;
        last_select_link = last_select_link->next;
        delete link;
    }
    root_select_link.next = NULL;
}

void ONScripterLabel::clearCurrentTextBuffer()
{
    sentence_font.clear();

    int num = (sentence_font.num_xy[0]*2+1)*sentence_font.num_xy[1];
    if (sentence_font.getTateyokoMode() == FontInfo::TATE_MODE)
        num = (sentence_font.num_xy[1]*2+1)*sentence_font.num_xy[1];

    if ( current_text_buffer->buffer2 &&
         current_text_buffer->num != num ){
        delete[] current_text_buffer->buffer2;
        current_text_buffer->buffer2 = NULL;
    }
    if ( !current_text_buffer->buffer2 ){
        current_text_buffer->buffer2 = new char[num];
        current_text_buffer->num = num;
    }

    current_text_buffer->buffer2_count = 0;
    num_chars_in_sentence = 0;
    internal_saveon_flag = true;

    text_info.fill( 0, 0, 0, 0 );
    cached_text_buffer = current_text_buffer;
}

void ONScripterLabel::shadowTextDisplay( SDL_Surface *surface, SDL_Rect &clip )
{
    if ( current_font->is_transparent ){

        SDL_Rect rect = {0, 0, screen_width, screen_height};
        if ( current_font == &sentence_font )
            rect = sentence_font_info.pos;

        if ( AnimationInfo::doClipping( &rect, &clip ) ) return;

        if ( rect.x + rect.w > surface->w ) rect.w = surface->w - rect.x;
        if ( rect.y + rect.h > surface->h ) rect.h = surface->h - rect.y;

        SDL_LockSurface( surface );
        ONSBuf *buf = (ONSBuf *)surface->pixels + rect.y * surface->w + rect.x;

        SDL_PixelFormat *fmt = surface->format;
        uchar3 color;
        color[0] = current_font->window_color[0] >> fmt->Rloss;
        color[1] = current_font->window_color[1] >> fmt->Gloss;
        color[2] = current_font->window_color[2] >> fmt->Bloss;

        for ( int i=rect.y ; i<rect.y + rect.h ; i++ ){
            for ( int j=rect.x ; j<rect.x + rect.w ; j++, buf++ ){
                *buf = (((*buf & fmt->Rmask) >> fmt->Rshift) * color[0] >> (8-fmt->Rloss)) << fmt->Rshift |
                    (((*buf & fmt->Gmask) >> fmt->Gshift) * color[1] >> (8-fmt->Gloss)) << fmt->Gshift |
                    (((*buf & fmt->Bmask) >> fmt->Bshift) * color[2] >> (8-fmt->Bloss)) << fmt->Bshift;
            }
            buf += surface->w - rect.w;
        }

        SDL_UnlockSurface( surface );
    }
    else if ( sentence_font_info.image_surface ){
        drawTaggedSurface( surface, &sentence_font_info, clip );
    }
}

void ONScripterLabel::newPage( bool next_flag )
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( current_text_buffer->buffer2_count != 0 ){
        current_text_buffer = current_text_buffer->next;
        if ( start_text_buffer == current_text_buffer )
            start_text_buffer = start_text_buffer->next;
    }

    if ( next_flag ){
        indent_offset = 0;
        //line_enter_status = 0;
    }

    clearCurrentTextBuffer();

    flush( refreshMode(), &sentence_font_info.pos );
}

struct ONScripterLabel::ButtonLink *ONScripterLabel::getSelectableSentence( char *buffer, FontInfo *info, bool flush_flag, bool nofile_flag )
{
    int current_text_xy[2];
    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];

    ButtonLink *button_link = new ButtonLink();
    button_link->button_type = ButtonLink::TMP_SPRITE_BUTTON;
    button_link->show_flag = 1;

    AnimationInfo *anim = new AnimationInfo();
    button_link->anim[0] = anim;

    anim->trans_mode = AnimationInfo::TRANS_STRING;
    anim->is_single_line = false;
    anim->num_of_cells = 2;
    anim->color_list = new uchar3[ anim->num_of_cells ];
    for (int i=0 ; i<3 ; i++){
        if (nofile_flag)
            anim->color_list[0][i] = info->nofile_color[i];
        else
            anim->color_list[0][i] = info->off_color[i];
        anim->color_list[1][i] = info->on_color[i];
    }
    setStr( &anim->file_name, buffer );
    anim->pos.x = info->x() * screen_ratio1 / screen_ratio2;
    anim->pos.y = info->y() * screen_ratio1 / screen_ratio2;
    anim->visible = true;

    setupAnimationInfo( anim, info );
    button_link->select_rect = button_link->image_rect = anim->pos;

    info->newLine();
    if (info->getTateyokoMode() == FontInfo::YOKO_MODE)
        info->xy[0] = current_text_xy[0];
    else
        info->xy[1] = current_text_xy[1];

    dirty_rect.add( button_link->image_rect );

    return button_link;
}

void ONScripterLabel::decodeExbtnControl( SDL_Surface *surface, const char *ctl_str, SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect )
{
    char sound_name[256];
    int i, sprite_no, sprite_no2, cell_no;

    while( char com = *ctl_str++ ){
        if (com == 'C' || com == 'c'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            sprite_no2 = sprite_no;
            cell_no = -1;
            if ( *ctl_str == '-' ){
                ctl_str++;
                sprite_no2 = getNumberFromBuffer( &ctl_str );
            }
            for (i=sprite_no ; i<=sprite_no2 ; i++)
                refreshSprite( surface, i, false, cell_no, NULL, NULL );
        }
        else if (com == 'P' || com == 'p'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if ( *ctl_str == ',' ){
                ctl_str++;
                cell_no = getNumberFromBuffer( &ctl_str );
            }
            else
                cell_no = 0;
            refreshSprite( surface, sprite_no, true, cell_no, check_src_rect, check_dst_rect );
        }
        else if (com == 'S' || com == 's'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if      (sprite_no < 0) sprite_no = 0;
            else if (sprite_no >= ONS_MIX_CHANNELS) sprite_no = ONS_MIX_CHANNELS-1;
            if ( *ctl_str != ',' ) continue;
            ctl_str++;
            if ( *ctl_str != '(' ) continue;
            ctl_str++;
            char *buf = sound_name;
            while (*ctl_str != ')' && *ctl_str != '\0' ) *buf++ = *ctl_str++;
            *buf++ = '\0';
            playSound(sound_name, SOUND_WAVE|SOUND_OGG, false, sprite_no);
            if ( *ctl_str == ')' ) ctl_str++;
        }
        else if (com == 'M' || com == 'm'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            SDL_Rect rect = sprite_info[ sprite_no ].pos;
            if ( *ctl_str != ',' ) continue;
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].pos.x = getNumberFromBuffer( &ctl_str ) * screen_ratio1 / screen_ratio2;
            if ( *ctl_str != ',' ) continue;
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].pos.y = getNumberFromBuffer( &ctl_str ) * screen_ratio1 / screen_ratio2;
            dirty_rect.add( rect );
            sprite_info[ sprite_no ].visible = true;
            dirty_rect.add( sprite_info[ sprite_no ].pos );
        }
    }
}

void ONScripterLabel::loadCursor( int no, const char *str, int x, int y, bool abs_flag )
{
    cursor_info[ no ].setImageName( str );
    cursor_info[ no ].pos.x = x;
    cursor_info[ no ].pos.y = y;

    parseTaggedString( &cursor_info[ no ] );
    setupAnimationInfo( &cursor_info[ no ] );
    if ( filelog_flag )
        script_h.findAndAddLog( script_h.log_info[ScriptHandler::FILE_LOG], cursor_info[ no ].file_name, true ); // a trick for save file
    cursor_info[ no ].abs_flag = abs_flag;
    if ( cursor_info[ no ].image_surface )
        cursor_info[ no ].visible = true;
    else
        cursor_info[ no ].remove();
}

void ONScripterLabel::saveAll()
{
    saveEnvData();
    saveGlovalData();
    if ( filelog_flag )  writeLog( script_h.log_info[ScriptHandler::FILE_LOG] );
    if ( labellog_flag ) writeLog( script_h.log_info[ScriptHandler::LABEL_LOG] );
    if ( kidokuskip_flag ) script_h.saveKidokuData();
}

void ONScripterLabel::loadEnvData()
{
    volume_on_flag = true;
    text_speed_no = 1;
    draw_one_page_flag = false;
    default_env_font = NULL;
    cdaudio_on_flag = true;
    default_cdrom_drive = NULL;
    kidokumode_flag = true;

    if (loadFileIOBuf( "envdata" ) == 0){
        if (readInt() == 1 && window_mode == false) menu_fullCommand();
        if (readInt() == 0) volume_on_flag = false;
        text_speed_no = readInt();
        if (readInt() == 1) draw_one_page_flag = true;
        readStr( &default_env_font );
        if (default_env_font == NULL)
            setStr(&default_env_font, DEFAULT_ENV_FONT);
        if (readInt() == 0) cdaudio_on_flag = false;
        readStr( &default_cdrom_drive );
        voice_volume = DEFAULT_VOLUME - readInt();
        se_volume = DEFAULT_VOLUME - readInt();
        music_volume = DEFAULT_VOLUME - readInt();
        if (readInt() == 0) kidokumode_flag = false;
    }
    else{
        setStr( &default_env_font, DEFAULT_ENV_FONT );
        voice_volume = se_volume = music_volume = DEFAULT_VOLUME;
    }
}

void ONScripterLabel::saveEnvData()
{
    file_io_buf_ptr = 0;
    bool output_flag = false;
    for (int i=0 ; i<2 ; i++){
        writeInt( fullscreen_mode?1:0, output_flag );
        writeInt( volume_on_flag?1:0, output_flag );
        writeInt( text_speed_no, output_flag );
        writeInt( draw_one_page_flag?1:0, output_flag );
        writeStr( default_env_font, output_flag );
        writeInt( cdaudio_on_flag?1:0, output_flag );
        writeStr( default_cdrom_drive, output_flag );
        writeInt( DEFAULT_VOLUME - voice_volume, output_flag );
        writeInt( DEFAULT_VOLUME - se_volume, output_flag );
        writeInt( DEFAULT_VOLUME - music_volume, output_flag );
        writeInt( kidokumode_flag?1:0, output_flag );
        writeInt( 0, output_flag ); // ?

        if (i==1) break;
        allocFileIOBuf();
        output_flag = true;
    }

    saveFileIOBuf( "envdata" );
}

int ONScripterLabel::refreshMode()
{
    int ret = REFRESH_NORMAL_MODE;

    if ( next_display_mode == TEXT_DISPLAY_MODE ||
         (system_menu_mode == SYSTEM_NULL) &&
         erase_text_window_mode == 0 &&
         (current_refresh_mode & REFRESH_SHADOW_MODE) &&
         text_on_flag ){
        ret = refresh_shadow_text_mode;
    }

    if (system_menu_mode == SYSTEM_NULL) current_refresh_mode = ret;

    return ret;
}

void ONScripterLabel::quit()
{
    saveAll();

    if ( cdrom_info ){
        SDL_CDStop( cdrom_info );
        SDL_CDClose( cdrom_info );
    }
    if ( midi_info ){
        Mix_HaltMusic();
        Mix_FreeMusic( midi_info );
    }
    if ( music_info ){
        Mix_HaltMusic();
        Mix_FreeMusic( music_info );
    }
}

void ONScripterLabel::disableGetButtonFlag()
{
    btndown_flag = false;

    getzxc_flag = false;
    gettab_flag = false;
    getpageup_flag = false;
    getpagedown_flag = false;
    getinsert_flag = false;
    getfunction_flag = false;
    getenter_flag = false;
    getcursor_flag = false;
    spclclk_flag = false;
}

int ONScripterLabel::getNumberFromBuffer( const char **buf )
{
    int ret = 0;
    while ( **buf >= '0' && **buf <= '9' )
        ret = ret*10 + *(*buf)++ - '0';

    return ret;
}
