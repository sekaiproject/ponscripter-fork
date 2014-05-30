/* -*- C++ -*-
 *
 *  PonscripterMessage.cpp - User message handling
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

#include "PonscripterMessage.h"

// system-specific libraries
#ifdef MACOSX
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#endif

// Displays a message to the user
// If possible, interrupts game (as it means something REALLY BAD happened)
int PonscripterMessage(MessageType message_type, const char* title, const char* message)
{
    // TODO: Linux (X11/etc), Windows

    // OS X
    // Pops up an OS X Cocoa message box
    #ifdef MACOSXX
        CFOptionFlags alert_level;

        switch(message_type) {
            case Error:
                alert_level = kCFUserNotificationStopAlertLevel;
                break;
            case Warning:
                alert_level = kCFUserNotificationCautionAlertLevel;
                break;
            default:
                alert_level = kCFUserNotificationNoteAlertLevel;
                break;
        }

        CFOptionFlags *alert_flags = NULL;

        // convert C strings to CoreFoundation strings for OSX
        CFStringRef cf_title = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
        CFStringRef cf_message = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);

        CFUserNotificationDisplayAlert(0, alert_level, NULL, NULL, NULL,
            cf_title,
            cf_message, NULL, NULL, NULL, alert_flags);

        if(cf_title) CFRelease(cf_title);
        if(cf_message) CFRelease(cf_message);

    // General
    // Prints it to stderr, as best we can
    #else
        char *severity = new char[SEVERITY_BUFFER_LENGTH];

        // choose message type
        switch(message_type) {
            case Error:
                strncpy(severity, "Error", SEVERITY_BUFFER_LENGTH-1);
                break;
            case Warning:
                strncpy(severity, "Warning", SEVERITY_BUFFER_LENGTH-1);
                break;
            default:
                strncpy(severity, "Note", SEVERITY_BUFFER_LENGTH-1);
                break;
        }

        fprintf(stderr, "** %s ** %s\n", title, severity);
        fprintf(stderr, "%s\n\n", message);
        delete[] severity;

    #endif

    // finished OK
    return 0;
}
