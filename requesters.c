/*
 * AmiSoundED - Sound Editor
 * Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <classes/requester.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/requester.h>
#include <proto/datatypes.h>
#include <string.h>

#include "requesters.h"
#include "locale_support.h"
#include "sounded.h"

int32 DoReq (struct Screen *screen, Object *window, Object *req) {
    int32 res = 0;
    if (req) {
        struct Window *win = NULL;

        if (window) {
            GetAttr(WINDOW_Window, window, (uint32 *)&win);
            SetAttrs(window, WA_BusyPointer, TRUE, TAG_END);
        }
        
        res = DoMethod(req, RM_OPENREQ, NULL, win, win ? NULL : screen);

        if (window) {
            SetAttrs(window, WA_BusyPointer, FALSE, TAG_END);
        }

        DisposeObject(req);
    }
    return res;
}

void AboutRequester (struct Screen *screen, Object *window) {
    Object *req;
    uint32 args[3];

    args[0] = (uint32)progname;
    args[1] = VERSION;
    args[2] = REVISION;

    req = RequesterObject,
        REQ_CharSet,    LocaleInfo.CodeSet,
        REQ_TitleText,  progname,
        REQ_BodyText,   GetString(&LocaleInfo, MSG_ABOUT_REQ),
        REQ_VarArgs,    args,
        REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
        TAG_END);

    DoReq(screen, window, req);
}

void ErrorRequester (struct Screen *screen, Object *window, int32 error, const char *filename) {
    Object *req;
    char buffer[256];
    const char *error_string;
    if ((error_string = GetDTString(error)) && error_string[0]) {
        strcpy(buffer, GetString(&LocaleInfo, MSG_ERROR));
        strcat(buffer, ": ");
        SNPrintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
            error_string, filename);
    } else {
        Fault(error, GetString(&LocaleInfo, MSG_ERROR), buffer, sizeof(buffer));
    }
    req = RequesterObject,
        REQ_CharSet,    LocaleInfo.CodeSet,
        REQ_Image,      REQIMAGE_ERROR,
        REQ_TitleText,  progname,
        REQ_BodyText,   buffer,
        REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
        TAG_END);
    DoReq(screen, window, req);
}
