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

#ifndef PLUGINS_H
#define PLUGINS_H

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef DATATYPES_DATATYPES_H
#include <datatypes/datatypes.h>
#endif

#ifndef SOUNDEDITOR_GC_H
#include "soundeditor_gc.h"
#endif

#ifndef GADGETS_FUELGAUGE_H
#include <gadgets/fuelgauge.h>
#endif

#include <stdarg.h>

struct SoundEDIFace {
    uint32 Version;
    uint32 Revision;
    struct Screen *(*GetProjectScreen)(APTR project);
    struct Window *(*GetProjectWindow)(APTR project);
    APTR (*CreateProgressBar)(APTR project, BOOL save);
    void (*DeleteProgressBar)(APTR bar);
    void VARARGS68K (*SetProgressBarAttrs)(APTR bar, ...);
};

struct PluginData {
    struct SoundEDIFace *ISoundED;
    struct ExecIFace *IExec;
    struct DOSIFace *IDOS;
    struct UtilityIFace *IUtility;
    struct IntuitionIFace *IIntuition;
    struct IFFParseIFace *IIFFParse;
};

#define LOADPLUGIN_MAGIC MAKE_ID('S','N','D','L')
#ifdef PLUGIN_HAS_SAVE_OPTIONS
#define PLUGIN_API_VERSION 3
#else
#define PLUGIN_API_VERSION 2
#endif

#define PLUGIN_FLAG_BUILTIN (0x80000000UL)

struct LoadPlugin {
    struct Node Link;
    uint16  TestSize;
    uint32  Magic;
    int32   API_Version;
    uint32  Flags;

    BPTR SegList;
    struct PluginData *Data;

    BOOL APICALL (*Init)(struct LoadPlugin *Self);
    void APICALL (*Exit)(struct LoadPlugin *Self);
    BOOL APICALL (*IsOurFile)(struct LoadPlugin *Self, BPTR file,
        const char *filename, const uint8 *test);
    int32 APICALL (*LoadSound)(struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
    int32 APICALL (*SaveSound)(struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
#ifdef PLUGIN_HAS_SAVE_OPTIONS
    Object **APICALL (*GetSaveOptions)(struct LoadPlugin *Self, APTR project, Object *sound);
    void APICALL (*FreeSaveOptions)(struct LoadPlugin *Self, Object **opts);
#endif
};

#ifndef OK
#define OK 0
#endif

#define MAX_CHANNELS 2

#define LOAD_SAVE_BUFFER 1024
#define LOAD_SAVE_BUFFER_SIZE 4096

#define ReadError(status) (((int32)(status) == -1) ? IoErr() : DTERROR_NOT_ENOUGH_DATA)

#endif
