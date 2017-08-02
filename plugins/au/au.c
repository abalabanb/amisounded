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
#include "au_format.h"
#include "au_codecs.h"
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
    { NULL, NULL, 0, 0, "Sun AU" },
    4,
    LOADPLUGIN_MAGIC,
    PLUGIN_API_VERSION,
    0,
    ZERO,
    NULL,
    Init,
    NULL,
    IsOurFile,
    LoadSound,
    NULL
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
    return *(uint32 *)test == AU_MAGIC;
}

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    struct AUHeader header;
    const struct AUCodec *codec = NULL;
    struct AUCodecData data = {0};
    int32 status, error = OK;
    int32 offset, length;
    int32 num_blocks, outbuffer_length;
    uint32 inbuffer_size, outbuffer_size;
    void *inbuffer = NULL, *outbuffer = NULL;
    int32 blocks_left, frames_left;
    int32 read_blocks, read_frames;
    APTR bar = NULL;
    struct sndArrayIO saio;
    uint64 filesize;
    
    window = ISoundED->GetProjectWindow(project);
    
    status = FRead(file, &header, 1, sizeof(header));
    if (status != sizeof(header)) {
        error = ReadError(status);
        goto out;
    }
    if (header.Magic != AU_MAGIC) {
        error = ERROR_OBJECT_WRONG_TYPE;
        goto out;
    }
    if (header.Encoding == AU_FMT_FRAGMENT) {
        error = ERROR_NOT_IMPLEMENTED;
        goto out;
    }
    
    if (header.DataOffset < sizeof(header)) {
        header.DataOffset = sizeof(header);
    }
    filesize = GetFileSize(file);
    if (filesize != (uint64)-1) {
        if (filesize > 0x7fffffff) {
            error = ERROR_OBJECT_TOO_LARGE;
            goto out;
        }
        header.DataSize = filesize - header.DataOffset;
    }
    
    if (header.DataSize == 0) {
        error = ERROR_BAD_NUMBER;
        goto out;
    }
    if (header.NumChannels != 1 && header.NumChannels != 2) {
        error = ERROR_BAD_NUMBER;
        goto out;
    }
    if (header.SampleRate == 0) {
        error = ERROR_BAD_NUMBER;
        goto out;
    }
    
    data.Header = AllocVec(header.DataOffset, MEMF_SHARED);
    if (!data.Header) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    CopyMem(&header, data.Header, sizeof(header));
    if (header.DataOffset > sizeof(header)) {
        int32 extra_size = header.DataOffset - sizeof(header);
        status = FRead(file, &data.Header[1], 1, extra_size);
        if (status != extra_size) {
            error = ReadError(status);
            goto out;
        }
    }
    
    codec = GetCodec(header.Encoding);
    data.Codec = codec;
    if (!codec) {
        error = DTERROR_UNKNOWN_COMPRESSION;
        goto out;
    }
    
    error = codec->Init(&data);
    if (error) goto out;
    
    length = header.DataSize / data.BlockSize * data.BlockFrames;
    num_blocks = LOAD_SAVE_BUFFER_SIZE / data.BlockSize;
    if (num_blocks == 0) num_blocks++;
    inbuffer_size = MIN(num_blocks * data.BlockSize, header.DataSize);
    outbuffer_length = num_blocks * data.BlockFrames;
    outbuffer_size = outbuffer_length * header.NumChannels * data.SampleSize;
    inbuffer = AllocVec(inbuffer_size, MEMF_SHARED);
    outbuffer = AllocVec(outbuffer_size, MEMF_PRIVATE);
    if (!inbuffer || !outbuffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    
    SetGadgetAttrs(G(sound), window, NULL,
        SOUND_Length,       length,
        SOUND_NumChannels,  header.NumChannels,
        SOUND_SampleSize,   data.SampleSize,
        SOUND_Frequency,    header.SampleRate,
        TAG_END);
    saio.MethodID = SNDM_WRITESAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = outbuffer;
    saio.ChanMask = (1 << header.NumChannels) - 1;
    saio.Mode = SNDWM_REPLACE;
    saio.BytesPerSample = data.SampleSize;
    saio.BytesPerFrame = header.NumChannels * data.SampleSize;
    offset = 0;
    frames_left = length;
    blocks_left = header.DataSize / data.BlockSize;
    read_blocks = num_blocks;
    bar = ISoundED->CreateProgressBar(project, FALSE);
    while (blocks_left > 0) {
        if (blocks_left < read_blocks) {
            read_blocks = blocks_left;
        }
        if (frames_left < read_frames) {
            read_frames = frames_left;
        }
        
        status = FRead(file, inbuffer, data.BlockSize, read_blocks);
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
            FUELGAUGE_Max,      length,
            FUELGAUGE_Level,    offset,
            TAG_END);
    }
    ISoundED->DeleteProgressBar(bar);
    if (window) {
        RefreshGList(G(sound), window, NULL, 1);
    }
    
out:
    if (codec && codec->Exit) codec->Exit(&data);
    FreeVec(inbuffer);
    FreeVec(outbuffer);
    FreeVec(data.Header);
    Close(file);

    return error;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    return ERROR_NOT_IMPLEMENTED;
}
