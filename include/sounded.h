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

#ifndef SOUNDED_H
#define SOUNDED_H

#include <exec/exec.h>
#include <dos/dos.h>
#include <utility/utility.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/icclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/speedbar.h>
#include <reaction/reaction_macros.h>
#include <workbench/workbench.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/speedbar.h>

#include "AmiSoundED_rev.h"
#define PROGNAME "AmiSoundED"
extern const char progname[];

#include "os4types.h"
#include "locale_support.h"
#include "soundeditor_gc.h"
#include "speedbar_support.h"

#ifndef ASOT_MACROS_H
#include "asot_macros.h"
#endif

extern struct LocaleInfo LocaleInfo;

struct StartupParams {
    struct SignalSemaphore *Lock;
    struct Screen *Screen;
    Class *WaveClass;
    struct List *Plugins;
};

#define ClassObject(class) NewObject(class, NULL

enum {
    MSG_DUMMY = 0,
    MSG_STARTUP,
    MSG_NEWPROJECT,
    MSG_QUIT,
    MSG_OPENPREFS,
    MSG_HIDE,
    MSG_UNHIDE,
    MSG_TOFRONT,
    MSG_NEWPREFS
};

struct MsgNode {
    struct Message Node;
    void *PrID;
    uint32 Class;
};

struct StartupMsg {
    struct MsgNode Msg;
    struct MsgPort *PrPort;
    struct StartupParams *PrParams;
    char *Filename;
};

struct QuitMsg {
    struct MsgNode Msg;
    BOOL Force;
};

#endif
