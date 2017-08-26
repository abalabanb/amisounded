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
#include <proto/icon.h>
#include "ext_window_class.h"
#include "prefs.h"
#include "debug.h"
#include "path.h"
#include <gadgets/button.h>

#define G(o) ((struct Gadget *)(o))

BOOL translate_gui = TRUE;

#define M_ID(x) (APTR)(x)

static struct NewMenu menuspecs[] = {
    { NM_TITLE, STR_ID(MSG_PROJECT_MENU), NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PROJECT_NEW), "N", 0, 0, M_ID(ACTION_NEW) },
        { NM_ITEM, STR_ID(MSG_PROJECT_OPEN), "O", 0, 0, M_ID(ACTION_OPEN) },
        { NM_ITEM, STR_ID(MSG_PROJECT_RECORD), "R", 0, 0, M_ID(ACTION_RECORD) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PROJECT_SAVE), "S", 0, 0, M_ID(ACTION_SAVE) },
        { NM_ITEM, STR_ID(MSG_PROJECT_SAVEAS), "A", 0, 0, M_ID(ACTION_SAVEAS) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PROJECT_ABOUT), "?", 0, 0, M_ID(ACTION_ABOUT) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PROJECT_CLOSE), "K", 0, 0, M_ID(ACTION_CLOSE) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PROJECT_QUIT), "Q", 0, 0, M_ID(ACTION_CLOSEALL) },
    { NM_TITLE, STR_ID(MSG_EDIT_MENU), NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_EDIT_CUT), "X", 0, 0, M_ID(ACTION_CUT) },
        { NM_ITEM, STR_ID(MSG_EDIT_COPY), "C", 0, 0, M_ID(ACTION_COPY) },
        { NM_ITEM, STR_ID(MSG_EDIT_PASTE), "V", 0, 0, M_ID(ACTION_PASTE) },
        { NM_ITEM, STR_ID(MSG_EDIT_DELETE), "D", 0, 0, M_ID(ACTION_DELETE) },
        { NM_ITEM, STR_ID(MSG_EDIT_MIX), NULL, 0, 0, NULL },
            { NM_SUB, STR_ID(MSG_EDIT_MIX_AVG), NULL, 0, 0, M_ID(ACTION_MIX_AVG) },
            { NM_SUB, STR_ID(MSG_EDIT_MIX_ADD), NULL, 0, 0, M_ID(ACTION_MIX_ADD) },
            { NM_SUB, STR_ID(MSG_EDIT_MIX_SUB), NULL, 0, 0, M_ID(ACTION_MIX_SUB) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_EDIT_VOLUME), NULL, 0, 0, M_ID(ACTION_VOLUME) },
        { NM_ITEM, STR_ID(MSG_EDIT_RESAMPLE), NULL, 0, 0, M_ID(ACTION_RESAMPLE) },
//  { NM_TITLE, STR_ID(MSG_EFFECTS_MENU), NULL, 0, 0, NULL },
//      { NM_ITEM, STR_ID(MSG_EFFECTS_ECHO), NULL, 0, 0, M_ID(ACTION_ECHO) },
    { NM_TITLE, STR_ID(MSG_PREFS_MENU), NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PREFS_SETTINGS), NULL, 0, 0, M_ID(ACTION_OPENPREFS) },
//      { NM_ITEM, STR_ID(MSG_PREFS_CREATEICONS), NULL, CHECKIT|MENUTOGGLE, 0, M_ID(ACTION_CREATEICONS) },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, STR_ID(MSG_PREFS_SAVESETTINGS), NULL, 0, 0, M_ID(ACTION_SAVESETTINGS) },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static const struct SpeedButtonSpec buttonspecs[] = {
    { ACTION_NEW, 0, "tbimages:new", "tbimages:new_s", "tbimages:new_g" },
    { ACTION_OPEN, 0, "tbimages:open", "tbimages:open_s", "tbimages:open_g" },
    { ACTION_SAVE, 0, "tbimages:save", "tbimages:save_s", "tbimages:save_g" },
    { ACTION_SAVEAS, 0, "tbimages:saveas", "tbimages:saveas_s", "tbimages:saveas_g" },
    { ACTION_PLAYSOUND, 0, "tbimages:tapeplay", "tbimages:tapeplay_s", "tbimages:tapeplay_g" },
    { ACTION_PLAYSELECTION, 0, "tbimages:tapeplaysel", "tbimages:tapeplaysel_s", "tbimages:tapeplaysel_g" },
    { ACTION_STOP, 0, "tbimages:tapestop", "tbimages:tapestop_s", "tbimages:tapestop_g" },
    { ACTION_RECORD, 0, "tbimages:taperec", "tbimages:taperec_s", "tbimages:taperec_g" },
    { ACTION_CUT, 0, "tbimages:cut", "tbimages:cut_s", "tbimages:cut_g" },
    { ACTION_COPY, 0, "tbimages:copy", "tbimages:copy_s", "tbimages:copy_g" },
    { ACTION_PASTE, 0, "tbimages:paste", "tbimages:paste_s", "tbimages:paste_g" },
    { ACTION_DELETE, 0, "tbimages:delete", "tbimages:delete_s", "tbimages:delete_g" },
    { ACTION_ZOOMOUTFULL, 0, "tbimages:zoomtomin", "tbimages:zoomtomin_s", "tbimages:zoomtomin_g" },
    { ACTION_ZOOMSELECTION, 0, "tbimages:zoomtosel", "tbimages:zoomtosel_s", "tbimages:zoomtosel_g" },
    { ACTION_ZOOMIN, 0, "tbimages:zoom_in", "tbimages:zoom_in_s", "tbimages:zoom_in_g" },
    { ACTION_ZOOMOUT, 0, "tbimages:zoom_out", "tbimages:zoom_out_s", "tbimages:zoom_out_g" },
    { 0 }
};

static struct HintInfo hintinfos[] = {
    { GID_SPEEDBAR, ACTION_NEW, STR_ID(MSG_NEW_GAD), 0 },
    { GID_SPEEDBAR, ACTION_OPEN, STR_ID(MSG_OPEN_GAD), 0 },
    { GID_SPEEDBAR, ACTION_SAVE, STR_ID(MSG_SAVE_GAD), 0 },
    { GID_SPEEDBAR, ACTION_SAVEAS, STR_ID(MSG_SAVEAS_GAD), 0 },
    { GID_SPEEDBAR, ACTION_PLAYSOUND, STR_ID(MSG_PLAYSOUND_GAD), 0 },
    { GID_SPEEDBAR, ACTION_PLAYSELECTION, STR_ID(MSG_PLAYSELECTION_GAD), 0 },
    { GID_SPEEDBAR, ACTION_STOP, STR_ID(MSG_STOP_GAD), 0 },
    { GID_SPEEDBAR, ACTION_RECORD, STR_ID(MSG_RECORD_GAD), 0 },
    { GID_SPEEDBAR, ACTION_CUT, STR_ID(MSG_CUT_GAD), 0 },
    { GID_SPEEDBAR, ACTION_COPY, STR_ID(MSG_COPY_GAD), 0 },
    { GID_SPEEDBAR, ACTION_PASTE, STR_ID(MSG_PASTE_GAD), 0 },
    { GID_SPEEDBAR, ACTION_DELETE, STR_ID(MSG_DELETE_GAD), 0 },
    { GID_SPEEDBAR, ACTION_ZOOMSELECTION, STR_ID(MSG_ZOOMSELECTION_GAD), 0 },
    { GID_SPEEDBAR, ACTION_ZOOMIN, STR_ID(MSG_ZOOMIN2X_GAD), 0 },
    { GID_SPEEDBAR, ACTION_ZOOMOUT, STR_ID(MSG_ZOOMOUT2X_GAD), 0 },
    { -1, -1, NULL, 0 }
};

static uint32 AppMsgHookFunc (struct Hook *hook, Object *window, struct AppMessage *msg) {
    Project *project;
    GetAttr(WINDOW_UserData, window, (uint32 *)&project);
    if (!(project->Flags & PRFLG_PLAYING)) {
        if (msg->am_Class == AMCLASSICON_Open && msg->am_NumArgs > 0) {
            char *filename;
            int i;
            for (i = 1; i < msg->am_NumArgs; i++) {
                filename = GetFullPath2(msg->am_ArgList[i].wa_Lock, msg->am_ArgList[i].wa_Name);
                if (filename) {
                    struct StartupMsg *msg;
                    msg = AllocMsg(sizeof(*msg), NULL);
                    if (msg) {
                        msg->Msg.PrID = project->PrID;
                        msg->Msg.Class = MSG_NEWPROJECT;
                        msg->Filename = filename;
                        PutMsg(project->MainPort, &msg->Msg.Node);
                    } else {
                        FreeVec((APTR)filename);
                    }
                }
            }
            filename = GetFullPath2(msg->am_ArgList[0].wa_Lock, msg->am_ArgList[0].wa_Name);
            if (filename) {
                LoadFromFilename(project, filename);
            }
        }
    }
    return 0;
}

static const struct Hook appmsghook = {
    {0}, (HOOKFUNC)AppMsgHookFunc, NULL, NULL
};

/*static const struct TagItem button2sbleft[] = {
    { GA_ID,    SPEEDBAR_ScrollLeft },
    { TAG_END,  0 }
};

static const struct TagItem button2sbright[] = {
    { GA_ID,    SPEEDBAR_ScrollRight },
    { TAG_END,  0 }
};*/

Project *Project_Init (struct StartupMsg *startmsg) {
    Project *project;
    project = AllocVecTags(sizeof(*project),    AVT_Type, MEMF_PRIVATE,
                                                AVT_ClearWithValue, 0,
                                                TAG_END);
    if (project) {
        struct StartupParams *params;
        project->PrID = startmsg->Msg.PrID;
        project->Params = params = startmsg->PrParams;

        ObtainSemaphore(params->Lock);
        if (translate_gui) {
            translate_gui = FALSE;
            TranslateMenus(&LocaleInfo, menuspecs);
            TranslateHints(&LocaleInfo, hintinfos);
        }
        ReleaseSemaphore(params->Lock);

        project->MainPort = startmsg->Msg.Node.mn_ReplyPort;
        project->SubPort = startmsg->PrPort;
        project->Signals[SIGNAL_MSGPORT] = 1 << startmsg->PrPort->mp_SigBit;

        project->OpenReq = AllocAslRequestTags(ASL_FileRequest,
            ASLFR_SleepWindow,  TRUE,
            TAG_END);
        project->SaveReq = AllocAslRequestTags(ASL_FileRequest,
            ASLFR_SleepWindow,  TRUE,
            ASLFR_DoSaveMode,   TRUE,
            TAG_END);

        project->Screen = params->Screen;

        project->SpeedButtons = MakeSpeedButtonList(project->Screen, buttonspecs);
        project->AppPort = AllocPort();

        project->Window = ExtWindowObject,
            WA_PubScreen,   project->Screen,
            WA_Flags,       WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET
                            |WFLG_NOCAREREFRESH|WFLG_SIZEBBOTTOM|WFLG_ACTIVATE,
            WA_IDCMP,       IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_MENUPICK,
            WINDOW_UserData,        project,
            WINDOW_Position,        WPOS_CENTERMOUSE,
            WINDOW_CharSet,         LocaleInfo.CodeSet,
            WINDOW_AppPort,         project->AppPort,
            WINDOW_AppWindow,       TRUE,
            WINDOW_AppMsgHook,      &appmsghook,
            WINDOW_IconifyGadget,   TRUE,
            WINDOW_SnapshotGadget,  TRUE,
            WINDOW_Icon,            GetDiskObjectNew("PROGDIR:"PROGNAME),
            WINDOW_NewMenu,         menuspecs,
            WINDOW_Layout,          project->Layout = VLayoutObject,
                LAYOUT_SpaceInner,      FALSE,
                LAYOUT_SpaceOuter,      FALSE,
                LAYOUT_AddChild,        project->Sound = ClassObject(params->WaveClass),
                    GA_ID,              GID_SOUNDGC,
                    GA_RelVerify,       TRUE,
                End,
                CHILD_NominalSize,      TRUE,
                CHILD_NoDispose,        TRUE,
                LAYOUT_AddChild,        HLayoutObject,
                    LAYOUT_SpaceInner,      FALSE,
                    LAYOUT_SpaceOuter,      FALSE,
                    /*LAYOUT_AddChild,      project->SBLeft = ButtonObject,
                        GA_ID,              GID_SBLEFT,
                        BUTTON_AutoButton,  BAG_LFARROW,
                    End,
                    CHILD_WeightedWidth,    0,*/
                    LAYOUT_AddChild,        project->SpeedBar = SpeedBarObject,
                        GA_ID,              GID_SPEEDBAR,
                        GA_RelVerify,       TRUE,
                        SPEEDBAR_Buttons,   project->SpeedButtons,
                    End,
                    /*LAYOUT_AddChild,      project->SBRight = ButtonObject,
                        GA_ID,              GID_SBRIGHT,
                        BUTTON_AutoButton,  BAG_RTARROW,
                    End,
                    CHILD_WeightedWidth,    0,*/
                End,
                CHILD_WeightedHeight,   0,
            End,
            WINDOW_HorizProp,   TRUE,
            WINDOW_HintInfo,    hintinfos,
            WINDOW_GadgetHelp,  TRUE,
        End;

        if (project->OpenReq && project->SaveReq && project->SpeedButtons &&
            project->Window)
        {
            RestoreWindowSize(project->Window, "project_window");

            GetAttr(WINDOW_HorizObject, project->Window, (uint32 *)&project->HProp);
            SetAttrs(project->HProp, ICA_TARGET, project->Sound, TAG_END);
            SetAttrs(project->Sound, ICA_TARGET, project->HProp, TAG_END);
            /*SetAttrs(project->SBLeft,
                ICA_TARGET, project->SpeedBar,
                ICA_MAP,    button2sbleft,
                TAG_END);
            SetAttrs(project->SBRight,
                ICA_TARGET, project->SpeedBar,
                ICA_MAP,    button2sbright,
                TAG_END);*/
            Project_UpdateProp(project);

            Project_WindowTitle(project);
            Project_IconTitle(project);
            Project_UpdateGUI(project);
            
            return project;
        }

        Project_Free(project);
    }
    return NULL;
}

void Project_Free (Project *project) {
    if (project) {
        DisposeObject(project->Window);
        DisposeObject(project->Sound);
        FreePort(project->AppPort);
        FreeSpeedButtonList(project->SpeedButtons);

        FreeVec(project->Filename);
        FreeVec(project->WindowTitle);
        FreeVec(project->IconTitle);

        FreeAslRequest(project->OpenReq);
        FreeAslRequest(project->SaveReq);

        FreeVec(project);
    }
}

void Project_ShowGUI (Project *project) {
    project->IWindow = RA_OpenWindow(project->Window);
    GetAttr(WINDOW_SigMask, project->Window, &project->Signals[SIGNAL_WINDOW]);
    Project_UpdateGUI(project);
}

uint32 Project_SigMask (Project *project) {
    return project->Signals[SIGNAL_MSGPORT] |
        project->Signals[SIGNAL_WINDOW] |
        project->Signals[SIGNAL_AHI];
}

void Project_HandleEvents (Project *project, uint32 sigmask) {
    if (sigmask & project->Signals[SIGNAL_WINDOW]) {
        uint32 res, gid;
        uint16 code;
        while ((res = RA_HandleInput(project->Window, &code)) != WMHI_LASTMSG) {
            switch (res & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW:
                    PerformAction(project, ACTION_CLOSE);
                    break;

                case WMHI_ICONIFY:
                    PerformAction(project, ACTION_DOICONIFY);
                    break;

                case WMHI_UNICONIFY:
                    PerformAction(project, ACTION_UNICONIFY);
                    break;

                case WMHI_SNAPSHOT:
                    SaveWindowSize(project->Window, "project_window", TRUE);
                    break;

                case WMHI_MENUPICK:
                    {
                    struct Menu *menu;
                    GetAttr(WINDOW_MenuStrip, project->Window, (uint32 *)&menu);
                    if (menu) {
                        struct MenuItem *item;
                        while (item = ItemAddress(menu, code)) {
                            PerformAction(project, (uint32)GTMENUITEM_USERDATA(item));
                            code = item->NextSelect;
                        }
                    }
                    }
                    break;

                case WMHI_GADGETUP:
                    switch (res & WMHI_GADGETMASK) {
                        case GID_SOUNDGC:
                            Project_UpdateGUI(project);
                            break;

                        case GID_SPEEDBAR:
                            PerformAction(project, code);
                            break;
                    }
                    break;
            }
        }
    }
    
    if (sigmask & project->Signals[SIGNAL_MSGPORT]) {
        struct MsgPort *port = project->SubPort;
        struct MsgNode *msg;
        while (msg = (struct MsgNode *)GetMsg(port)) {
            switch (msg->Node.mn_Node.ln_Type) {
                case NT_MESSAGE:
                    switch (msg->Class) {
                        case MSG_QUIT:
                            PerformAction(project, ((struct QuitMsg *)msg)->Force ?
                                ACTION_FORCEQUIT : ACTION_CLOSE);
                            break;
                            
                        case MSG_HIDE:
                            PerformAction(project, ACTION_DOICONIFY);
                            break;
                            
                        case MSG_UNHIDE:
                        case MSG_TOFRONT:
                            PerformAction(project, ACTION_UNICONIFY);
                            if (msg->Class == MSG_TOFRONT) {
                                ScreenToFront(project->Screen);
                            }
                            break;
                            
                        case MSG_NEWPREFS:
                            if (project->IWindow) {
                                DoGadgetMethod(G(project->Sound), project->IWindow, NULL,
                                    GM_LAYOUT, NULL, TRUE);
                                RefreshGList(G(project->Sound), project->IWindow, NULL, 1);
                            }
                            break;
                    }
                    if (msg->Node.mn_ReplyPort)
                        ReplyMsg(&msg->Node);
                    else
                        FreeMsg(&msg->Node);
                    break;

                case NT_REPLYMSG:
                    switch (msg->Class) {
                    }
                    FreeMsg(&msg->Node);
                    break;
            }
        }
    }
}

void OnOffMenu (struct Window *window, uint16 code, BOOL enable) {
    if (enable) {
        OnMenu(window, code);
    } else {
        OffMenu(window, code);
    }
}

void Project_UpdateGUI (Project *project) {
    struct Window *window = project->IWindow;
    struct Node *button;
    BOOL disabled;
    uint32 has_selection;
    uint32 area_selected;
    BOOL is_empty;

    GetAttrs(project->Sound,
        SOUND_HasSelection, &has_selection,
        SOUND_AreaSelected, &area_selected,
        TAG_END);
    is_empty = !GetSoundLength(project->Sound);

    if (window) {
        if (project->Flags & PRFLG_PLAYING) {
            OffMenu(window, SHIFTMENU(0)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
            OffMenu(window, SHIFTMENU(1)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
//          OffMenu(window, SHIFTMENU(2)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
        } else {
            OnMenu(window, SHIFTMENU(0)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
            OnMenu(window, SHIFTMENU(1)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
//          OnMenu(window, SHIFTMENU(2)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB));
            OnOffMenu(window, SHIFTMENU(0)|SHIFTITEM(4)|SHIFTSUB(NOSUB), !is_empty);
            OnOffMenu(window, SHIFTMENU(0)|SHIFTITEM(5)|SHIFTSUB(NOSUB), !is_empty);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(0)|SHIFTSUB(NOSUB), area_selected);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(1)|SHIFTSUB(NOSUB), area_selected);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(2)|SHIFTSUB(NOSUB), has_selection || is_empty);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(3)|SHIFTSUB(NOSUB), area_selected);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(4)|SHIFTSUB(NOSUB), has_selection);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(6)|SHIFTSUB(NOSUB), !is_empty);
            OnOffMenu(window, SHIFTMENU(1)|SHIFTITEM(7)|SHIFTSUB(NOSUB), !is_empty);
//          OnOffMenu(window, SHIFTMENU(2)|SHIFTITEM(NOITEM)|SHIFTSUB(NOSUB), !is_empty);
        }
    }

    SetAttrs(project->SpeedBar,
        SPEEDBAR_Buttons,   ~0,
        TAG_END);
    button = GetHead(project->SpeedButtons);
    if (project->Flags & PRFLG_PLAYING) {
        while (button) {
            disabled = TRUE;
            switch (button->ln_Pri) {
                case ACTION_NEW:
                case ACTION_ZOOMIN:
                case ACTION_ZOOMOUT:
                case ACTION_ZOOMSELECTION:
                case ACTION_STOP:
                    disabled = FALSE;
                    break;
            }
            SetSpeedButtonNodeAttrs(button,
                SBNA_Disabled,  disabled,
                TAG_END);
            button = GetSucc(button);
        }
    } else {
        while (button) {
            disabled = FALSE;
            switch (button->ln_Pri) {
                case ACTION_SAVE:
                case ACTION_SAVEAS:
                case ACTION_PLAYSOUND:
                    disabled = is_empty;
                    break;
                case ACTION_PLAYSELECTION:
                    disabled = is_empty || !has_selection;
                    break;
                case ACTION_CUT:
                case ACTION_COPY:
                case ACTION_DELETE:
                    disabled = !area_selected;
                    break;
                case ACTION_PASTE:
                    disabled = !is_empty && !has_selection;
                    break;
                case ACTION_STOP:
                    disabled = TRUE;
                    break;
            }
            SetSpeedButtonNodeAttrs(button,
                SBNA_Disabled,  disabled,
                TAG_END);
            button = GetSucc(button);
        }
    }
    SetGadgetAttrs((struct Gadget *)
        project->SpeedBar, window, NULL,
        SPEEDBAR_Buttons,   project->SpeedButtons,
        TAG_END);
}
