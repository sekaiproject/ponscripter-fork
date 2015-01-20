/* -*- C++ -*-
 *
 *  PonscripterLabel.cpp - Execution block parser of Ponscripter
 *
 *  Copyright (c) 2001-2008 Ogapee (original ONScripter, of which this
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
#include "PonscripterMessage.h"
#include "resources.h"
#include <ctype.h>

#ifdef MACOSX
namespace Carbon {
#include <sys/stat.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
}
#endif
#ifdef WIN32
#include <windows.h>
#include "SDL_syswm.h"
#include "winres.h"
typedef HRESULT (WINAPI * GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
#define PATH_MAX MAX_PATH
#endif
#ifdef LINUX
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <pwd.h>
#endif

#ifdef STEAM
  #ifdef _WIN32
    /* I assume it's a bug; steam_api doesn't preprocess without errors on mingw/win32 with this defined */
    #undef _WIN32
    #include <steam_api.h>
    #define _WIN32
  #else
    #include <steam_api.h>
  #endif
#endif

extern "C" void waveCallback(int channel);

#define DEFAULT_AUDIOBUF 4096

#define REGISTRY_FILE "registry.txt"
#define DLL_FILE "dll.txt"
#define DEFAULT_ENV_FONT "Sans"

typedef int (PonscripterLabel::*PonscrFun)(const pstring&);
static class sfunc_lut_t {
    typedef dictionary<pstring, PonscrFun>::t dic_t;
    dic_t dict;
public:
    sfunc_lut_t();
    PonscrFun get(pstring what) const {
        dic_t::const_iterator it = dict.find(what);
        if (it == dict.end()) return 0;
        return it->second;
    }
} func_lut;
sfunc_lut_t::sfunc_lut_t() {
    dict["abssetcursor"]     = &PonscripterLabel::setcursorCommand;
    dict["allsphide"]        = &PonscripterLabel::allsphideCommand;
    dict["allsp2hide"]       = &PonscripterLabel::allsp2hideCommand;
    dict["allspresume"]      = &PonscripterLabel::allspresumeCommand;
    dict["allsp2resume"]     = &PonscripterLabel::allsp2resumeCommand;
    dict["amsp"]             = &PonscripterLabel::mspCommand;
    dict["amsp2"]            = &PonscripterLabel::mspCommand;
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
    dict["bidirect"]         = &PonscripterLabel::bidirectCommand;
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
    dict["csp2"]             = &PonscripterLabel::cspCommand;
    dict["definereset"]      = &PonscripterLabel::defineresetCommand;
    dict["delay"]            = &PonscripterLabel::delayCommand;
    dict["deletescreenshot"] = &PonscripterLabel::deletescreenshotCommand;
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
    dict["gettextextent"]    = &PonscripterLabel::haeleth_text_extentCommand;
    dict["gettextheight"]    = &PonscripterLabel::haeleth_text_heightCommand;
    dict["gettextspeed"]     = &PonscripterLabel::gettextspeedCommand;
    dict["gettimer"]         = &PonscripterLabel::gettimerCommand;
    dict["getversion"]       = &PonscripterLabel::getversionCommand;
    dict["getvoicevol"]      = &PonscripterLabel::getvoicevolCommand;
    dict["getzxc"]           = &PonscripterLabel::getzxcCommand;
    dict["h_breakstr"]       = &PonscripterLabel::haeleth_char_setCommand;
    dict["h_centreline"]     = &PonscripterLabel::haeleth_centre_lineCommand;
    dict["h_fontstyle"]      = &PonscripterLabel::haeleth_font_styleCommand;
    dict["h_indentstr"]      = &PonscripterLabel::haeleth_char_setCommand;
    dict["h_ligate"]         = &PonscripterLabel::haeleth_ligate_controlCommand;
    dict["h_locate"]         = &PonscripterLabel::locateCommand;
    dict["h_mapfont"]        = &PonscripterLabel::haeleth_map_fontCommand;
    dict["h_rendering"]      = &PonscripterLabel::haeleth_hinting_modeCommand;
    dict["h_textheight"]     = &PonscripterLabel::haeleth_text_heightCommand;
    dict["h_textextent"]     = &PonscripterLabel::haeleth_text_extentCommand;
    dict["h_defwindow"]      = &PonscripterLabel::haeleth_defwindowCommand;
    dict["h_usewindow"]      = &PonscripterLabel::haeleth_usewindowCommand;
    dict["h_usewindow3"]     = &PonscripterLabel::haeleth_usewindowCommand;
    dict["h_speedpercent"]   = &PonscripterLabel::haeleth_speedpercentCommand;
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
    dict["localestring"]     = &PonscripterLabel::localestringCommand;
    dict["locate"]           = &PonscripterLabel::locateCommand;
    dict["logsp"]            = &PonscripterLabel::logspCommand;
    dict["logsp2"]           = &PonscripterLabel::logspCommand;
    dict["logsp2utf"]        = &PonscripterLabel::logspCommand;
    dict["lookbackbutton"]   = &PonscripterLabel::lookbackbuttonCommand;
    dict["lookbackflush"]    = &PonscripterLabel::lookbackflushCommand;
    dict["loopbgm"]          = &PonscripterLabel::loopbgmCommand;
    dict["loopbgmstop"]      = &PonscripterLabel::loopbgmstopCommand;
    dict["lr_trap"]          = &PonscripterLabel::trapCommand;
    dict["lsp"]              = &PonscripterLabel::lspCommand;
    dict["lsp2"]             = &PonscripterLabel::lspCommand;
    dict["lsph"]             = &PonscripterLabel::lspCommand;
    dict["lsph2"]            = &PonscripterLabel::lspCommand;
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
    dict["msp2"]             = &PonscripterLabel::mspCommand;
    dict["nega"]             = &PonscripterLabel::negaCommand;
    dict["ofscopy"]          = &PonscripterLabel::ofscopyCommand;
    dict["ofscpy"]           = &PonscripterLabel::ofscopyCommand;
    dict["pbreakstr"]        = &PonscripterLabel::haeleth_char_setCommand;
    dict["pcenterline"]      = &PonscripterLabel::haeleth_centre_lineCommand;
    dict["pdefwindow"]       = &PonscripterLabel::haeleth_defwindowCommand;
    dict["pfontstyle"]       = &PonscripterLabel::haeleth_font_styleCommand;
    dict["pindentstr"]       = &PonscripterLabel::haeleth_char_setCommand;
    dict["play"]             = &PonscripterLabel::playCommand;
    dict["playonce"]         = &PonscripterLabel::playCommand;
    dict["playstop"]         = &PonscripterLabel::playstopCommand;
    dict["plocate"]          = &PonscripterLabel::locateCommand;
    dict["pligate"]          = &PonscripterLabel::haeleth_ligate_controlCommand;
    dict["pmapfont"]         = &PonscripterLabel::haeleth_map_fontCommand;
    dict["prendering"]       = &PonscripterLabel::haeleth_hinting_modeCommand;
    dict["print"]            = &PonscripterLabel::printCommand;
    dict["prnum"]            = &PonscripterLabel::prnumCommand;
    dict["prnumclear"]       = &PonscripterLabel::prnumclearCommand;
    dict["pspeedpercent"]    = &PonscripterLabel::haeleth_speedpercentCommand;
    dict["pusewindow"]       = &PonscripterLabel::haeleth_usewindowCommand;
    dict["pusewindow3"]      = &PonscripterLabel::haeleth_usewindowCommand;
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
    dict["shell"]          = &PonscripterLabel::shellCommand;
    dict["sp_rgb_gradation"] = &PonscripterLabel::sp_rgb_gradationCommand;
    dict["spbtn"]            = &PonscripterLabel::spbtnCommand;
    dict["spclclk"]          = &PonscripterLabel::spclclkCommand;
    dict["split"]            = &PonscripterLabel::splitCommand;
    dict["splitstring"]      = &PonscripterLabel::splitCommand;
    dict["spreload"]         = &PonscripterLabel::spreloadCommand;
    dict["spstr"]            = &PonscripterLabel::spstrCommand;
    dict["steamsetachieve"]  = &PonscripterLabel::steamsetachieveCommand;
    dict["stop"]             = &PonscripterLabel::stopCommand;
    dict["strsp"]            = &PonscripterLabel::strspCommand;
    dict["systemcall"]       = &PonscripterLabel::systemcallCommand;
    dict["tablegoto"]        = &PonscripterLabel::tablegotoCommand;
    dict["tablegoto1"]       = &PonscripterLabel::tablegotoCommand;
    dict["debugtablegoto"]   = &PonscripterLabel::tablegotoCommand;
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
    dict["vsp2"]             = &PonscripterLabel::vspCommand;
    dict["vsp_when"]         = &PonscripterLabel::vsp_whenCommand;
    dict["vsp2_when"]        = &PonscripterLabel::vsp_whenCommand;
    dict["wait"]             = &PonscripterLabel::waitCommand;
    dict["waittimer"]        = &PonscripterLabel::waittimerCommand;
    dict["wave"]             = &PonscripterLabel::waveCommand;
    dict["waveloop"]         = &PonscripterLabel::waveCommand;
    dict["wavestop"]         = &PonscripterLabel::wavestopCommand;
}

static void SDL_Quit_Wrapper()
{
#ifdef STEAM
  SteamAPI_Shutdown();
#endif
    SDL_Quit();
}

#ifdef STEAM
void PonscripterLabel::initSteam() {
    if(!SteamAPI_Init()) {
      fprintf(stderr, "Unable to initialize steam; cloud and achievements won't work\n");
    }
}
#endif

void PonscripterLabel::initSDL()
{
    /* ---------------------------------------- */
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(-1);
    }
    atexit(SDL_Quit_Wrapper); // work-around for OS/2

#ifdef ENABLE_JOYSTICK
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0 && SDL_GameControllerOpen(0) != 0)
        printf("Initialize GAMECONTROLLER\n");
    else
        fprintf(stderr, "Couldn't initialize SDL gamecontroller: %s\n", SDL_GetError());

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0 && SDL_JoystickOpen(0) != 0)
        printf("Initialize JOYSTICK\n");
#endif

#if defined (PSP) || defined (IPODLINUX)
    SDL_ShowCursor(SDL_DISABLE);
#endif

    /* ---------------------------------------- */
    /* Initialize SDL */

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

    /*screen_surface = SDL_SetVideoMode(screen_width, screen_height, screen_bpp,
        DEFAULT_VIDEO_SURFACE_FLAG | (fullscreen_mode ? fullscreen_flags : 0));*/


    wm_title_string = DEFAULT_WM_TITLE;
    wm_icon_string = DEFAULT_WM_ICON;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    screen = SDL_CreateWindow(wm_title_string,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen_width, screen_height,
        (fullscreen_mode ? fullscreen_flags : 0) | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_PRESENTVSYNC);
    if(renderer == NULL) {
      fprintf(stderr, "Couldn't create SDL renderer: %s\n", SDL_GetError());
      exit(-1);
    }


    SDL_RenderSetLogicalSize(renderer, screen_width, screen_height);


    screen_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    if(screen_tex == 0) {
      fprintf(stderr, "Couldn't create SDL texture: %s\n", SDL_GetError());
      exit(-1);
    }

    SDL_SetTextureBlendMode(screen_tex, SDL_BLENDMODE_NONE);
    if(screen_tex == 0) {
      fprintf(stderr, "Couldn't set SDL texture blend mode: %s\n", SDL_GetError());
      exit(-1);
    }

    /* ---------------------------------------- */
    /* Check if VGA screen is available. */
#if defined (PDA) && (PDA_WIDTH == 640)
    if (screen_surface == 0) {
        screen_ratio1 /= 2;
        screen_width  /= 2;
        screen_height /= 2;
        //screen_surface = SDL_SetVideoMode(screen_width,screen_height,screen_bpp,
        //    DEFAULT_VIDEO_SURFACE_FLAG | (fullscreen_mode? fullscreen_flags : 0));
    }
#endif
    underline_value = screen_height - 1;

    //if (screen_surface == 0) {
    //    fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
    //            screen_width, screen_height, screen_bpp, SDL_GetError());
    //    exit(-1);
    //}

    /* ---------------------------------------- */
    /* Set the icon */
    SDL_Surface* icon = IMG_Load("icon.png");
    //use icon.png preferably, but try embedded resources if not found
    //(cmd-line option --use-app-icons to prefer app resources over icon.png)
    //(Mac apps can set use-app-icons in a pns.cfg file within the
    //bundle, to have it always use the bundle icns)
#ifndef MACOSX
    if (!icon || use_app_icons) {
        const InternalResource* internal_icon = getResource("icon.png");
        if (internal_icon) {
            if (icon) SDL_FreeSurface(icon);
            SDL_RWops* rwicon = SDL_RWFromConstMem(internal_icon->buffer,
                                                   internal_icon->size);
            icon = IMG_Load_RW(rwicon, 0);
            use_app_icons = false;
        }
    }
#endif //!MACOSX
    // If an icon was found (and desired), use it.
    if (icon && !use_app_icons) {
#if defined(MACOSX) || defined(WIN32)
#if defined(MACOSX)
        //resize the (usually 32x32) icon to 128x128
        SDL_Surface *tmp2 = SDL_CreateRGBSurface(0, 128, 128,
                                                 32, 0x00ff0000, 0x0000ff00,
                                                 0x000000ff, 0xff000000);
#elif defined(WIN32)
        //resize the icon to 32x32
        SDL_Surface *tmp2 = SDL_CreateRGBSurface(0, 32, 32,
                                                 32, 0x00ff0000, 0x0000ff00,
                                                 0x000000ff, 0xff000000);
#endif //MACOSX, WIN32
        SDL_Surface *tmp = SDL_ConvertSurface( icon, tmp2->format, SDL_SWSURFACE );
        if ((tmp->w == tmp2->w) && (tmp->h == tmp2->h)) {
            //already the right size, just use converted surface as-is
            SDL_FreeSurface(tmp2);
            tmp2 = icon;
            icon = tmp;
            SDL_FreeSurface(tmp2);
        } else {
            //resize converted surface
            AnimationInfo::resizeSurface(tmp, tmp2);
            SDL_FreeSurface(tmp);
            tmp = icon;
            icon = tmp2;
            SDL_FreeSurface(tmp);
        }
#endif //MACOSX || WIN32
        SDL_SetWindowIcon(screen, icon);
    }
    if (icon)
        SDL_FreeSurface(icon);



    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    openAudio();
}


void PonscripterLabel::openAudio(int freq, Uint16 format, int channels)
{
    if (Mix_OpenAudio(freq, format, channels, DEFAULT_AUDIOBUF) < 0) {
        fprintf(stderr, "Couldn't open audio device!\n"
                        "  reason: [%s].\n", SDL_GetError());
        audio_open_flag = false;
    }
    else {
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
      sin_table(NULL), cos_table(NULL), whirl_table(NULL),
      breakup_cells(NULL), breakup_cellforms(NULL), breakup_mask(NULL),
      music_cmd(getenv("PLAYER_CMD")),
      midi_cmd(getenv("MUSIC_CMD"))
{
#if defined (USE_X86_GFX) && !defined(MACOSX)
    // determine what functions the cpu supports (Mion)
    {
        unsigned int func, eax, ebx, ecx, edx;
        func = AnimationInfo::CPUF_NONE;
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) != 0) {
            printf("System info: Intel CPU, with functions: ");
            if (edx & bit_MMX) {
                func |= AnimationInfo::CPUF_X86_MMX;
                printf("MMX ");
            }
            if (edx & bit_SSE) {
                func |= AnimationInfo::CPUF_X86_SSE;
                printf("SSE ");
            }
            if (edx & bit_SSE2) {
                func |= AnimationInfo::CPUF_X86_SSE2;
                printf("SSE2 ");
            }
            printf("\n");
        }
        AnimationInfo::setCpufuncs(func);
    }
#elif defined(USE_PPC_GFX) && defined(MACOSX)
    // Determine if this PPC CPU supports AltiVec (Roto)
    {
        unsigned int func = AnimationInfo::CPUF_NONE;
        int altivec_present = 0;

        size_t length = sizeof(altivec_present);
        int error = sysctlbyname("hw.optional.altivec", &altivec_present, &length, NULL, 0);
        if(error) {
            AnimationInfo::setCpufuncs(AnimationInfo::CPUF_NONE);
            return;
        }
        if(altivec_present) {
            func |= AnimationInfo::CPUF_PPC_ALTIVEC;
            printf("System info: PowerPC CPU, supports altivec\n");
        } else {
            printf("System info: PowerPC CPU, DOES NOT support altivec\n");
        }
        AnimationInfo::setCpufuncs(func);
    }
#else
    AnimationInfo::setCpufuncs(AnimationInfo::CPUF_NONE);
#endif

    disable_rescale_flag = false;
    edit_flag            = false;
    fullscreen_mode      = false;
    minimized_flag       = false;
    fullscreen_flags     = SDL_WINDOW_FULLSCREEN_DESKTOP;
    window_mode          = false;
#ifdef WIN32
    current_user_appdata = false;
#endif
    use_app_icons        = false;
    skip_to_wait         = 0;
    sprite_info          = new AnimationInfo[MAX_SPRITE_NUM];
    sprite2_info         = new AnimationInfo[MAX_SPRITE2_NUM];
    enable_wheeldown_advance_flag = false;

    for (int i = 0; i < MAX_SPRITE2_NUM; ++i)
        sprite2_info[i].affine_flag = true;
    global_speed_modifier = 100;
}


PonscripterLabel::~PonscripterLabel()
{
    reset();
    delete[] sprite_info;
    delete[] sprite2_info;
}


void PonscripterLabel::setDebugMode()
{
    ++debug_level;
}


void PonscripterLabel::setRegistryFile(const char* filename)
{
    registry_file = filename;
}


void PonscripterLabel::setDLLFile(const char* filename)
{
    dll_file = filename;
}


void PonscripterLabel::setArchivePath(const pstring& path)
{
    archive_path.clear();
    archive_path.add(path);
    printf("archive_path: %s\n", (const char*)(archive_path.get_all_paths()));
}


void PonscripterLabel::setSavePath(const pstring& path)
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


#ifdef WIN32
void PonscripterLabel::setUserAppData()
{
    current_user_appdata = true;
}
#endif


void PonscripterLabel::setUseAppIcons()
{
    use_app_icons = true;
}


void PonscripterLabel::setPreferredWidth(const char *widthstr)
{
    int width = atoi(widthstr);
    //minimum preferred window width of 160 (gets ridiculous if smaller)
    if (width > 160)
        preferred_width = width;
    else if (width > 0)
        preferred_width = 160;
}


void PonscripterLabel::enableButtonShortCut()
{
    force_button_shortcut_flag = true;
}


void PonscripterLabel::enableWheelDownAdvance()
{
    enable_wheeldown_advance_flag = true;
}


void PonscripterLabel::disableCpuGfx()
{
    AnimationInfo::setCpufuncs(AnimationInfo::CPUF_NONE);
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


void PonscripterLabel::setGameIdentifier(const char *gameid)
{
    cmdline_game_id = gameid;
}

#ifdef WIN32
int makeFolder(pstring *path) {
    if(CreateDirectory(*path, NULL) == 0) {
        DWORD err = GetLastError();
        if(err != ERROR_ALREADY_EXISTS) {
          fprintf(stderr, "Warning, unable to create directory: %s\n", path->data);
          return 1;
        }
    }
    return 0;
}
#elif defined(MACOSX)
int makeFolder(pstring *path) {
    using namespace Carbon;
    if (mkdir(*path, 0755) == 0 || errno == EEXIST)
        return 0;
    fprintf(stderr, "Warning, unable to create directory: %s\n", path->data);
    return 1;
}
#else // Linux
int makeFolder(pstring *path) {
    if (mkdir(*path, 0755) == 0 || errno == EEXIST)
        return 0;
    fprintf(stderr, "Warning, unable to create directory: %s\n", path->data);
    return 1;
}
#endif

/* Local_GetSavePath() is the fallback for if steam is defined,
   but SteamAPI_Init failed, or if the user requests a local save dir */
#ifdef WIN32
pstring Local_GetSavePath()
{
    /* These defines are used elsewhere. They are normally created in the non-steam GetSavePath fn */
#define CSIDL_COMMON_APPDATA 0x0023 // for [Profiles]\All Users\Application Data
#define CSIDL_APPDATA        0x001a // for [Profiles]\[User]\Application Data
    /* Assume the working-dir is where we want our save path
       . We could use GetModuleFileNameW if this is an issue */
    pstring rv = "saves/";
    if(makeFolder(&rv) == 0) {
      return rv;
    }
    return "";
}
#elif defined(MACOSX)
pstring Local_GetSavePath()
{
    pstring rv = "";

    // if we're bundled, return the dir just outside the bundle
    // eg: parent dir will contain:   Ponscripter.app   and   saves/
    using namespace Carbon;
    CFURLRef url;
    const CFIndex max_path = 32768;
    Uint8 path[max_path];
    CFBundleRef bundle = CFBundleGetMainBundle();
    bool is_bundled = false;

    if (bundle) {
        CFURLRef bundleurl = CFBundleCopyBundleURL(bundle);
        if (bundleurl) {
            Boolean validpath =
                CFURLGetFileSystemRepresentation(bundleurl, true,
                                                 path, max_path);
            if (validpath) {
                pstring p_path = pstring((char*) path);
                // this is a really stupid hack
                // unfortunately, I don't think OSX has an isBundled() call, or anything like it,
                // and CFBundleGetMainBundle returns a bundle even if unbundled... super duper smart

                // check if contains ".app/" or ends with ".app" to see if we're inside an app bundle
                is_bundled = (p_path.caselessfind(".app/") > -1) || (p_path.caselessfind(".app") == p_path.length() - 4);

                if (is_bundled) {
                    rv = p_path;
                    rv += "/../";  // create outside bundle, so saves folder appears alongside .app
                }
            }
            CFRelease(bundleurl);
        }
    }

    rv += "saves/";

    if (makeFolder(&rv) == 0)
        return rv;

    // If that fails, die.
    CFOptionFlags *alert_flags;
    CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL,
        CFSTR("Save Directory Failure"),
        CFSTR("Could not create save directory."), NULL, NULL, NULL, alert_flags);
    exit(1);
}
#else // LINUX and hope everything else is linux-like
pstring Local_GetSavePath() // POSIX-ish version
{
    char *programPath = (char *)malloc(PATH_MAX);
    size_t pathLen = PATH_MAX;
    ssize_t readLen = 0;
    bool wholeLinkRead = true;

    /* Because PATH_MAX is not really the max, this bit
       tries to make sure to allocate enough anyways */
    readLen = readlink("/proc/self/exe", programPath, pathLen);
    if(readLen > pathLen) {
        pathLen = readLen + 1;
        programPath = (char *)realloc(programPath, pathLen);
        readLen = readlink("/proc/self/exe", programPath, pathLen);
    }
    if(readLen == -1) {
        fprintf(stderr, "Error getting current program path. Saves might not work\n");
        return "";
    }

    char *programDir = strdup(dirname(programPath));
    free(programPath);

    pstring rv = pstring(programDir) + "/saves/";
    free(programDir);

    if(makeFolder(&rv) == 0)
        return rv;

    return "";
}
#endif //WIN32 / OSX / LINUX

#ifdef STEAM

pstring Steam_GetSavePath() {
  if(SteamApps()) {
      uint32 folderLen = PATH_MAX;
      char *installFolder = (char *)malloc(folderLen + 1);
      folderLen = SteamApps()->GetAppInstallDir(SteamUtils()->GetAppID(),
                installFolder, folderLen);
      if(folderLen > PATH_MAX) {
          installFolder = (char *)realloc((void *)installFolder, folderLen);
          folderLen = SteamApps()->GetAppInstallDir(SteamUtils()->GetAppID(),
                    installFolder, folderLen);
      }
      pstring rv = pstring(installFolder) + "/saves/";

      if(makeFolder(&rv) == 0) {
        return rv;
      }
  }
  fprintf(stderr, "Unable to get steam's save path; falling back to relative save path.\n");
  return Local_GetSavePath();
}

#endif //STEAM

#ifdef WIN32
pstring Platform_GetSavePath(pstring gameid, bool current_user_appdata) // Windows version
{
    //Convert gameid from UTF-8 to Wide (Unicode) and thence to system ANSI
    // (since SDL 1.2 doesn't support Unicode app compilation)
    //Thought: Maybe in the future allow non-ANSI UTF8 chars in save path,
    // by using _wmkdir instead of CreateDirectory?
    // But then the save folder won't open in Explorer in debug mode,
    // since only ShellExecuteA (ANSI) is available to non-Unicode apps

    int len = MultiByteToWideChar(CP_UTF8, 0, (const char*)gameid, -1, NULL, 0);
    wchar_t *u16_tmp = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, (const char*)gameid, -1, u16_tmp, len);
    len = WideCharToMultiByte(CP_ACP, 0, u16_tmp, -1, NULL, 0, NULL, NULL);
    char *cvt = new char[len+1];
    WideCharToMultiByte(CP_ACP, 0, u16_tmp, -1, cvt, len, NULL, NULL);
    pstring ansi_gameid = cvt;
    delete[] u16_tmp; delete[] cvt;
    // Replace troublesome characters for Windows
    replace_ascii(ansi_gameid, '\\', '_');
    replace_ascii(ansi_gameid, '?', '_');
    replace_ascii(ansi_gameid, '/', '_');
    // On Windows, store saves in [Profiles]\All Users\Application Data.
    // Permit saves to be per-user rather than shared if
    // option --current-user-appdata is specified
    HMODULE shdll = LoadLibrary("shfolder");
    pstring rv;
    if (shdll) {
        GETFOLDERPATH gfp =
            GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
        if (gfp) {
            char hpath[MAX_PATH];
#define CSIDL_COMMON_APPDATA 0x0023 // for [Profiles]\All Users\Application Data
#define CSIDL_APPDATA        0x001a // for [Profiles]\[User]\Application Data
            HRESULT res;
            if (current_user_appdata)
                res = gfp(0, CSIDL_APPDATA, 0, 0, hpath);
            else
                res = gfp(0, CSIDL_COMMON_APPDATA, 0, 0, hpath);
            if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
                rv = pstring(hpath) + DELIMITER + ansi_gameid + DELIMITER;
                makeFolder(&rv);
            }
        }
        FreeLibrary(shdll);
    }
    return rv;
}
#elif defined MACOSX
pstring Platform_GetSavePath(pstring gameid) // MacOS X version
{
    // On Mac OS X, place in ~/Library/Application Support/<gameid>/
    replace_ascii(gameid, '/', '_');
    using namespace Carbon;
    FSRef appsupport;
    FSFindFolder(kUserDomain, kApplicationSupportFolderType, kDontCreateFolder,
                 &appsupport);
    char path[32768];
    FSRefMakePath(&appsupport, (UInt8*) path, 32768);
    pstring rv = pstring(path) + DELIMITER + gameid + DELIMITER;
    if (makeFolder(&rv) == 0)
        return rv;
    // If that fails, die.
    CFOptionFlags *alert_flags;
    PonscripterMessage(Error, "Save Directory Failure", "Could not create save directory.");
    exit(1);
}
#elif defined LINUX
pstring Platform_GetSavePath(pstring gameid) // POSIX version
{
    if (!gameid) {
        fprintf(stderr, "No gameid\n");
        exit(1);
    }

    // On Linux (and other POSIX-a-likes), place in ~/.gameid
    replace_ascii(gameid, ' ', '_');
    replace_ascii(gameid, '/', '_');
    replace_ascii(gameid, '(', '_');
    replace_ascii(gameid, ')', '_');
    replace_ascii(gameid, '[', '_');
    replace_ascii(gameid, ']', '_');
    passwd* pwd = getpwuid(getuid());
    if (pwd) {
        pstring rv = pstring(pwd->pw_dir) + "/." + gameid + '/';
        if (makeFolder(&rv) == 0)
            return rv;
    }
    // Error; either getpwuid failed, or we couldn't create a save
    // directory.  Either way, issue a warning and then give up.
    fprintf(stderr, "Warning: could not create save directory ~/.%s.\n",
            (const char*) gameid);
    return "";
}
#else
// Stub for unknown platforms
pstring Platform_GetSavePath(const pstring& gameid)
{
    return "";
}
#endif

pstring PonscripterLabel::getSavePath(const pstring gameid) {
#ifdef STEAM
    return Steam_GetSavePath();
#else
    #ifdef LOCAL_SAVEDIR
        return Local_GetSavePath();
    #else
        #ifdef WIN32
        return Platform_GetSavePath(gameid, current_user_appdata);
        #else
        return Platform_GetSavePath(gameid);
        #endif
    #endif
#endif // STEAM
}

// Retrieve a game identifier.
pstring getGameId(ScriptHandler& script_h)
{
    // Ideally, this will have been supplied with a ;gameid directive.
    if (script_h.game_identifier)
        return script_h.game_identifier;

    // If it wasn't, first we try to find the game's name as given in a
    // versionstr or caption command.
    ScriptHandler::LabelInfo define = script_h.lookupLabel("define");
    pstring caption, versionstr;
    script_h.pushCurrent(define.start_address);
    while (script_h.getLineByAddress(script_h.getCurrent())
           < define.num_of_lines)
    {
        pstring t = script_h.readToken(true);
        t.trim();
        if (t == "caption") {
            caption = script_h.readStrValue();
        }
        else if (t == "versionstr") {
            versionstr = script_h.readStrValue();
        }
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
        return zentohan(caption.length() <= versionstr.length()
                        ? caption
                        : versionstr);
    }

    // The fallback position is to generate a semi-unique ID using the
    // length of the script file as a cheap hash.
    caption.format("Ponscripter-%x", script_h.getScriptBufferLength());
    return caption;
}

int PonscripterLabel::init(const char* preferred_script)
{
#ifdef STEAM
    initSteam();
#endif

    // On Mac OS X, archives may be stored in the application bundle.
    // On other platforms the location will either be in the EXE
    // directory, the current directory, or somewhere unpredictable
    // that must be specified with the -r option; in all such cases we
    // assume the current directory if nothing else was specified.
    if (archive_path.get_num_paths()==0) {
#ifdef MACOSX
        // Store archives etc in the application bundle by default, but
        // also check the application root directory and current directory.
        if (isBundled()) {
            pstring path = bundleResPath();
            if (path) {
                archive_path.add(path);
            }

            // Now add the application path.
            path = bundleAppPath();
            if (path) {
                archive_path.add(path);
                // Add the next directory up as a fallback.
                path += "/..";
                archive_path.add(path);
            } else {
                //if we couldn't find the application path, we still need
                //something - use current dir and parent
                archive_path.add(".");
                archive_path.add("..");
            }
        }
        else {
            // Not in a bundle: just use current dir and parent as normal.
            archive_path.add(".");
            archive_path.add("..");
        }
#else
        archive_path.add(".");
        archive_path.add("..");
#endif
    }

    if (key_exe_file) {
        createKeyTable(key_exe_file);
        script_h.setKeyTable(key_table);
    }

    if (open(preferred_script)) return -1;

    // Try to determine an appropriate location for saved games.
    if (!script_h.save_path)
        script_h.save_path = getSavePath(getGameId(script_h));

    // If we couldn't find anything obvious, fall back on ONScripter
    // behaviour of putting saved games in the archive path.
    if (!script_h.save_path) script_h.save_path = archive_path.get_path(0);

#ifdef WIN32
    if (debug_level > 0) {
        // to make it easier to debug user issues on Windows, open
        // the current directory, save_path and Ponscripter output folders
        // in Explorer
        HMODULE shdll = LoadLibrary("shell32");
        if (shdll) {
            char hpath[MAX_PATH];
            bool havefp = false;
            GETFOLDERPATH gfp =
                GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
            if (gfp) {
                HRESULT res = gfp(0, CSIDL_APPDATA, 0, 0, hpath); //user-based
                if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
                    havefp = true;
                    sprintf((char *)&hpath + strlen(hpath), "%s%s",
                            DELIMITER, "Ponscripter");
                }
            }
            typedef HINSTANCE (WINAPI *SHELLEXECUTE)(HWND, LPCSTR, LPCSTR,
                               LPCSTR, LPCSTR, int);
            SHELLEXECUTE shexec =
                SHELLEXECUTE(GetProcAddress(shdll, "ShellExecuteA"));
            if (shexec) {
                shexec(NULL, "open", "", NULL, NULL, SW_SHOWNORMAL);
                shexec(NULL, "open", (const char *)script_h.save_path,
                       NULL, NULL, SW_SHOWNORMAL);
                if (havefp)
                    shexec(NULL, "open", hpath, NULL, NULL, SW_SHOWNORMAL);
            }
            FreeLibrary(shdll);
        }
    }
#endif //WIN32

    initSDL();
    initLocale();

    image_surface = SDL_CreateRGBSurface(0, 1, 1, 32, 0x00ff0000,
                        0x0000ff00, 0x000000ff, 0xff000000);

    screen_surface = SDL_CreateRGBSurface(0, screen_width, screen_height, 32, 0x00ff0000,
                        0x0000ff00, 0x000000ff, 0xff000000);


    accumulation_surface =
        AnimationInfo::allocSurface(screen_width, screen_height);
    backup_surface =
        AnimationInfo::allocSurface(screen_width, screen_height);
    effect_src_surface =
        AnimationInfo::allocSurface(screen_width, screen_height);
    effect_dst_surface =
        AnimationInfo::allocSurface(screen_width, screen_height);
    effect_tmp_surface =
        AnimationInfo::allocSurface(screen_width, screen_height);
    SDL_SetSurfaceAlphaMod(accumulation_surface, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceAlphaMod(backup_surface, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceAlphaMod(effect_src_surface, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceAlphaMod(effect_dst_surface, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceAlphaMod(effect_tmp_surface, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceAlphaMod(screen_surface, SDL_ALPHA_OPAQUE);

    SDL_SetSurfaceBlendMode(accumulation_surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(screen_surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(backup_surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(effect_src_surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(effect_dst_surface, SDL_BLENDMODE_NONE);
    SDL_SetSurfaceBlendMode(effect_tmp_surface, SDL_BLENDMODE_NONE);

    screenshot_surface = 0;
    text_info.num_of_cells = 1;
    text_info.allocImage(screen_width, screen_height);
    text_info.fill(0, 0, 0, 0);

    // ----------------------------------------
    // Sound related variables
    wave_file_name.trunc(0);
    midi_file_name.trunc(0);
    midi_info  = 0;
    mp3_sample = 0;
    music_file_name.trunc(0);
    music_buffer = 0;
    music_buffer_length = 0;
    music_info = 0;
    music_struct.ovi = 0;
    music_struct.is_mute = false;
    music_struct.voice_sample = 0;

    loop_bgm_name[0].trunc(0);
    loop_bgm_name[1].trunc(0);

    int i;
    for (i = 0; i < ONS_MIX_CHANNELS + ONS_MIX_EXTRA_CHANNELS;
         i++) wave_sample[i] = 0;

    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    trap_dist.trunc(0);

    draw_one_page_flag = false;

    for (i = 0; i < MAX_PARAM_NUM; i++)
        bar_info[i] = prnum_info[i] = 0;

    defineresetCommand("definereset");
    readToken();

    loadEnvData();

    InitialiseFontSystem(&archive_path);

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

    if (sin_table) delete[] sin_table;
    if (cos_table) delete[] cos_table;
    sin_table = cos_table = NULL;
    if (whirl_table) delete[] whirl_table;
    whirl_table = NULL;

    if (breakup_cells) delete[] breakup_cells;
    if (breakup_mask) delete[] breakup_mask;
    if (breakup_cellforms) delete[] breakup_cellforms;

    disableGetButtonFlag();

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    key_pressed_flag = false;
    shift_pressed_status = 0;
    ctrl_pressed_status  = 0;
    display_mode = NORMAL_DISPLAY_MODE;
    event_mode = IDLE_EVENT_MODE;
    did_leavetext = false;
    all_sprite_hide_flag = false;
    all_sprite2_hide_flag = false;

    current_over_button = 0;
    variable_edit_mode  = NOT_EDIT_MODE;

    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = false;

    resetSentenceFont();

    getret_str.trunc(0);
    getret_int = 0;

    // ----------------------------------------
    // Sound related variables

    wave_play_loop_flag  = false;
    midi_play_loop_flag  = false;
    music_play_loop_flag = false;
    mp3save_flag = false;

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
        script_h.getVariableData(i).reset(false);

    for (i = 0; i < 3; i++) human_order[i] = 2 - i; // "rcl"

    refresh_shadow_text_mode = REFRESH_NORMAL_MODE | REFRESH_SHADOW_MODE |
                               REFRESH_TEXT_MODE;
    erase_text_window_mode = 1;
    skip_flag      = false;
    monocro_flag   = false;
    nega_mode      = 0;
    clickstr_state = CLICK_NONE;
    trap_mode      = TRAP_NONE;
    trap_dist.trunc(0);

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
    stopAllDWAVE();
    loop_bgm_name[1].trunc(0);

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    bg_info.file_name = "black";
    createBackground();
    for (i = 0; i < 3; i++) tachi_info[i].reset();
    for (i = 0; i < MAX_SPRITE_NUM; i++) sprite_info[i].reset();
    for (i = 0; i < MAX_SPRITE2_NUM; i++) sprite2_info[i].reset();
    barclearCommand("barclear");
    prnumclearCommand("prnumclear");
    for (i = 0; i < 2; i++) cursor_info[i].reset();
    for (i = 0; i < 4; i++) lookback_info[i].reset();

    // Initialize character sets
    DefaultLigatures(9);
    indent_chars.clear(); //Mion: removing default indent chars
    break_chars.clear();
    break_chars.insert(0x0020); //Mion: removing default break chars except space
    //Mion: use "pindentstr basic" or "pbreakstr basic" to activate defaults

    dirty_rect.fill(screen_width, screen_height);
}


void PonscripterLabel::resetSentenceFont()
{
    Fontinfo::default_encoding = Default;
    sentence_font.reset();
    sentence_font.top_x = 21;
    sentence_font.top_y = 16;
    sentence_font.pitch_x = 0;
    sentence_font.pitch_y = 2;
    sentence_font.area_x = 23 * (sentence_font.size() + sentence_font.pitch_x);
    sentence_font.area_y = 16 * (sentence_font.size() + sentence_font.pitch_y);
    sentence_font.wait_time = 20;
    sentence_font.window_color.set(0x99);
    sentence_font_info.reset();
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width;
    sentence_font_info.pos.h = screen_height;
}

void PonscripterLabel::rerender() {
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, screen_tex, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void PonscripterLabel::flush(int refresh_mode, SDL_Rect* rect, bool clear_dirty_flag,
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
                for (int i = 0; i < dirty_rect.num_history; i++) {
                    flushDirect(dirty_rect.history[i], refresh_mode, false);
                }

                flushDirect(*rect, refresh_mode);
            }
        }
    }

    if (clear_dirty_flag) dirty_rect.clear();
}


void PonscripterLabel::flushDirect(SDL_Rect &rect, int refresh_mode, bool updaterect)
{
  refreshSurface(accumulation_surface, &rect, refresh_mode);

  if(!updaterect) return;
  SDL_BlitSurface(accumulation_surface, &rect, screen_surface, &rect);

  if(SDL_UpdateTexture(screen_tex, NULL, screen_surface->pixels, screen_surface->pitch)) {
    fprintf(stderr,"Error updating texture: %s\n", SDL_GetError());
  }
}


void PonscripterLabel::mouseOverCheck(int x, int y)
{
    size_t c = 0;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    bool have_buttons = false;

    // We seek buttons in reverse order in order to preserve an
    // NScripter behaviour: if buttons overlap, it uses whichever was
    // defined last.  ONScripter handles this by traversing the entire
    // list every time, but traversing backwards lets us shortcircuit.
    for (ButtonElt::reverse_iterator it = buttons.rbegin();
         it != buttons.rend(); ++it, ++c) {
        const SDL_Rect& r = it->second.select_rect;
        have_buttons = true;
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            button = it->first;
            break;
        }
    }

    if (have_buttons && (current_over_button != button)) {
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = { 0, 0, 0, 0 };
        SDL_Rect check_dst_rect = { 0, 0, 0, 0 };
        if (current_over_button != 0) {
            ButtonElt& curr_btn = buttons[current_over_button];

            curr_btn.show_flag = 0;
            check_src_rect = curr_btn.image_rect;
            if (curr_btn.isSprite()) {
                sprite_info[curr_btn.sprite_no].visible(true);
                sprite_info[curr_btn.sprite_no].setCell(0);
            }
            else if (curr_btn.isTmpSprite()) {
                curr_btn.show_flag = 1;
                curr_btn.anim[0]->visible(true);
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
            if (system_menu_mode != SYSTEM_NULL) {
                if (menuselectvoice_file_name[MENUSELECTVOICE_OVER].length() > 0)
                    playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
                              SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            else {
                if (selectvoice_file_name[SELECTVOICE_OVER].length() > 0)
                    playSound(selectvoice_file_name[SELECTVOICE_OVER],
                              SOUND_WAVE | SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }

            ButtonElt& new_btn = buttons[button];

            check_dst_rect = new_btn.image_rect;
            if (new_btn.isSprite()) {
                sprite_info[new_btn.sprite_no].setCell(1);
                sprite_info[new_btn.sprite_no].visible(true);
                if (new_btn.button_type == ButtonElt::EX_SPRITE_BUTTON) {
                    decodeExbtnControl(new_btn.exbtn_ctl,
                                       &check_src_rect, &check_dst_rect);
                }
            }
            else if (new_btn.isTmpSprite()) {
                new_btn.show_flag = 1;
                new_btn.anim[0]->visible(true);
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

    // Protect against infinite loops (these should only occur when
    // there's a bug in Ponscripter, but as I've just found such a
    // bug, it's clearly not unthinkable!)
    //    Mion: a bug like not initializing loops to zero?
    int loops = 0;
    const char* last_pointer = NULL;
    int last_offset = 0;

    while (current_line < current_label_info.num_of_lines) {

        if (debug_level > 0) {
            // Protect against infinite loops
            if ((script_h.getCurrent() == last_pointer) &&
                (last_offset == string_buffer_offset)) {
                ++loops;
                if (loops == 32)
                    errorAndExit("Likely infinite loop detected.");
            }
            else {
                loops = 0;
                last_pointer = script_h.getCurrent();
                last_offset = string_buffer_offset;
            }

            pstring cmd = script_h.getStrBuf();
            cmd.trunc(32);
            for (int i = 0; i < cmd.length(); ++i)
                if (cmd[i] == ' ' || cmd[i] == '\n') { cmd.trunc(i); break; }
            printf("*****  executeLabel %s:%d/%d:%d:%d [%s] *****\n",
                   (const char*) current_label_info.name,
                   current_line,
                   current_label_info.num_of_lines,
                   string_buffer_offset, display_mode,
                   (const char*) cmd);
            fflush(stdout);
        }

        if (script_h.readStrBuf(0) == '~') {
            last_tilde = script_h.getNext();
            readToken();
            continue;
        }
        if (break_flag && script_h.getStrBuf() != "next") {
            if (script_h.readStrBuf(string_buffer_offset) == 0x0a)
                current_line++;

            if (script_h.readStrBuf(string_buffer_offset) != ':'
                && script_h.readStrBuf(string_buffer_offset) != ';'
                && script_h.readStrBuf(string_buffer_offset) != 0x0a)
                script_h.skipToken();

            readToken();
            continue;
        }

        if (kidokuskip_flag && skip_flag && kidokumode_flag
            && !script_h.isKidoku())
            setSkipMode(false);

        const char* current = script_h.getCurrent();
        int ret = ScriptParser::parseLine();
        if (ret == RET_NOMATCH) ret = this->parseLine();

        if (ret & RET_SKIP_LINE) {
            script_h.skipLine();
            if (++current_line >= current_label_info.num_of_lines) break;
        }

        if (ret & RET_REREAD) script_h.setCurrent(current);

        if (!(ret & RET_NOREAD)) {
            if (script_h.readStrBuf(string_buffer_offset) == 0x0a) {
                if (skip_to_wait)
                    flush(refreshMode());
                skip_to_wait = 0;
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

    // FIXME: this may produce unexpected results if the character it
    // looks at was part of a ligature (can this happen? Does it
    // matter?)
    if (string_buffer_offset < 5) return false;
    const char* c = script_h.getStrBuf();
    c = file_encoding->Previous(c + string_buffer_offset, c);
    const wchar p = file_encoding->DecodeChar(c);
    return p == '.' || p == 0xff0e || p == ',' || p == 0xff0c
        || p == ':' || p == 0xff1a || p == ';' || p == 0xff1b
        || p == '!' || p == 0xff01 || p == '?' || p == 0xff1f;
}

int PonscripterLabel::parseLine()
{
    int ret = 0;
    pstring cmd = script_h.getStrBuf();
    bool is_orig_cmd = false;
    if (cmd[0] == '_') {
        cmd.remove(0, 1);
        is_orig_cmd = true;
    }

    if (!script_h.isText()) {

        if (cmd[0] == 0x0a)
            return RET_CONTINUE;
        else if (cmd[0] == 'v' && cmd[1] >= '0' && cmd[1] <= '9')
            return vCommand(cmd);
        else if (cmd[0] == 'd' && cmd[1] == 'v' && cmd[2] >= '0' &&
                 cmd[2] <= '9')
            return dvCommand(cmd);

        PonscrFun f = func_lut.get(cmd);
        if (f) {
            if (is_orig_cmd && (debug_level > 0)) {
                printf("** executing builtin command '%s' **\n",
                       (const char*) cmd);
                fflush(stdout);
            }
            return (this->*f)(cmd);
        }

        errorAndCont("unknown command [" + cmd + "]");

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text */
    if (current_mode == DEFINE_MODE)
        errorAndExit("text cannot be displayed in define section.");

//--------INDENT ROUTINE--------------------------------------------------------
    if (sentence_font.GetXOffset() == 0 && sentence_font.GetYOffset() == 0) {
        const wchar first_ch = file_encoding->DecodeWithLigatures
            (script_h.getStrBuf(string_buffer_offset), sentence_font);
        if (is_indent_char(first_ch))
            sentence_font.SetIndent(first_ch);
        else
            sentence_font.ClearIndent();
    }
//--------END INDENT ROUTINE----------------------------------------------------

    ret = textCommand();

//--------LINE BREAKING ROUTINE-------------------------------------------------

    int lf;
    Fontinfo f = sentence_font;
    const wchar first_ch =
        file_encoding->DecodeWithLigatures(script_h.getStrBuf(string_buffer_offset),
                                      f, lf);

    if (is_break_char(first_ch) && !new_line_skip_flag) {
        int l;
        const char* it = script_h.getStrBuf(string_buffer_offset) + lf;
        wchar next_ch = file_encoding->DecodeWithLigatures(it, f, l);
        float len = f.GlyphAdvance(first_ch, next_ch);
        while (1) {
            // For each character (not char!) before a break is found,
            // get unicode.
            wchar ch = file_encoding->DecodeWithLigatures(it, f, l);
      cont: it += l;

	    // Check for special sequences.
	    if (ch >= 0x10 && ch < 0x20) {
		f.processCode(it - l);
		continue;
	    }

            // Check for token breaks.
            if (!ch || ch == '\n' || ch == '@' || ch == '\\' || is_break_char(ch))
                break;

            // Look for an inline command.
            if (ch == '!' && (*it == 's' || *it == 'd' || *it == 'w')) {
                // !sd
                if (it[0] == 's' && it[1] == 'd') {
                    it += 2;
                    continue;
                }
                // ![sdw]<int>
                do { ++it; } while (script_h.isadigit(*it));
                continue;
            }
            else if (ch == '#') {
                //#rrggbb: check all figures are in order
                bool ok = true;
                for (int offs = 1; ok && offs <= 6; ++offs)
                    ok &= script_h.isaxdigit(it[offs]);
                if (ok) {
                    it += 7; // really 7? or should it be 6?
                    continue;
                }
            }

            // No inline command?  Use the glyph metrics, then!
            next_ch = file_encoding->DecodeWithLigatures(it, f, l);
            len += f.GlyphAdvance(ch, next_ch);
            ch = next_ch;
            goto cont;
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
            // Currently we use four em widths, which is an arbitrary
            // figure that may need tweaking.
            const float minlen = f.em_width() * 4;
            if (len < minlen) len = minlen;
        }
        if (len > 0 && f.isNoRoomFor(len)) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
            f.newLine();
        }
    }
//-----END LINE BREAKING ROUTINE------------------------------------------------

    if (script_h.readStrBuf(string_buffer_offset) == 0x0a) {
        ret = RET_CONTINUE; // suppress RET_CONTINUE | RET_NOREAD
        if (!sentence_font.isLineEmpty() && !new_line_skip_flag) {
            current_text_buffer->addBuffer(0x0a);
            sentence_font.newLine();
        }
        //event_mode = IDLE_EVENT_MODE;

        // haeleth 20081214.
        // Fixing infinite loop on line matching /^\^$/ with pretextgosub
        //
        // Before, we always reset line_enter_status to 0.  Now we
        // only do so if it was > 1, i.e. if something has been printed.
        // This prevents an infinite loop where the same line is visited
        // again and again with line_enter_status == 0.
        if (line_enter_status > 1)
            line_enter_status = 0;
    }

    if ((ret & RET_CONTINUE) && (ret & RET_WAIT)) {
        // Mion: handle case of text output while in skip mode
        //(this is hacky, but necessary until I can do a code overhaul)
        ret &= ~RET_WAIT;
        string_buffer_offset += lf;
    }

    return ret;
}


/* ---------------------------------------- */
void PonscripterLabel::refreshMouseOverButton()
{
    current_over_button = 0;
    mouseOverCheck(current_button_state.x, current_button_state.y);
}

void PonscripterLabel::warpMouse(int x, int y) {
  /* Convert logical to window coordinates
     since SDL_WarpMouse takes real coordinates,
     but practically everything else uses logical */
  float scale_x, scale_y;
  SDL_Rect viewport;
  SDL_RenderGetViewport(renderer, &viewport);
  SDL_RenderGetScale(renderer, &scale_x, &scale_y);

  x += viewport.x;
  y += viewport.y;

  x = x * scale_x;
  y = y * scale_y;

  SDL_WarpMouseInWindow(screen, x, y);

  last_mouse_x = x;
  last_mouse_y = y;
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
        SDL_Rect rect = { 0, 0, (uint16_t)screen_width, (uint16_t)screen_height };
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
//TextBuffer_dumpstate();
}


PonscripterLabel::ButtonElt
PonscripterLabel::getSelectableSentence(const pstring& buffer, Fontinfo* info,
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
    anim->visible(true);

    setupAnimationInfo(anim, info);
    rv.select_rect = rv.image_rect = anim->pos;

    info->newLine();
    info->SetXY(current_x);

    dirty_rect.add(rv.image_rect);

    return rv;
}


void
PonscripterLabel::decodeExbtnControl(const pstring& ctl_string,
                                     SDL_Rect* check_src_rect,
                                     SDL_Rect* check_dst_rect)
{
    const char* ctl_str = ctl_string;
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
            if (sprite_info[sprite_no].visible(true))
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
        cursor_info[no].visible(true);
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
    volume_on_flag      = true;
    text_speed_no       = 1;
    draw_one_page_flag  = false;
    kidokumode_flag     = true;
    use_default_volume = true;
    fullscreen_flags    = SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (loadFileIOBuf("envdata") == 0) {
        use_default_volume = false;
        bool do_fullscreen = false;
        if (readInt() == 1 && window_mode == false)
            do_fullscreen = true;
        if (readInt() == 0)
            volume_on_flag = false;
        text_speed_no = readInt();
        if (readInt() == 1)
            draw_one_page_flag = true;
        default_env_font = readStr();
        if (!default_env_font)
            default_env_font = DEFAULT_ENV_FONT;
        readInt(); // old cdrom drive support
        readStr(); // old cdrom drive name
        voice_volume = DEFAULT_VOLUME - readInt();
        se_volume    = DEFAULT_VOLUME - readInt();
        music_volume = DEFAULT_VOLUME - readInt();
        if (readInt() == 0)
            kidokumode_flag = false;
        readInt();  // 0  //Mion: added from onscripter
        readChar(); // 0
        int dummy = readInt();  // 1000
        if (dummy == 1000)
            dummy = readInt(); //Mion: in case it's an older envdata
        if (dummy == 0x534e4f50) {
            // Ponscripter extras
            fullscreen_flags = readInt();
        }

        if (do_fullscreen) menu_fullCommand("menu_full");
    }
    else {
        default_env_font = DEFAULT_ENV_FONT;
        voice_volume = se_volume = music_volume = DEFAULT_VOLUME;
    }
    // set the volumes of channels
    channelvolumes[0] = voice_volume;
    for ( int i=1 ; i<ONS_MIX_CHANNELS ; i++ )
        channelvolumes[i] = se_volume;
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
        writeInt(0, output_flag); // old cdrom drive enable
        writeStr("", output_flag); // old cdrom drive name
        writeInt(DEFAULT_VOLUME - voice_volume, output_flag);
        writeInt(DEFAULT_VOLUME - se_volume, output_flag);
        writeInt(DEFAULT_VOLUME - music_volume, output_flag);
        writeInt(kidokumode_flag ? 1 : 0, output_flag);
        writeInt(0, output_flag);
        writeChar(0, output_flag); // ?
        writeInt(1000, output_flag);

        // Ponscripter extras
        writeInt(0x534e4f50, output_flag);
        writeInt(fullscreen_flags, output_flag);

        if (i == 1) break;
        allocFileIOBuf();
        output_flag = true;
    }

    saveFileIOBuf("envdata");
}


int PonscripterLabel::refreshMode()
{
    return (display_mode == TEXT_DISPLAY_MODE) ?
           refresh_shadow_text_mode : (int) REFRESH_NORMAL_MODE;
}


void PonscripterLabel::quit()
{
    saveAll();

    if (midi_info) {
        Mix_HaltMusic();
        Mix_FreeMusic(midi_info);
    }
    if (music_info) {
        Mix_HaltMusic();
        Mix_FreeMusic(music_info);
    }
#ifdef STEAM
    SteamAPI_Shutdown();
#endif
}


void PonscripterLabel::disableGetButtonFlag()
{
    btndown_flag     = false;
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

void PonscripterLabel::setAutoMode(bool mode)
{
    if (mode) setSkipMode(false);
    if (mode != automode_flag) {
        automode_flag = mode;
        for (int i = 0; i < MAX_SPRITE_NUM; ++i) {
            switch (sprite_info[i].enablemode) {
            case 1: // enable in automode
                if (sprite_info[i].enabled(mode))
                    dirty_rect.add(sprite_info[i].pos);
                break;
            case 2: // enable outside automode
                if (sprite_info[i].enabled(!mode))
                    dirty_rect.add(sprite_info[i].pos);
                break;
            }
        }
        for (int i = 0; i < MAX_SPRITE2_NUM; ++i) {
            switch (sprite2_info[i].enablemode) {
            case 1: // enable in automode
                if (sprite2_info[i].enabled(mode))
                    dirty_rect.add(sprite2_info[i].bounding_rect);
                break;
            case 2: // enable outside automode
                if (sprite2_info[i].enabled(!mode))
                    dirty_rect.add(sprite2_info[i].bounding_rect);
                break;
            }
        }
        // Haeleth note: this seems to be necessary to cut short the
        // auto-mode timer on exiting auto mode.
        if (!mode) advancePhase(0);
    }
}

void PonscripterLabel::setSkipMode(bool mode)
{
    skip_flag = mode;
}
