/* -*- C++ -*-
 *
 *  Ponscripter.cpp -- main function of Ponscripter
 *
 *  Copyright (c) 2001-2006 Ogapee (original ONScripter, of which this
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
#include <sys/stat.h>
#include "version.h"

static void optionHelp()
{
    printf("Ponscripter version %s (NScr %d.%02d)\n",
        ONS_VERSION, NSC_VERSION / 100, NSC_VERSION % 100);
    printf("Usage: ponscripter [option ...] [root path]\n");
    printf("      --cdaudio\t\tuse CD audio if available\n");
    printf("      --cdnumber no\tchoose the CD-ROM drive number\n");
    printf("      --registry file\tset a registry file\n");
    printf("      --dll file\tset a dll file\n");
    printf("  -r, --root path\tset the root path to the archives\n");
    printf("  -s, --save path\tset the path to use for saved games"
           "(default: platform-dependent)\n");
    printf("      --script path\tset the script filename\n");
    printf("      --fullscreen\tstart in fullscreen mode\n");
    printf("      --window\t\tstart in window mode\n");
    printf("      --force-button-shortcut\tignore useescspc and getenter "
           "command\n");
    printf("      --force-png-alpha\t\talways use PNG alpha channels\n");
    printf("      --force-png-nscmask\talways use NScripter-style masks\n");
    printf("      --enable-wheeldown-advance\tadvance the text on mouse "
           "wheeldown event\n");
//    printf( "     --set-tag-page-origin-to-1\tsyntax option for setting "
//            "'gettaglog' origin to 1 instead of 0\n");
//    printf( "     --answer-dialog-with-yes-ok\thave 'yesnobox' and "
//            "'okcancelbox' give 'yes/ok' result\n");
    printf("  -d, --debug\t\trun in debug mode (repeat for verbosity)\n");
    printf("  -h, --help\t\tshow this help and exit\n");
    printf("  -v, --version\t\tshow the version information and exit\n");
    exit(0);
}


static void optionVersion()
{
    printf("Ponscripter version %s (NScr %d.%02d)\n",
        ONS_VERSION, NSC_VERSION / 100, NSC_VERSION % 100);
    printf("Based on ONScripter by Ogapee <ogapee@aqua.dti2.ne.jp>\n");
    printf("Currently maintained by \"Uncle\" Mion Sonozaki <UncleMion@gmail.com>\n\n");
    printf("Copyright (c) 2001-2010 Ogapee, 2006-2010 insani, Haeleth, Sonozaki et al.\n");
    printf("This is free software; see the source for copying conditions.\n");
    exit(0);
}


#ifdef QWS
int SDL_main(int argc, char** argv)
#elif defined (PSP)
extern "C" int main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    PonscripterLabel ons;
    pstring preferred_script = "";

#ifdef PSP
    ons.disableRescale();
    ons.enableButtonShortCut();
#endif

    // ----------------------------------------
    // Parse options
    argv++;
    while (argc > 1) {
        if (argv[0][0] == '-') {
            if (!strcmp(argv[0] + 1, "d") || !strcmp(argv[0] + 1, "-debug")) {
                ons.setDebugMode();
            }
            if (!strcmp(argv[0] + 1, "h") || !strcmp(argv[0] + 1, "-help")) {
                optionHelp();
            }
            else if (!strcmp(argv[0] + 1, "v") || !strcmp(argv[0] + 1, "-version")) {
                optionVersion();
            }
            else if (!strcmp(argv[0] + 1, "-cdaudio")) {
                ons.enableCDAudio();
            }
            else if (!strcmp(argv[0] + 1, "-cdnumber")) {
                argc--;
                argv++;
                ons.setCDNumber(atoi(argv[0]));
            }
            else if (!strcmp(argv[0] + 1, "-registry")) {
                argc--;
                argv++;
                ons.setRegistryFile(argv[0]);
            }
            else if (!strcmp(argv[0] + 1, "-dll")) {
                argc--;
                argv++;
                ons.setDLLFile(argv[0]);
            }
            else if (!strcmp(argv[0] + 1, "r") || !strcmp(argv[0] + 1, "-root")) {
                argc--;
                argv++;
                ons.setArchivePath(argv[0]);
            }
            else if (!strcmp(argv[0] + 1, "s") || !strcmp(argv[0] + 1, "-save")) {
                argc--;
                argv++;
                ons.setSavePath(argv[0]);
            }
            else if (!strcmp(argv[0] + 1, "-script")) {
                argc--;
                argv++;
                preferred_script = argv[0];
            }
            else if (!strcmp(argv[0] + 1, "-fullscreen")) {
                ons.setFullscreenMode();
            }
            else if (!strcmp(argv[0] + 1, "-window")) {
                ons.setWindowMode();
            }
            else if (!strcmp(argv[0] + 1, "-force-button-shortcut")) {
                ons.enableButtonShortCut();
            }
            else if (!strcmp(argv[0] + 1, "-enable-wheeldown-advance")) {
                ons.enableWheelDownAdvance();
            }
            else if (!strcmp(argv[0] + 1, "-disable-rescale")) {
                ons.disableRescale();
            }
            else if (!strcmp(argv[0] + 1, "-edit")) {
                ons.enableEdit();
            }
            else if (!strcmp(argv[0] + 1, "-key-exe")) {
                argc--;
                argv++;
                ons.setKeyEXE(argv[0]);
            }
	    else if (!strcmp(argv[0] + 1, "-force-png-alpha")) {
		ons.setMaskType(1);
	    }
	    else if (!strcmp(argv[0] + 1, "-force-png-nscmask")) {
		ons.setMaskType(2);
	    }
            else {
                printf(" unknown option %s\n", argv[0]);
            }
        }
        else if (!ons.hasArchivePath() || !preferred_script) {
            // Split parameter appropriately.
            pstring path = argv[0];
            pstring file = "";
            int i = path.reversefind(DELIMITER, path.length());
            if (ons.hasArchivePath()) {
                file = path;
                path = "";
            }
            else {
                // If the item is visible and a directory, take it as
                // the archive path; otherwise assume it must be a
                // script filename.
                struct stat info;
                if (stat(path, &info) == -1 ||
                    !S_ISDIR(info.st_mode))
                {
                    if (i >= 0) {
                        file = path.midstr(i + 1, path.length() - i);
                        path.trunc(i);
                    }
                    else {
                        file = path;
                        path = "";
                    }
                }
            }
                
            if (path) ons.setArchivePath(path);
            if (file) {
                if (preferred_script)
                    optionHelp();
                else
                    preferred_script = file;
            }
        }
        else {
            optionHelp();
        }

        argc--;
        argv++;
    }

    // ----------------------------------------
    // Run Ponscripter

    const char* s = preferred_script;
    if (*s == 0) s = NULL;
    if (ons.init(s)) exit(-1);

    ons.eventLoop();

    exit(0);
}

