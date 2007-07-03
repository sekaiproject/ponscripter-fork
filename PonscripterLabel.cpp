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

typedef int (PonscripterLabel::*PonscrFun)(const string&);
static class sfunc_lut_t {
    typedef dictionary<string, PonscrFun>::t dic_t;
    dic_t dict;
public:
    sfunc_lut_t();
    PonscrFun get(string what) const {
	dic_t::const_iterator it = dict.find(what);
	if (it == dict.end()) return 0;
	return it->second;
    }
} func_lut;
sfunc_lut_t::sfunc_lut_t() {
    dict["abssetcursor"]     = &PonscripterLabel::setcursorCommand;
    dict["allsphide"]        = &PonscripterLabel::allsphideCommand;
    dict["allspresume"]      = &PonscripterLabel::allspresumeCommand;
    dict["amsp"]             = &PonscripterLabel::amspCommand;
    dict["autoclick"]        = &PonscripterLabel::autoclickCommand;
    dict["automode_time"]    = &PonscripterLabel::automode_timeCommand;
    dict["avi"]              = &PonscripterLabel::aviCommand;
    dict["bar"]              = &PonscripterLabel::barCommand;
    dict["barclear"]         = &PonscripterLabel::barclearCommand;
    dict["bg"]               = &PonscripterLabel::bgCommand;
    dict["bgcopy"]           = &PonscripterLabel::bgcopyCommand;
    dict["bgcpy"]            = &PonscripterLabel::bgcopyCommand;
    dict["bgm"]              = &PonscripterLabel::mp3Command;
    dict["bgmonce"]          = &PonscripterLabel::mp3Command;
    dict["bgmstop"]          = &PonscripterLabel::playstopCommand;
    dict["bgmvol"]           = &PonscripterLabel::mp3volCommand;
    dict["blt"]              = &PonscripterLabel::bltCommand;
    dict["br"]               = &PonscripterLabel::brCommand;
    dict["br2"]              = &PonscripterLabel::brCommand;
    dict["btn"]              = &PonscripterLabel::btnCommand;
    dict["btndef"]           = &PonscripterLabel::btndefCommand;
    dict["btndown"]          = &PonscripterLabel::btndownCommand;
    dict["btntime"]          = &PonscripterLabel::btntimeCommand;
    dict["btntime2"]         = &PonscripterLabel::btntimeCommand;
    dict["btnwait"]          = &PonscripterLabel::btnwaitCommand;
    dict["btnwait2"]         = &PonscripterLabel::btnwaitCommand;
    dict["caption"]          = &PonscripterLabel::captionCommand;
    dict["cell"]             = &PonscripterLabel::cellCommand;
    dict["cellcheckexbtn"]   = &PonscripterLabel::exbtnCommand;
    dict["cellcheckspbtn"]   = &PonscripterLabel::spbtnCommand;
    dict["checkpage"]        = &PonscripterLabel::checkpageCommand;
    dict["chvol"]            = &PonscripterLabel::chvolCommand;
    dict["cl"]               = &PonscripterLabel::clCommand;
    dict["click"]            = &PonscripterLabel::clickCommand;
    dict["csel"]             = &PonscripterLabel::selectCommand;
    dict["cselbtn"]          = &PonscripterLabel::cselbtnCommand;
    dict["cselgoto"]         = &PonscripterLabel::cselgotoCommand;
    dict["csp"]              = &PonscripterLabel::cspCommand;
    dict["definereset"]      = &PonscripterLabel::defineresetCommand;
    dict["delay"]            = &PonscripterLabel::delayCommand;
    dict["draw"]             = &PonscripterLabel::drawCommand;
    dict["drawbg"]           = &PonscripterLabel::drawbgCommand;
    dict["drawbg2"]          = &PonscripterLabel::drawbg2Command;
    dict["drawclear"]        = &PonscripterLabel::drawclearCommand;
    dict["drawfill"]         = &PonscripterLabel::drawfillCommand;
    dict["drawsp"]           = &PonscripterLabel::drawspCommand;
    dict["drawsp2"]          = &PonscripterLabel::drawsp2Command;
    dict["drawsp3"]          = &PonscripterLabel::drawsp3Command;
    dict["drawtext"]         = &PonscripterLabel::drawtextCommand;
    dict["dwave"]            = &PonscripterLabel::dwaveCommand;
    dict["dwaveload"]        = &PonscripterLabel::dwaveCommand;
    dict["dwaveloop"]        = &PonscripterLabel::dwaveCommand;
    dict["dwaveplay"]        = &PonscripterLabel::dwaveCommand;
    dict["dwaveplayloop"]    = &PonscripterLabel::dwaveCommand;
    dict["dwavestop"]        = &PonscripterLabel::dwavestopCommand;
    dict["end"]              = &PonscripterLabel::endCommand;
    dict["erasetextwindow"]  = &PonscripterLabel::erasetextwindowCommand;
    dict["exbtn"]            = &PonscripterLabel::exbtnCommand;
    dict["exbtn_d"]          = &PonscripterLabel::exbtnCommand;
    dict["exec_dll"]         = &PonscripterLabel::exec_dllCommand;
    dict["existspbtn"]       = &PonscripterLabel::spbtnCommand;
    dict["fileexist"]        = &PonscripterLabel::fileexistCommand;
    dict["game"]             = &PonscripterLabel::gameCommand;
    dict["getbgmvol"]        = &PonscripterLabel::getmp3volCommand;
    dict["getbtntimer"]      = &PonscripterLabel::gettimerCommand;
    dict["getcselnum"]       = &PonscripterLabel::getcselnumCommand;
    dict["getcselstr"]       = &PonscripterLabel::getcselstrCommand;
    dict["getcursor"]        = &PonscripterLabel::getcursorCommand;
    dict["getcursorpos"]     = &PonscripterLabel::getcursorposCommand;
    dict["getenter"]         = &PonscripterLabel::getenterCommand;
    dict["getfunction"]      = &PonscripterLabel::getfunctionCommand;
    dict["getinsert"]        = &PonscripterLabel::getinsertCommand;
    dict["getlog"]           = &PonscripterLabel::getlogCommand;
    dict["getmousepos"]      = &PonscripterLabel::getmouseposCommand;
    dict["getmp3vol"]        = &PonscripterLabel::getmp3volCommand;
    dict["getpage"]          = &PonscripterLabel::getpageCommand;
    dict["getpageup"]        = &PonscripterLabel::getpageupCommand;
    dict["getreg"]           = &PonscripterLabel::getregCommand;
    dict["getret"]           = &PonscripterLabel::getretCommand;
    dict["getscreenshot"]    = &PonscripterLabel::getscreenshotCommand;
    dict["getsevol"]         = &PonscripterLabel::getsevolCommand;
    dict["getspmode"]        = &PonscripterLabel::getspmodeCommand;
    dict["getspsize"]        = &PonscripterLabel::getspsizeCommand;
    dict["gettab"]           = &PonscripterLabel::gettabCommand;
    dict["gettag"]           = &PonscripterLabel::gettagCommand;
    dict["gettext"]          = &PonscripterLabel::gettextCommand;
    dict["gettimer"]         = &PonscripterLabel::gettimerCommand;
    dict["getversion"]       = &PonscripterLabel::getversionCommand;
    dict["getvoicevol"]      = &PonscripterLabel::getvoicevolCommand;
    dict["getzxc"]           = &PonscripterLabel::getzxcCommand;
    dict["h_breakstr"]       = &PonscripterLabel::haeleth_char_setCommand;
    dict["h_centreline"]     = &PonscripterLabel::haeleth_centre_lineCommand;
    dict["h_fontstyle"]      = &PonscripterLabel::haeleth_font_styleCommand;
    dict["h_indentstr"]      = &PonscripterLabel::haeleth_char_setCommand;
    dict["h_ligate"]         = &PonscripterLabel::haeleth_ligate_controlCommand;
    dict["h_mapfont"]        = &PonscripterLabel::haeleth_map_fontCommand;
    dict["h_rendering"]      = &PonscripterLabel::haeleth_hinting_modeCommand;
    dict["h_textextent"]     = &PonscripterLabel::haeleth_text_extentCommand;
    dict["humanorder"]       = &PonscripterLabel::humanorderCommand;
    dict["indent"]           = &PonscripterLabel::indentCommand;
    dict["input"]            = &PonscripterLabel::inputCommand;
    dict["isdown"]           = &PonscripterLabel::isdownCommand;
    dict["isfull"]           = &PonscripterLabel::isfullCommand;
    dict["ispage"]           = &PonscripterLabel::ispageCommand;
    dict["isskip"]           = &PonscripterLabel::isskipCommand;
    dict["jumpb"]            = &PonscripterLabel::jumpbCommand;
    dict["jumpf"]            = &PonscripterLabel::jumpfCommand;
    dict["ld"]               = &PonscripterLabel::ldCommand;
    dict["loadgame"]         = &PonscripterLabel::loadgameCommand;
    dict["locate"]           = &PonscripterLabel::locateCommand;
    dict["logsp"]            = &PonscripterLabel::logspCommand;
    dict["logsp2"]           = &PonscripterLabel::logspCommand;
    dict["lookbackbutton"]   = &PonscripterLabel::lookbackbuttonCommand;
    dict["lookbackflush"]    = &PonscripterLabel::lookbackflushCommand;
    dict["loopbgm"]          = &PonscripterLabel::loopbgmCommand;
    dict["loopbgmstop"]      = &PonscripterLabel::loopbgmstopCommand;
    dict["lr_trap"]          = &PonscripterLabel::trapCommand;
    dict["lsp"]              = &PonscripterLabel::lspCommand;
    dict["lsph"]             = &PonscripterLabel::lspCommand;
    dict["menu_automode"]    = &PonscripterLabel::menu_automodeCommand;
    dict["menu_full"]        = &PonscripterLabel::menu_fullCommand;
    dict["menu_window"]      = &PonscripterLabel::menu_windowCommand;
    dict["monocro"]          = &PonscripterLabel::monocroCommand;
    dict["movemousecursor"]  = &PonscripterLabel::movemousecursorCommand;
    dict["mp3"]              = &PonscripterLabel::mp3Command;
    dict["mp3fadeout"]       = &PonscripterLabel::mp3fadeoutCommand;
    dict["mp3loop"]          = &PonscripterLabel::mp3Command;
    dict["mp3save"]          = &PonscripterLabel::mp3Command;
    dict["mp3stop"]          = &PonscripterLabel::playstopCommand;
    dict["mp3vol"]           = &PonscripterLabel::mp3volCommand;
    dict["mpegplay"]         = &PonscripterLabel::mpegplayCommand;
    dict["msp"]              = &PonscripterLabel::mspCommand;
    dict["nega"]             = &PonscripterLabel::negaCommand;
    dict["ofscopy"]          = &PonscripterLabel::ofscopyCommand;
    dict["ofscpy"]           = &PonscripterLabel::ofscopyCommand;
    dict["play"]             = &PonscripterLabel::playCommand;
    dict["playonce"]         = &PonscripterLabel::playCommand;
    dict["playstop"]         = &PonscripterLabel::playstopCommand;
    dict["print"]            = &PonscripterLabel::printCommand;
    dict["prnum"]            = &PonscripterLabel::prnumCommand;
    dict["prnumclear"]       = &PonscripterLabel::prnumclearCommand;
    dict["puttext"]          = &PonscripterLabel::puttextCommand;
    dict["quake"]            = &PonscripterLabel::quakeCommand;
    dict["quakex"]           = &PonscripterLabel::quakeCommand;
    dict["quakey"]           = &PonscripterLabel::quakeCommand;
    dict["repaint"]          = &PonscripterLabel::repaintCommand;
    dict["reset"]            = &PonscripterLabel::resetCommand;
    dict["resettimer"]       = &PonscripterLabel::resettimerCommand;
    dict["rmode"]            = &PonscripterLabel::rmodeCommand;
    dict["rnd"]              = &PonscripterLabel::rndCommand;
    dict["rnd2"]             = &PonscripterLabel::rndCommand;
    dict["say"]              = &PonscripterLabel::haeleth_sayCommand;
    dict["savefileexist"]    = &PonscripterLabel::savefileexistCommand;
    dict["savegame"]         = &PonscripterLabel::savegameCommand;
    dict["saveoff"]          = &PonscripterLabel::saveoffCommand;
    dict["saveon"]           = &PonscripterLabel::saveonCommand;
    dict["savescreenshot"]   = &PonscripterLabel::savescreenshotCommand;
    dict["savescreenshot2"]  = &PonscripterLabel::savescreenshotCommand;
    dict["savetime"]         = &PonscripterLabel::savetimeCommand;
    dict["select"]           = &PonscripterLabel::selectCommand;
    dict["selectbtnwait"]    = &PonscripterLabel::btnwaitCommand;
    dict["selgosub"]         = &PonscripterLabel::selectCommand;
    dict["selnum"]           = &PonscripterLabel::selectCommand;
    dict["setcursor"]        = &PonscripterLabel::setcursorCommand;
    dict["setwindow"]        = &PonscripterLabel::setwindowCommand;
    dict["setwindow2"]       = &PonscripterLabel::setwindow2Command;
    dict["setwindow3"]       = &PonscripterLabel::setwindow3Command;
    dict["sevol"]            = &PonscripterLabel::sevolCommand;
    dict["skipoff"]          = &PonscripterLabel::skipoffCommand;
    dict["sp_rgb_gradation"] = &PonscripterLabel::sp_rgb_gradationCommand;
    dict["spbtn"]            = &PonscripterLabel::spbtnCommand;
    dict["spclclk"]          = &PonscripterLabel::spclclkCommand;
    dict["split"]            = &PonscripterLabel::splitCommand;
    dict["splitstring"]      = &PonscripterLabel::splitCommand;
    dict["spreload"]         = &PonscripterLabel::spreloadCommand;
    dict["spstr"]            = &PonscripterLabel::spstrCommand;
    dict["stop"]             = &PonscripterLabel::stopCommand;
    dict["strsp"]            = &PonscripterLabel::strspCommand;
    dict["systemcall"]       = &PonscripterLabel::systemcallCommand;
    dict["tablegoto"]        = &PonscripterLabel::tablegotoCommand;
    dict["tal"]              = &PonscripterLabel::talCommand;
    dict["tateyoko"]         = &PonscripterLabel::tateyokoCommand;
    dict["texec"]            = &PonscripterLabel::texecCommand;
    dict["textbtnwait"]      = &PonscripterLabel::btnwaitCommand;
    dict["textclear"]        = &PonscripterLabel::textclearCommand;
    dict["texthide"]         = &PonscripterLabel::texthideCommand;
    dict["textoff"]          = &PonscripterLabel::textoffCommand;
    dict["texton"]           = &PonscripterLabel::textonCommand;
    dict["textshow"]         = &PonscripterLabel::textshowCommand;
    dict["textspeed"]        = &PonscripterLabel::textspeedCommand;
    dict["trap"]             = &PonscripterLabel::trapCommand;
    dict["voicevol"]         = &PonscripterLabel::voicevolCommand;
    dict["vsp"]              = &PonscripterLabel::vspCommand;
    dict["wait"]             = &PonscripterLabel::waitCommand;
    dict["waittimer"]        = &PonscripterLabel::waittimerCommand;
    dict["wave"]             = &PonscripterLabel::waveCommand;
    dict["waveloop"]         = &PonscripterLabel::waveCommand;
    dict["wavestop"]         = &PonscripterLabel::wavestopCommand;
}

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

    wm_title_string = DEFAULT_WM_TITLE;
    wm_icon_string = DEFAULT_WM_ICON;
    SDL_WM_SetCaption(wm_title_string.c_str(), wm_icon_string.c_str());

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
    : registry_file(REGISTRY_FILE),
      dll_file(DLL_FILE),
      music_cmd(getenv("PLAYER_CMD")),
      midi_cmd(getenv("MUSIC_CMD"))
{
    cdrom_drive_number = 0;
    cdaudio_flag  = false;
    enable_wheeldown_advance_flag = false;
    disable_rescale_flag = false;
    edit_flag       = false;
    fullscreen_mode = false;
    window_mode     = false;
    skip_to_wait    = 0;
    glyph_surface   = 0;
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
    registry_file = filename;
}


void PonscripterLabel::setDLLFile(const char* filename)
{
    dll_file = filename;
}


void PonscripterLabel::setArchivePath(string path)
{
    archive_path = path + DELIMITER;
}


void PonscripterLabel::setSavePath(string path)
{
    script_h.save_path = path + DELIMITER;
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
    key_exe_file = filename;
}

#ifdef MACOSX
string MacOSX_SeekArchive()
{
    // Store archives etc in the application bundle by default, but
    // fall back to the application root directory if the bundle
    // doesn't contain any script files.
    using namespace Carbon;
    string rv;
    CFURLRef url;
    const CFIndex max_path = 32768;
    UInt8 path[max_path];
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
	if ((url = CFBundleCopyResourcesDirectoryURL(bundle))) {
	    if (CFURLGetFileSystemRepresentation(url, true, path, max_path))
		rv = string(path) + '/';
            CFRelease(url);
	}
	if (rv) {
	    // Verify that this is the archive path by checking for a script
	    // file.
	    ScriptHandler::ScriptFilename::iterator it =
		script_h.script_filenames.begin();
	    for (; it != script_h.script_filenames.end(); ++it) {
		string s = rv + it->filename;
		FSRef ref;
		// If we've found a script file, we've found the archive
		// path, so return it.
		if (FSPathMakeRef(s.u_str(), &ref, 0) == noErr &&
		    FSGetCatalogInfo(&ref, kFSCatInfoNone, 0, 0, 0, 0) == noErr)
		    return rv;
	    }
	}
	// There were no script files in the application bundle.
	// Assume that the script files are intended to be in the
	// application path.
	if ((url = CFBundleCopyBundleURL(bundle))) {
	    CFURLRef app =
		CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault,
							 url);
	    CFRelease(url);
	    if (app) {
		Boolean valid =
		    CFURLGetFileSystemRepresentation(app, true, path, max_path);
		CFRelease(appurl);
		if (valid) return string(path) + '/';
	    }
	}
    }
    return "";
}
#endif

#ifdef WIN32
string Platform_GetSavePath(string gameid) // Windows version
{
    // On Windows, store saves in [Profiles]/All Users/Application Data.
    // TODO: optionally permit saves to be per-user rather than shared?
    gameid.replace('\\', '_');    
    HMODULE shdll = LoadLibrary("shfolder");
    string rv;
    if (shdll) {
	GETFOLDERPATH gfp =
	    GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
	if (gfp) {
	    char hpath[MAX_PATH];
	    HRESULT res = gfp(0, 0x0023, 0, 0, hpath);
	    if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
		rv = string(hpath) + '/' + gameid + '/';
		CreateDirectory(rv.c_str(), 0);
	    }
	}
	FreeLibrary(shdll);
    }
    return rv;
}
#elif defined MACOSX
string Platform_GetSavePath(string gameid) // MacOS X version
{
    // On Mac OS X, place in ~/Library/Application Support/<gameid>/
    gameid.replace('/', '_');
    using namespace Carbon;
    FSRef appsupport;
    FSFindFolder(kUserDomain, kApplicationSupportFolderType, kDontCreateFolder,
		 &appsupport);
    char path[32768];
    FSRefMakePath(&appsupport, (UInt8*) path, 32768);
    mkdir(path, 0755);
    return string(path) + '/' + gameid + '/';
}
#elif defined LINUX
string Platform_GetSavePath(string gameid) // POSIX version
{
    // On Linux (and other POSIX-a-likes), place in ~/.gameid
    gameid.replace(' ', '_');
    gameid.replace('/', '_');
    gameid.replace('(', '_');
    gameid.replace(')', '_');		   
    gameid.replace('[', '_');
    gameid.replace(']', '_');		   
    passwd* pwd = getpwuid(getuid());
    if (pwd) {
	string rv = string(pwd->pw_dir) + "/." + gameid + '/';
	if (mkdir(rv.c_str(), 0755) == 0 || errno == EEXIST)
	    return rv;
    }
    // Error; either getpwuid failed, or we couldn't create a save
    // directory.  Either way, issue a warning and then give up.
    fprintf(stderr, "Warning: could not create save directory ~/.%s.\n",
	    gameid.c_str());
    return "";
}
#else
string Platform_GetSavePath(const string& gameid) // Stub for unknown platforms
{
    return "";
}
#endif

// Retrieve a game identifier.
string getGameId(ScriptHandler& script_h)
{
    // Ideally, this will have been supplied with a ;gameid directive.
    if (script_h.game_identifier)
	return script_h.game_identifier;

    // If it wasn't, first we try to find the game's name as given in a
    // versionstr or caption command.
    ScriptHandler::LabelInfo define = script_h.lookupLabel("define");
    string caption, versionstr;
    script_h.pushCurrent(define.start_address);
    while (script_h.getLineByAddress(script_h.getCurrent())
	   < define.num_of_lines)
    {
	string t = script_h.readToken(true);
	t.trim();
	if (t == "caption")
	    caption = script_h.readStrValue();
	else if (t == "versionstr")
	    versionstr = script_h.readStrValue();
	if (!t || t[0] == ';' || script_h.getCurrent() >= script_h.getNext())
	    script_h.skipLine();
	else if (t == "game")
	    break;
	else
	    script_h.skipToken();
    }
    script_h.popCurrent();
    if (caption || versionstr) {
	caption.trim();
	versionstr.trim();
	string& id = caption == versionstr || caption.size() < versionstr.size()
	           ? caption
	           : versionstr;
	id.zentohan();
	return id;
    }
    
    // The fallback position is to generate a semi-unique ID using the
    // length of the script file as a cheap hash.
    return "Ponscripter-" + str(script_h.getScriptBufferLength(), 16);
}

int PonscripterLabel::init()
{
    // On Mac OS X, archives may be stored in the application bundle.
    // On other platforms the location will either be in the EXE
    // directory, the current directory, or somewhere unpredictable
    // that must be specified with the -r option; in all such cases we
    // assume the current directory if nothing else was specified.
#ifdef MACOSX
    if (!archive_path) archive_path = MacOSX_SeekArchive();
#endif
   
    if (key_exe_file) {
        createKeyTable(key_exe_file);
        script_h.setKeyTable(key_table);
    }

    if (open()) return -1;

    // Try to determine an appropriate location for saved games.
    if (!script_h.save_path)
	script_h.save_path = Platform_GetSavePath(getGameId(script_h));

    // If we couldn't find anything obvious, fall back on ONScripter
    // behaviour of putting saved games in the archive path.
    if (!script_h.save_path) script_h.save_path = archive_path;

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

    wave_file_name.clear();
    midi_file_name.clear();
    midi_info  = 0;
    mp3_sample = 0;
    music_file_name.clear();
    mp3_buffer = 0;
    music_info = 0;
    music_ovi  = 0;

    loop_bgm_name[0].clear();
    loop_bgm_name[1].clear();

    int i;
    for (i = 0; i < ONS_MIX_CHANNELS + ONS_MIX_EXTRA_CHANNELS;
         i++) wave_sample[i] = 0;

    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    trap_dist.clear();
    resize_buffer = new unsigned char[16];
    resize_buffer_size = 16;

    for (i = 0; i < MAX_PARAM_NUM; i++) bar_info[i] = prnum_info[i] = 0;

    defineresetCommand("definereset");
    readToken();

    loadEnvData();

    InitialiseFontSystem(archive_path);

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

    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = false;

    resetSentenceFont();

    getret_str.clear();
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

    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE |
                               REFRESH_TEXT_MODE;
    erase_text_window_mode = 1;
    skip_flag    = false;
    monocro_flag = false;
    nega_mode = 0;
    clickstr_state = CLICK_NONE;
    trap_mode = TRAP_NONE;
    trap_dist.clear();

    saveon_flag = true;
    internal_saveon_flag = true;

    textgosub_clickstr_state = CLICK_NONE;
    indent_offset = 0;
    line_enter_status = 0;

    resetSentenceFont();

    deleteNestInfo();
    deleteButtons();
    select_links.clear();

    stopCommand("stop");
    loopbgmstopCommand("loopbgmstop");
    loop_bgm_name[1].clear();

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    bg_info.file_name = "black";
    createBackground();
    for (i = 0; i < 3; i++) tachi_info[i].reset();
    for (i = 0; i < MAX_SPRITE_NUM; i++) sprite_info[i].reset();
    barclearCommand("barclear");
    prnumclearCommand("prnumclear");
    for (i = 0; i < 2; i++) cursor_info[i].reset();
    for (i = 0; i < 4; i++) lookback_info[i].reset();
    sentence_font_info.reset();

    // Initialize character sets
    DefaultLigatures(1);
    indent_chars.clear();
    indent_chars.insert(0x0028);
    indent_chars.insert(0x2014);
    indent_chars.insert(0x2018);
    indent_chars.insert(0x201c);
    indent_chars.insert(0x300c);
    indent_chars.insert(0x300e);
    indent_chars.insert(0xff08);
    indent_chars.insert(0xff5e);
    indent_chars.insert(0xff62);
    break_chars.clear();
    break_chars.insert(0x0020);
    break_chars.insert(0x002d);
    break_chars.insert(0x2013);
    break_chars.insert(0x2014);

    dirty_rect.fill(screen_width, screen_height);
}


void PonscripterLabel::resetSentenceFont()
{
    Fontinfo::default_encoding = Default;
    sentence_font.reset();
    sentence_font.top_x = 21;
    sentence_font.top_y = 16;
    sentence_font.area_x = screen_width - 21;
    sentence_font.area_y = screen_height - 16;
    sentence_font.pitch_x = 0;
    sentence_font.pitch_y = 0;
    sentence_font.wait_time = 20;
    sentence_font.window_color.set(0x99);
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
            if (dirty_rect.area >= dirty_rect.bounding_box.w *
		                   dirty_rect.bounding_box.h) {
                flushDirect(dirty_rect.bounding_box, refresh_mode);
	    }
            else {
                for (int i = 0; i < dirty_rect.num_history; i++)
                    flushDirect(dirty_rect.history[i], refresh_mode, false);
		SDL_UpdateRects(screen_surface, dirty_rect.num_history,
				dirty_rect.history);
            }
        }
    }

    if (clear_dirty_flag) dirty_rect.clear();
}


void PonscripterLabel::flushDirect(SDL_Rect &rect, int refresh_mode, bool updaterect)
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
    if (updaterect) SDL_UpdateRect(screen_surface, rect.x, rect.y, rect.w, rect.h);
}


void PonscripterLabel::mouseOverCheck(int x, int y)
{
    size_t c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;

    for (ButtonElt::iterator it = buttons.begin(); it != buttons.end();
	 ++it, ++c) {
	const SDL_Rect& r = it->second.select_rect;
	if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
	    button = it->first;
	    break;
	}
    }

    if (current_over_button != button) {
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = { 0, 0, 0, 0 };
        SDL_Rect check_dst_rect = { 0, 0, 0, 0 };
        if (current_over_button != 0) {
	    ButtonElt& curr_btn = buttons[current_over_button];

            curr_btn.show_flag = 0;
            check_src_rect = curr_btn.image_rect;
            if (curr_btn.isSprite()) {
                sprite_info[curr_btn.sprite_no].visible = true;
                sprite_info[curr_btn.sprite_no].setCell(0);
            }
            else if (curr_btn.isTmpSprite()) {
                curr_btn.show_flag = 1;
                curr_btn.anim[0]->visible = true;
                curr_btn.anim[0]->setCell(0);
            }
            else if (curr_btn.anim[1]) {
                curr_btn.show_flag = 2;
            }
            dirty_rect.add(curr_btn.image_rect);
        }

        if (exbtn_d_button.exbtn_ctl) {
            decodeExbtnControl(exbtn_d_button.exbtn_ctl,
			       &check_src_rect, &check_dst_rect);
        }

        if (c < buttons.size()) {
            if (system_menu_mode != SYSTEM_NULL)
                playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
			  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
            else
		playSound(selectvoice_file_name[SELECTVOICE_OVER],
			  SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);

	    ButtonElt& new_btn = buttons[button];
	    
            check_dst_rect = new_btn.image_rect;
            if (new_btn.isSprite()) {
                sprite_info[new_btn.sprite_no].setCell(1);
                sprite_info[new_btn.sprite_no].visible = true;
                if (new_btn.button_type == ButtonElt::EX_SPRITE_BUTTON) {
                    decodeExbtnControl(new_btn.exbtn_ctl,
				       &check_src_rect, &check_dst_rect);
                }
            }
            else if (new_btn.isTmpSprite()) {
                new_btn.show_flag = 1;
                new_btn.anim[0]->visible = true;
                new_btn.anim[0]->setCell(1);
            }
            else if (new_btn.button_type == ButtonElt::NORMAL_BUTTON ||
                     new_btn.button_type == ButtonElt::LOOKBACK_BUTTON) {
                new_btn.show_flag = 1;
            }
            dirty_rect.add(new_btn.image_rect);
            shortcut_mouse_line = buttons.find(button);
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
		   current_label_info.name.c_str(),
		   current_line,
		   current_label_info.num_of_lines,
		   string_buffer_offset, display_mode);

        if (script_h.getStringBuffer()[0] == '~') {
            last_tilde = script_h.getNext();
            readToken();
            continue;
        }
        if (break_flag && script_h.getStringBuffer() != "next") {
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

        const char* current = script_h.getCurrent();
        int ret = ScriptParser::parseLine();
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
    endCommand("end");
}


bool PonscripterLabel::check_orphan_control()
{
    // Check whether the current break point follows a logical break
    // in the text.  This is used to prevent short words being
    // stranded at the end of a line.
    if (string_buffer_offset < 5) return false;
    const char* c = script_h.getStringBuffer().c_str();
    const wchar p =
	encoding->Decode(encoding->Previous(c + string_buffer_offset, c));
    return p == '.' || p == 0xff0e || p == ',' || p == 0xff0c
        || p == ':' || p == 0xff1a || p == ';' || p == 0xff1b
        || p == '!' || p == 0xff01 || p == '?' || p == 0xff1f;
}


int PonscripterLabel::parseLine()
{
    int ret = 0;
    string cmd = script_h.getStringBuffer();
    if (cmd[0] == '_') cmd.shift();

    if (!script_h.isText()) {
	PonscrFun f = func_lut.get(cmd);
	if (f) return (this->*f)(cmd);

        if (cmd[0] == 0x0a)
            return RET_CONTINUE;
        else if (cmd[0] == 'v' && cmd[1] >= '0' && cmd[1] <= '9')
            return vCommand(cmd);
        else if (cmd[0] == 'd' && cmd[1] == 'v' && cmd[2] >= '0' &&
                 cmd[2] <= '9')
            return dvCommand(cmd);

        fprintf(stderr, " command [%s] is not supported yet!!\n", cmd.c_str());

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text */
    if (current_mode == DEFINE_MODE)
	errorAndExit("text cannot be displayed in define section.");

//--------INDENT ROUTINE--------------------------------------------------------
    if (sentence_font.GetXOffset() == 0 && sentence_font.GetYOffset() == 0) {
        const wchar first_ch =
	    encoding->Decode(script_h.getStringBuffer().c_str() +
			     string_buffer_offset);
        if (is_indent_char(first_ch))
            sentence_font.SetIndent(first_ch);
        else
            sentence_font.ClearIndent();
    }
//--------END INDENT ROUTINE----------------------------------------------------

    ret = textCommand();

//--------LINE BREAKING ROUTINE-------------------------------------------------
    const wchar first_ch =
	encoding->Decode(script_h.getStringBuffer().c_str() +
			 string_buffer_offset);
    if (is_break_char(first_ch) && !new_line_skip_flag) {
        const char* it = script_h.getStringBuffer().c_str() + string_buffer_offset
	    + encoding->CharacterBytes(script_h.getStringBuffer().c_str() +
				       string_buffer_offset);
        float len = sentence_font.GlyphAdvance(first_ch, encoding->Decode(it));
        while (1) {
            // For each character (not char!) before a break is found,
            // get unicode.
            wchar ch = encoding->Decode(it);
            it += encoding->CharacterBytes(it);

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
            len += sentence_font.GlyphAdvance(ch, encoding->Decode(it));
        }
        if (check_orphan_control()) {
            // If this is the start of a sentence, or follows some
            // other punctuation that makes this desirable, we pretend
            // short words have a minimum length.
            // Consider the case where we are rendering "O how I love
            // thee, pie! I eat of thee each day" in a fixed-width
            // font in a text window 25 characters wide.  A naive
            // approach would lead to rendering this as
            //    -------------------------
            //    O how I love thee, pie! I
            //    eat of thee each day.
            //    -------------------------
            // By treating the second "I", which begins a new
            // sentence, as though it was four letters long, we get
            // instead the more readable layout
            //    -------------------------
            //    O how I love thee, pie!
            //    I eat of thee each day.
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


SDL_Surface* PonscripterLabel::loadImage(const string& file_name,
					 bool* has_alpha)
{
    if (!file_name) return 0;
    unsigned long length = ScriptHandler::cBR->getFileLength(file_name);
    if (length == 0) {
        if (file_name != DEFAULT_LOOKBACK_NAME0 &&
            file_name != DEFAULT_LOOKBACK_NAME1 &&
            file_name != DEFAULT_LOOKBACK_NAME2 &&
            file_name != DEFAULT_LOOKBACK_NAME3 &&
	    file_name != DEFAULT_CURSOR0 &&
	    file_name != DEFAULT_CURSOR1)
          fprintf(stderr, " *** can't find file [%s] ***\n", file_name.c_str());
        return 0;
    }
    if (filelog_flag) script_h.file_log.add(file_name);

    unsigned char* buffer = new unsigned char[length];
    int location;
    ScriptHandler::cBR->getFile(file_name, buffer, &location);
    SDL_Surface* tmp = IMG_Load_RW(SDL_RWFromMem(buffer, length), 1);

    if (!tmp && file_name.substr(file_name.rfind('.') + 1).wicompare("jpg")) {
        fprintf(stderr, " *** force-loading a JPEG image [%s]\n",
		file_name.c_str());
        SDL_RWops* src = SDL_RWFromMem(buffer, length);
        tmp = IMG_LoadJPG_RW(src);
        SDL_RWclose(src);
    }
    if (tmp && has_alpha) *has_alpha = tmp->format->Amask;

    delete[] buffer;
    if (!tmp) {
        fprintf(stderr, " *** can't load file [%s] ***\n", file_name.c_str());
        return 0;
    }

    SDL_Surface* ret = SDL_ConvertSurface(tmp, image_surface->format,
                           SDL_SWSURFACE);
    if (ret
        && screen_ratio2 != screen_ratio1
        && (!disable_rescale_flag ||
	    location == BaseReader::ARCHIVE_TYPE_NONE)) {
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

#ifndef BPP16
    // Hack to detect when a PNG image is likely to have an old-style
    // mask.  We assume that an old-style mask is intended if the
    // image either has no alpha channel, or the alpha channel it has
    // is completely opaque.  This behaviour can be overridden with
    // the --force-png-alpha and --force-png-nscmask command-line
    // options.
    if (has_alpha && *has_alpha) {
	if (png_mask_type == PNG_MASK_USE_NSCRIPTER)
	    *has_alpha = false;
	else if (png_mask_type == PNG_MASK_AUTODETECT) {	
	    SDL_LockSurface(ret);
	    const Uint32 aval = *(Uint32*)ret->pixels & ret->format->Amask;
	    if (aval != 0xffUL << ret->format->Ashift) goto breakme;
	    *has_alpha = false;
	    for (int y=0; y<ret->h; ++y) {
		Uint32* pixbuf = (Uint32*)((char*)ret->pixels + y * ret->pitch);
		for (int x=0; x<ret->w; ++x, ++pixbuf) {
		    if (*pixbuf & ret->format->Amask != aval) {
			*has_alpha = true;
			goto breakme;
		    }
		}
	    }
	breakme:
	    SDL_UnlockSurface(ret);
	}
    }
#else
#warning "BPP16 defined: PNGs with NScripter-style masks will not work as expected"
#endif
    
    return ret;
}


/* ---------------------------------------- */
void PonscripterLabel::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = 0;
    SDL_GetMouseState(&mx, &my);
    mouseOverCheck(mx, my);
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
        rgb_t color(current_font->window_color.r >> fmt->Rloss,
                     current_font->window_color.g >> fmt->Gloss,
                     current_font->window_color.b >> fmt->Bloss);

        for (int i = rect.y; i < rect.y + rect.h; i++) {
            for (int j = rect.x; j < rect.x + rect.w; j++, buf++) {
                *buf = (((*buf & fmt->Rmask) >> fmt->Rshift) * color.r >>
                        (8 - fmt->Rloss)) << fmt->Rshift |
                       (((*buf & fmt->Gmask) >> fmt->Gshift) * color.g >>
                        (8 - fmt->Gloss)) << fmt->Gshift |
		       (((*buf & fmt->Bmask) >> fmt->Bshift) * color.b >>
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


PonscripterLabel::ButtonElt
PonscripterLabel::getSelectableSentence(const string& buffer, Fontinfo* info,
					bool flush_flag, bool nofile_flag)
{
    ButtonElt rv;
    float current_x;
    current_x = info->GetXOffset();

    rv.button_type = ButtonElt::TMP_SPRITE_BUTTON;
    rv.show_flag = 1;

    AnimationInfo* anim = new AnimationInfo();
    rv.anim[0] = anim;

    anim->trans_mode = AnimationInfo::TRANS_STRING;
    anim->is_single_line = false;

    anim->num_of_cells = 2;
    anim->color_list.resize(2);
    anim->color_list[0] = nofile_flag ? info->nofile_color : info->off_color;
    anim->color_list[1] = info->on_color;

    anim->file_name = buffer;
    anim->pos.x   = Sint16(floor(info->GetX() * screen_ratio1 / screen_ratio2));
    anim->pos.y   = info->GetY() * screen_ratio1 / screen_ratio2;
    anim->visible = true;

    setupAnimationInfo(anim, info);
    rv.select_rect = rv.image_rect = anim->pos;

    info->newLine();
    info->SetXY(current_x);

    dirty_rect.add(rv.image_rect);

    return rv;
}


void
PonscripterLabel::decodeExbtnControl(const string& ctl_string,
				     SDL_Rect* check_src_rect,
				     SDL_Rect* check_dst_rect)
{
    const char* ctl_str = ctl_string.c_str();
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
            else if (sprite_no >= ONS_MIX_CHANNELS)
		sprite_no = ONS_MIX_CHANNELS - 1;
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
    if (filelog_flag) script_h.file_log.add(cursor_info[no].file_name);
    cursor_info[no].abs_flag = abs_flag;
    if (cursor_info[no].image_surface)
        cursor_info[no].visible = true;
    else
        cursor_info[no].remove();
}


void PonscripterLabel::saveAll()
{
    saveEnvData();
    saveGlobalData();
    if (filelog_flag) script_h.file_log.write(script_h);
    if (labellog_flag) script_h.label_log.write(script_h);
    if (kidokuskip_flag) script_h.saveKidokuData();
}


void PonscripterLabel::loadEnvData()
{
    volume_on_flag = true;
    text_speed_no  = 1;
    draw_one_page_flag = false;
    cdaudio_on_flag = true;
    default_cdrom_drive.clear();
    kidokumode_flag = true;

    if (loadFileIOBuf("envdata") == 0) {
        if (readInt() == 1 && window_mode == false)
	    menu_fullCommand("menu_full");
        if (readInt() == 0)
	    volume_on_flag = false;
        text_speed_no = readInt();
        if (readInt() == 1)
	    draw_one_page_flag = true;
        default_env_font = readStr();
        if (!default_env_font)
	    default_env_font = DEFAULT_ENV_FONT;
        if (readInt() == 0)
	    cdaudio_on_flag = false;
        default_cdrom_drive = readStr();
        voice_volume = DEFAULT_VOLUME - readInt();
        se_volume    = DEFAULT_VOLUME - readInt();
        music_volume = DEFAULT_VOLUME - readInt();
        if (readInt() == 0)
	    kidokumode_flag = false;
    }
    else {
        default_env_font = DEFAULT_ENV_FONT;
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

