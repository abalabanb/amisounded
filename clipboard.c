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

#include "sounded.h"
#include "project.h"
#include "requesters.h"
#include "debug.h"
#include "iffparse_support.h"
#include "aiff_support.h"
#include "progressbar.h"
#include <datatypes/soundclass.h>
#include <proto/iffparse.h>
#include <classes/requester.h>
#include <proto/requester.h>
#include <string.h>

#define COPY_PASTE_BUFFER 1024

#define G(o) ((struct Gadget *)(o))
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static int32 WriteSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 length, uint32 chanmask);
static int32 ReadSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 chanmask);
static int32 MixSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 length, uint32 chanmask, int32 mix_type);

BOOL CutSoundData (Project *project) {
    return CopySoundData(project) && DeleteSoundData(project);
}

BOOL CopySoundData (Project *project) {
    Object *sound = project->Sound;
    uint32 offset, length, chanmask;
    struct IFFHandle *iff;
    BOOL ret = FALSE;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    if (!GetSelection(sound, &offset, &length, &chanmask, FALSE)) {
        goto out;
    }
    iff = OpenClip(0, IFFF_WRITE);
    if (iff) {
        ret = !WriteSoundClip(project, iff, offset, length, chanmask);
        CloseClip(iff);
    }

out:
    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);

    return ret;
}

BOOL PasteSoundData (Project *project) {
    Object *sound = project->Sound;
    uint32 offset, length, chanmask;
    struct IFFHandle *iff;
    int32 error = ERROR_OBJECT_NOT_FOUND;
    Object *req;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    if (!GetSelection(sound, &offset, &length, &chanmask, TRUE)) {
        if (GetSoundLength(sound) != 0) goto out;
        offset = 0;
        chanmask = 1;
    }
    iff = OpenClip(0, IFFF_READ);
    if (iff) {
        error = ReadSoundClip(project, iff, offset, chanmask);
        CloseClip(iff);
    }

    switch (error) {
    
        case OK:
            break;
    
        case ERROR_OBJECT_NOT_FOUND:
        case ERROR_OBJECT_WRONG_TYPE:
            req = RequesterObject,
                REQ_Image,      REQIMAGE_ERROR,
                REQ_CharSet,    LocaleInfo.CodeSet,
                REQ_TitleText,  progname,
                REQ_BodyText,   GetString(&LocaleInfo, MSG_NOCLIP_REQ),
                REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
            End;
            DoReq(project->Screen, project->Window, req);
            break;
            
        default:
            ErrorRequester(project->Screen, project->Window, error, "0");
            break;

    }

out:
    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);

    return !error;
}

BOOL DeleteSoundData (Project *project) {
    Object *sound = project->Sound;
    uint32 offset, length, chanmask;
    BOOL ret = FALSE;
    
    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);
    
    if (!GetSelection(sound, &offset, &length, &chanmask, FALSE)) {
        goto out;
    }
    ret = DoGadgetMethod(G(sound), project->IWindow, NULL,
        SNDM_DELETESAMPLES, NULL, offset, length);

out:
    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
    
    return ret;
}

BOOL MixSoundData (Project *project, int32 mix_type) {
    Object *sound = project->Sound;
    uint32 offset, length, chanmask;
    struct IFFHandle *iff;
    int32 error = ERROR_OBJECT_NOT_FOUND;
    Object *req;
    
    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);
    
    if (!GetSelection(sound, &offset, &length, &chanmask, TRUE)) {
        goto out;
    }
    iff = OpenClip(0, IFFF_READ);
    if (iff) {
        error = MixSoundClip(project, iff, offset, length, chanmask, mix_type);
        CloseClip(iff);
    } else {
        error = ERROR_OBJECT_NOT_FOUND;
    }

    switch (error) {
    
        case OK:
            break;
    
        case ERROR_OBJECT_NOT_FOUND:
        case ERROR_OBJECT_WRONG_TYPE:
            req = RequesterObject,
                REQ_Image,      REQIMAGE_ERROR,
                REQ_CharSet,    LocaleInfo.CodeSet,
                REQ_TitleText,  progname,
                REQ_BodyText,   GetString(&LocaleInfo, MSG_NOCLIP_REQ),
                REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
            End;
            DoReq(project->Screen, project->Window, req);
            break;
            
        default:
            ErrorRequester(project->Screen, project->Window, error, "0");
            break;

    }

out:
    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
    
    return !error;
}

BOOL GetSelection (Object *sound, uint32 *offset, uint32 *length, uint32 *chanmask, BOOL allow_empty) {
    struct SoundSelection sel = { ~0, ~0 };
    GetAttrs(sound,
        SOUND_Selection,    &sel,
        SOUND_ChanMask,     chanmask,
        TAG_END);
    if (sel.LastSample == (uint32)~0 || *chanmask == 0) return FALSE;
    *offset = sel.FirstSample;
    *length = sel.LastSample - sel.FirstSample + 1;
    if (!allow_empty && sel.FirstSample == sel.LastSample) return FALSE;
    return TRUE;
}

static int32 WriteSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 length, uint32 chanmask)
{
    Object *sound = project->Sound;
    uint32 num_channels = 0;
    uint32 write_channels = 0;
    uint32 sample_size = 0;
    uint32 frequency = 0;
    int32 error = OK;
    const uint32 buflen = COPY_PASTE_BUFFER;
    uint32 bufsiz, getlen;
    void *buffer = NULL;
    APTR bar = NULL;
    GetAttrs(sound,
        SOUND_NumChannels,      &num_channels,
        SOUND_SampleSize,       &sample_size,
        SOUND_SamplesPerSec,    &frequency,
        TAG_END);
    if (chanmask) {
        uint32 tmp;
        for (tmp = chanmask; tmp; tmp >>= 1) {
            if (tmp & 1) write_channels++;
        }
    }
    if (num_channels == 0 || write_channels == 0 || sample_size == 0 || frequency == 0) {
        return ERROR_REQUIRED_ARG_MISSING;
    }
    if (sample_size == 1 && (write_channels == 1 || write_channels == 2)) {
        struct VoiceHeader vhdr;
        struct sndArrayIO saio;
        int32 chan, chans_written;
        uint32 mask;
        uint32 orig_offset = offset;
        uint32 orig_length = length;

        bufsiz = buflen;
        buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
        if (!buffer) return ERROR_NO_FREE_STORE;

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

        if (write_channels == 2) {
            static const SampleType sampletype = SAMPLETYPE_Stereo;

            error = PushChunk(iff, ID_8SVX, ID_CHAN, sizeof(SampleType));
            if (error) goto out;

            error = WriteChunkBytes(iff, &sampletype, sizeof(SampleType));
            if (error < 0) goto out;

            error = PopChunk(iff);
            if (error) goto out;
        }

        error = PushChunk(iff, ID_8SVX, ID_BODY, length * write_channels);
        if (error) goto out;

        saio.MethodID = SNDM_READSAMPLEARRAY;
        saio.GInfo = NULL;
        saio.ArrayPtr = buffer;
        saio.Mode = SNDRM_DEFAULT;
        saio.BytesPerSample = 1;
        saio.BytesPerFrame = 1;
        bar = CreateProgressBar(project, TRUE);
        chans_written = 0;
        for (chan = 0, mask = 1; chan < num_channels; chan++, mask <<= 1) {
            if (chanmask & mask) {
                offset = orig_offset;
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
                    SetProgressBarAttrs(bar,
                        FUELGAUGE_Percent,  TRUE,
                        FUELGAUGE_Max,      orig_length * write_channels,
                        FUELGAUGE_Level,    chans_written * orig_length + offset,
                        TAG_END);
                }
                chans_written++;
            }
        }

        error = PopChunk(iff);
        if (error) goto out;

        error = PopChunk(iff);
        if (error) goto out;
    } else {
        struct Common comm;
        struct SampledSoundHeader ssnd;
        struct sndArrayIO saio;
        uint32 frame_size;
        int32 orig_length = length;

        frame_size = sample_size * write_channels;
        bufsiz = buflen * frame_size;
        buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
        if (!buffer) return ERROR_NO_FREE_STORE;

        error = PushChunk(iff, ID_AIFF, ID_FORM, IFFSIZE_UNKNOWN);
        if (error) goto out;

        error = PushChunk(iff, ID_AIFF, ID_COMM, sizeof(comm));
        if (error) goto out;

        comm.NumChannels = write_channels;
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
        saio.ChanMask = chanmask;
        saio.Mode = SNDRM_DEFAULT;
        saio.BytesPerSample = sample_size;
        saio.BytesPerFrame = frame_size;
        bar = CreateProgressBar(project, TRUE);
        while (length > 0) {
            getlen = MIN(buflen, length);
            saio.Offset = offset;
            saio.Length = getlen;
            DoMethodA(sound, (Msg)&saio);

            error = WriteChunkRecords(iff, buffer, frame_size, getlen);
            if (error < 0) goto out;

            offset += getlen;
            length -= getlen;
            SetProgressBarAttrs(bar,
                FUELGAUGE_Percent,  TRUE,
                FUELGAUGE_Max,      orig_length,
                FUELGAUGE_Level,    offset,
                TAG_END);
        }

        error = PopChunk(iff);
        if (error) goto out;

        error = PopChunk(iff);
        if (error) goto out;
    }
out:
    DeleteProgressBar(bar);
    FreeVec(buffer);
    return error;
}

static const int32 stop_chunks[] = {
    ID_8SVX,    ID_FORM,
    ID_8SVX,    ID_VHDR,
    ID_8SVX,    ID_CHAN,
    ID_8SVX,    ID_BODY,
    ID_AIFF,    ID_FORM,
    ID_AIFF,    ID_COMM,
    ID_AIFF,    ID_SSND,
    ID_AIFC,    ID_FORM,
    ID_AIFC,    ID_FVER,
    ID_AIFC,    ID_COMM,
    ID_AIFC,    ID_SSND
};

static int32 ReadSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 chanmask)
{
    Object *sound = project->Sound;
    struct Window *window = project->IWindow;
    uint32 length = 0;
    uint32 num_channels = 0;
    uint32 write_channels = 0;
    uint32 read_channels = 1;
    uint32 sample_size = 0;
    uint32 frequency = 0;
    int32 status, error = OK;
    const uint32 buflen = COPY_PASTE_BUFFER;
    uint32 bufsiz, getlen;
    uint32 sample_length = 0;
    void *buffer = NULL;
    APTR bar = NULL;
    struct ContextNode *chunk;
    GetAttrs(sound,
        SOUND_NumChannels,      &num_channels,
        SOUND_SampleSize,       &sample_size,
        SOUND_SamplesPerSec,    &frequency,
        SOUND_Length,           &sample_length,
        TAG_END);
    if (chanmask) {
        uint32 tmp;
        for (tmp = chanmask; tmp; tmp >>= 1) {
            if (tmp & 1) write_channels++;
        }
    }
    if (num_channels == 0 || write_channels == 0 || sample_size == 0 || frequency == 0) {
        return ERROR_REQUIRED_ARG_MISSING;
    }
    if (sample_length == 0) {
        offset = 0;
    }
    error = StopChunks(iff, stop_chunks, sizeof(stop_chunks) >> 3);
    if (error) return error;
    error = ParseIFF(iff, IFFPARSE_SCAN);
    if (error) return ERROR_OBJECT_NOT_FOUND;
    chunk = CurrentChunk(iff);
    if (chunk->cn_Type == ID_8SVX && chunk->cn_ID == ID_FORM) {
        BOOL vhdr_found = FALSE;
        BOOL done = FALSE;
        struct VoiceHeader vhdr;
        SampleType sampletype;
        struct sndArrayIO saio;
        int32 chan, chans_read;
        uint32 mask;
        uint32 orig_offset = offset;
        uint32 orig_length;

        bufsiz = buflen;
        buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
        if (!buffer) {
            error = ERROR_NO_FREE_STORE;
            goto out;
        }

        saio.MethodID = SNDM_WRITESAMPLEARRAY;
        saio.GInfo = NULL;
        saio.ArrayPtr = buffer;
        saio.Mode = SNDWM_INSERT;
        saio.BytesPerSample = 1;
        saio.BytesPerFrame = 1;

        while (!done && !error && !(error = ParseIFF(iff, IFFPARSE_SCAN))) {
            chunk = CurrentChunk(iff);
            switch (chunk->cn_ID) {

                case ID_VHDR:
                    vhdr_found = TRUE;
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
                    if (vhdr.vh_Compression != CMP_NONE) {
                        error = ERROR_NOT_IMPLEMENTED;
                        break;
                    }
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
                            read_channels = 1;
                            break;

                        case SAMPLETYPE_Stereo:
                            read_channels = 2;
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
                    orig_length = chunk->cn_Size / read_channels;
                    done = TRUE;
                    if (sample_length == 0) {
                        num_channels = read_channels;
                        write_channels = read_channels;
                        chanmask = (1 << num_channels) - 1;
                        SetAttrs(sound,
                            SOUND_Length,           0,
                            SOUND_NumChannels,      num_channels,
                            SOUND_SampleSize,       1,
                            SOUND_SamplesPerSec,    vhdr.vh_SamplesPerSec,
                            TAG_END);
                    }
                    bar = CreateProgressBar(project, FALSE);
                    chans_read = 0;
                    for (chan = 0, mask = 1; chan < num_channels; chan++, mask <<= 1) {
                        if (chanmask & mask) {
                            offset = orig_offset;
                            length = orig_length;
                            while (length > 0) {
                                getlen = MIN(buflen, length);
                                status = ReadChunkBytes(iff, buffer, getlen);
                                if (status != getlen) {
                                    error = IFFErr(status);
                                    break;
                                }

                                saio.Offset = offset;
                                saio.Length = getlen;
                                saio.ChanMask = mask;
                                DoMethodA(sound, (Msg)&saio);

                                offset += getlen;
                                length -= getlen;
                                SetProgressBarAttrs(bar,
                                    FUELGAUGE_Percent,  TRUE,
                                    FUELGAUGE_Max,      orig_length * MIN(read_channels, write_channels),
                                    FUELGAUGE_Level,    chans_read * orig_length + offset,
                                    TAG_END);
                            }
                            chans_read++;
                            if (--read_channels == 0) break;
                            saio.Mode = SNDWM_REPLACE;
                        }
                        if (error) break;
                    }
                    if (sample_length == 0) {
                        DoMethod(sound, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
                    }
                    if (window) {
                        RefreshGList(G(sound), window, NULL, 1);
                    }
                    break;

            }
        }
    } else
    if ((chunk->cn_Type == ID_AIFF || chunk->cn_Type == ID_AIFC) && chunk->cn_ID == ID_FORM) {
        uint32 fver;
        uint32 comm_size = 0;
        struct ExtCommon comm;
        struct sndArrayIO saio;
        uint32 frame_size;
        BOOL done = FALSE;
        int32 orig_length;

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
                    read_channels = comm.NumChannels;
                    if (read_channels != 1 && read_channels != 2) {
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
                    error = ParseSSND(iff, buffer, bufsiz);
                    if (error) break;
                    done = TRUE;
                    if (sample_length == 0) {
                        num_channels = read_channels;
                        write_channels = read_channels;
                        chanmask = (1 << num_channels) - 1;
                        SetAttrs(sound,
                            SOUND_Length,       0,
                            SOUND_NumChannels,  num_channels,
                            SOUND_SampleSize,   sample_size,
                            SOUND_Frequency,    frequency,
                            TAG_END);
                    }
                    frame_size = sample_size * num_channels;
                    bufsiz = buflen * frame_size;
                    buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
                    if (!buffer) {
                        error = ERROR_NO_FREE_STORE;
                        break;
                    }
                    saio.MethodID = SNDM_WRITESAMPLEARRAY;
                    saio.GInfo = NULL;
                    saio.ArrayPtr = buffer;
                    saio.ChanMask = chanmask;
                    saio.Mode = SNDWM_INSERT;
                    saio.BytesPerSample = sample_size;
                    saio.BytesPerFrame = frame_size;
                    orig_length = length;
                    bar = CreateProgressBar(project, FALSE);
                    while (length > 0) {
                        getlen = MIN(buflen, length);
                        status = ReadChunkRecords(iff, buffer, frame_size, getlen);
                        if (status != getlen) {
                            error = IFFErr(status);
                            break;
                        }

                        saio.Offset = offset;
                        saio.Length = getlen;
                        DoMethodA(sound, (Msg)&saio);

                        offset += getlen;
                        length -= getlen;
                        SetProgressBarAttrs(bar,
                            FUELGAUGE_Percent,  TRUE,
                            FUELGAUGE_Max,      orig_length,
                            FUELGAUGE_Level,    offset,
                            TAG_END);
                    }
                    if (sample_length == 0) {
                        DoMethod(sound, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
                    }/* else {
                        uint32 sel[2];
                        sel[0] = offset;
                        sel[1] = orig_length;
                        SetAttrs(sound, SOUND_Selection, sel, TAG_END);
                    }*/
                    if (window) {
                        RefreshGList(G(sound), window, NULL, 1);
                        RefreshGList(G(project->HProp), window, NULL, 1);
                    }
                    break;

            }
        }
    } else {
        error = ERROR_OBJECT_WRONG_TYPE;
    }
out:
    DeleteProgressBar(bar);
    FreeVec(buffer);
    return error;
}

static int32 MixSoundClip (Project *project, struct IFFHandle *iff, uint32 offset,
    uint32 length, uint32 chanmask, int32 mix_type)
{
    Object *sound = project->Sound;
    struct Window *window = project->IWindow;
    uint32 num_channels = 0;
    uint32 write_channels = 0;
    uint32 read_channels = 1;
    uint32 sample_size = 0;
    uint32 frequency = 0;
    int32 status, error = OK;
    const uint32 buflen = COPY_PASTE_BUFFER;
    uint32 bufsiz, getlen, bufsamples, samples;
    uint32 sample_length = 0;
    void *buffer = NULL, *mix1 = NULL, *mix2 = NULL;
    uint8 *cpy_src, *cpy_dst;
    int32 *mix_src, *mix_dst;
    int64 sample;
    APTR bar = NULL;
    struct ContextNode *chunk;
    GetAttrs(sound,
        SOUND_NumChannels,      &num_channels,
        SOUND_SampleSize,       &sample_size,
        SOUND_SamplesPerSec,    &frequency,
        SOUND_Length,           &sample_length,
        TAG_END);
    if (chanmask) {
        uint32 tmp;
        for (tmp = chanmask; tmp; tmp >>= 1) {
            if (tmp & 1) write_channels++;
        }
    }
    if (num_channels == 0 || write_channels == 0 || sample_size == 0 || frequency == 0 ||
        sample_length == 0)
    {
        return ERROR_REQUIRED_ARG_MISSING;
    }
    error = StopChunks(iff, stop_chunks, sizeof(stop_chunks) >> 3);
    if (error) return error;
    error = ParseIFF(iff, IFFPARSE_SCAN);
    if (error) return ERROR_OBJECT_NOT_FOUND;
    chunk = CurrentChunk(iff);
    if (chunk->cn_Type == ID_8SVX && chunk->cn_ID == ID_FORM) {
        BOOL vhdr_found = FALSE;
        BOOL done = FALSE;
        struct VoiceHeader vhdr;
        SampleType sampletype;
        struct sndArrayIO saio;
        int32 chan, chans_read;
        uint32 mask;
        uint32 orig_offset = offset;
        uint32 orig_length;
        uint32 orig_read_length;
        uint32 read_length;

        bufsamples = buflen;

        bufsiz = buflen;
        buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
        mix1 = AllocVecTags(bufsamples << 2, AVT_Type, MEMF_PRIVATE, TAG_END);
        mix2 = AllocVecTags(bufsamples << 2, AVT_Type, MEMF_PRIVATE, TAG_END);
        if (!buffer || !mix1 || !mix2) {
            error = ERROR_NO_FREE_STORE;
            goto out;
        }

        saio.GInfo = NULL;
        saio.ArrayPtr = mix2;
        saio.Mode = SNDRM_DEFAULT;
        saio.BytesPerSample = 4;
        saio.BytesPerFrame = 4;

        while (!done && !error && !(error = ParseIFF(iff, IFFPARSE_SCAN))) {
            chunk = CurrentChunk(iff);
            switch (chunk->cn_ID) {

                case ID_VHDR:
                    vhdr_found = TRUE;
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
                    if (vhdr.vh_Compression != CMP_NONE) {
                        error = ERROR_NOT_IMPLEMENTED;
                        break;
                    }
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
                            read_channels = 1;
                            break;

                        case SAMPLETYPE_Stereo:
                            read_channels = 2;
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
                    orig_read_length = chunk->cn_Size / read_channels;
                    if (length == 0) {
                        orig_length = MIN(orig_read_length, sample_length - offset);
                    } else {
                        orig_length = MIN(read_length, length);
                    }
                    done = TRUE;
                    bar = CreateProgressBar(project, FALSE);
                    chans_read = 0;
                    for (chan = 0, mask = 1; chan < num_channels; chan++, mask <<= 1) {
                        if (chanmask & mask) {
                            offset = orig_offset;
                            length = orig_length;
                            read_length = orig_read_length;
                            
                            while (length > 0) {
                                if (chans_read < read_channels && read_length > 0) {
                                    getlen = MIN(buflen, read_length);
                                    status = ReadChunkBytes(iff, buffer, getlen);
                                    if (status != getlen) {
                                        error = IFFErr(status);
                                        break;
                                    }

                                    samples = getlen;
                                    cpy_src = buffer;
                                    cpy_dst = mix1;
                                    while (samples--) {
                                        *cpy_dst = *cpy_src++;
                                        cpy_dst += 4;
                                    }
                                } else {
                                    getlen = MIN(buflen, length);
                                    ClearMem(mix1, getlen << 2);
                                }

                                saio.MethodID = SNDM_READSAMPLEARRAY;
                                saio.Offset = offset;
                                saio.Length = getlen;
                                saio.ChanMask = mask;
                                DoMethodA(sound, (Msg)&saio);

                                samples = getlen;
                                mix_src = mix1;
                                mix_dst = mix2;
                                switch (mix_type) {

                                    case MIX_AVG:
                                        while (samples--) {
                                            sample = (int64)*mix_dst + *mix_src++;
                                            *mix_dst++ = sample >> 1;
                                        }
                                        break;
                                    
                                    case MIX_ADD:
                                        while (samples--) {
                                            sample = (int64)*mix_dst + *mix_src++;
                                            if (sample >= 0x7fffffff)
                                                *mix_dst++ = 0x7fffffff;
                                            else if (sample <= -0x80000000)
                                                *mix_dst++ = -0x80000000;
                                            else
                                                *mix_dst++ = sample;
                                        }
                                        break;
                                        
                                    case MIX_SUB:
                                        while (samples--) {
                                            sample = (int64)*mix_dst - *mix_src++;
                                            if (sample >= 0x7fffffff)
                                                *mix_dst++ = 0x7fffffff;
                                            else if (sample <= -0x80000000)
                                                *mix_dst++ = -0x80000000;
                                            else
                                                *mix_dst++ = sample;
                                        }
                                        break;
                                        
                                }

                                saio.MethodID = SNDM_WRITESAMPLEARRAY;
                                DoMethodA(sound, (Msg)&saio);

                                offset += getlen;
                                length -= getlen;
                                read_length -= getlen;
                                SetProgressBarAttrs(bar,
                                    FUELGAUGE_Percent,  TRUE,
                                    FUELGAUGE_Max,      orig_length * write_channels,
                                    FUELGAUGE_Level,    chans_read * orig_length + offset,
                                    TAG_END);
                            }
                            chans_read++;
                            saio.Mode = SNDWM_REPLACE;
                        }
                        if (error) break;
                    }
                    if (window) {
                        RefreshGList(G(sound), window, NULL, 1);
                    }
                    break;

            }
        }
    } else
    if ((chunk->cn_Type == ID_AIFF || chunk->cn_Type == ID_AIFC) && chunk->cn_ID == ID_FORM) {
        uint32 fver;
        uint32 comm_size = 0;
        struct ExtCommon comm;
        struct sndArrayIO saio;
        uint32 frame_size;
        BOOL done = FALSE;
        int32 orig_length;
        int32 read_length;
        int32 chan;
        
        bufsamples = buflen * write_channels;

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
                    read_channels = comm.NumChannels;
                    if (read_channels != 1 && read_channels != 2) {
                        error = ERROR_NOT_IMPLEMENTED;
                        break;
                    }
                    frequency = extended2long(comm.FramesPerSec);
                    read_length = comm.NumFrames;
                    if (length == 0) {
                        length = MIN(read_length, sample_length - offset);
                    } else {
                        length = MIN(read_length, length);
                    }
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
                    error = ParseSSND(iff, buffer, bufsiz);
                    if (error) break;
                    done = TRUE;
                    frame_size = sample_size * num_channels;
                    bufsiz = buflen * frame_size;
                    buffer = AllocVecTags(bufsiz, AVT_Type, MEMF_SHARED, TAG_END);
                    mix1 = AllocVecTags(bufsamples << 2, AVT_Type, MEMF_PRIVATE, TAG_END);
                    mix2 = AllocVecTags(bufsamples << 2, AVT_Type, MEMF_PRIVATE, TAG_END);
                    if (!buffer || !mix1 || !mix2) {
                        error = ERROR_NO_FREE_STORE;
                        break;
                    }
                    saio.GInfo = NULL;
                    saio.ArrayPtr = mix2;
                    saio.ChanMask = (1 << write_channels) - 1;
                    saio.Mode = SNDRM_DEFAULT;
                    saio.BytesPerSample = 4;
                    saio.BytesPerFrame = write_channels << 2;
                    bar = CreateProgressBar(project, FALSE);
                    orig_length = length;
                    while (length > 0) {
                        if (read_length > 0) {
                            getlen = MIN(buflen, read_length);
                            status = ReadChunkRecords(iff, buffer, frame_size, getlen);
                            if (status != getlen) {
                                error = IFFErr(status);
                                break;
                            }

                            samples = getlen;
                            cpy_src = buffer;
                            cpy_dst = mix1;
                            while (samples--) {
                                int32 copy = MIN(sample_size, 4);
                                int32 skip = 4 - copy;
                                for (chan = 0; chan < write_channels; chan++) {
                                    if (chan < read_channels) {
                                        memcpy(cpy_dst, cpy_src, copy);
                                        memset(cpy_dst + copy, 0, skip);
                                        cpy_src += sample_size;
                                        cpy_dst += 4;
                                    } else {
                                        *(int32 *)cpy_dst = 0;
                                        cpy_dst += 4;
                                    }
                                }
                            }
                        } else {
                            getlen = MIN(buflen, length);
                            ClearMem(mix1, (getlen * write_channels) << 2);
                        }

                        saio.MethodID = SNDM_READSAMPLEARRAY;
                        saio.Offset = offset;
                        saio.Length = getlen;
                        DoMethodA(sound, (Msg)&saio);
                        
                        samples = getlen * write_channels;
                        mix_src = mix1;
                        mix_dst = mix2;
                        switch (mix_type) {

                            case MIX_AVG:
                                while (samples--) {
                                    sample = (int64)*mix_dst + *mix_src++;
                                    *mix_dst++ = sample >> 1;
                                }
                                break;
                                    
                            case MIX_ADD:
                                while (samples--) {
                                    sample = (int64)*mix_dst + *mix_src++;
                                    if (sample >= 0x7fffffff)
                                        *mix_dst++ = 0x7fffffff;
                                    else if (sample <= -0x80000000)
                                        *mix_dst++ = -0x80000000;
                                    else
                                        *mix_dst++ = sample;
                                }
                                break;
                                        
                            case MIX_SUB:
                                while (samples--) {
                                    sample = (int64)*mix_dst - *mix_src++;
                                    if (sample >= 0x7fffffff)
                                        *mix_dst++ = 0x7fffffff;
                                    else if (sample <= -0x80000000)
                                        *mix_dst++ = -0x80000000;
                                    else
                                        *mix_dst++ = sample;
                                }
                                break;
                                        
                        }

                        saio.MethodID = SNDM_WRITESAMPLEARRAY;
                        DoMethodA(sound, (Msg)&saio);

                        offset += getlen;
                        length -= getlen;
                        read_length -= getlen;
                        SetProgressBarAttrs(bar,
                            FUELGAUGE_Percent,  TRUE,
                            FUELGAUGE_Max,      orig_length,
                            FUELGAUGE_Level,    offset,
                            TAG_END);
                    }
                    /*{
                        uint32 sel[2];
                        sel[0] = offset;
                        sel[1] = orig_length;
                        SetAttrs(sound, SOUND_Selection, sel, TAG_END);
                    }*/
                    if (window) {
                        RefreshGList(G(sound), window, NULL, 1);
                        RefreshGList(G(project->HProp), window, NULL, 1);
                    }
                    break;

            }
        }
    } else {
        error = ERROR_OBJECT_WRONG_TYPE;
    }
out:
    DeleteProgressBar(bar);
    FreeVec(buffer);
    FreeVec(mix1);
    FreeVec(mix2);
    return error;
}
