/*
 * AmiSoundED - Sound Editor
 * Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
 * Copyright (C) 2017 Alexandre Balaban <github@balaban.fr>
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
#include "progressbar.h"
#include <proto/fuelgauge.h>
#include <stdarg.h>

#define G(o) ((struct Gadget *)(o))

struct ProgressBar *CreateProgressBar (Project *project, BOOL save) {
    struct ProgressBar *pb;
    pb = AllocVecTags(sizeof(*pb),  AVT_Type, MEMF_PRIVATE,
                                    AVT_ClearWithValue, 0,
                                    TAG_END);
    if (pb) {
        struct Window *main_window = project->IWindow;

        pb->Screen = project->Screen;
        pb->Window = WindowObject,
            WA_Title,           save ?
                                GetString(&LocaleInfo, MSG_SAVING) :
                                GetString(&LocaleInfo, MSG_LOADING),
            WA_Flags,           WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_NOCAREREFRESH,
            WA_IDCMP,           0,
            WA_PubScreen,       pb->Screen,
            WINDOW_CharSet,     LocaleInfo.CodeSet,
            WINDOW_Position,    main_window ? WPOS_CENTERWINDOW : WPOS_CENTERSCREEN,
            WINDOW_RefWindow,   main_window,
            WINDOW_Layout,      LayoutObject,
                LAYOUT_AddChild,    pb->ProgressBar = FuelGaugeObject,
                    FUELGAUGE_Justification,    FGJ_CENTER,
                    FUELGAUGE_Ticks,            5,
                    FUELGAUGE_ShortTicks,       TRUE,
                End,
                CHILD_MinWidth,     300,
                CHILD_MinHeight,    40,
            End,
        End;
        if (pb->Window && RA_OpenWindow(pb->Window)) {
            return pb;
        }
        DeleteProgressBar(pb);
    }
    return NULL;
}

void DeleteProgressBar (struct ProgressBar *pb) {
    if (pb) {
        DisposeObject(pb->Window);
        FreeVec(pb);
    }
}

void VARARGS68K SetProgressBarAttrs (struct ProgressBar *pb, ...) {
    if (pb) {
        struct Window *window;
        va_list tags;
        va_startlinear(tags, pb);

        GetAttr(WINDOW_Window, pb->Window, (uint32 *)&window);
        SetGadgetAttrsA(G(pb->ProgressBar),
            window, NULL, va_getlinearva(tags, struct TagItem *));

        va_end(tags);
    }
}
