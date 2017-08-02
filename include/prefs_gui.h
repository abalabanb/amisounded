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

#ifndef PREFS_GUI_H
#define PREFS_GUI_H

#include "soundeditor_gc_private.h"

enum {
    GID_UNUSED = 0,
    GID_PENS,
    GID_RED,
    GID_GREEN,
    GID_BLUE,
    GID_SAVE,
    GID_USE,
    GID_CANCEL
};

enum {
    SIGNAL_MSGPORT = 0,
    SIGNAL_WINDOW,
    NUM_SIGNALS
};

#define PRFLG_QUIT      0x00000001

typedef struct {
    void *PrID;
    struct StartupParams *Params;
    struct MsgPort *MainPort, *SubPort;

    struct Screen *Screen;
    uint32 Colors[NUM_COLORS];
    struct List *Pens;
    int32 ColorPen;
    int32 ColorIndex;
    struct MsgPort *AppPort;
    struct Window *IWindow;
    Object *Window;
    Object *Page;
    Object *Color;
    Object *Red;
    Object *Green;
    Object *Blue;
    
    uint32 Flags;
    uint32 Signals[NUM_SIGNALS];
} PrefsGUI;

/* prefs_main.c */
int prefs_main ();

/* prefs_gui.c */
PrefsGUI *PrefsGUI_Init (struct StartupMsg *startmsg);
void PrefsGUI_Free (PrefsGUI *prefs);
void PrefsGUI_ShowGUI (PrefsGUI *prefs);
uint32 PrefsGUI_SigMask (PrefsGUI *prefs);
void PrefsGUI_HandleEvents (PrefsGUI *prefs, uint32 sigmask);
void PrefsGUI_UpdateRGBSliders (PrefsGUI *prefs);
void PrefsGUI_UpdateColor (PrefsGUI *prefs);

#endif
