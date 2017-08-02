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
#include "iffparse_support.h"
#include "aiff_support.h"
#include <datatypes/datatypes.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

static BOOL Init (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "IFF-AIFF" },
    12,
    LOADPLUGIN_MAGIC,
    PLUGIN_API_VERSION,
    0,
    ZERO,
    NULL,
    Init,
    NULL,
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
struct IFFParseIFace *IIFFParse;

static BOOL Init (struct LoadPlugin *Self) {
    struct PluginData *data = Self->Data;
    ISoundED = data->ISoundED;
    IExec = data->IExec;
    IDOS = data->IDOS;
    IIntuition = data->IIntuition;
    IIFFParse = data->IIFFParse;
    return TRUE;
}

static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test)
{
    uint32 id = *(uint32 *)&test[0];
    uint32 type = *(uint32 *)&test[8];
    return id == ID_FORM && (type == ID_AIFF || type == ID_AIFC);
}

static const uint32 stop_chunks[] = {
    ID_AIFF,    ID_COMM,
    ID_AIFF,    ID_SSND,
    ID_AIFC,    ID_FVER,
    ID_AIFC,    ID_COMM,
    ID_AIFC,    ID_SSND
};

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    struct IFFHandle *iff;
    struct ContextNode *chunk;
    uint32 fver;
    uint32 comm_size = 0;
    struct ExtCommon comm;
    int32 orig_length, length, frame_size, offset;
    int32 num_channels, frequency, sample_size;
    int32 status, error = OK;
    BOOL done = FALSE;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz, getlen;
    void *buffer = NULL;
    APTR bar;
    struct sndArrayIO saio;

    window = ISoundED->GetProjectWindow(project);

    iff = OpenIFF_File(file, IFFF_READ);
    if (!iff) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    error = StopChunks(iff, stop_chunks, sizeof(stop_chunks) >> 3);
    if (error) goto out;
    while (!done && !error && !(error = ParseIFF(iff, IFFPARSE_SCAN))) {
        chunk = CurrentChunk(iff);
        switch (chunk->cn_ID) {

            case ID_FVER:
                if (chunk->cn_Size != 4) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                status = ReadChunkBytes(iff, &fver, sizeof(fver));
                if (status != sizeof(fver)) {
                    error = IFFErr(status);
                    break;
                }
                if (fver != AIFCVersion1) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                break;

            case ID_COMM:
                comm.CompType = COMP_NONE;
                comm_size = (chunk->cn_Type == ID_AIFF) ?
                    sizeof(struct Common) : sizeof(struct ExtCommon);
                if (chunk->cn_Size != comm_size) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                status = ReadChunkBytes(iff, &comm, comm_size);
                if (status != comm_size) {
                    error = IFFErr(status);
                    break;
                }
                if (comm.CompType != COMP_NONE) {
                    error = DTERROR_UNKNOWN_COMPRESSION;
                    break;
                }
                num_channels = comm.NumChannels;
                if (num_channels != 1 && num_channels != 2) {
                    error = ERROR_NOT_IMPLEMENTED;
                    break;
                }
                frequency = extended2long(comm.FramesPerSec);
                length = comm.NumFrames;
                sample_size = (comm.BitsPerSample + 7) >> 3;
                if (frequency == 0 || length == 0 || sample_size == 0) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                break;

            case ID_SSND:
                if (!comm_size) {
                    error = IFFERR_MANGLED;
                    break;
                }
                frame_size = sample_size * num_channels;
                bufsiz = buflen * frame_size;
                buffer = AllocVec(bufsiz, MEMF_SHARED);
                if (!buffer) {
                    error = ERROR_NO_FREE_STORE;
                    break;
                }
                error = ParseSSND(iff, buffer, bufsiz);
                if (error) break;
                done = TRUE;
                SetGadgetAttrs(G(sound), window, NULL,
                    SOUND_Length,       length,
                    SOUND_NumChannels,  num_channels,
                    SOUND_SampleSize,   sample_size,
                    SOUND_Frequency,    frequency,
                    TAG_END);
                saio.MethodID = SNDM_WRITESAMPLEARRAY;
                saio.GInfo = NULL;
                saio.ArrayPtr = buffer;
                saio.ChanMask = (1 << num_channels) - 1;
                saio.Mode = SNDWM_REPLACE;
                saio.BytesPerSample = sample_size;
                saio.BytesPerFrame = frame_size;
                offset = 0;
                orig_length = length;
                bar = ISoundED->CreateProgressBar(project, FALSE);
                while (length > 0) {
                    getlen = MIN(buflen, length);
                    status = ReadChunkRecords(iff, buffer, frame_size, getlen);
                    if (status != getlen) {
                        error = IFFErr(status);
                        break;
                    }

                    saio.Offset = offset;
                    saio.Length = getlen;
                    DoGadgetMethodA(G(sound), window, NULL, (Msg)&saio);

                    offset += getlen;
                    length -= getlen;
                    ISoundED->SetProgressBarAttrs(bar,
                        FUELGAUGE_Percent,  TRUE,
                        FUELGAUGE_Max,      orig_length,
                        FUELGAUGE_Level,    offset,
                        TAG_END);
                }
                ISoundED->DeleteProgressBar(bar);
                if (window) {
                    RefreshGList(G(sound), window, NULL, 1);
                }
                break;

        }
    }

out:
    FreeVec(buffer);
    CloseIFF_File(iff);
    Close(file);

    return error;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct IFFHandle *iff;
    struct Common comm;
    struct SampledSoundHeader ssnd;
    int32 orig_length, length, frame_size, offset;
    int32 num_channels, frequency, sample_size;
    int32 error = OK;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz, getlen;
    void *buffer = NULL;
    APTR bar;
    struct sndArrayIO saio;

    iff = OpenIFF_File(file, IFFF_WRITE);
    if (!iff) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

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

    frame_size = sample_size * num_channels;
    bufsiz = buflen * frame_size;
    buffer = AllocVec(bufsiz, MEMF_SHARED);
    if (!buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    error = PushChunk(iff, ID_AIFF, ID_FORM, IFFSIZE_UNKNOWN);
    if (error) goto out;

    error = PushChunk(iff, ID_AIFF, ID_COMM, sizeof(comm));
    if (error) goto out;

    comm.NumChannels = num_channels;
    comm.NumFrames = length;
    comm.BitsPerSample = sample_size << 3;
    long2extended(frequency, comm.FramesPerSec);
    error = WriteChunkBytes(iff, &comm, sizeof(comm));
    if (error < 0) goto out;

    error = PopChunk(iff);
    if (error) goto out;

    error = PushChunk(iff, ID_AIFF, ID_SSND, sizeof(ssnd) + length * frame_size);
    if (error) goto out;

    ssnd.DataOffset = 0;
    ssnd.BlockSize = 0;
    error = WriteChunkBytes(iff, &ssnd, sizeof(ssnd));
    if (error < 0) goto out;

    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = buffer;
    saio.ChanMask = (1 << num_channels) - 1;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = sample_size;
    saio.BytesPerFrame = frame_size;
    offset = 0;
    orig_length = length;
    bar = ISoundED->CreateProgressBar(project, TRUE);
    while (length > 0) {
        getlen = MIN(buflen, length);
        saio.Offset = offset;
        saio.Length = getlen;
        DoMethodA(sound, (Msg)&saio);

        error = WriteChunkRecords(iff, buffer, frame_size, getlen);
        if (error < 0) goto out;

        offset += getlen;
        length -= getlen;
        ISoundED->SetProgressBarAttrs(bar,
            FUELGAUGE_Percent,  TRUE,
            FUELGAUGE_Max,      orig_length,
            FUELGAUGE_Level,    offset,
            TAG_END);
    }
    ISoundED->DeleteProgressBar(bar);

    error = PopChunk(iff);
    if (error) goto out;

    error = PopChunk(iff);
    if (error) goto out;

out:
    FreeVec(buffer);
    CloseIFF_File(iff);
    Close(file);

    return error;
}
