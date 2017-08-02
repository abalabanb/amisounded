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

#ifndef PREFS_H
#define PREFS_H

#ifndef SOUNDED_H
#include "sounded.h"
#endif

#define GUI_PREFS_FILENAME PROGNAME".a500.org.xml"

extern uint32 AppID;

/* prefs.c */
BOOL LoadProgramPrefs (int argc, char **argv);
void WriteProgramPrefs (BOOL env, BOOL envarc);
void FreeProgramPrefs ();
void RestoreWindowSize (Object *window, const char *name);
void SaveWindowSize (Object *window, const char *name, BOOL envarc);
void RestoreSoundEditorColors (Class *cl);
void SaveSoundEditorColors (uint32 *colors);

/* prefs_main.c */
int prefs_main ();

#endif
