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
#include "progressbar.h"

BOOL LoadSound (Project *project, const char *filename) {
    struct Window *window = project->IWindow;
    Object *sound = project->Sound;
    struct List *plugins = project->Params->Plugins;
    BPTR file;
    int32 error = OK;
    struct LoadPlugin *plugin = NULL;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    file = Open(filename, MODE_OLDFILE);
    if (file) {
        plugin = FindLoader(plugins, file, filename);
        if (plugin) {
            error = plugin->LoadSound(project, sound, file);
        } else {
            APTR bar;
            Close(file);
            bar = CreateProgressBar(project, FALSE);
            SetProgressBarAttrs(bar,
                FUELGAUGE_Percent,  FALSE,
                GA_Text,            GetString(&LocaleInfo, MSG_LOADINGUSINGDT),
                TAG_END);
            error = LoadSoundDT(window, sound, filename);
            DeleteProgressBar(bar);
        }
        SetAttrs(sound, SOUND_HasChanged, FALSE, TAG_END);
    } else {
        error = IoErr();
    }
    if (error) {
        ErrorRequester(project->Screen, project->Window, error, filename);
    }
    if (plugin && plugin->SaveSound) {
        project->SavePlugin = plugin;
    } else {
        project->SavePlugin = NULL;
    }

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
    Project_UpdateGUI(project);
    return !error;
}

uint32 GetSoundLength (Object *sound) {
    uint32 length = 0;
    GetAttr(SOUND_Length, sound, &length);
    return length;
}
