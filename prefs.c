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
#include <proto/intuition.h>
#include <proto/application.h>
#include "soundeditor_gc.h"
#include "soundeditor_gc_private.h"

#include "prefs.h"

uint32 AppID;
static PrefsObject *Prefs;

BOOL LoadProgramPrefs (int argc, char **argv) {
    if (argc == 0) {
        AppID = RegisterApplication(progname,
            REGAPP_URLIdentifier,       "a500.org",
            REGAPP_UniqueApplication,   TRUE,
            REGAPP_WBStartup,           argv,
            REGAPP_NoIcon,              TRUE,
            REGAPP_HasPrefsWindow,      TRUE,
            REGAPP_HasIconifyFeature,   TRUE,
            REGAPP_CanCreateNewDocs,    TRUE,
            TAG_END);
    } else {
        AppID = RegisterApplication(progname,
            REGAPP_URLIdentifier,       "a500.org",
            REGAPP_UniqueApplication,   TRUE,
            REGAPP_FileName,            argv[0],
            REGAPP_NoIcon,              TRUE,
            REGAPP_HasPrefsWindow,      TRUE,
            REGAPP_HasIconifyFeature,   TRUE,
            REGAPP_CanCreateNewDocs,    TRUE,
            TAG_END);
    }
    if (!AppID) {
        return FALSE;
    }
    Prefs = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_END);
    if (!Prefs) return FALSE;
    ReadPrefs(Prefs, READPREFS_FileName, "ENV:"GUI_PREFS_FILENAME, TAG_END);
    return TRUE;
}

void WriteProgramPrefs (BOOL env, BOOL envarc) {
    if (env) {
        WritePrefs(Prefs, WRITEPREFS_FileName, "ENV:"GUI_PREFS_FILENAME, TAG_END);
    }
    if (envarc) {
        WritePrefs(Prefs, WRITEPREFS_FileName, "ENVARC:"GUI_PREFS_FILENAME, TAG_END);
    }
}

void FreeProgramPrefs () {
    if (AppID) {
        PrefsDictionary(Prefs, NULL, ALPO_Release, 0, TAG_END);
        UnregisterApplicationA(AppID, NULL);
    }
}

void RestoreWindowSize (Object *window, const char *name) {
    if (window) {
        PrefsObject *window_prefs;
        int32 width, height;
        window_prefs = DictGetObjectForKey(Prefs, name);
        width = DictGetIntegerForKey(window_prefs, "width", 0);
        height = DictGetIntegerForKey(window_prefs, "height", 0);
        if (width && height) {
            SetAttrs(window,
                WA_Width,   width,
                WA_Height,  height,
                TAG_END);
        }
    }
}

void SaveWindowSize (Object *window, const char *name, BOOL envarc) {
    int32 width = 0, height = 0;
    if (!envarc) return;
    if (window) {
        GetAttrs(window,
            WA_Width,   &width,
            WA_Height,  &height,
            TAG_END);
    }
    if (width && height) {
        PrefsObject *window_prefs;
        window_prefs = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_END);
        if (!DictSetObjectForKey(Prefs, window_prefs, name)) {
            PrefsDictionary(window_prefs, NULL, ALPO_Release, 0, TAG_END);
            return;
        }
        DictSetObjectForKey(window_prefs, PrefsNumber(NULL, NULL,
            ALPO_Alloc, 0, ALPONUM_SetLong, width, TAG_END), "width");
        DictSetObjectForKey(window_prefs, PrefsNumber(NULL, NULL,
            ALPO_Alloc, 0, ALPONUM_SetLong, height, TAG_END), "height");
        WriteProgramPrefs(TRUE, FALSE);
        
        if (envarc) {
            PrefsObject *saved_prefs;
            saved_prefs = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_END);
            if (saved_prefs) {
                ReadPrefs(saved_prefs,
                    READPREFS_FileName, "ENVARC:"GUI_PREFS_FILENAME,
                    TAG_END);
                window_prefs = PrefsDictionary(window_prefs, NULL, ALPO_Retain, 0, TAG_END);
                DictSetObjectForKey(saved_prefs, window_prefs, name);
                WritePrefs(saved_prefs, WRITEPREFS_FileName, "ENVARC:"GUI_PREFS_FILENAME, TAG_END);
                PrefsDictionary(saved_prefs, NULL, ALPO_Release, 0, TAG_END);
            }
        }
    }
}

void RestoreSoundEditorColors (Class *cl) {
    if (cl) {
        GlobalData *gd = GLOB_DATA(cl);
        PrefsObject *gadget_prefs;
        PrefsObject *color_prefs;
        gadget_prefs = DictGetObjectForKey(Prefs, "soundeditor.gadget");
        color_prefs = DictGetObjectForKey(gadget_prefs, "colors");
        gd->Colors[COL_BKG1] = DictGetIntegerForKey(color_prefs, "bkg1", 0x000000);
        gd->Colors[COL_BKG2] = DictGetIntegerForKey(color_prefs, "bkg2", 0x5f5f5f);
        gd->Colors[COL_LINES] = DictGetIntegerForKey(color_prefs, "zerolines", 0xffffff);
        gd->Colors[COL_WAVEFORM] = DictGetIntegerForKey(color_prefs, "waveform", 0x00ff00);
        gd->Colors[COL_ALTBKG1] = DictGetIntegerForKey(color_prefs, "selbkg1", 0x7f00ff);
        gd->Colors[COL_ALTBKG2] = DictGetIntegerForKey(color_prefs, "selbkg2", 0x220044);
        gd->Colors[COL_ALTWAVEFORM] = DictGetIntegerForKey(color_prefs, "selwaveform", 0xAA66ff);
        InitGradSpec(&gd->GradSpec[0], 0, gd->Colors[COL_BKG1],
            gd->Colors[COL_BKG2]);
        InitGradSpec(&gd->GradSpec[1], 180, gd->Colors[COL_BKG1],
            gd->Colors[COL_BKG2]);
        InitGradSpec(&gd->GradSpec[2], 0, gd->Colors[COL_ALTBKG1],
            gd->Colors[COL_ALTBKG2]);
        InitGradSpec(&gd->GradSpec[3], 180, gd->Colors[COL_ALTBKG1],
            gd->Colors[COL_ALTBKG2]);
    }
}

void SaveSoundEditorColors (uint32 *colors) {
    PrefsObject *gadget_prefs;
    PrefsObject *color_prefs;
    gadget_prefs = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_END);
    if (!DictSetObjectForKey(Prefs, gadget_prefs, "soundeditor.gadget")) {
        PrefsDictionary(gadget_prefs, NULL, ALPO_Release, 0, TAG_END);
        return;
    }
    color_prefs = PrefsDictionary(NULL, NULL, ALPO_Alloc, 0, TAG_END);
    if (!DictSetObjectForKey(gadget_prefs, color_prefs, "colors")) {
        PrefsDictionary(color_prefs, NULL, ALPO_Release, 0, TAG_END);
        return;
    }
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_BKG1], TAG_END), "bkg1");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_BKG2], TAG_END), "bkg2");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_LINES], TAG_END), "zerolines");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_WAVEFORM], TAG_END), "waveform");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_ALTBKG1], TAG_END), "selbkg1");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_ALTBKG2], TAG_END), "selbkg2");
    DictSetObjectForKey(color_prefs, PrefsNumber(NULL, NULL,
        ALPO_Alloc, 0, ALPONUM_SetLong, colors[COL_ALTWAVEFORM], TAG_END), "selwaveform");
}
