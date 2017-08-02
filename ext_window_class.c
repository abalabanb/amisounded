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

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/sysiclass.h>
#include <classes/window.h>
#include <gadgets/button.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include "ext_window_class.h"

typedef struct {
    struct Window *Window;
    int32 HasSnapshot;
    Object *SnapshotButton;
    Object *SnapshotImage;
    APTR DrawInfo;
} ExtWindow;

static uint32 ExtWindow_Dispatch (Class *cl, Object *o, Msg msg);

Class * InitExtWindowClass () {
    Class *cl;
    cl = MakeClass("extwindow.class", NULL, WINDOW_GetClass(), sizeof(ExtWindow), 0);
    if (cl) {
        cl->cl_Dispatcher.h_Entry = ExtWindow_Dispatch;
    }
    return cl;
}

static void ParseTaglist (struct TagItem *taglist, ExtWindow *extwin);
static void FreeSnapshotButton (ExtWindow *extwin, struct Window *window);

static uint32 ExtWindow_Dispatch (Class *cl, Object *o, Msg msg) {
    uint32 ret;
    struct Window *window;
    ExtWindow *extwin;

    switch (msg->MethodID) {

        case OM_NEW:
            {
                ExtWindow tmp = {0};
                ParseTaglist(((struct opSet *)msg)->ops_AttrList, &tmp);
                ret = IDoSuperMethodA(cl, o, msg);
                if (o = (Object *)ret) {
                    extwin = INST_DATA(cl, o);
                    CopyMem(&tmp, extwin, sizeof(ExtWindow));
                }
            }
            break;

        case WM_OPEN:
            extwin = INST_DATA(cl, o);
            ret = IDoSuperMethodA(cl, o, msg);
            if (!extwin->Window && (window = (struct Window *)ret)) {
                extwin->Window = window;
                if (extwin->HasSnapshot) {
                    extwin->DrawInfo = GetScreenDrawInfo(window->WScreen);
                    extwin->SnapshotImage = NewObject(NULL, "sysiclass",
                        SYSIA_DrawInfo, extwin->DrawInfo,
                        SYSIA_Which,    SNAPSHOTIMAGE,
                        TAG_END);
                    if (extwin->SnapshotImage) {
                        extwin->SnapshotButton = NewObject(NULL, "buttongclass",
                            GA_ID,          GID_SNAPSHOT,
                            GA_RelVerify,   TRUE,
                            //GA_SysGadget, TRUE,
                            GA_Image,       extwin->SnapshotImage,
                            GA_Titlebar,    TRUE,
                            GA_TopBorder,   TRUE,
                            GA_RelRight,    0,
                            TAG_END);
                        if (extwin->SnapshotButton) {
                            AddGadget(window, (struct Gadget *)extwin->SnapshotButton, 0);
                            RefreshGList((struct Gadget *)extwin->SnapshotButton, window,
                                NULL, 1);
                            break;
                        }
                    }
                    FreeSnapshotButton(extwin, window);
                }
            }
            break;

        case WM_HANDLEINPUT:
            ret = IDoSuperMethodA(cl, o, msg);
            switch (ret & WMHI_CLASSMASK) {
                case WMHI_GADGETUP:
                    extwin = INST_DATA(cl, o);
                    switch (ret & WMHI_GADGETMASK) {
                        case GID_SNAPSHOT:
                            if (extwin->SnapshotButton) ret = WMHI_SNAPSHOT;
                            break;
                    }
                    break;
            }
            break;

        case WM_ICONIFY:
        case WM_CLOSE:
        case OM_DISPOSE:
            extwin = INST_DATA(cl, o);
            if (window = extwin->Window) {
                extwin->Window = NULL;
                if (extwin->HasSnapshot) {
                    FreeSnapshotButton(extwin, window);
                }
            }
            // fall through
        default:
            ret = IDoSuperMethodA(cl, o, msg);
            break;

    }

    return ret;
}

static void ParseTaglist (struct TagItem *taglist, ExtWindow *extwin) {
    struct TagItem *tstate, *ti;
    tstate = taglist;
    while (ti = NextTagItem(&tstate)) {
        switch (ti->ti_Tag) {
            case WINDOW_SnapshotGadget:
                extwin->HasSnapshot = ti->ti_Data;
                ti->ti_Tag = TAG_IGNORE;
                break;
        }
    }
}

static void FreeSnapshotButton (ExtWindow *extwin, struct Window *window) {
    if (extwin->SnapshotButton) {
        RemoveGadget(window, (struct Gadget *)extwin->SnapshotButton);
        DisposeObject(extwin->SnapshotButton);
        extwin->SnapshotButton = NULL;
    }
    DisposeObject(extwin->SnapshotImage);
    extwin->SnapshotImage = NULL;
    FreeScreenDrawInfo(window->WScreen, extwin->DrawInfo);
    extwin->DrawInfo = NULL;
}
