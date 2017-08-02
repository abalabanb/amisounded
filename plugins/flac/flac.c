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
#include <proto/intuition.h>
#include <FLAC/all.h>
#include <stdlib.h>
#include "FLAC_rev.h"

static const char USED verstag[] = VERSTAG;

#define G(o) ((struct Gadget *)(o))
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define FLAC_MAGIC MAKE_ID('f','L','a','C')

static BOOL Init (struct LoadPlugin *Self);
static void Exit (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "FLAC" },
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
static struct SignalSemaphore *LoadLock;
static struct SignalSemaphore *SaveLock;

static BOOL Init (struct LoadPlugin *Self) {
    struct PluginData *data = Self->Data;
    ISoundED = data->ISoundED;
    IExec = data->IExec;
    IDOS = data->IDOS;
    IIntuition = data->IIntuition;
    NewlibBase = OpenLibrary("newlib.library", 52);
    INewlib = GetInterface(NewlibBase, "main", 1, NULL);
    if (!INewlib) return FALSE;
    return TRUE;
}

static void Exit (struct LoadPlugin *Self) {
    DropInterface(INewlib);
    INewlib = NULL;
    CloseLibrary(NewlibBase);
    NewlibBase = NULL;
}

static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test)
{
    return *(uint32 *)test == FLAC_MAGIC;
}

struct client_data {
    BPTR FileHandle;
    uint32 BitsPerSample;
    uint32 NumChannels;
    uint32 Frequency;
    uint32 TotalSamples;
    const FLAC__int32 * const *Buffer;
    uint32 DecodedSamples;
};

static FLAC_StreamDecoderReadStatus flac_decoder_read (const FLAC__StreamDecoder *decoder,
    FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    struct client_data *data = client_data;
    BPTR file = data->FileHandle;
    if (*bytes > 0) {
        *bytes = FRead(file, buffer, sizeof(FLAC__byte), *bytes);
        if (*bytes > 0)
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
        else if (*bytes == 0)
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        else
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

static FLAC__StreamDecoderSeekStatus flac_decoder_seek (const FLAC__StreamDecoder *decoder,
    FLAC__uint64 absolute_byte_offset, void *client_data)
{
    struct client_data *data = client_data;
    BPTR file = data->FileHandle;
    if (!ChangeFilePosition(file, absolute_byte_offset, OFFSET_BEGINNING))
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    else
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus flac_decoder_tell (const FLAC__StreamDecoder *decoder,
    FLAC__uint64 *absolute_byte_offset, void *client_data)
{
    struct client_data *data = client_data;
    BPTR file = data->FileHandle;
    int64 pos;
    pos = GetFilePosition(file);
    if (pos == -1 && IoErr())
        return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    else {
        *absolute_byte_offset = pos;
        return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }
}

static FLAC__StreamDecoderLengthStatus flac_decoder_length (const FLAC__StreamDecoder *decoder,
    FLAC__uint64 *stream_length, void *client_data)
{
    struct client_data *data = client_data;
    BPTR file = data->FileHandle;
    int64 size;
    size = GetFileSize(file);
    if (size == -1 && IoErr())
        return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    else {
        *stream_length = size;
        return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }
}

static FLAC__bool flac_decoder_eof (const FLAC__StreamDecoder *decoder, void *client_data) {
    struct client_data *data = client_data;
    BPTR file = data->FileHandle;
    int byte;
    UnGetC(file, byte = FGetC(file));
    return (byte == -1) ? true : false;
}

static FLAC__StreamDecoderWriteStatus flac_decoder_write (const FLAC__StreamDecoder *decoder,
    const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    struct client_data *data = client_data;
    if (frame->header.bits_per_sample != data->BitsPerSample ||
        frame->header.channels != data->NumChannels ||
        frame->header.sample_rate != data->Frequency)
    {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    data->Buffer = buffer;
    data->DecodedSamples = frame->header.blocksize;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_decoder_error (const FLAC__StreamDecoder *decoder,
    FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    // no-op
}

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    FLAC__StreamDecoder *decoder = NULL;
    int32 error = OK;
    struct sndArrayIO saio;
    
    window = ISoundED->GetProjectWindow(project);
    
    decoder = FLAC__stream_decoder_new();
    if (!decoder) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }
    
out:
    if (decoder) FLAC__stream_decoder_delete(decoder);
    IDOS->Close(file);
    
    return error;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    int32 error = OK;
    struct sndArrayIO saio;
    APTR bar = NULL;
    
out:
    ISoundED->DeleteProgressBar(bar);
    IDOS->Close(file);
    
    return error;
}
