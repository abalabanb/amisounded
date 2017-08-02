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
#include "prefs.h"
#include "prefs_gui.h"
#include <intuition/icclass.h>
#include <gadgets/clicktab.h>
#include <gadgets/slider.h>
#include <gadgets/button.h>
#include <images/label.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/clicktab.h>
#include <proto/listbrowser.h>
#include <proto/slider.h>
#include <proto/label.h>
#include "locale_support.h"
#include "ext_window_class.h"

#define G(o) ((struct Gadget *)(o))

static BOOL translate_gui = TRUE;

static struct ColumnInfo pen_columns[] = {
    {  1, STR_ID(MSG_PEN),         0 },
    { -1, STR_ID(-1),              0 }
};

static const char *pen_descriptions[NUM_COLORS + 1] = {
    STR_ID(MSG_BKG1_PEN),
    STR_ID(MSG_BKG2_PEN),
    STR_ID(MSG_ZEROLINES_PEN),
    STR_ID(MSG_WAVEFORM_PEN),
    STR_ID(MSG_SELBKG1_PEN),
    STR_ID(MSG_SELBKG2_PEN),
    STR_ID(MSG_SELWAVEFORM_PEN),
    STR_ID(-1)
};

static uint32 IdcmpHookFunc (struct Hook *hook, Object *window, struct IntuiMessage *msg) {
    if (msg->Class == IDCMP_IDCMPUPDATE) {
        PrefsGUI *prefs;
        GetAttr(WINDOW_UserData, window, (uint32 *)&prefs);
        switch (GetTagData(GA_ID, GID_UNUSED, msg->IAddress)) {
            case GID_RED:
            case GID_GREEN:
            case GID_BLUE:
                PrefsGUI_UpdateColor(prefs);
                break;
        }
    }
    return 0;
}

const struct Hook IdcmpHook = {
    { NULL, NULL },
    (HOOKFUNC)IdcmpHookFunc,
    NULL,
    NULL
};

static void SavePrefs (PrefsGUI *prefs, BOOL envarc);

PrefsGUI *PrefsGUI_Init (struct StartupMsg *startmsg) {
    PrefsGUI *prefs;
    prefs = AllocMem(sizeof(*prefs), MEMF_PRIVATE|MEMF_CLEAR);
    if (prefs) {
        struct StartupParams *params;
        const char *clicktab_labels[2];
        prefs->PrID = startmsg->Msg.PrID;
        prefs->Params = params = startmsg->PrParams;

        ObtainSemaphore(params->Lock);
        if (translate_gui) {
            translate_gui = FALSE;
            TranslateColumnTitles(&LocaleInfo, pen_columns);
            TranslateLabelArray(&LocaleInfo, pen_descriptions);
        }
        ReleaseSemaphore(params->Lock);

        prefs->MainPort = startmsg->Msg.Node.mn_ReplyPort;
        prefs->SubPort = startmsg->PrPort;
        prefs->Signals[SIGNAL_MSGPORT] = 1 << startmsg->PrPort->mp_SigBit;

        clicktab_labels[0] = GetString(&LocaleInfo, MSG_COLORS_GAD);
        clicktab_labels[1] = NULL;
        
        prefs->Pens = AllocList();
        if (!prefs->Pens) {
            goto out;
        } else {
            const char * const *pen = pen_descriptions;
            struct Node *node;
            while (*pen) {
                node = AllocListBrowserNode(1, LBNCA_Text, *pen++, TAG_END);
                if (!node) goto out;
                AddTail(prefs->Pens, node);
            }
        }
        
        prefs->Screen = params->Screen;
        
        prefs->ColorPen = ObtainPen(prefs->Screen->ViewPort.ColorMap, -1, 0, 0, 0, PENF_EXCLUSIVE);
        if (prefs->ColorPen == -1) goto out;
        
        prefs->AppPort = CreateMsgPort();
        if (!prefs->AppPort) goto out;
        
        prefs->Window = ExtWindowObject,
            WA_PubScreen,   prefs->Screen,
            WA_Title,       GetString(&LocaleInfo, MSG_SETTINGS_WIN),
            WA_Flags,       WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_SIZEGADGET
                            |WFLG_NOCAREREFRESH|WFLG_ACTIVATE,
            WA_IDCMP,       IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_IDCMPUPDATE,
            WINDOW_UserData,        prefs,
            WINDOW_Position,        WPOS_CENTERSCREEN,
            WINDOW_CharSet,         LocaleInfo.CodeSet,
            WINDOW_AppPort,         prefs->AppPort,
            WINDOW_IconifyGadget,   TRUE,
            WINDOW_SnapshotGadget,  TRUE,
            WINDOW_Icon,            GetDiskObjectNew("PROGDIR:"PROGNAME),
            WINDOW_IDCMPHook,       &IdcmpHook,
            WINDOW_IDCMPHookBits,   IDCMP_IDCMPUPDATE,
            WINDOW_Layout,          VLayoutObject,
                LAYOUT_AddChild,            ClickTabObject,
                    GA_Text,                clicktab_labels,
                    CLICKTAB_PageGroup,     prefs->Page = PageObject,
                        LAYOUT_DeferLayout, TRUE,
                        PAGE_Add,           VLayoutObject,
                            LAYOUT_AddChild,            ListBrowserObject,
                                GA_ID,                      GID_PENS,
                                GA_RelVerify,               TRUE,
                                LISTBROWSER_ColumnTitles,   TRUE,
                                LISTBROWSER_ColumnInfo,     pen_columns,
                                LISTBROWSER_Labels,         prefs->Pens,
                                LISTBROWSER_ShowSelected,   TRUE,
                                LISTBROWSER_Selected,       0,
                                LISTBROWSER_VerticalProp,   FALSE,
                                LISTBROWSER_MinVisible,     NUM_COLORS-1,
                            End,
                            LAYOUT_AddChild,                HLayoutObject,
                                LAYOUT_AddChild,            prefs->Color = ButtonObject,
                                    GA_ReadOnly,            TRUE,
                                    BUTTON_BackgroundPen,   prefs->ColorPen,
                                End,
                                CHILD_WeightedWidth,        0,
                                LAYOUT_AddChild,            VLayoutObject,
                                    LAYOUT_AddChild,        prefs->Red = SliderObject,
                                        GA_ID,              GID_RED,
                                        GA_RelVerify,       TRUE,
                                        ICA_TARGET,         ICTARGET_IDCMP,
                                        SLIDER_Orientation, SLIDER_HORIZONTAL,
                                        SLIDER_Max,         255,
                                        SLIDER_Ticks,       33,
                                        SLIDER_ShortTicks,  TRUE,
                                        SLIDER_LevelFormat, "%ld",
                                    End,
                                    CHILD_Label,            LabelObject,
                                        LABEL_Text,         GetString(&LocaleInfo, MSG_RED_GAD),
                                    End,
                                    LAYOUT_AddChild,        prefs->Green = SliderObject,
                                        GA_ID,              GID_GREEN,
                                        GA_RelVerify,       TRUE,
                                        ICA_TARGET,         ICTARGET_IDCMP,
                                        SLIDER_Orientation, SLIDER_HORIZONTAL,
                                        SLIDER_Max,         255,
                                        SLIDER_Ticks,       33,
                                        SLIDER_ShortTicks,  TRUE,
                                        SLIDER_LevelFormat, "%ld",
                                    End,
                                    CHILD_Label,            LabelObject,
                                        LABEL_Text,         GetString(&LocaleInfo, MSG_GREEN_GAD),
                                    End,
                                    LAYOUT_AddChild,        prefs->Blue = SliderObject,
                                        GA_ID,              GID_BLUE,
                                        GA_RelVerify,       TRUE,
                                        ICA_TARGET,         ICTARGET_IDCMP,
                                        SLIDER_Orientation, SLIDER_HORIZONTAL,
                                        SLIDER_Max,         255,
                                        SLIDER_Ticks,       33,
                                        SLIDER_ShortTicks,  TRUE,
                                        SLIDER_LevelFormat, "%ld",
                                    End,
                                    CHILD_Label,            LabelObject,
                                        LABEL_Text,         GetString(&LocaleInfo, MSG_BLUE_GAD),
                                    End,
                                End,
                            End,
                            CHILD_WeightedHeight,       0,
                        End,
                    End,
                End,
                LAYOUT_AddChild,            HLayoutObject,
                    LAYOUT_BevelStyle,      BVS_SBAR_VERT,
                    LAYOUT_SpaceOuter,      TRUE,
                    LAYOUT_BottomSpacing,   0,
                    LAYOUT_EvenSize,        TRUE,
                    LAYOUT_AddChild,        ButtonObject,
                        GA_ID,              GID_SAVE,
                        GA_RelVerify,       TRUE,
                        GA_Text,            GetString(&LocaleInfo, MSG_SAVE_GAD),
                    End,
                    CHILD_WeightedWidth,    0,
                    LAYOUT_AddChild,        ButtonObject,
                        GA_ID,              GID_USE,
                        GA_RelVerify,       TRUE,
                        GA_Text,            GetString(&LocaleInfo, MSG_USE_GAD),
                    End,
                    CHILD_WeightedWidth,    0,
                    LAYOUT_AddChild,        ButtonObject,
                        GA_ID,              GID_CANCEL,
                        GA_RelVerify,       TRUE,
                        GA_Text,            GetString(&LocaleInfo, MSG_CANCEL_GAD),
                    End,
                    CHILD_WeightedWidth,    0,
                End,
                CHILD_WeightedHeight,       0,
            End,
        End;

        if (prefs->Window) {
            GlobalData *gd = GLOB_DATA(params->WaveClass);
        
            RestoreWindowSize(prefs->Window, "prefs_window");
            
            CopyMem(gd->Colors, prefs->Colors, sizeof(gd->Colors));
            PrefsGUI_UpdateRGBSliders(prefs);
            
            return prefs;
        }
        
out:
        PrefsGUI_Free(prefs);
    }
    return NULL;
}

void PrefsGUI_Free (PrefsGUI *prefs) {
    if (prefs) {
        DisposeObject(prefs->Window);
        DeleteMsgPort(prefs->AppPort);
        ReleasePen(prefs->Screen->ViewPort.ColorMap, prefs->ColorPen);
        if (prefs->Pens) {
            FreeListBrowserList(prefs->Pens);
            FreeList(prefs->Pens);
        }
        FreeMem(prefs, sizeof(*prefs));
    }
}

void PrefsGUI_ShowGUI (PrefsGUI *prefs) {
    prefs->IWindow = RA_OpenWindow(prefs->Window);
    GetAttr(WINDOW_SigMask, prefs->Window, &prefs->Signals[SIGNAL_WINDOW]);
}

uint32 PrefsGUI_SigMask (PrefsGUI *prefs) {
    return prefs->Signals[SIGNAL_MSGPORT] |
        prefs->Signals[SIGNAL_WINDOW];
}

void PrefsGUI_HandleEvents (PrefsGUI *prefs, uint32 sigmask) {
    if (sigmask & prefs->Signals[SIGNAL_WINDOW]) {
        uint32 res, gid;
        uint16 code;
        while ((res = RA_HandleInput(prefs->Window, &code)) != WMHI_LASTMSG) {
            switch (res & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW:
                    prefs->Flags |= PRFLG_QUIT;
                    break;
                    
                case WMHI_ICONIFY:
                    RA_Iconify(prefs->Window);
                    prefs->IWindow = NULL;
                    GetAttr(WINDOW_SigMask, prefs->Window, &prefs->Signals[SIGNAL_WINDOW]);
                    break;
                    
                case WMHI_UNICONIFY:
                    PrefsGUI_ShowGUI(prefs);
                    break;
                    
                case WMHI_SNAPSHOT:
                    SaveWindowSize(prefs->Window, "prefs_window", TRUE);
                    break;
    
                case WMHI_MENUPICK:
                    break;
                    
                case WMHI_GADGETUP:
                    gid = res & WMHI_GADGETMASK;
                    switch (gid) {
                        case GID_PENS:
                            prefs->ColorIndex = code;
                            PrefsGUI_UpdateRGBSliders(prefs);
                            break;
                            
                        case GID_RED:
                        case GID_GREEN:
                        case GID_BLUE:
                            PrefsGUI_UpdateColor(prefs);
                            break;
                    
                        case GID_SAVE:
                        case GID_USE:
                            SavePrefs(prefs, gid == GID_SAVE);
                            /* fall through */
                        case GID_CANCEL:
                            prefs->Flags |= PRFLG_QUIT;
                            break;
                    }
                    break;
            }
        }
    }
    
    if (sigmask & prefs->Signals[SIGNAL_MSGPORT]) {
        struct MsgPort *port = prefs->SubPort;
        struct MsgNode *msg;
        while (msg = (struct MsgNode *)GetMsg(port)) {
            switch (msg->Node.mn_Node.ln_Type) {
                case NT_MESSAGE:
                    switch (msg->Class) {
                        case MSG_QUIT:
                            prefs->Flags |= PRFLG_QUIT;
                            break;
                        
                        case MSG_HIDE:
                            RA_Iconify(prefs->Window);
                            prefs->IWindow = NULL;
                            GetAttr(WINDOW_SigMask, prefs->Window, &prefs->Signals[SIGNAL_WINDOW]);
                            break;
                        
                        case MSG_UNHIDE:
                        case MSG_TOFRONT:
                            PrefsGUI_ShowGUI(prefs);
                            if (msg->Class == MSG_TOFRONT) {
                                ScreenToFront(prefs->Screen);
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

static void SavePrefs (PrefsGUI *prefs, BOOL envarc) {
    GlobalData *gd = GLOB_DATA(prefs->Params->WaveClass);
    struct MsgNode *msg;
    
    CopyMem(prefs->Colors, gd->Colors, sizeof(gd->Colors));
    InitGradSpec(&gd->GradSpec[0], 0, gd->Colors[COL_BKG1],
        gd->Colors[COL_BKG2]);
    InitGradSpec(&gd->GradSpec[1], 180, gd->Colors[COL_BKG1],
        gd->Colors[COL_BKG2]);
    InitGradSpec(&gd->GradSpec[2], 0, gd->Colors[COL_ALTBKG1],
        gd->Colors[COL_ALTBKG2]);
    InitGradSpec(&gd->GradSpec[3], 180, gd->Colors[COL_ALTBKG1],
        gd->Colors[COL_ALTBKG2]);
    
    SaveSoundEditorColors(prefs->Colors);
    WriteProgramPrefs(TRUE, envarc);
    
    msg = AllocMsg(sizeof(*msg), NULL);
    if (msg) {
        msg->PrID = prefs->PrID;
        msg->Class = MSG_NEWPREFS;
        PutMsg(prefs->MainPort, &msg->Node);
    }
}

void PrefsGUI_UpdateRGBSliders (PrefsGUI *prefs) {
    int32 color = prefs->ColorIndex;
    uint32 red, green, blue;
    red = (prefs->Colors[color] >> 16) & 0xff;
    green = (prefs->Colors[color] >> 8) & 0xff;
    blue = (prefs->Colors[color]) & 0xff;
    SetRGB32(&prefs->Screen->ViewPort, prefs->ColorPen,
        red * 0x01010101, green * 0x01010101, blue * 0x01010101);
    RefreshPageGadget(G(prefs->Color), prefs->Page, prefs->IWindow, NULL);
    SetPageGadgetAttrs(G(prefs->Red), prefs->Page, prefs->IWindow, NULL,
        SLIDER_Level,   red,
        TAG_END);
    SetPageGadgetAttrs(G(prefs->Green), prefs->Page, prefs->IWindow, NULL,
        SLIDER_Level,   green,
        TAG_END);
    SetPageGadgetAttrs(G(prefs->Blue), prefs->Page, prefs->IWindow, NULL,
        SLIDER_Level,   blue,
        TAG_END);
}

void PrefsGUI_UpdateColor (PrefsGUI *prefs) {
    int32 color = prefs->ColorIndex;
    uint32 red, green, blue;
    GetAttr(SLIDER_Level, prefs->Red, &red);
    GetAttr(SLIDER_Level, prefs->Green, &green);
    GetAttr(SLIDER_Level, prefs->Blue, &blue);
    prefs->Colors[color] = (red << 16)|(green << 8)|(blue);
    SetRGB32(&prefs->Screen->ViewPort, prefs->ColorPen,
        red * 0x01010101, green * 0x01010101, blue * 0x01010101);
    RefreshPageGadget(G(prefs->Color), prefs->Page, prefs->IWindow, NULL);
}
