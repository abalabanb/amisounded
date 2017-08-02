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

#include "sounded.h"
#include "project.h"
#include "plugins_private.h"
#include "requesters.h"
#include "debug.h"
#include <gadgets/chooser.h>
#include <proto/chooser.h>

BOOL SaveSound (Project *project, const char *filename) {
    struct Window *window = project->IWindow;
    Object *sound = project->Sound;
    struct LoadPlugin *plugin = project->SavePlugin;
    BPTR file;
    int32 error = OK;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    file = Open(filename, MODE_NEWFILE);
    if (file) {
        error = plugin->SaveSound(project, sound, file);
    } else {
        error = IoErr();
    }
    if (!error) {
        SetAttrs(sound, SOUND_HasChanged, FALSE, TAG_END);
    } else {
        ErrorRequester(project->Screen, project->Window, error, filename);
    }

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
    Project_UpdateGUI(project);
    return !error;
}

enum {
    GID_OK = 1,
    GID_CANCEL
};

struct LoadPlugin *OutputFormatRequester (Project *project) {
    struct Window *main_window = project->IWindow;
    Object *window;
    Object *format;
    struct Window *win;
    struct LoadPlugin *plugin = NULL;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    window = WindowObject,
        WA_PubScreen,       project->Screen,
        WA_Flags,           WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET
                            |WFLG_NOCAREREFRESH|WFLG_ACTIVATE,
        WA_IDCMP,           IDCMP_CLOSEWINDOW|IDCMP_GADGETUP,
        WA_Title,           progname,
        WINDOW_Position,    main_window ? WPOS_CENTERWINDOW : WPOS_CENTERSCREEN,
        WINDOW_RefWindow,   main_window,
        WINDOW_CharSet,     LocaleInfo.CodeSet,
        WINDOW_Layout,      VLayoutObject,
            LAYOUT_AddChild,    HLayoutObject,
                LAYOUT_BevelStyle,      BVS_SBAR_VERT,
                LAYOUT_Label,           GetString(&LocaleInfo, MSG_OUTPUTFORMAT_LBL),
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_AddChild,        format = ChooserObject,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, output_formats,
                End,
            End,
            LAYOUT_AddChild,    HLayoutObject,
                LAYOUT_BevelStyle,      BVS_SBAR_VERT,
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_BottomSpacing,   0,
                LAYOUT_EvenSize,        TRUE,
                LAYOUT_AddChild,        ButtonObject,
                    GA_ID,              GID_OK,
                    GA_RelVerify,       TRUE,
                    GA_Text,            GetString(&LocaleInfo, MSG_OK_GAD),
                End,
                CHILD_WeightedWidth,    0,
                LAYOUT_AddChild,        ButtonObject,
                    GA_ID,              GID_CANCEL,
                    GA_RelVerify,       TRUE,
                    GA_Text,            GetString(&LocaleInfo, MSG_CANCEL_GAD),
                End,
                CHILD_WeightedWidth,    0,
            End,
        End,
    End;

    if (window && (win = RA_OpenWindow(window))) {
        uint32 sigmask;
        uint32 res;
        uint16 code;
        BOOL done = FALSE;
        GetAttr(WINDOW_SigMask, window, &sigmask);
        while (!done) {
            Wait(sigmask);
            while (WMHI_LASTMSG != (res = RA_HandleInput(window, &code))) {
                switch (res & WMHI_CLASSMASK) {

                    case WMHI_CLOSEWINDOW:
                        done = GID_CANCEL;
                        break;

                    case WMHI_GADGETUP:
                        switch (res & WMHI_GADGETMASK) {

                            case GID_OK:
                                done = GID_OK;
                                break;

                            case GID_CANCEL:
                                done = GID_CANCEL;
                                break;

                        }
                        break;

                }
            }
        }
        if (done == GID_OK) {
            uint32 index = 0;
            GetAttr(CHOOSER_Selected, format, &index);
            plugin = output_plugins[index];
        }
    }

    DisposeObject(window);

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);

    return plugin;
}
