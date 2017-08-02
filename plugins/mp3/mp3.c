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
#include <datatypes/datatypes.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <mad.h>
#include <string.h>
#include "riff-wave.h"
#include "lame/lame.h"
#include "MP3_rev.h"

static const char USED verstag[] = VERSTAG;

#define INPUT_BUFFER_SIZE 8192
#define OUTPUT_FRAMES 2048

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
    { NULL, NULL, 0, 1, "MP3" },
    22,
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
struct UtilityIFace *IUtility;
struct IntuitionIFace *IIntuition;
struct Library *NewlibBase;
struct Interface *INewlib;
static struct SignalSemaphore *LoadLock;
static struct SignalSemaphore *SaveLock;

static BOOL Init (struct LoadPlugin *Self) {
    struct PluginData *data = Self->Data;
    ISoundED = data->ISoundED;
    IExec = data->IExec;
    IDOS = data->IDOS;
    IUtility = data->IUtility;
    IIntuition = data->IIntuition;
    NewlibBase = OpenLibrary("newlib.library", 52);
    INewlib = GetInterface(NewlibBase, "main", 1, NULL);
    LoadLock = AllocSysObject(ASOT_SEMAPHORE, NULL);
    SaveLock = AllocSysObject(ASOT_SEMAPHORE, NULL);
    if (!INewlib || !LoadLock || !SaveLock) {
        return FALSE;
    }
    return TRUE;
}

static void Exit (struct LoadPlugin *Self) {
    FreeSysObject(ASOT_SEMAPHORE, LoadLock);
    FreeSysObject(ASOT_SEMAPHORE, SaveLock);
    SaveLock = NULL;
    DropInterface(INewlib);
    INewlib = NULL;
    CloseLibrary(NewlibBase);
    NewlibBase = NULL;
}

static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test)
{
    const char *extension;
    extension = strrchr(filename, '.');
    if (extension && !Stricmp(extension+1, "mp3")) {
        return TRUE;
    }
    if (*(uint32 *)&test[0] == ID_RIFF &&
        *(uint32 *)&test[8] == ID_WAVE &&
        *(uint32 *)&test[12] == ID_fmt &&
        *(uint16 *)&test[20] == WAVE_FORMAT_MPEGLAYER3)
    {
        return TRUE;
    }
    return FALSE;
}

#define ID3_TAG_FLAG_FOOTERPRESENT 0x10

int tagtype (const unsigned char *data, int length) {
    if (length >= 3 && data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
        return 128; /* ID3V1 */

    if (length >= 10 &&
        (data[0] == 'I' && data[1] == 'D' && data[2] == '3') &&
        data[3] < 0xff && data[4] < 0xff &&
        data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
    {     /* ID3V2 */
        unsigned char flags;
        unsigned int size;
        flags = data[5];
        size = (data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9];
        if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
            size += 10;
        return 10 + size;
    }

    return 0;
}

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    struct mad_stream *stream = NULL;
    struct mad_frame *frame = NULL;
    struct mad_synth *synth = NULL;
    mad_timer_t *timer = NULL;
    uint8 *inbuffer = NULL;
    int32 *outbuffer = NULL, *dst;
    int32 status, error = OK;
    int32 tagsize, left;
    BOOL more_data, eof;
    int32 num_channels, chan;
    int32 offset, getlen, i;
    int32 frame_size;
    const uint32 buflen = OUTPUT_FRAMES;
    uint32 bufsiz;
    mad_fixed_t sample;
    struct sndArrayIO saio;
    int32 update;

    ObtainSemaphore(LoadLock);

    window = ISoundED->GetProjectWindow(project);

    stream = AllocVec(sizeof(*stream), MEMF_CLEAR|MEMF_PRIVATE);
    frame = AllocVec(sizeof(*frame), MEMF_CLEAR|MEMF_PRIVATE);
    synth = AllocVec(sizeof(*synth), MEMF_CLEAR|MEMF_PRIVATE);
    timer = AllocVec(sizeof(*timer), MEMF_CLEAR|MEMF_PRIVATE);
    inbuffer = AllocVec(INPUT_BUFFER_SIZE, MEMF_CLEAR|MEMF_SHARED);
    if (!stream || !frame || !synth || !timer || !inbuffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);
    mad_timer_reset(timer);

    do {
        more_data = FALSE;

        status = FRead(file, inbuffer, 1, INPUT_BUFFER_SIZE);
        if (status <= 0) {
            error = ReadError(status);
            goto out;
        }

        mad_stream_buffer(stream, inbuffer, status);
        stream->error = 0;

        while (mad_frame_decode(frame, stream)) {
            left = stream->bufend - stream->this_frame;
            if (left <= 8) {
                MoveMem((APTR)stream->this_frame, inbuffer, left);
                status = FRead(file, inbuffer + left, 1, INPUT_BUFFER_SIZE - left);
                if (status <= 0) {
                    error = ReadError(status);
                    goto out;
                }
                left += status;
                mad_stream_buffer(stream, inbuffer, left);
                stream->error = 0;
            }
            tagsize = MAX(tagtype(stream->this_frame, left), 1);
            if (tagsize > left) {
                tagsize -= left;
                while (tagsize > 0) {
                    if (tagsize < INPUT_BUFFER_SIZE) {
                        status = FRead(file, inbuffer, 1, tagsize);
                    } else {
                        status = FRead(file, inbuffer, 1, INPUT_BUFFER_SIZE);
                    }
                    if (status <= 0) {
                        error = ReadError(status);
                        goto out;
                    }
                    tagsize -= status;
                }
                more_data = TRUE;
                break;
            }
            mad_stream_skip(stream, tagsize);
        }
    } while (more_data);

    switch (frame->header.mode) {
        case MAD_MODE_SINGLE_CHANNEL:
        case MAD_MODE_DUAL_CHANNEL:
        case MAD_MODE_JOINT_STEREO:
        case MAD_MODE_STEREO:
            num_channels = MAD_NCHANNELS(&frame->header);
            break;
        default:
            error = DTERROR_INVALID_DATA;
            goto out;
    }

    frame_size = num_channels << 2;
    bufsiz = buflen * frame_size;
    outbuffer = AllocVec(bufsiz, MEMF_PRIVATE);
    if (!outbuffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    SetGadgetAttrs(G(sound), window, NULL,
        SOUND_Length,       0,
        SOUND_NumChannels,  num_channels,
        SOUND_SampleSize,   4,
        SOUND_Frequency,    frame->header.samplerate,
        TAG_END);

    mad_timer_add(timer, frame->header.duration);
    mad_synth_frame(synth, frame);
    offset = 0;
    eof = FALSE;

    saio.MethodID = SNDM_WRITESAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = outbuffer;
    saio.Offset = 0;
    saio.ChanMask = (1 << num_channels) - 1;
    saio.Mode = SNDWM_INSERT;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = frame_size;
    update = 0;
    for (;;) {
        getlen = MIN(buflen, synth->pcm.length - offset);
        dst = outbuffer;
        for (i = 0; i < getlen; i++) {
            for (chan = 0; chan < num_channels; chan++) {
                sample = synth->pcm.samples[chan][offset];
                if (sample < -MAD_F_ONE)
                    sample = -MAD_F_ONE;
                else if (sample >= MAD_F_ONE)
                    sample = MAD_F_ONE-1;
                *dst++ = sample << (31 - MAD_F_FRACBITS);
            }
            offset++;
        }

        saio.Length = getlen;
        DoMethodA(sound, (Msg)&saio);
        saio.Offset += getlen;
        if (++update == 512) {
            DoGadgetMethod(G(sound), window, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
            update = 0;
        }

        if (/*!getlen ||*/ eof) break;

        if (stream->error == MAD_ERROR_BUFLEN) {
            left = stream->bufend - stream->next_frame;
            MoveMem((APTR)stream->next_frame, inbuffer, left);
            status = FRead(file, inbuffer + left, 1, INPUT_BUFFER_SIZE - left);
            if (status < 0) {
                error = IoErr();
                break;
            }
            if (status == 0) {
                eof = TRUE;
                ClearMem(inbuffer + left, MAD_BUFFER_GUARD);
                status = MAD_BUFFER_GUARD;
            }
            mad_stream_buffer(stream, inbuffer, status + left);
            stream->error = 0;
        }
        if (mad_frame_decode(frame, stream)) {
            if (MAD_RECOVERABLE(stream->error)) {
                tagsize = tagtype(stream->this_frame, stream->bufend - stream->this_frame);
                if (tagsize == 0) {
                    if (!eof) {
                        DebugPrintF("MP3: recoverable frame error - %s\n",
                            mad_stream_errorstr(stream));
                    }
                } else {
                    mad_stream_skip(stream, tagsize);
                }
                continue;
            } else {
                if (stream->error == MAD_ERROR_BUFLEN) {
                    continue;
                } else {
                    DebugPrintF("MP3: unrecoverable frame error - %s\n",
                            mad_stream_errorstr(stream));
                    error = DTERROR_INVALID_DATA;
                    break;
                }
            }
        }
        mad_timer_add(timer, frame->header.duration);
        mad_synth_frame(synth, frame);
        offset = 0;
    }
    DoGadgetMethod(G(sound), window, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);

out:
    FreeVec(stream);
    FreeVec(frame);
    FreeVec(synth);
    FreeVec(timer);
    FreeVec(inbuffer);
    FreeVec(outbuffer);
    Close(file);

    ReleaseSemaphore(LoadLock);

    return error;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    int32 offset, orig_length, length, num_channels, frequency;
    lame_global_flags *lame = NULL;
    int32 written, status, error = OK;
    void *buffer[MAX_CHANNELS] = { NULL, NULL };
    void *mp3buffer = NULL;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    const uint32 bufsiz = LOAD_SAVE_BUFFER << 2;
    const uint32 mp3buffer_size = (LOAD_SAVE_BUFFER*5/4) + 7200;
    int32 getlen;
    APTR bar = NULL;
    struct sndArrayIO saio;

    ObtainSemaphore(SaveLock);

    GetAttrs(sound,
        SOUND_Length,       &length,
        SOUND_NumChannels,  &num_channels,
        SOUND_Frequency,    &frequency,
        TAG_END);
    if (length == 0 || num_channels == 0 || frequency == 0) {
        error = ERROR_REQUIRED_ARG_MISSING;
        goto out;
    }
    if (num_channels > 2) {
        error = ERROR_NOT_IMPLEMENTED;
        goto out;
    }

    lame = lame_init();
    if (!lame) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    if (lame_set_num_channels(lame, num_channels) < 0) {
        error = ERROR_BAD_NUMBER;
        goto out;
    }
    lame_set_in_samplerate(lame, frequency);
    lame_set_bWriteVbrTag(lame, 0);

    if (lame_init_params(lame) < 0) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    buffer[0] = AllocVec(bufsiz, MEMF_PRIVATE);
    buffer[1] = AllocVec(bufsiz, MEMF_PRIVATE);
    mp3buffer = AllocVec(mp3buffer_size, MEMF_SHARED);
    if (!buffer[0] || !buffer[1] || !mp3buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    if (num_channels == 1) {
        ClearMem(buffer[1], bufsiz);
    }

    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = 4;
    offset = 0;
    orig_length = length;
    bar = ISoundED->CreateProgressBar(project, TRUE);
    while (length > 0) {
        getlen = MIN(buflen, length);
        saio.ArrayPtr = buffer[0];
        saio.Offset = offset;
        saio.Length = getlen;
        saio.ChanMask = 0x01;
        DoMethodA(sound, (Msg)&saio);
        if (num_channels == 2) {
            saio.ArrayPtr = buffer[1];
            saio.ChanMask = 0x02;
            DoMethodA(sound, (Msg)&saio);
        }

        written = lame_encode_buffer_long2(lame, buffer[0], buffer[1],
            getlen, mp3buffer, mp3buffer_size);
        if (written < 0) {
            error = ERROR_NO_FREE_STORE;
            goto out;
        }
        status = FWrite(file, mp3buffer, 1, written);
        if (status != written) {
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

    written = lame_encode_flush(lame, mp3buffer, 7200);
    if (written < 0) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    status = FWrite(file, mp3buffer, 1, written);
    if (status != written) {
        error = IoErr();
        goto out;
    }

out:
    ISoundED->DeleteProgressBar(bar);
    FreeVec(buffer[0]);
    FreeVec(buffer[1]);
    FreeVec(mp3buffer);
    lame_close(lame);
    Close(file);

    ReleaseSemaphore(SaveLock);

    return error;
}
