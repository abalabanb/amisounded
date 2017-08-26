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

#include "plugins_private.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <stdlib.h>
#include "debug.h"

#include <dos/obsolete.h>

extern struct SoundEDIFace ISoundED;
struct PluginData PluginData;

static struct LoadPlugin * const builtin_loaders[] = { NULL };

const char **output_formats;
struct LoadPlugin **output_plugins;

static void InitPlugin (struct List *list, struct LoadPlugin *plugin, BPTR seglist);

static int plugincmp (const void *arg1, const void *arg2) {
    return Stricmp((*(struct Node **)arg1)->ln_Name, (*(struct Node **)arg2)->ln_Name);
}

void LoadPlugins (struct List *list) {
    PluginData.ISoundED = &ISoundED;
    PluginData.IExec = IExec;
    PluginData.IDOS = IDOS;
    PluginData.IUtility = IUtility;
    PluginData.IIntuition = IIntuition;
    PluginData.IIFFParse = IIFFParse;

    TEST_SIZE(list) = 0;

    /* Built-in plugins */
    {
        struct LoadPlugin * const *ptr;

        ptr = builtin_loaders;
        while (*ptr) {
            InitPlugin(list, *ptr++, ZERO);
        }
    }

    /* Dynamically loaded plugins */
    {
        BPTR dir;
        void *buffer;
        struct ExAllControl *eac;
        struct ExAllData *ead;
        struct LoadPlugin *ptr;
        int32 more;
        BPTR seglist;

        buffer = AllocVecTags(1024, AVT_Type, MEMF_SHARED, TAG_END);
        dir = Lock("PROGDIR:Plugins", ACCESS_READ);
        eac = AllocDosObject(DOS_EXALLCONTROL, NULL);

        if (buffer && dir && eac) {
            dbug(("Scanning...\n"));
            do {
                more = ExAll(dir, buffer, 1024, ED_TYPE, eac);
                if (!more && IoErr() != ERROR_NO_MORE_ENTRIES) {
                    break;
                }
                if (eac->eac_Entries == 0) {
                    dbug(("eac_Entries == 0\n"));
                    continue;
                }
                for (ead = buffer; ead; ead = ead->ed_Next) {
                    if (!EAD_IS_FILE(ead)) {
                        dbug(("(DIR) %s\n", ead->ed_Name));
                        continue;
                    }
                    dbug(("%s\n", ead->ed_Name));
                    dir = CurrentDir(dir);
                    seglist = LoadSeg(ead->ed_Name);
                    dir = CurrentDir(dir);
                    InitPlugin(list, NULL, seglist);
                }
            } while (more);
        }

        FreeDosObject(DOS_EXALLCONTROL, eac);
        UnLock(dir);
        FreeVec(buffer);
    }

    {
        struct LoadPlugin *plugin;
        int32 num_savers = 0;
        const void **src, **dst;

        plugin = (APTR)GetHead(list);
        while (plugin) {
            if (plugin->SaveSound) {
                num_savers++;
            }
            plugin = (APTR)GetSucc((APTR)plugin);
        }

        output_plugins = AllocVecTags((num_savers+1) << 2, AVT_Type, MEMF_SHARED, TAG_END);
        output_formats = AllocVecTags((num_savers+1) << 2, AVT_Type, MEMF_SHARED, TAG_END);
        if (!output_plugins || !output_formats) return;
        dst = (const void **)output_plugins;
        plugin = (APTR)GetHead(list);
        while (plugin) {
            if (plugin->SaveSound) {
                *dst++ = plugin;
            }
            plugin = (APTR)GetSucc((APTR)plugin);
        }
        *dst = NULL;
        src = (const void **)output_plugins;
        qsort(src, num_savers, 4, plugincmp);
        dst = (const void **)output_formats;
        while (*src) {
            *dst++ = (*(struct Node **)src++)->ln_Name;
        }
        *dst = NULL;
    }
}

void FreePlugins (struct List *list) {
    struct LoadPlugin *plugin;
    if (!list) return;

    FreeVec(output_plugins);
    FreeVec(output_formats);
    output_plugins = NULL;
    output_formats = NULL;

    while (plugin = (APTR)RemHead(list)) {
        if (plugin->Exit) plugin->Exit();
        if (!(plugin->Flags & PLUGIN_FLAG_BUILTIN)) {
            UnLoadSeg(plugin->SegList);
        }
    }
}

static void InitPlugin (struct List *list, struct LoadPlugin *plugin, BPTR seglist) {
    if (!plugin && seglist) {
        plugin = (APTR)RunCommand(seglist, 4096, "", 0);
    }
    if (!plugin || !TypeOfMem(plugin) ||
        plugin->Magic != LOADPLUGIN_MAGIC ||
        plugin->API_Version < 2 || plugin->API_Version > 3)
    {
        UnLoadSeg(seglist);
        return;
    }
    plugin->SegList = seglist;
    if (!seglist) plugin->Flags |= PLUGIN_FLAG_BUILTIN;
    plugin->Data = &PluginData;
    if (!plugin->Init || plugin->Init()) {
        Enqueue(list, &plugin->Link);
        if (plugin->TestSize > TEST_SIZE(list)) {
            TEST_SIZE(list) = plugin->TestSize;
        }
        return;
    }
    if (plugin->Exit) plugin->Exit();
    UnLoadSeg(seglist);
}

void TrashMem (uint8 *buffer, uint32 size) {
    uint32 longs = size >> 2;
    uint32 bytes = size & 3;
    while (longs--) {
        *(uint32 *)buffer = 0xDEADBEEF;
        buffer += 4;
    }
    switch (bytes) {
        case 3:
            *buffer++ = 0xDE;
            *buffer++ = 0xAD;
            *buffer++ = 0xBE;
            break;
        case 2:
            *buffer++ = 0xDE;
            *buffer++ = 0xAD;
            break;
        case 1:
            *buffer++ = 0xDE;
            break;
    }
}

struct LoadPlugin *FindLoader (struct List *list, BPTR file, const char *filename) {
    struct LoadPlugin *plugin;
    uint32 test_size = TEST_SIZE(list);
    void *test = NULL;
    if (test_size > 0) {
        test = AllocVecTags(test_size, AVT_Type, MEMF_SHARED, TAG_END);
        if (!test) return NULL;
        TrashMem(test, test_size);
        Read(file, test, test_size);
        if (!ChangeFilePosition(file, 0, OFFSET_BEGINNING)) {
            FreeVec(test);
            return NULL;
        }
    }
    plugin = (APTR)GetHead(list);
    while (plugin) {
        if (plugin->IsOurFile && plugin->IsOurFile(file, filename, test)) {
            FreeVec(test);
            return plugin;
        }
        plugin = (APTR)GetSucc(&plugin->Link);
    }
    FreeVec(test);
    return NULL;
}
