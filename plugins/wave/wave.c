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
#include "riff-wave.h"
#include "wave_codecs.h"
#include <datatypes/datatypes.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

static BOOL Init (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "RIFF-WAVE" },
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

static BOOL Init (struct LoadPlugin *Self) {
    struct PluginData *data = Self->Data;
    ISoundED = data->ISoundED;
    IExec = data->IExec;
    IDOS = data->IDOS;
    IIntuition = data->IIntuition;
    return TRUE;
}

static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test)
{
    uint32 id = *(uint32 *)&test[0];
    uint32 type = *(uint32 *)&test[8];
    return id == ID_RIFF && type == ID_WAVE;
}

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    struct RIFFHeader riff;
    struct RIFFChunk chunk;
    struct WaveFormatEx *fmt = NULL;
    const struct WaveCodec *codec = NULL;
    struct WaveCodecData data = {0};
    int32 status, error = OK;
    BOOL done = FALSE;
    int32 offset, orig_length, length;
    int32 num_blocks, outbuffer_length;
    uint32 inbuffer_size, outbuffer_size;
    void *inbuffer = NULL, *outbuffer = NULL;
    int32 blocks_left, frames_left;
    int32 read_blocks, read_frames;
    APTR bar;
    struct sndArrayIO saio;

    window = ISoundED->GetProjectWindow(project);

    status = FRead(file, &riff, 1, sizeof(riff));
    if (status != sizeof(riff)) {
        error = ReadError(status);
        goto out;
    }
    if (riff.ID != ID_RIFF || riff.Type != ID_WAVE) {
        error = ERROR_OBJECT_WRONG_TYPE;
        goto out;
    }

    length = 0;
    while (!done && !error) {
        status = FRead(file, &chunk, 1, sizeof(chunk));
        if (status != sizeof(chunk)) {
            error = ReadError(status);
            break;
        }
        write_le32(&chunk.Size, chunk.Size);
        switch (chunk.ID) {

            case ID_fmt:
                if (chunk.Size != sizeof(struct WaveFormat) &&
                    chunk.Size < sizeof(struct WaveFormatEx))
                {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                fmt = AllocVec(MAX(chunk.Size, 18), MEMF_CLEAR|MEMF_SHARED);
                data.Format = fmt;
                if (!fmt) {
                    error = ERROR_NO_FREE_STORE;
                    break;
                }
                status = FRead(file, fmt, 1, chunk.Size);
                if (status != chunk.Size) {
                    error = ReadError(status);
                    break;
                }
                write_le16(&fmt->NumChannels, fmt->NumChannels);
                write_le32(&fmt->SamplesPerSec, fmt->SamplesPerSec);
                write_le16(&fmt->BlockAlign, fmt->BlockAlign);
                write_le16(&fmt->BitsPerSample, fmt->BitsPerSample);
                if (chunk.Size >= sizeof(struct WaveFormatEx)) {
                    fmt->ExtraSize = chunk.Size - sizeof(struct WaveFormatEx);
                } else {
                    fmt->ExtraSize = 0;
                }
                data.Chunk = chunk;
                codec = GetCodec(fmt->FormatTag);
                data.Codec = codec;
                if (codec) {
                    error = codec->Init(&data);
                    if (error) break;
                } else {
                    error = DTERROR_UNKNOWN_COMPRESSION;
                    break;
                }
                if (fmt->NumChannels != 1 && fmt->NumChannels != 2) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                if (fmt->SamplesPerSec == 0) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                num_blocks = LOAD_SAVE_BUFFER_SIZE / fmt->BlockAlign;
                if (num_blocks == 0) num_blocks++;
                inbuffer_size = num_blocks * fmt->BlockAlign;
                outbuffer_length = num_blocks * data.BlockFrames;
                outbuffer_size = outbuffer_length * fmt->NumChannels * data.SampleSize;
                inbuffer = AllocVec(inbuffer_size, MEMF_SHARED);
                outbuffer = AllocVec(outbuffer_size, MEMF_PRIVATE);
                if (!inbuffer || !outbuffer) {
                    error = ERROR_NO_FREE_STORE;
                    break;
                }
                break;

            case ID_fact:
                if (chunk.Size != sizeof(length)) {
                    error = ERROR_BAD_NUMBER;
                    break;
                }
                status = FRead(file, &length, 1, sizeof(length));
                if (status != sizeof(length)) {
                    error = ReadError(status);
                    break;
                }
                write_le32(&length, length);
                break;

            case ID_data:
                if (!fmt) {
                    error = DTERROR_NOT_ENOUGH_DATA;
                    break;
                }
                if (fmt->BlockAlign && data.BlockFrames) {
                    int32 max_length;
                    max_length = (chunk.Size / fmt->BlockAlign) * data.BlockFrames;
                    if (length <= 0 || max_length > (length + data.BlockFrames) ||
                        length > max_length)
                    {
                        length = max_length;
                    }
                }
                done = TRUE;
                SetGadgetAttrs(G(sound), window, NULL,
                    SOUND_Length,       length,
                    SOUND_NumChannels,  fmt->NumChannels,
                    SOUND_SampleSize,   data.SampleSize,
                    SOUND_Frequency,    fmt->SamplesPerSec,
                    TAG_END);
                saio.MethodID = SNDM_WRITESAMPLEARRAY;
                saio.GInfo = NULL;
                saio.ArrayPtr = outbuffer;
                saio.ChanMask = (1 << fmt->NumChannels) - 1;
                saio.Mode = SNDWM_REPLACE;
                saio.BytesPerSample = data.SampleSize;
                saio.BytesPerFrame = fmt->NumChannels * data.SampleSize;
                offset = 0;
                frames_left = length;
                read_frames = outbuffer_length;
                blocks_left = chunk.Size / fmt->BlockAlign;
                read_blocks = num_blocks;
                orig_length = length;
                bar = ISoundED->CreateProgressBar(project, FALSE);
                while (blocks_left > 0) {
                    if (blocks_left < read_blocks) {
                        read_blocks = blocks_left;
                    }
                    if (frames_left < read_frames) {
                        read_frames = frames_left;
                    }

                    status = FRead(file, inbuffer, fmt->BlockAlign, read_blocks);
                    if (status != read_blocks) {
                        error = ReadError(status);
                        break;
                    }
                    status = codec->Decode(&data, inbuffer, outbuffer, read_blocks, read_frames);
                    if (status != read_frames) {
                        error = DTERROR_INVALID_DATA;
                        break;
                    }

                    saio.Offset = offset;
                    saio.Length = read_frames;
                    DoGadgetMethodA(G(sound), window, NULL, (Msg)&saio);

                    offset += read_frames;
                    blocks_left -= read_blocks;
                    frames_left -= read_frames;
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

            default:
                if (!ChangeFilePosition(file, (chunk.Size+1)&~1, OFFSET_CURRENT)) {
                    error = IoErr();
                    break;
                }
                break;

        }
    }

out:
    if (codec && codec->Exit) codec->Exit(&data);
    FreeVec(inbuffer);
    FreeVec(outbuffer);
    FreeVec(fmt);
    Close(file);

    return error;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct RIFFHeader riff;
    struct RIFFChunk chunk;
    struct WaveFormat fmt;
    int32 orig_length, length, frame_size, offset;
    int32 num_channels, frequency, sample_size;
    int32 error = OK;
    const uint32 buflen = LOAD_SAVE_BUFFER;
    uint32 bufsiz, getlen, samples;
    uint8 *buffer = NULL, *ptr, tmp;
    APTR bar;
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

    sample_size = MIN(sample_size, 4);
    frame_size = sample_size * num_channels;
    bufsiz = buflen * frame_size;
    buffer = AllocVec(bufsiz, MEMF_SHARED);
    if (!buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    riff.ID = ID_RIFF;
    write_le32(&riff.Size, 20 + sizeof(fmt) + length * frame_size);
    riff.Type = ID_WAVE;
    if (FWrite(file, &riff, 1, sizeof(riff)) != sizeof(riff)) {
        error = IoErr();
        goto out;
    }

    chunk.ID = ID_fmt;
    write_le32(&chunk.Size, sizeof(fmt));
    fmt.FormatTag = WAVE_FORMAT_PCM;
    write_le16(&fmt.NumChannels, num_channels);
    write_le32(&fmt.SamplesPerSec, frequency);
    write_le32(&fmt.AvgBytesPerSec, frequency * frame_size);
    write_le16(&fmt.BlockAlign, frame_size);
    write_le16(&fmt.BitsPerSample, sample_size << 3);
    if (FWrite(file, &chunk, 1, sizeof(chunk)) != sizeof(chunk) ||
        FWrite(file, &fmt, 1, sizeof(fmt)) != sizeof(fmt))
    {
        error = IoErr();
        goto out;
    }

    chunk.ID = ID_data;
    write_le32(&chunk.Size, length * frame_size);
    if (FWrite(file, &chunk, 1, sizeof(chunk)) != sizeof(chunk)) {
        error = IoErr();
        goto out;
    }

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

        ptr = buffer;
        samples = getlen * num_channels;
        switch (sample_size) {
            case 1:
                while (samples--) {
                    *ptr++ ^= 0x80;
                }
                break;
            case 2:
                while (samples--) {
                    write_le16(ptr, *(uint16 *)ptr);
                    ptr += 2;
                }
                break;
            case 3:
                while (samples--) {
                    tmp = ptr[0];
                    ptr[0] = ptr[2];
                    ptr[2] = tmp;
                    ptr += 3;
                }
                break;
            case 4:
                while (samples--) {
                    write_le32(ptr, *(uint32 *)ptr);
                    ptr += 4;
                }
                break;
        }

        if (FWrite(file, buffer, frame_size, getlen) != getlen) {
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
    ISoundED->DeleteProgressBar(bar);

out:
    FreeVec(buffer);
    Close(file);

    return error;
}
