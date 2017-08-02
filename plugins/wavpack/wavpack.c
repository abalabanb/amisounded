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

#include "plugins.h"
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <wavpack/wavpack.h>
#include "WavPack_rev.h"

static const char USED verstag[] = VERSTAG;

#define G(o) ((struct Gadget *)(o))
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static BOOL Init (struct LoadPlugin *Self);
static void Exit (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "WavPack" },
    4,
    LOADPLUGIN_MAGIC,
    PLUGIN_API_VERSION,
    0,
    ZERO,
    NULL,
    Init,
    Exit,
    IsOurFile,
    LoadSound,
    SaveSound
};

int _start () {
    return (int)&plugin;
}

struct SoundEDIFace *ISoundED;
struct ExecIFace *IExec;
struct DOSIFace *IDOS;
struct IntuitionIFace *IIntuition;
struct Library *NewlibBase;
struct Interface *INewlib;

static BOOL Init (struct LoadPlugin *Self) {
    struct PluginData *data = Self->Data;
    ISoundED = data->ISoundED;
    IExec = data->IExec;
    IDOS = data->IDOS;
    IIntuition = data->IIntuition;
    NewlibBase = OpenLibrary("newlib.library", 52);
    INewlib = GetInterface(NewlibBase, "main", 1, NULL);
    if (!INewlib) {
        return FALSE;
    }
    return TRUE;
}

static void Exit (struct LoadPlugin *Self) {
    DropInterface(INewlib);
    INewlib = NULL;
    CloseLibrary(NewlibBase);
    NewlibBase = NULL;
}

#define WV_MAGIC MAKE_ID('w','v','p','k')

static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test)
{
    return *(uint32 *)test == WV_MAGIC;
}

static int32_t ReadBytes (void *id, void *data, int32_t len) {
    BPTR file = (BPTR)id;
    int32_t res;
    res = FRead(file, data, 1, len);
    return res == -1 ? 0 : res;
}

static uint32_t GetPos (void *id) {
    BPTR file = (BPTR)id;
    uint64_t res;
    res = GetFilePosition(file);
    return (res > 0x7fffffff) ? -1 : res;
}

static int SetPosAbs (void *id, uint32_t pos) {
    BPTR file = (BPTR)id;
    return !ChangeFilePosition(file, pos, OFFSET_BEGINNING);
}

static int SetPosRel (void *id, int32_t delta, int mode) {
    BPTR file = (BPTR)id;
    switch (mode) {
        case SEEK_SET: mode = OFFSET_BEGINNING; break;
        case SEEK_CUR: mode = OFFSET_CURRENT; break;
        case SEEK_END: mode = OFFSET_END; break;
        default: return -1;
    }
    return !ChangeFilePosition(file, delta, mode);
}

static int PushBackByte (void *id, int c) {
    BPTR file = (BPTR)id;
    return UnGetC(file, c) ? c : EOF;
}

static uint32_t GetLength (void *id) {
    BPTR file = (BPTR)id;
    uint64_t res;
    if (!file) return 0;
    res = GetFileSize(file);
    return (res > 0x7fffffff) ? 0 : res;
}

static int CanSeek (void *id) {
    return TRUE;
}

static WavpackStreamReader wpc_reader = {
    ReadBytes,
    GetPos,
    SetPosAbs,
    SetPosRel,
    PushBackByte,
    GetLength,
    CanSeek,
    NULL
};

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    int32 offset, orig_length, length, num_channels, sample_size, frequency, mode;
    WavpackContext *wpc = NULL;
    char error_str[80];
    void *buffer = NULL;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz;
    int32 frame_size, getlen;
    int32 status, error = OK;
    APTR bar = NULL;
    struct sndArrayIO saio;
    int32 update;
    
    window = ISoundED->GetProjectWindow(project);
    
    wpc = WavpackOpenFileInputEx(&wpc_reader, (APTR)file, NULL, error_str, OPEN_2CH_MAX|OPEN_NORMALIZE, 0);
    if (!wpc) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    
    num_channels = WavpackGetReducedChannels(wpc);
    frequency = WavpackGetSampleRate(wpc);
    sample_size = WavpackGetBytesPerSample(wpc);
    length = WavpackGetNumSamples(wpc);
    mode = WavpackGetMode(wpc);

    frame_size = num_channels << 2;
    bufsiz = buflen * frame_size;
    buffer = AllocVec(bufsiz, MEMF_PRIVATE);
    if (!buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    SetIoErr(0);
    saio.MethodID = SNDM_WRITESAMPLEARRAY;
    saio.GInfo = NULL;
    if (length == -1) {
        SetGadgetAttrs(G(sound), window, NULL,
            SOUND_Length,       0,
            SOUND_NumChannels,  num_channels,
            SOUND_SampleSize,   sample_size,
            SOUND_Frequency,    frequency,
            TAG_END);
        saio.Mode = SNDWM_INSERT;
    } else {
        SetGadgetAttrs(G(sound), window, NULL,
            SOUND_Length,       length,
            SOUND_NumChannels,  num_channels,
            SOUND_SampleSize,   sample_size,
            SOUND_Frequency,    frequency,
            TAG_END);
        bar = ISoundED->CreateProgressBar(project, TRUE);
        saio.Mode = SNDWM_REPLACE;
    }
    saio.ChanMask = (1 << num_channels)-1;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = frame_size;
    offset = 0;
    orig_length = length;
    update = 0;
    while (length == -1 || length > 0) {
        if (length == -1) {
            getlen = WavpackUnpackSamples(wpc, buffer, buflen);
            if (getlen == 0) {
                error = IoErr();
                break;
            }
        } else {
            getlen = MIN(buflen, length);
            status = WavpackUnpackSamples(wpc, buffer, getlen);
            if (status != getlen) {
                error = IoErr();
                break;
            }
        }
        
        if (mode & MODE_FLOAT) {
            float32 *src = buffer;
            int32 *dst = buffer;
            int32 count = getlen * num_channels;
            while (count--) {
                if (*src >= 1.0)
                    *dst++ = 2147483647;
                else if (*src <= -1.0)
                    *dst++ = -2147483648;
                else
                    *dst++ = floor(*src * 2147483648.0);
                src++;
            }
        } else if (sample_size < 4) {
            int32 shift = (4 - sample_size) << 3;
            int32 *ptr = buffer;
            int32 count = getlen * num_channels;
            while (count--) {
                *ptr++ <<= shift;
            }
        }
        
        saio.ArrayPtr = buffer;
        saio.Offset = offset;
        saio.Length = getlen;
        offset += getlen;
        if (length == -1) {
            DoMethodA(sound, (Msg)&saio);
            if (++update == 512) {
                DoGadgetMethod(G(sound), window, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
                update = 0;
            }
        } else {
            length -= getlen;
            DoGadgetMethodA(G(sound), window, NULL, (Msg)&saio);
            ISoundED->SetProgressBarAttrs(bar,
                FUELGAUGE_Percent,  TRUE,
                FUELGAUGE_Max,      orig_length,
                FUELGAUGE_Level,    offset,
                TAG_END);
        }
    }
    if (window && length != -1) {
        RefreshGList(G(sound), window, NULL, 1);
    }
    
out:
    ISoundED->DeleteProgressBar(bar);
    if (wpc) WavpackCloseFile(wpc);
    FreeVec(buffer);
    Close(file);
    
    return error;
}

static int BlockOutput (APTR id, void *data, int32_t len) {
    BPTR file = (BPTR)id;
    if (FWrite(file, data, 1, len) != len) {
        return FALSE;
    }
    return TRUE;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    int32 offset, orig_length, length, num_channels, sample_size, frequency;
    WavpackContext *wpc = NULL;
    int32 error = OK;
    WavpackConfig config;
    void *buffer = NULL;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz;
    int32 frame_size, getlen;
    APTR bar = NULL;
    struct sndArrayIO saio;

    GetAttrs(sound,
        SOUND_Length,       &length,
        SOUND_NumChannels,  &num_channels,
        SOUND_SampleSize,   &sample_size,
        SOUND_Frequency,    &frequency,
        TAG_END);
    if (length == 0 || num_channels == 0 || sample_size == 0 || frequency == 0) {
        error = ERROR_REQUIRED_ARG_MISSING;
        goto out;
    }
    if (num_channels > 2) {
        error = ERROR_NOT_IMPLEMENTED;
        goto out;
    }

    frame_size = num_channels << 2;
    bufsiz = buflen * frame_size;
    buffer = AllocVec(bufsiz, MEMF_PRIVATE);
    if (!buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    wpc = WavpackOpenFileOutput(BlockOutput, (APTR)file, NULL);
    if (!wpc) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    
    memset(&config, 0, sizeof(config));
    config.bits_per_sample = sample_size << 3;
    config.bytes_per_sample = sample_size;
    config.channel_mask = (num_channels == 1) ? 4 : 3;
    config.num_channels = num_channels;
    config.sample_rate = frequency;
    if (!WavpackSetConfiguration(wpc, &config, length) || !WavpackPackInit(wpc)) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    
    SetIoErr(0);
    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ChanMask = (1 << num_channels)-1;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = frame_size;
    offset = 0;
    orig_length = length;
    bar = ISoundED->CreateProgressBar(project, TRUE);
    while (length > 0) {
        getlen = MIN(buflen, length);
        saio.ArrayPtr = buffer;
        saio.Offset = offset;
        saio.Length = getlen;
        DoMethodA(sound, (Msg)&saio);
        
        if (sample_size < 4) {
            uint32 shift = (4 - sample_size) << 3;
            int32 *ptr = buffer;
            int32 count = getlen * num_channels;
            while (count--) {
                *ptr++ >>= shift;
            }
        }
        
        if (!WavpackPackSamples(wpc, buffer, getlen)) {
            error = IoErr();
            goto out;
        }
        
        offset += getlen;
        length -= getlen;
        ISoundED->SetProgressBarAttrs(bar,
            FUELGAUGE_Percent,  TRUE,
            FUELGAUGE_Max,      orig_length,
            FUELGAUGE_Level,    offset,
            TAG_END);
    }
    if (!WavpackFlushSamples(wpc)) {
        error = IoErr();
        goto out;
    }
    
out:
    ISoundED->DeleteProgressBar(bar);
    if (wpc) WavpackCloseFile(wpc);
    FreeVec(buffer);
    Close(file);

    return error;
}
