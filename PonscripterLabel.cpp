/* -*- C++ -*-
 *
 *  PonscripterLabel.cpp - Execution block parser of Ponscripter
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

#include "PonscripterLabel.h"
#include "resources.h"
#include "utf8_util.h"
#include <ctype.h>

#ifdef MACOSX
namespace Carbon {
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
}
#include <sys/stat.h>
#endif
#ifdef WIN32
#include <windows.h>
typedef HRESULT (WINAPI * GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
#endif
#ifdef LINUX
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

extern "C" void waveCallback(int channel);

#define DEFAULT_AUDIOBUF 4096

#define REGISTRY_FILE "registry.txt"
#define DLL_FILE "dll.txt"
#define DEFAULT_ENV_FONT "Sans"
#define DEFAULT_VOLUME 100

typedef int (PonscripterLabel::* FuncList)();
static struct FuncLUT {
    char command[40];
    FuncList method;
} func_lut[] =
{
    { "wavestop", &PonscripterLabel::wavestopCommand },
    { "waveloop", &PonscripterLabel::waveCommand },
    { "wave", &PonscripterLabel::waveCommand },
    { "waittimer", &PonscripterLabel::waittimerCommand },
    { "wait", &PonscripterLabel::waitCommand },
    { "vsp", &PonscripterLabel::vspCommand },
    { "voicevol", &PonscripterLabel::voicevolCommand },
    { "trap", &PonscripterLabel::trapCommand },
    { "textspeed", &PonscripterLabel::textspeedCommand },
    { "textshow", &PonscripterLabel::textshowCommand },
    { "texton", &PonscripterLabel::textonCommand },
    { "textoff", &PonscripterLabel::textoffCommand },
    { "texthide", &PonscripterLabel::texthideCommand },
    { "textclear", &PonscripterLabel::textclearCommand },
    { "textbtnwait", &PonscripterLabel::btnwaitCommand },
    { "texec", &PonscripterLabel::texecCommand },
    { "tateyoko", &PonscripterLabel::tateyokoCommand },
    { "tal", &PonscripterLabel::talCommand },
    { "tablegoto", &PonscripterLabel::tablegotoCommand },
    { "systemcall", &PonscripterLabel::systemcallCommand },
    { "strsp", &PonscripterLabel::strspCommand },
    { "stop", &PonscripterLabel::stopCommand },
    { "sp_rgb_gradation", &PonscripterLabel::sp_rgb_gradationCommand },
    { "spstr", &PonscripterLabel::spstrCommand },
    { "spreload", &PonscripterLabel::spreloadCommand },
    { "splitstring", &PonscripterLabel::splitCommand },
    { "split", &PonscripterLabel::splitCommand },
    { "spclclk", &PonscripterLabel::spclclkCommand },
    { "spbtn", &PonscripterLabel::spbtnCommand },
    { "skipoff", &PonscripterLabel::skipoffCommand },
    { "sevol", &PonscripterLabel::sevolCommand },
    { "setwindow3", &PonscripterLabel::setwindow3Command },
    { "setwindow2", &PonscripterLabel::setwindow2Command },
    { "setwindow", &PonscripterLabel::setwindowCommand },
    { "setcursor", &PonscripterLabel::setcursorCommand },
    { "selnum", &PonscripterLabel::selectCommand },
    { "selgosub", &PonscripterLabel::selectCommand },
    { "selectbtnwait", &PonscripterLabel::btnwaitCommand },
    { "select", &PonscripterLabel::selectCommand },
    { "savetime", &PonscripterLabel::savetimeCommand },
    { "savescreenshot2", &PonscripterLabel::savescreenshotCommand },
    { "savescreenshot", &PonscripterLabel::savescreenshotCommand },
    { "saveon", &PonscripterLabel::saveonCommand },
    { "saveoff", &PonscripterLabel::saveoffCommand },
    { "savegame", &PonscripterLabel::savegameCommand },
    { "savefileexist", &PonscripterLabel::savefileexistCommand },
    { "rnd", &PonscripterLabel::rndCommand },
    { "rnd2", &PonscripterLabel::rndCommand },
    { "rmode", &PonscripterLabel::rmodeCommand },
    { "resettimer", &PonscripterLabel::resettimerCommand },
    { "reset", &PonscripterLabel::resetCommand },
    { "repaint", &PonscripterLabel::repaintCommand },
    { "quakey", &PonscripterLabel::quakeCommand },
    { "quakex", &PonscripterLabel::quakeCommand },
    { "quake", &PonscripterLabel::quakeCommand },
    { "puttext", &PonscripterLabel::puttextCommand },
    { "prnumclear", &PonscripterLabel::prnumclearCommand },
    { "prnum", &PonscripterLabel::prnumCommand },
    { "print", &PonscripterLabel::printCommand },
    { "playstop", &PonscripterLabel::playstopCommand },
    { "playonce", &PonscripterLabel::playCommand },
    { "play", &PonscripterLabel::playCommand },
    { "ofscpy", &PonscripterLabel::ofscopyCommand },
    { "ofscopy", &PonscripterLabel::ofscopyCommand },
    { "nega", &PonscripterLabel::negaCommand },
    { "msp", &PonscripterLabel::mspCommand },
    { "mpegplay", &PonscripterLabel::mpegplayCommand },
    { "mp3vol", &PonscripterLabel::mp3volCommand },
    { "mp3stop", &PonscripterLabel::playstopCommand },
    { "mp3save", &PonscripterLabel::mp3Command },
    { "mp3loop", &PonscripterLabel::mp3Command },
    { "mp3fadeout", &PonscripterLabel::mp3fadeoutCommand },
    { "mp3", &PonscripterLabel::mp3Command },
    { "movemousecursor", &PonscripterLabel::movemousecursorCommand },
    { "monocro", &PonscripterLabel::monocroCommand },
    { "menu_window", &PonscripterLabel::menu_windowCommand },
    { "menu_full", &PonscripterLabel::menu_fullCommand },
    { "menu_automode", &PonscripterLabel::menu_automodeCommand },
    { "lsph", &PonscripterLabel::lspCommand },
    { "lsp", &PonscripterLabel::lspCommand },
    { "lr_trap", &PonscripterLabel::trapCommand },
    { "loopbgmstop", &PonscripterLabel::loopbgmstopCommand },
    { "loopbgm", &PonscripterLabel::loopbgmCommand },
    { "lookbackflush", &PonscripterLabel::lookbackflushCommand },
    { "lookbackbutton", &PonscripterLabel::lookbackbuttonCommand },
    { "logsp2", &PonscripterLabel::logspCommand },
    { "logsp", &PonscripterLabel::logspCommand },
    { "locate", &PonscripterLabel::locateCommand },
    { "loadgame", &PonscripterLabel::loadgameCommand },
    { "ld", &PonscripterLabel::ldCommand },
    { "jumpf", &PonscripterLabel::jumpfCommand },
    { "jumpb", &PonscripterLabel::jumpbCommand },
    { "isfull", &PonscripterLabel::isfullCommand },
    { "isskip", &PonscripterLabel::isskipCommand },
    { "ispage", &PonscripterLabel::ispageCommand },
    { "isdown", &PonscripterLabel::isdownCommand },
    { "input", &PonscripterLabel::inputCommand },
    { "indent", &PonscripterLabel::indentCommand },
    { "humanorder", &PonscripterLabel::humanorderCommand },
    { "h_textextent", &PonscripterLabel::haeleth_text_extentCommand },
    { "h_rendering", &PonscripterLabel::haeleth_hinting_modeCommand },
    { "h_mapfont", &PonscripterLabel::haeleth_map_fontCommand },
    { "h_ligate", &PonscripterLabel::haeleth_ligature_controlCommand },
    { "h_indentstr", &PonscripterLabel::haeleth_char_setCommand },
    { "h_fontstyle", &PonscripterLabel::haeleth_font_styleCommand },
    { "h_centreline", &PonscripterLabel::haeleth_centre_lineCommand },
    { "h_breakstr", &PonscripterLabel::haeleth_char_setCommand },
    { "getzxc", &PonscripterLabel::getzxcCommand },
    { "getvoicevol", &PonscripterLabel::getvoicevolCommand },
    { "getversion", &PonscripterLabel::getversionCommand },
    { "gettimer", &PonscripterLabel::gettimerCommand },
    { "getspsize", &PonscripterLabel::getspsizeCommand },
    { "getspmode", &PonscripterLabel::getspmodeCommand },
    { "getsevol", &PonscripterLabel::getsevolCommand },
    { "getscreenshot", &PonscripterLabel::getscreenshotCommand },
    { "gettext", &PonscripterLabel::gettextCommand },
    { "gettag", &PonscripterLabel::gettagCommand },
    { "gettab", &PonscripterLabel::gettabCommand },
    { "getret", &PonscripterLabel::getretCommand },
    { "getreg", &PonscripterLabel::getregCommand },
    { "getpageup", &PonscripterLabel::getpageupCommand },
    { "getpage", &PonscripterLabel::getpageCommand },
    { "getmp3vol", &PonscripterLabel::getmp3volCommand },
    { "getmousepos", &PonscripterLabel::getmouseposCommand },
    { "getlog", &PonscripterLabel::getlogCommand },
    { "getinsert", &PonscripterLabel::getinsertCommand },
    { "getfunction", &PonscripterLabel::getfunctionCommand },
    { "getenter", &PonscripterLabel::getenterCommand },
    { "getcursorpos", &PonscripterLabel::getcursorposCommand },
    { "getcursor", &PonscripterLabel::getcursorCommand },
    { "getcselstr", &PonscripterLabel::getcselstrCommand },
    { "getcselnum", &PonscripterLabel::getcselnumCommand },
    { "getbtntimer", &PonscripterLabel::gettimerCommand },
    { "getbgmvol", &PonscripterLabel::getmp3volCommand },
    { "game", &PonscripterLabel::gameCommand },
    { "fileexist", &PonscripterLabel::fileexistCommand },
    { "existspbtn", &PonscripterLabel::spbtnCommand },
    { "exec_dll", &PonscripterLabel::exec_dllCommand },
    { "exbtn_d", &PonscripterLabel::exbtnCommand },
    { "exbtn", &PonscripterLabel::exbtnCommand },
    { "erasetextwindow", &PonscripterLabel::erasetextwindowCommand },
    { "end", &PonscripterLabel::endCommand },
    { "dwavestop", &PonscripterLabel::dwavestopCommand },
    { "dwaveplayloop", &PonscripterLabel::dwaveCommand },
    { "dwaveplay", &PonscripterLabel::dwaveCommand },
    { "dwaveloop", &PonscripterLabel::dwaveCommand },
    { "dwaveload", &PonscripterLabel::dwaveCommand },
    { "dwave", &PonscripterLabel::dwaveCommand },
    { "drawtext", &PonscripterLabel::drawtextCommand },
    { "drawsp3", &PonscripterLabel::drawsp3Command },
    { "drawsp2", &PonscripterLabel::drawsp2Command },
    { "drawsp", &PonscripterLabel::drawspCommand },
    { "drawfill", &PonscripterLabel::drawfillCommand },
    { "drawclear", &PonscripterLabel::drawclearCommand },
    { "drawbg2", &PonscripterLabel::drawbg2Command },
    { "drawbg", &PonscripterLabel::drawbgCommand },
    { "draw", &PonscripterLabel::drawCommand },
    { "delay", &PonscripterLabel::delayCommand },
    { "definereset", &PonscripterLabel::defineresetCommand },
    { "csp", &PonscripterLabel::cspCommand },
    { "cselgoto", &PonscripterLabel::cselgotoCommand },
    { "cselbtn", &PonscripterLabel::cselbtnCommand },
    { "csel", &PonscripterLabel::selectCommand },
    { "click", &PonscripterLabel::clickCommand },
    { "cl", &PonscripterLabel::clCommand },
    { "chvol", &PonscripterLabel::chvolCommand },
    { "checkpage", &PonscripterLabel::checkpageCommand },
    { "cellcheckspbtn", &PonscripterLabel::spbtnCommand },
    { "cellcheckexbtn", &PonscripterLabel::exbtnCommand },
    { "cell", &PonscripterLabel::cellCommand },
    { "caption", &PonscripterLabel::captionCommand },
    { "btnwait2", &PonscripterLabel::btnwaitCommand },
    { "btnwait", &PonscripterLabel::btnwaitCommand },
    { "btntime2", &PonscripterLabel::btntimeCommand },
    { "btntime", &PonscripterLabel::btntimeCommand },
    { "btndown", &PonscripterLabel::btndownCommand },
    { "btndef", &PonscripterLabel::btndefCommand },
    { "btn", &PonscripterLabel::btnCommand },
    { "br2", &PonscripterLabel::brCommand },
    { "br", &PonscripterLabel::brCommand },
    { "blt", &PonscripterLabel::bltCommand },
    { "bgmvol", &PonscripterLabel::mp3volCommand },
    { "bgmstop", &PonscripterLabel::playstopCommand },
    { "bgmonce", &PonscripterLabel::mp3Command },
    { "bgm", &PonscripterLabel::mp3Command },
    { "bgcpy", &PonscripterLabel::bgcopyCommand },
    { "bgcopy", &PonscripterLabel::bgcopyCommand },
    { "bg", &PonscripterLabel::bgCommand },
    { "barclear", &PonscripterLabel::barclearCommand },
    { "bar", &PonscripterLabel::barCommand },
    { "avi", &PonscripterLabel::aviCommand },
    { "automode_time", &PonscripterLabel::automode_timeCommand },
    { "autoclick", &PonscripterLabel::autoclickCommand },
    { "amsp", &PonscripterLabel::amspCommand },
    { "allspresume", &PonscripterLabel::allspresumeCommand },
    { "allsphide", &PonscripterLabel::allsphideCommand },
    { "abssetcursor", &PonscripterLabel::setcursorCommand },
    { "", 0 }
};

static void SDL_Quit_Wrapper()
{
    SDL_Quit();
}


void PonscripterLabel::initSDL()
{
    /* ---------------------------------------- */
    /* Initialize SDL */

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(-1);
    }
    atexit(SDL_Quit_Wrapper); // work-around for OS/2

    if (cdaudio_flag && SDL_InitSubSystem(SDL_INIT_CDROM) < 0) {
        fprintf(stderr, "Couldn't initialize CD-ROM: %s\n", SDL_GetError());
        exit(-1);
    }

#ifdef ENABLE_JOYSTICK
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0 && SDL_JoystickOpen(0) != 0)
        printf("Initialize JOYSTICK\n");
#endif

#if defined (PSP) || defined (IPODLINUX)
    SDL_ShowCursor(SDL_DISABLE);
#endif

    /* ---------------------------------------- */
    /* Initialize SDL */

    // Don't set an icon on OS X - the applicaton bundle's icon is
    // used anyway, and is usually higher resolution.
#ifndef MACOSX
    // Apparently the window icon must be set before the display is
    // initialised.  An ONScripter-style PNG in the current folder
    // takes precedence.
    SDL_Surface* icon = IMG_Load("icon.png");
    // If that doesn't exist, try using an internal resource.
    if (!icon) {
        const InternalResource* internal_icon = getResource("icon.png");
        if (internal_icon) {
            SDL_RWops* rwicon = SDL_RWFromConstMem(internal_icon->buffer,
                                    internal_icon->size);
            icon = IMG_Load_RW(rwicon, 0);
        }
    }
    // If an icon was found, use it.
    if (icon) SDL_WM_SetIcon(icon, 0);
#endif

#ifdef BPP16
    screen_bpp = 16;
#else
    screen_bpp = 32;
#endif

#if defined (PDA) && defined (PDA_WIDTH)
    screen_ratio1 *= PDA_WIDTH;
    screen_ratio2 *= 320;
    screen_width  = screen_width * PDA_WIDTH / 320;
    screen_height = screen_height * PDA_WIDTH / 320;
#elif defined(PDA) && defined(PDA_AUTOSIZE)
    SDL_Rect **modes;
    modes = SDL_ListModes(NULL, 0);
    if (modes == (SDL_Rect **)0){
        fprintf(stderr, "No Video mode available.\n");
        exit(-1);
    }
    else if (modes == (SDL_Rect **)-1){
        // no restriction
    }
 	else{
        int width;
        if (modes[0]->w * 3 > modes[0]->h * 4)
            width = (modes[0]->h / 3) * 4;
        else
            width = (modes[0]->w / 4) * 4;
        screen_ratio1 *= width;
        screen_ratio2 *= 320;
        screen_width   = screen_width  * width / 320;
        screen_height  = screen_height * width / 320;
    }
#endif

    screen_surface = SDL_SetVideoMode(screen_width, screen_height, screen_bpp,
        DEFAULT_VIDEO_SURFACE_FLAG | (fullscreen_mode ? SDL_FULLSCREEN : 0));

    /* ---------------------------------------- */
    /* Check if VGA screen is available. */
#if defined (PDA) && (PDA_WIDTH == 640)
    if (screen_surface == 0) {
        screen_ratio1 /= 2;
        screen_width  /= 2;
        screen_height /= 2;
        screen_surface = SDL_SetVideoMode(screen_width,screen_height,screen_bpp,
	    DEFAULT_VIDEO_SURFACE_FLAG | (fullscreen_mode? SDL_FULLSCREEN : 0));
    }
#endif
    underline_value = screen_height - 1;

    if (screen_surface == 0) {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
            screen_width, screen_height, screen_bpp, SDL_GetError());
        exit(-1);
    }

    wm_title_string = new char[strlen(DEFAULT_WM_TITLE) + 1];
    memcpy(wm_title_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_TITLE) + 1);
    wm_icon_string = new char[strlen(DEFAULT_WM_ICON) + 1];
    memcpy(wm_icon_string, DEFAULT_WM_TITLE, strlen(DEFAULT_WM_ICON) + 1);
    SDL_WM_SetCaption(wm_title_string, wm_icon_string);

    openAudio();
}


void PonscripterLabel::openAudio()
{
#if defined (PDA) && !defined (PSP)
    const int sample_rate = 22050;
#else
    const int sample_rate = 44100;
#endif

    if (Mix_OpenAudio(sample_rate, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS,
            DEFAULT_AUDIOBUF) < 0) {
        fprintf(stderr, "Couldn't open audio device!\n"
                        "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else {
        int freq;
        Uint16 format;
        int channels;

        Mix_QuerySpec(&freq, &format, &channels);
        audio_format.format = format;
        audio_format.freq = freq;
        audio_format.channels = channels;

        audio_open_flag = true;

        Mix_AllocateChannels(ONS_MIX_CHANNELS + ONS_MIX_EXTRA_CHANNELS);
        Mix_ChannelFinished(waveCallback);
    }
}


PonscripterLabel::PonscripterLabel()
{
    cdrom_drive_number = 0;
    cdaudio_flag  = false;
    registry_file = 0;
    setStr(&registry_file, REGISTRY_FILE);
    dll_file = 0;
    setStr(&dll_file, DLL_FILE);
    getret_str = 0;
    enable_wheeldown_advance_flag = false;
    disable_rescale_flag = false;
    edit_flag       = false;
    key_exe_file    = 0;
    fullscreen_mode = false;
    window_mode     = false;
    skip_to_wait    = 0;
    indent_chars    = 0;
    break_chars     = 0;
    glyph_surface   = 0;

    // External Players
    music_cmd = getenv("PLAYER_CMD");
    midi_cmd  = getenv("MUSIC_CMD");
}


PonscripterLabel::~PonscripterLabel()
{
    if (glyph_surface) SDL_FreeSurface(glyph_surface);
    reset();
}


void PonscripterLabel::enableCDAudio()
{
    cdaudio_flag = true;
}


void PonscripterLabel::setCDNumber(int cdrom_drive_number)
{
    this->cdrom_drive_number = cdrom_drive_number;
}


void PonscripterLabel::setRegistryFile(const char* filename)
{
    setStr(&registry_file, filename);
}


void PonscripterLabel::setDLLFile(const char* filename)
{
    setStr(&dll_file, filename);
}


void PonscripterLabel::setArchivePath(const char* path)
{
    if (archive_path) delete[] archive_path;
    archive_path = new char[strlen(path) + 2];
    sprintf(archive_path, "%s%c", path, DELIMITER);
}


void PonscripterLabel::setSavePath(const char* path)
{
    if (script_h.save_path) delete[] script_h.save_path;
    script_h.save_path = new char[strlen(path) + 2];
    sprintf(script_h.save_path, "%s%c", path, DELIMITER);
}


void PonscripterLabel::setFullscreenMode()
{
    fullscreen_mode = true;
}


void PonscripterLabel::setWindowMode()
{
    window_mode = true;
}


void PonscripterLabel::enableButtonShortCut()
{
    force_button_shortcut_flag = true;
}


void PonscripterLabel::enableWheelDownAdvance()
{
    enable_wheeldown_advance_flag = true;
}


void PonscripterLabel::disableRescale()
{
    disable_rescale_flag = true;
}


void PonscripterLabel::enableEdit()
{
    edit_flag = true;
}


void PonscripterLabel::setKeyEXE(const char* filename)
{
    setStr(&key_exe_file, filename);
}


int PonscripterLabel::init()
{
    if (archive_path == 0) {
#ifdef MACOSX
        // On OS X, store archives etc in the application bundle by
        // default.
        using namespace Carbon;
        ProcessSerialNumber psn;
        GetCurrentProcess(&psn);
        FSRef bundle;
        GetProcessBundleLocation(&psn, &bundle);
        char bpath[32768];
        FSRefMakePath(&bundle, (UInt8*) bpath, 32768);
        archive_path = new char[strlen(bpath) + 32];
        sprintf(archive_path, "%s/Contents/Resources/", bpath);
#else
        // Otherwise, data is either stored with the executable, or in
        // some unpredictable location that must be defined by using
        // "-r PATH" in a launcher script.
        archive_path = "";
#endif
    }

    if (key_exe_file) {
        createKeyTable(key_exe_file);
        script_h.setKeyTable(key_table);
    }

    if (open()) return -1;

    if (script_h.save_path == 0) {
        // Per-platform configuration for saved games.
        const char* gameid = script_h.game_identifier ? script_h.
                             game_identifier : "Ponscripter";
#ifdef WIN32
        // On Windows, store in [Profiles]/All Users/Application Data.
        // TODO: optionally permit saves to be per-user rather than shared?
        HMODULE shdll = LoadLibrary("shfolder");
        if (shdll) {
            GETFOLDERPATH gfp =
		GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
            if (gfp) {
                char hpath[MAX_PATH];
                HRESULT res = gfp(0, 0x0023, 0, 0, hpath);
                if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
                    script_h.
                    save_path = new char[strlen(hpath) + strlen(gameid) + 3];
                    sprintf(script_h.save_path, "%s/%s/", hpath, gameid);
                    CreateDirectory(script_h.save_path, 0);
                }
            }
            FreeLibrary(shdll);
        }
        if (script_h.save_path == 0) {
            // Error; assume ancient Windows. In this case it's safe
            // to use the archive path.
            script_h.save_path = archive_path;
        }
#elif defined MACOSX
        // On OS X, place in subfolder of ~/Library/Preferences.
        using namespace Carbon;
        FSRef home;
        FSFindFolder(kUserDomain, kPreferencesFolderType, kDontCreateFolder,
		     &home);
        char hpath[32768];
        FSRefMakePath(&home, (UInt8*) hpath, 32768);
        script_h.save_path = new char[strlen(hpath) + strlen(gameid) + 8];
        sprintf(script_h.save_path, "%s/%s Data/", hpath, gameid);
        mkdir(script_h.save_path, 0755);
#elif defined LINUX
        // On Linux (and other POSIX-a-likes), place in ~/.gameid
        passwd* pwd = getpwuid(getuid());
        if (pwd) {
            script_h.
            save_path = new char[strlen(pwd->pw_dir) + strlen(gameid) + 4];
            sprintf(script_h.save_path, "%s/.%s/", pwd->pw_dir, gameid);
            if (mkdir(script_h.save_path, 0755) != 0 && errno != EEXIST) {
                delete[] script_h.save_path;
                script_h.save_path = 0;
            }
        }
        if (script_h.save_path == 0) {
            // Error; either getpwuid failed, or we couldn't create a
            // save directory.  Either way, issue a warning and then
            // fall back on default ONScripter behaviour.
            fprintf(stderr, "Warning: could not create save directory ~/.%s.\n",
		    gameid);
            script_h.save_path = archive_path;
        }
#else
        // Fall back on default ONScripter behaviour if we don't have
        // any better ideas.
        script_h.save_path = archive_path;
#endif
    }
    if (script_h.game_identifier) {
        delete[] script_h.game_identifier;
        script_h.game_identifier = 0;
    }
    else {
        fprintf(stderr,
            "This game is not intended to be played with Ponscripter.\n"
            "Please play it with NScripter instead, or with a properly "
	    "compatible clone\nsuch as ONScripter.\n");
        exit(-1);
    }

    initSDL();

    image_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1, 32, 0x00ff0000,
                        0x0000ff00, 0x000000ff, 0xff000000);

    accumulation_surface =
	AnimationInfo::allocSurface(screen_width, screen_height);
    accumulation_comp_surface =
	AnimationInfo::allocSurface(screen_width, screen_height);
    effect_src_surface =
	AnimationInfo::allocSurface(screen_width, screen_height);
    effect_dst_surface =
	AnimationInfo::allocSurface(screen_width, screen_height);
    SDL_SetAlpha(accumulation_surface, 0, SDL_ALPHA_OPAQUE);
    SDL_SetAlpha(accumulation_comp_surface, 0, SDL_ALPHA_OPAQUE);    
    SDL_SetAlpha(effect_src_surface, 0, SDL_ALPHA_OPAQUE);
    SDL_SetAlpha(effect_dst_surface, 0, SDL_ALPHA_OPAQUE);
    screenshot_surface = 0;
    text_info.num_of_cells = 1;
    text_info.allocImage(screen_width, screen_height);
    text_info.fill(0, 0, 0, 0);

    // ----------------------------------------
    // Sound related variables
    this->cdaudio_flag = cdaudio_flag;
    cdrom_info = 0;
    if (cdaudio_flag) {
        if (cdrom_drive_number >= 0 && cdrom_drive_number < SDL_CDNumDrives())
            cdrom_info = SDL_CDOpen(cdrom_drive_number);
        if (!cdrom_info) {
            fprintf(stderr, "Couldn't open default CD-ROM: %s\n", SDL_GetError());
        }
        else if (cdrom_info && !CD_INDRIVE(SDL_CDStatus(cdrom_info))) {
            fprintf(stderr, "no CD-ROM in the drive\n");
            SDL_CDClose(cdrom_info);
            cdrom_info = 0;
        }
    }

    wave_file_name = 0;
    midi_file_name = 0;
    midi_info  = 0;
    mp3_sample = 0;
    music_file_name = 0;
    mp3_buffer = 0;
    music_info = 0;
    music_ovi  = 0;

    loop_bgm_name[0] = 0;
    loop_bgm_name[1] = 0;

    int i;
    for (i = 0; i < ONS_MIX_CHANNELS + ONS_MIX_EXTRA_CHANNELS;
         i++) wave_sample[i] = 0;

    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    trap_dist = 0;
    resize_buffer = new unsigned char[16];
    resize_buffer_size = 16;

    for (i = 0; i < MAX_PARAM_NUM; i++) bar_info[i] = prnum_info[i] = 0;

    defineresetCommand();
    readToken();

    loadEnvData();

    // Initialize character sets
    DefaultLigatures(1);
    const int ic = 9;
    unsigned short defindent[ic] = { 0x0028, 0x2014, 0x2018, 0x201c, 0x300c,
                                     0x300e, 0xff08, 0xff5e, 0xff62 };
    indent_chars = new unsigned short[ic];
    for (i = 0; i < ic; ++i) indent_chars[i] = defindent[i];
    const int bc = 4;
    unsigned short defbreak[bc] = { 0x0020, 0x002d, 0x2013, 0x2014 };
    break_chars = new unsigned short[bc];
    for (i = 0; i < bc; ++i) break_chars[i] = defbreak[i];

    return 0;
}


void PonscripterLabel::reset()
{
    automode_flag  = false;
    automode_time  = 3000;
    autoclick_time = 0;
    remaining_time = -1;
    btntime2_flag  = false;
    btntime_value  = 0;
    btnwait_time = 0;

    disableGetButtonFlag();

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    key_pressed_flag = false;
    shift_pressed_status = 0;
    ctrl_pressed_status  = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    all_sprite_hide_flag = false;

    if (resize_buffer_size != 16) {
        delete[] resize_buffer;
        resize_buffer = new unsigned char[16];
        resize_buffer_size = 16;
    }

    current_over_button = 0;
    variable_edit_mode  = NOT_EDIT_MODE;

    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE |
                               REFRESH_TEXT_MODE;
    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = false;

    resetSentenceFont();

    setStr(&getret_str, 0);
    getret_int = 0;

    // ----------------------------------------
    // Sound related variables

    wave_play_loop_flag  = false;
    midi_play_loop_flag  = false;
    music_play_loop_flag = false;
    cd_play_loop_flag = false;
    mp3save_flag = false;
    current_cd_track = -1;

    resetSub();

    /* ---------------------------------------- */
    /* Load global variables if available */
    if (loadFileIOBuf("global.sav") == 0)
        readVariables(script_h.global_variable_border, VARIABLE_RANGE);
}


void PonscripterLabel::resetSub()
{
    int i;

    for (i = 0; i < script_h.global_variable_border; i++)
        script_h.variable_data[i].reset(false);

    for (i = 0; i < 3; i++) human_order[i] = 2 - i; // "rcl"

    erase_text_window_mode = 1;
    skip_flag    = false;
    monocro_flag = false;
    nega_mode = 0;
    clickstr_state = CLICK_NONE;
    trap_mode = TRAP_NONE;
    setStr(&trap_dist, 0);

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
    setStr(&loop_bgm_name[1], 0);

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    setStr(&bg_info.file_name, "black");
    createBackground();
    for (i = 0; i < 3; i++) tachi_info[i].reset();
    for (i = 0; i < MAX_SPRITE_NUM; i++) sprite_info[i].reset();
    barclearCommand();
    prnumclearCommand();
    for (i = 0; i < 2; i++) cursor_info[i].reset();
    for (i = 0; i < 4; i++) lookback_info[i].reset();
    sentence_font_info.reset();

    if (indent_chars) {
        delete[] indent_chars; indent_chars = 0;
    }
    if (break_chars) {
        delete[] break_chars; break_chars = 0;
    }

    dirty_rect.fill(screen_width, screen_height);
}


void PonscripterLabel::resetSentenceFont()
{
    FontInfo::default_encoding = Default;
    sentence_font.reset();
    sentence_font.top_x     = 21;
    sentence_font.top_y     = 16;
    sentence_font.area_x    = screen_width - 21;
    sentence_font.area_y    = screen_height - 16;
    sentence_font.pitch_x   = 0;
    sentence_font.pitch_y   = 0;
    sentence_font.wait_time = 20;
    sentence_font.window_color[0] = sentence_font.
                                    window_color[1] = sentence_font.
                                                      window_color[2] = 0x99;
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width + 1;
    sentence_font_info.pos.h = screen_height + 1;
}


void PonscripterLabel::
flush(int refresh_mode, SDL_Rect* rect, bool clear_dirty_flag,
      bool direct_flag)
{
    if (direct_flag) {
        flushDirect(*rect, refresh_mode);
    }
    else {
        if (rect) dirty_rect.add(*rect);

        if (dirty_rect.area > 0) {
            if (dirty_rect.area >= dirty_rect.bounding_box.w * dirty_rect.
                bounding_box.h) {
                flushDirect(dirty_rect.bounding_box, refresh_mode);
            }
            else {
                for (int i = 0; i < dirty_rect.num_history; i++) {
                    //printf("%d: ", i );
                    flushDirect(dirty_rect.history[i], refresh_mode);
                }
            }
        }
    }

    if (clear_dirty_flag) dirty_rect.clear();
}


void PonscripterLabel::flushDirect(SDL_Rect &rect, int refresh_mode)
{
    refreshSurface(accumulation_surface, &rect, refresh_mode);

    if (refresh_mode != REFRESH_NONE_MODE &&
	!(refresh_mode & REFRESH_CURSOR_MODE)) {
        if (refresh_mode & REFRESH_SHADOW_MODE)
            refreshSurface(accumulation_comp_surface, &rect,
			   (refresh_mode & ~REFRESH_SHADOW_MODE
			                 & ~REFRESH_TEXT_MODE)
			                 | REFRESH_COMP_MODE);
        else
            refreshSurface(accumulation_comp_surface, &rect,
			   refresh_mode | refresh_shadow_text_mode
			                | REFRESH_COMP_MODE);
    }
    
    SDL_BlitSurface(accumulation_surface, &rect, screen_surface, &rect);
    SDL_UpdateRect(screen_surface, rect.x, rect.y, rect.w, rect.h);
}


void PonscripterLabel::mouseOverCheck(int x, int y)
{
    int c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    ButtonLink* p_button_link = root_button_link.next;
    while (p_button_link) {
        if (x >= p_button_link->select_rect.x
	 && x < p_button_link->select_rect.x + p_button_link->select_rect.w
         && y >= p_button_link->select_rect.y
	 && y < p_button_link->select_rect.y + p_button_link->select_rect.h)
	{
            button = p_button_link->no;
            break;
        }
        p_button_link = p_button_link->next;
        c++;
    }

    if (current_over_button != button) {
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = { 0, 0, 0, 0 };
        SDL_Rect check_dst_rect = { 0, 0, 0, 0 };
        if (current_over_button != 0) {
            current_button_link->show_flag = 0;
            check_src_rect = current_button_link->image_rect;
	    const int cbt = current_button_link->button_type;
            if (cbt == ButtonLink::SPRITE_BUTTON ||
		cbt == ButtonLink::EX_SPRITE_BUTTON)
	    {
                sprite_info[current_button_link->sprite_no].visible = true;
                sprite_info[current_button_link->sprite_no].setCell(0);
            }
            else if (cbt == ButtonLink::TMP_SPRITE_BUTTON) {
                current_button_link->show_flag = 1;
                current_button_link->anim[0]->visible = true;
                current_button_link->anim[0]->setCell(0);
            }
            else if (current_button_link->anim[1] != 0) {
                current_button_link->show_flag = 2;
            }
            dirty_rect.add(current_button_link->image_rect);
        }

        if (exbtn_d_button_link.exbtn_ctl) {
            decodeExbtnControl(exbtn_d_button_link.exbtn_ctl, &check_src_rect,
			       &check_dst_rect);
        }

        if (p_button_link) {
            if (system_menu_mode != SYSTEM_NULL) {
                if (menuselectvoice_file_name[MENUSELECTVOICE_OVER])
                    playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
                        SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            else {
                if (selectvoice_file_name[SELECTVOICE_OVER])
                    playSound(selectvoice_file_name[SELECTVOICE_OVER],
                        SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            check_dst_rect = p_button_link->image_rect;
	    const int pbt = p_button_link->button_type;
            if (pbt == ButtonLink::SPRITE_BUTTON ||
                pbt == ButtonLink::EX_SPRITE_BUTTON) {
                sprite_info[p_button_link->sprite_no].setCell(1);
                sprite_info[p_button_link->sprite_no].visible = true;
                if (pbt == ButtonLink::EX_SPRITE_BUTTON) {
                    decodeExbtnControl(p_button_link->exbtn_ctl,
				       &check_src_rect, &check_dst_rect);
                }
            }
            else if (pbt == ButtonLink::TMP_SPRITE_BUTTON) {
                p_button_link->show_flag = 1;
                p_button_link->anim[0]->visible = true;
                p_button_link->anim[0]->setCell(1);
            }
            else if (pbt == ButtonLink::NORMAL_BUTTON ||
                     pbt == ButtonLink::LOOKBACK_BUTTON) {
                p_button_link->show_flag = 1;
            }
            dirty_rect.add(p_button_link->image_rect);
            current_button_link = p_button_link;
            shortcut_mouse_line = c;
        }

        flush(refreshMode());
        dirty_rect = dirty;
    }
    current_over_button = button;
}


void PonscripterLabel::executeLabel()
{
    executeLabelTop:

    while (current_line < current_label_info.num_of_lines) {
        if (debug_level > 0)
            printf("*****  executeLabel %s:%d/%d:%d:%d *****\n",
                current_label_info.name,
                current_line,
                current_label_info.num_of_lines,
                string_buffer_offset, display_mode);

        if (script_h.getStringBuffer()[0] == '~') {
            last_tilde.next_script = script_h.getNext();
            readToken();
            continue;
        }
        if (break_flag && !script_h.isName("next")) {
            if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a)
                current_line++;

            if (script_h.getStringBuffer()[string_buffer_offset] != ':'
                && script_h.getStringBuffer()[string_buffer_offset] != ';'
                && script_h.getStringBuffer()[string_buffer_offset] != 0x0a)
                script_h.skipToken();

            readToken();
            continue;
        }

        if (kidokuskip_flag && skip_flag && kidokumode_flag && !script_h.
            isKidoku()) skip_flag = false;

        char* current = script_h.getCurrent();
        int   ret = ScriptParser::parseLine();
        if (ret == RET_NOMATCH) ret = this->parseLine();

        if (ret & RET_SKIP_LINE) {
            script_h.skipLine();
            if (++current_line >= current_label_info.num_of_lines) break;
        }

        if (ret & RET_REREAD) script_h.setCurrent(current);

        if (!(ret & RET_NOREAD)) {
            if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a) {
                string_buffer_offset = 0;
                if (++current_line >= current_label_info.num_of_lines) break;
            }
            readToken();
        }

        if (ret & RET_WAIT) return;
    }

    current_label_info = script_h.lookupLabelNext(current_label_info.name);
    current_line = 0;

    if (current_label_info.start_address != 0) {
        script_h.setCurrent(current_label_info.label_header);
        readToken();
        goto executeLabelTop;
    }

    fprintf(stderr, " ***** End *****\n");
    endCommand();
}


bool PonscripterLabel::is_break_char(const unsigned short c) const
{
    if (break_chars) {
        const unsigned short* istr = break_chars;
        do {
            if (c == *istr) return true;
        } while (*(++istr));
        return false;
    }
    return c == ' ';
}


bool PonscripterLabel::is_indent_char(const unsigned short c) const
{
    if (indent_chars) {
        const unsigned short* istr = indent_chars;
        do {
            if (c == *istr) return true;
        } while (*(++istr));
    }
    return false;
}


bool PonscripterLabel::check_orphan_control()
{
    // Check whether the current break point follows a logical break
    // in the text.  This is used to prevent short words being
    // stranded at the end of a line.
    if (string_buffer_offset < 5) return false;
    const unsigned short p =
        UnicodeOfUTF8(PreviousCharacter(script_h.getStringBuffer() +
					string_buffer_offset));
    return p == '.' || p == 0xff0e || p == ',' || p == 0xff0c
        || p == ':' || p == 0xff1a || p == ';' || p == 0xff1b
        || p == '!' || p == 0xff01 || p == '?' || p == 0xff1f;
}


int PonscripterLabel::parseLine()
{
    int ret, lut_counter = 0;
    const char* s_buf = script_h.getStringBuffer();
    const char* cmd = script_h.getStringBuffer();
    if (cmd[0] == '_') cmd++;

    if (!script_h.isText()) {
        while (func_lut[lut_counter].method) {
            if (!strcmp(func_lut[lut_counter].command, cmd)) {
                return (this->*func_lut[lut_counter].method)();
            }
            lut_counter++;
        }

        if (s_buf[0] == 0x0a)
            return RET_CONTINUE;
        else if (s_buf[0] == 'v' && s_buf[1] >= '0' && s_buf[1] <= '9')
            return vCommand();
        else if (s_buf[0] == 'd' && s_buf[1] == 'v' && s_buf[2] >= '0' &&
                 s_buf[2] <= '9')
            return dvCommand();

        fprintf(stderr, " command [%s] is not supported yet!!\n", s_buf);

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text */
    if (current_mode == DEFINE_MODE)
	errorAndExit("text cannot be displayed in define section.");

//--------INDENT ROUTINE--------------------------------------------------------
    if (sentence_font.GetXOffset() == 0 && sentence_font.GetYOffset() == 0) {
        const unsigned short first_ch = UnicodeOfUTF8(script_h.
                                            getStringBuffer() +
                                            string_buffer_offset);
        if (is_indent_char(first_ch))
            sentence_font.SetIndent(first_ch);
        else
            sentence_font.ClearIndent();
    }
//--------END INDENT ROUTINE----------------------------------------------------

    ret = textCommand();

//--------LINE BREAKING ROUTINE-------------------------------------------------
    const unsigned short first_ch = UnicodeOfUTF8(script_h.getStringBuffer() +
                                        string_buffer_offset);
    if (is_break_char(first_ch) && !new_line_skip_flag) {
        char* it = script_h.getStringBuffer() + string_buffer_offset
                   + CharacterBytes(script_h.getStringBuffer() +
                       string_buffer_offset);
        float len = sentence_font.GlyphAdvance(first_ch, UnicodeOfUTF8(it));
        while (1) {
            // For each character (not char!) before a break is found,
            // get unicode.
            unsigned short ch = UnicodeOfUTF8(it);
            it += CharacterBytes(it);

            // Check for token breaks.
            if (!ch || ch == '\n' || ch == '@' || ch == '\\'
		|| is_break_char(ch))
                break;

            // Look for an inline command.
            if (ch == '!' && (*it == 's' || *it == 'd' || *it == 'w')) {
                // !sd
                if (it[0] == 's' && it[1] == 'd') {
                    it += 2;
		    continue;
                }
                // ![sdw]<int>
                do { ++it; } while (isdigit(*it));
                continue;
            }
            else if (ch == '#') {
                //#rrggbb: check all figures are in order
                bool ok = true;
                for (int offs = 1; ok && offs <= 6; ++offs)
		    ok &= isxdigit(it[offs]);
                if (ok) {
                    it += 7;
                    continue;
                }
            }

            // No inline command?  Use the glyph metrics, then!
            len += sentence_font.GlyphAdvance(ch, UnicodeOfUTF8(it));
        }
        if (check_orphan_control()) {
            // If this is the start of a sentence, or follows some
            // other punctuation that makes this desirable, we pretend
            // short words have a minimum length.
            // Consider the case where we are rendering "We all of us
            // love pies. I eat them every day" in a fixed-width font
            // in a text window 25 characters wide.  A naive approach
            // would lead to rendering this as
            //    -------------------------
            //    We all of us love pies. I
            //    eat them every day.
            //    -------------------------
            // By treating the second "I", which follows a full stop,
            // as though it was four letters long, we get instead the
            // more readable layout
            //    -------------------------
            //    We all of us love pies.
            //    I eat them every day.
            //    -------------------------
            // Currently we use four em widths, which is an arbtrary
            // figure that may need tweaking.
            const float minlen = sentence_font.em_width() * 4;
            if (len < minlen) len = minlen;
        }
        if (len > 0 && sentence_font.isNoRoomFor(len)) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
        }
    }
//-----END LINE BREAKING ROUTINE------------------------------------------------

    if (script_h.getStringBuffer()[string_buffer_offset] == 0x0a) {
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
        }
        //event_mode = IDLE_EVENT_MODE;
        line_enter_status = 0;
    }

    return ret;
}


SDL_Surface* PonscripterLabel::loadImage(char* file_name, bool* has_alpha)
{
    if (!file_name) return 0;
    unsigned long length = ScriptHandler::cBR->getFileLength(file_name);
    if (length == 0) {
        if (strcmp(file_name, DEFAULT_LOOKBACK_NAME0) != 0 &&
            strcmp(file_name, DEFAULT_LOOKBACK_NAME1) != 0 &&
            strcmp(file_name, DEFAULT_LOOKBACK_NAME2) != 0 &&
            strcmp(file_name, DEFAULT_LOOKBACK_NAME3) != 0 &&
	    strcmp(file_name, DEFAULT_CURSOR1) != 0)
            fprintf(stderr, " *** can't find file [%s] ***\n", file_name);
        return 0;
    }
    if (filelog_flag)
        script_h.findAndAddLog(script_h.log_info[ScriptHandler::FILE_LOG],
			       file_name, true);
    unsigned char* buffer = new unsigned char[length];
    int location;
    ScriptHandler::cBR->getFile(file_name, buffer, &location);
    SDL_Surface* tmp = IMG_Load_RW(SDL_RWFromMem(buffer, length), 1);

    char* ext = strrchr(file_name, '.');
    if (!tmp && ext && (!strcmp(ext + 1, "JPG") || !strcmp(ext + 1, "jpg"))) {
        fprintf(stderr, " *** force-loading a JPG image [%s]\n", file_name);
        SDL_RWops* src = SDL_RWFromMem(buffer, length);
        tmp = IMG_LoadJPG_RW(src);
        SDL_RWclose(src);
    }
    if (has_alpha) *has_alpha = tmp->format->Amask;

    delete[] buffer;
    if (!tmp) {
        fprintf(stderr, " *** can't load file [%s] ***\n", file_name);
        return 0;
    }

    SDL_Surface* ret = SDL_ConvertSurface(tmp, image_surface->format,
                           SDL_SWSURFACE);
    if (ret
        && screen_ratio2 != screen_ratio1
        && (!disable_rescale_flag || location == BaseReader::
            ARCHIVE_TYPE_NONE)) {
        SDL_Surface* src_s = ret;

        int w, h;
        if ((w = src_s->w * screen_ratio1 / screen_ratio2) == 0) w = 1;
        if ((h = src_s->h * screen_ratio1 / screen_ratio2) == 0) h = 1;
        SDL_PixelFormat* fmt = image_surface->format;
        ret = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, fmt->BitsPerPixel,
		       fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);

        resizeSurface(src_s, ret);
        SDL_FreeSurface(src_s);
    }
    SDL_FreeSurface(tmp);

    return ret;
}


/* ---------------------------------------- */
void PonscripterLabel::deleteButtonLink()
{
    ButtonLink* b1 = root_button_link.next;

    while (b1) {
        ButtonLink* b2 = b1;
        b1 = b1->next;
        delete b2;
    }
    root_button_link.next = 0;

    if (exbtn_d_button_link.exbtn_ctl) {
	delete[] exbtn_d_button_link.exbtn_ctl;
	exbtn_d_button_link.exbtn_ctl = 0;
    }
}


void PonscripterLabel::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = 0;
    current_button_link = root_button_link.next;
    SDL_GetMouseState(&mx, &my);
    mouseOverCheck(mx, my);
}


/* ---------------------------------------- */
/* Delete select link */
void PonscripterLabel::deleteSelectLink()
{
    SelectLink* link, * last_select_link = root_select_link.next;

    while (last_select_link) {
        link = last_select_link;
        last_select_link = last_select_link->next;
        delete link;
    }
    root_select_link.next = 0;
}


void PonscripterLabel::clearCurrentTextBuffer()
{
    sentence_font.clear();

    current_text_buffer->clear();

    num_chars_in_sentence = 0;
    internal_saveon_flag  = true;

    text_info.fill(0, 0, 0, 0);
    cached_text_buffer = current_text_buffer;
}


void PonscripterLabel::shadowTextDisplay(SDL_Surface* surface, SDL_Rect &clip)
{
    if (current_font->is_transparent) {
        SDL_Rect rect = { 0, 0, screen_width, screen_height };
        if (current_font == &sentence_font)
            rect = sentence_font_info.pos;

        if (AnimationInfo::doClipping(&rect, &clip)) return;

        if (rect.x + rect.w > surface->w) rect.w = surface->w - rect.x;
        if (rect.y + rect.h > surface->h) rect.h = surface->h - rect.y;

        SDL_LockSurface(surface);
        ONSBuf* buf = (ONSBuf*) surface->pixels + rect.y * surface->w + rect.x;

        SDL_PixelFormat* fmt = surface->format;
        uchar3 color;
        color[0] = current_font->window_color[0] >> fmt->Rloss;
        color[1] = current_font->window_color[1] >> fmt->Gloss;
        color[2] = current_font->window_color[2] >> fmt->Bloss;

        for (int i = rect.y; i < rect.y + rect.h; i++) {
            for (int j = rect.x; j < rect.x + rect.w; j++, buf++) {
                *buf = (((*buf & fmt->Rmask) >> fmt->Rshift) * color[0] >>
                        (8 - fmt->Rloss)) << fmt->Rshift |
                       (((*buf & fmt->Gmask) >> fmt->Gshift) * color[1] >>
                        (8 - fmt->Gloss)) << fmt->Gshift |
		       (((*buf & fmt->Bmask) >> fmt->Bshift) * color[2] >>
                        (8 - fmt->Bloss)) << fmt->Bshift;
            }
            buf += surface->w - rect.w;
        }

        SDL_UnlockSurface(surface);
    }
    else if (sentence_font_info.image_surface) {
        drawTaggedSurface(surface, &sentence_font_info, clip);
    }
}


void PonscripterLabel::newPage(bool next_flag)
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if (!current_text_buffer->empty()) {
        current_text_buffer = current_text_buffer->next;
        if (start_text_buffer == current_text_buffer)
            start_text_buffer = start_text_buffer->next;
    }

    if (next_flag) {
        indent_offset = 0;
        //line_enter_status = 0;
    }

    clearCurrentTextBuffer();

    flush(refreshMode(), &sentence_font_info.pos);
}


PonscripterLabel::ButtonLink*
PonscripterLabel::getSelectableSentence(char* buffer, FontInfo* info,
				       bool flush_flag, bool nofile_flag)
{
    float current_x;
    current_x = info->GetXOffset();

    ButtonLink* button_link = new ButtonLink();
    button_link->button_type = ButtonLink::TMP_SPRITE_BUTTON;
    button_link->show_flag = 1;

    AnimationInfo* anim = new AnimationInfo();
    button_link->anim[0] = anim;

    anim->trans_mode = AnimationInfo::TRANS_STRING;
    anim->is_single_line = false;
    anim->num_of_cells = 2;
    anim->color_list = new uchar3[anim->num_of_cells];
    for (int i = 0; i < 3; i++) {
        if (nofile_flag)
            anim->color_list[0][i] = info->nofile_color[i];
        else
            anim->color_list[0][i] = info->off_color[i];
        anim->color_list[1][i] = info->on_color[i];
    }
    setStr(&anim->file_name, buffer);
    anim->pos.x   = Sint16(floor(info->GetX() * screen_ratio1 / screen_ratio2));
    anim->pos.y   = info->GetY() * screen_ratio1 / screen_ratio2;
    anim->visible = true;

    setupAnimationInfo(anim, info);
    button_link->select_rect = button_link->image_rect = anim->pos;

    info->newLine();
    info->SetXY(current_x);

    dirty_rect.add(button_link->image_rect);

    return button_link;
}


void
PonscripterLabel::decodeExbtnControl(const char* ctl_str,
				     SDL_Rect* check_src_rect,
				     SDL_Rect* check_dst_rect)
{
    char sound_name[256];
    int  i, sprite_no, sprite_no2, cell_no;

    while (char com = *ctl_str++) {
        if (com == 'C' || com == 'c') {
            sprite_no  = getNumberFromBuffer(&ctl_str);
            sprite_no2 = sprite_no;
            cell_no = -1;
            if (*ctl_str == '-') {
                ctl_str++;
                sprite_no2 = getNumberFromBuffer(&ctl_str);
            }
            for (i = sprite_no; i <= sprite_no2; i++)
                refreshSprite(i, false, cell_no, 0, 0);
        }
        else if (com == 'P' || com == 'p') {
            sprite_no = getNumberFromBuffer(&ctl_str);
            if (*ctl_str == ',') {
                ctl_str++;
                cell_no = getNumberFromBuffer(&ctl_str);
            }
            else
                cell_no = 0;
            refreshSprite(sprite_no, true, cell_no,
			  check_src_rect, check_dst_rect);
        }
        else if (com == 'S' || com == 's') {
            sprite_no = getNumberFromBuffer(&ctl_str);
            if (sprite_no < 0) sprite_no = 0;
            else if (sprite_no >=
                     ONS_MIX_CHANNELS) sprite_no = ONS_MIX_CHANNELS - 1;
            if (*ctl_str != ',') continue;
            ctl_str++;
            if (*ctl_str != '(') continue;
            ctl_str++;
            char* buf = sound_name;
            while (*ctl_str != ')' && *ctl_str != '\0') *buf++ = *ctl_str++;
            *buf++ = '\0';
            playSound(sound_name, SOUND_WAVE | SOUND_OGG, false, sprite_no);
            if (*ctl_str == ')') ctl_str++;
        }
        else if (com == 'M' || com == 'm') {
            sprite_no = getNumberFromBuffer(&ctl_str);
            SDL_Rect rect = sprite_info[sprite_no].pos;
            if (*ctl_str != ',') continue;
            ctl_str++; // skip ','
            sprite_info[sprite_no].pos.x = getNumberFromBuffer(&ctl_str) *
                                           screen_ratio1 / screen_ratio2;
            if (*ctl_str != ',') continue;
            ctl_str++; // skip ','
            sprite_info[sprite_no].pos.y = getNumberFromBuffer(&ctl_str) *
                                           screen_ratio1 / screen_ratio2;
            dirty_rect.add(rect);
            sprite_info[sprite_no].visible = true;
            dirty_rect.add(sprite_info[sprite_no].pos);
        }
    }
}


void PonscripterLabel::loadCursor(int no, const char* str, int x, int y,
                                 bool abs_flag)
{
    cursor_info[no].setImageName(str);
    cursor_info[no].pos.x = x;
    cursor_info[no].pos.y = y;

    parseTaggedString(&cursor_info[no]);
    setupAnimationInfo(&cursor_info[no]);
    if (filelog_flag)
        script_h.findAndAddLog(script_h.log_info[ScriptHandler::FILE_LOG],
			       cursor_info[no].file_name, true);
    cursor_info[no].abs_flag = abs_flag;
    if (cursor_info[no].image_surface)
        cursor_info[no].visible = true;
    else
        cursor_info[no].remove();
}


void PonscripterLabel::saveAll()
{
    saveEnvData();
    saveGlovalData();
    if (filelog_flag) writeLog(script_h.log_info[ScriptHandler::FILE_LOG]);
    if (labellog_flag) writeLog(script_h.log_info[ScriptHandler::LABEL_LOG]);
    if (kidokuskip_flag) script_h.saveKidokuData();
}


void PonscripterLabel::loadEnvData()
{
    volume_on_flag = true;
    text_speed_no  = 1;
    draw_one_page_flag = false;
    default_env_font    = 0;
    cdaudio_on_flag     = true;
    default_cdrom_drive = 0;
    kidokumode_flag = true;

    if (loadFileIOBuf("envdata") == 0) {
        if (readInt() == 1 && window_mode == false) menu_fullCommand();
        if (readInt() == 0) volume_on_flag = false;
        text_speed_no = readInt();
        if (readInt() == 1) draw_one_page_flag = true;
        readStr(&default_env_font);
        if (default_env_font == 0)
            setStr(&default_env_font, DEFAULT_ENV_FONT);
        if (readInt() == 0) cdaudio_on_flag = false;
        readStr(&default_cdrom_drive);
        voice_volume = DEFAULT_VOLUME - readInt();
        se_volume    = DEFAULT_VOLUME - readInt();
        music_volume = DEFAULT_VOLUME - readInt();
        if (readInt() == 0) kidokumode_flag = false;
    }
    else {
        setStr(&default_env_font, DEFAULT_ENV_FONT);
        voice_volume = se_volume = music_volume = DEFAULT_VOLUME;
    }
}


void PonscripterLabel::saveEnvData()
{
    file_io_buf_ptr = 0;
    bool output_flag = false;
    for (int i = 0; i < 2; i++) {
        writeInt(fullscreen_mode ? 1 : 0, output_flag);
        writeInt(volume_on_flag ? 1 : 0, output_flag);
        writeInt(text_speed_no, output_flag);
        writeInt(draw_one_page_flag ? 1 : 0, output_flag);
        writeStr(default_env_font, output_flag);
        writeInt(cdaudio_on_flag ? 1 : 0, output_flag);
        writeStr(default_cdrom_drive, output_flag);
        writeInt(DEFAULT_VOLUME - voice_volume, output_flag);
        writeInt(DEFAULT_VOLUME - se_volume, output_flag);
        writeInt(DEFAULT_VOLUME - music_volume, output_flag);
        writeInt(kidokumode_flag ? 1 : 0, output_flag);
        writeChar(0, output_flag); // ?
        writeInt(1000, output_flag);

        if (i == 1) break;
        allocFileIOBuf();
        output_flag = true;
    }

    saveFileIOBuf("envdata");
}


int PonscripterLabel::refreshMode()
{
    return display_mode == TEXT_DISPLAY_MODE
	 ? refresh_shadow_text_mode
	 : REFRESH_NORMAL_MODE;
}


void PonscripterLabel::quit()
{
    saveAll();

    if (cdrom_info) {
        SDL_CDStop(cdrom_info);
        SDL_CDClose(cdrom_info);
    }
    if (midi_info) {
        Mix_HaltMusic();
        Mix_FreeMusic(midi_info);
    }
    if (music_info) {
        Mix_HaltMusic();
        Mix_FreeMusic(music_info);
    }
}


void PonscripterLabel::disableGetButtonFlag()
{
    btndown_flag = false;

    getzxc_flag      = false;
    gettab_flag      = false;
    getpageup_flag   = false;
    getpagedown_flag = false;
    getinsert_flag   = false;
    getfunction_flag = false;
    getenter_flag    = false;
    getcursor_flag   = false;
    spclclk_flag     = false;
}


int PonscripterLabel::getNumberFromBuffer(const char** buf)
{
    int ret = 0;
    while (**buf >= '0' && **buf <= '9')
        ret = ret * 10 + *(*buf)++ - '0';

    return ret;
}

