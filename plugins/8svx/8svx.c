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
#include <datatypes/datatypes.h>
#include <datatypes/soundclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>

#ifndef CMP_EXPDELTA
#define CMP_EXPDELTA 2
#endif

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

static BOOL Init (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "IFF-8SVX" },
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
    return id == ID_FORM && type == ID_8SVX;
}

static const uint32 stop_chunks[] = {
    ID_8SVX,    ID_VHDR,
    ID_8SVX,    ID_CHAN,
    ID_8SVX,    ID_BODY
};

static const uint8 FibCompTab[16] = {
    -34, -21, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 21
};

static const uint8 ExpCompTab[16] = {
    -128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64
};

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    struct IFFHandle *iff;
    struct ContextNode *chunk;
    BOOL vhdr_found = FALSE;
    struct VoiceHeader vhdr;
    SampleType sampletype;
    int32 length, offset;
    int32 num_channels;
    int32 orig_length;
    int32 status, error = OK;
    BOOL done = FALSE;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz, getlen;
    uint8 *buffer = NULL, *buf2, *src, *dst;
    int32 chan, bytes;
    uint32 mask;
    const uint8 *comptab;
    uint8 tmp, sample;
    APTR bar;
    struct sndArrayIO saio;

    window = ISoundED->GetProjectWindow(project);

    num_channels = 1;
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

            case ID_VHDR:
                if (chunk->cn_Size != sizeof(vhdr)) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                status = ReadChunkBytes(iff, &vhdr, sizeof(vhdr));
                if (status != sizeof(vhdr)) {
                    error = IFFErr(status);
                    break;
                }
                if (vhdr.vh_SamplesPerSec == 0) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                switch (vhdr.vh_Compression) {

                    case CMP_NONE:
                        break;

                    case CMP_FIBDELTA:
                        comptab = FibCompTab;
                        break;

                    case CMP_EXPDELTA:
                        comptab = ExpCompTab;
                        break;

                    default:
                        error = DTERROR_UNKNOWN_COMPRESSION;
                        break;

                }
                vhdr_found = TRUE;
                break;

            case ID_CHAN:
                if (chunk->cn_Size != sizeof(SampleType)) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                status = ReadChunkBytes(iff, &sampletype, sizeof(SampleType));
                if (status != sizeof(SampleType)) {
                    error = IFFErr(status);
                    break;
                }
                switch (sampletype) {

                    case SAMPLETYPE_Left:
                    case SAMPLETYPE_Right:
                        num_channels = 1;
                        break;

                    case SAMPLETYPE_Stereo:
                        num_channels = 2;
                        break;

                    default:
                        error = ERROR_BAD_NUMBER;
                        break;

                }
                break;

            case ID_BODY:
                if (!vhdr_found) {
                    error = IFFERR_MANGLED;
                    break;
                }
                orig_length = chunk->cn_Size / num_channels;
                bufsiz = buflen;
                if (vhdr.vh_Compression != CMP_NONE) {
                    orig_length = ((orig_length - 2) << 1) + 1;
                }
                buffer = AllocVec(bufsiz, MEMF_SHARED);
                if (!buffer) {
                    error = ERROR_NO_FREE_STORE;
                    break;
                }
                buf2 = buffer + (bufsiz >> 1);
                done = TRUE;
                SetGadgetAttrs(G(sound), window, NULL,
                    SOUND_Length,       orig_length,
                    SOUND_NumChannels,  num_channels,
                    SOUND_SampleSize,   1,
                    SOUND_Frequency,    vhdr.vh_SamplesPerSec,
                    TAG_END);
                saio.MethodID = SNDM_WRITESAMPLEARRAY;
                saio.GInfo = NULL;
                saio.ArrayPtr = buffer;
                saio.Mode = SNDWM_REPLACE;
                saio.BytesPerSample = 1;
                saio.BytesPerFrame = 1;
                bar = ISoundED->CreateProgressBar(project, FALSE);
                for (chan = 0, mask = 1; chan < num_channels; chan++, mask <<= 1) {
                    offset = 0;
                    length = orig_length;
                    if (vhdr.vh_Compression != CMP_NONE) {
                        status = ReadChunkBytes(iff, buffer, 2);
                        if (status != 2) {
                            error = IFFErr(status);
                            break;
                        }
                        sample = buffer[1];

                        buffer[0] = sample;

                        saio.Offset = offset;
                        saio.Length = 1;
                        saio.ChanMask = mask;
                        DoMethodA(sound, (Msg)&saio);
                        offset++;
                        length--;
                    }
                    while (length > 0) {
                        getlen = MIN(buflen, length);
                        if (vhdr.vh_Compression == CMP_NONE) {
                            status = ReadChunkBytes(iff, buffer, getlen);
                            if (status != getlen) {
                                error = IFFErr(status);
                                break;
                            }
                        } else {
                            bytes = getlen >> 1;
                            status = ReadChunkBytes(iff, buf2, bytes);
                            if (status != bytes) {
                                error = IFFErr(status);
                                break;
                            }
                            src = buf2;
                            dst = buffer;
                            while (bytes--) {
                                tmp = *src++;
                                sample += comptab[tmp >> 4];
                                *dst++ = sample;
                                sample += comptab[tmp & 15];
                                *dst++ = sample;
                            }
                        }

                        saio.Offset = offset;
                        saio.Length = getlen;
                        saio.ChanMask = mask;
                        DoGadgetMethodA(G(sound), window, NULL, (Msg)&saio);

                        offset += getlen;
                        length -= getlen;
                        ISoundED->SetProgressBarAttrs(bar,
                            FUELGAUGE_Percent,  TRUE,
                            FUELGAUGE_Max,      orig_length * num_channels,
                            FUELGAUGE_Level,    chan * orig_length + offset,
                            TAG_END);
                    }
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
    struct VoiceHeader vhdr;
    int32 orig_length, length, offset;
    int32 num_channels, frequency;
    int32 error = OK;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz, getlen;
    int32 chan;
    uint32 mask;
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
        SOUND_Frequency,    &frequency,
        TAG_END);
    if (length == 0 || num_channels == 0 || frequency == 0) {
        error = ERROR_REQUIRED_ARG_MISSING;
        goto out;
    }

    bufsiz = buflen;
    buffer = AllocVec(bufsiz, MEMF_SHARED);
    if (!buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    error = PushChunk(iff, ID_8SVX, ID_FORM, IFFSIZE_UNKNOWN);
    if (error) goto out;

    error = PushChunk(iff, ID_8SVX, ID_VHDR, sizeof(vhdr));
    if (error) goto out;

    vhdr.vh_OneShotHiSamples = length;
    vhdr.vh_RepeatHiSamples = 0;
    vhdr.vh_SamplesPerHiCycle = 0;
    vhdr.vh_SamplesPerSec = frequency;
    vhdr.vh_Octaves = 0;
    vhdr.vh_Compression = 0;
    vhdr.vh_Volume = 0x10000;
    error = WriteChunkBytes(iff, &vhdr, sizeof(vhdr));
    if (error < 0) goto out;

    error = PopChunk(iff);
    if (error) goto out;

    if (num_channels == 2) {
        static const SampleType sampletype = SAMPLETYPE_Stereo;

        error = PushChunk(iff, ID_8SVX, ID_CHAN, sizeof(SampleType));
        if (error) goto out;

        error = WriteChunkBytes(iff, &sampletype, sizeof(SampleType));
        if (error < 0) goto out;

        error = PopChunk(iff);
        if (error) goto out;
    }

    error = PushChunk(iff, ID_8SVX, ID_BODY, length * num_channels);
    if (error) goto out;

    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = buffer;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = 1;
    saio.BytesPerFrame = 1;
    orig_length = length;
    bar = ISoundED->CreateProgressBar(project, TRUE);
    for (chan = 0, mask = 1; chan < num_channels; chan++, mask <<= 1) {
        offset = 0;
        length = orig_length;
        while (length > 0) {
            getlen = MIN(buflen, length);
            saio.Offset = offset;
            saio.Length = getlen;
            saio.ChanMask = mask;
            DoMethodA(sound, (Msg)&saio);

            error = WriteChunkBytes(iff, buffer, getlen);
            if (error < 0) goto out;

            offset += getlen;
            length -= getlen;
            ISoundED->SetProgressBarAttrs(bar,
                FUELGAUGE_Percent,  TRUE,
                FUELGAUGE_Max,      orig_length * num_channels,
                FUELGAUGE_Level,    chan * orig_length + offset,
                TAG_END);
        }
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
