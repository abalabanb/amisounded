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
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <stdlib.h>
#include "OGG_rev.h"

static const char USED verstag[] = VERSTAG;

#define G(o) ((struct Gadget *)(o))
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define OGG_MAGIC MAKE_ID('O','g','g','S')
#define INPUT_BUFFER_SIZE 8192

static BOOL Init (struct LoadPlugin *Self);
static void Exit (struct LoadPlugin *Self);
static BOOL IsOurFile (struct LoadPlugin *Self, BPTR file,
    const char *filename, const uint8 *test);
static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);
static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file);

struct LoadPlugin plugin = {
    { NULL, NULL, 0, 0, "OGG" },
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
    return *(uint32 *)test == OGG_MAGIC;
}

static size_t ogg_read (APTR buffer, size_t size, size_t nitems, APTR file) {
    size_t items_read;
    items_read = FRead((BPTR)file, buffer, size, nitems);
    return items_read == -1 ? 0 : items_read;
}

static int ogg_seek (APTR file, ogg_int64_t offset, int mode) {
    if (!file) return -1;
    switch (mode) {
        case SEEK_SET:
            mode = OFFSET_BEGINNING;
            break;
        case SEEK_CUR:
            mode = OFFSET_CURRENT;
            break;
        case SEEK_END:
            mode = OFFSET_END;
            break;
        default:
            return -1;
    }
    return !ChangeFilePosition((BPTR)file, offset, mode) ? -1 : 0;
}

static int ogg_close (APTR file) {
    return 0;
}

static long ogg_tell (APTR file) {
    return GetFilePosition((BPTR)file);
}

static int32 LoadSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    struct Window *window;
    OggVorbis_File *vf = NULL;
    vorbis_info *vi;
    char *buffer = NULL, *ptr, tmp;
    int32 buf_len, buf_start, buf_end;
    BOOL eof;
    int current_section;
    int32 status, error = OK;
    int32 num_channels;
    int32 frame_size, frames, samples;
    struct sndArrayIO saio;
    int32 update;
    static const ov_callbacks callbacks = {
        ogg_read,
        ogg_seek,
        ogg_close,
        ogg_tell
    };

    window = ISoundED->GetProjectWindow(project);

    buf_len = INPUT_BUFFER_SIZE;
    vf = AllocVec(sizeof(*vf), MEMF_CLEAR|MEMF_PRIVATE);
    buffer = AllocVec(buf_len, MEMF_SHARED);
    if (!vf || !buffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    if (ov_open_callbacks((APTR)file, vf, NULL, 0, callbacks) < 0) {
        error = DTERROR_INVALID_DATA;
        goto out;
    }

    vi = ov_info(vf, -1);

    num_channels = vi->channels;

    SetGadgetAttrs(G(sound), window, NULL,
        SOUND_Length,       0,
        SOUND_NumChannels,  num_channels,
        SOUND_SampleSize,   2,
        SOUND_Frequency,    vi->rate,
        TAG_END);

    frame_size = num_channels << 1;
    saio.MethodID = SNDM_WRITESAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = buffer;
    saio.Offset = 0;
    saio.ChanMask = (1 << num_channels) - 1;
    saio.Mode = SNDWM_INSERT;
    saio.BytesPerSample = 2;
    saio.BytesPerFrame = frame_size;
    buf_start = buf_end = 0;
    eof = FALSE;
    current_section = -1;
    update = 0;
    for (;;) {
        if (buf_start == buf_end) {
            if (eof) break;
            buf_start = buf_end = 0;
            while (buf_end < buf_len) {
                status = ov_read(vf, buffer + buf_end, buf_len - buf_end, 0, 2, 1, &current_section);
                if (status == 0) {
                    eof = TRUE;
                    break;
                } else if (status == OV_HOLE) {
                    DebugPrintF("OGG: hole in stream - probably harmless\n");
                } else if (status < 0) {
                    eof = TRUE;
                    error = DTERROR_INVALID_DATA;
                    break;
                } else {
                    buf_end += status;
                }
            }
            if (eof && buf_end == 0) break;
        }

        ptr = buffer;
        frames = (buf_end - buf_start) / frame_size;
        samples = (buf_end - buf_start) >> 1;
        while (samples--) {
            tmp = ptr[0];
            ptr[0] = ptr[1];
            ptr[1] = tmp;
            ptr += 2;
        }
        buf_start = buf_end;

        saio.Length = frames;
        DoMethodA(sound, (Msg)&saio);
        saio.Offset += frames;
        if (++update == 512) {
            DoGadgetMethod(G(sound), window, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
            update = 0;
        }
    }
    DoGadgetMethod(G(sound), window, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);

out:
    FreeVec(vf);
    FreeVec(buffer);
    Close(file);

    return error;
}

static int32 write_page (ogg_page *page, BPTR file) {
    if (FWrite(file, page->header, 1, page->header_len) != page->header_len ||
        FWrite(file, page->body, 1, page->body_len) != page->body_len)
    {
        return IoErr();
    }
    return OK;
}

static int32 SaveSound (struct LoadPlugin *Self, APTR project, Object *sound, BPTR file) {
    int32 offset, orig_length, length, num_channels, frequency;
    vorbis_info *vi = NULL;
    vorbis_block *vb = NULL;
    vorbis_dsp_state *vd = NULL;
    ogg_stream_state *os = NULL;
    ogg_page *opg = NULL;
    ogg_packet *opk = NULL;
    int32 *inbuffer = NULL, *src;
    float32 **outbuffer, *dst;
    int32 getlen, frames_left;
    const int32 buflen = LOAD_SAVE_BUFFER;
    const int32 bufsiz = LOAD_SAVE_BUFFER << 2;
    int32 status, error = OK;
    APTR bar = NULL;
    struct sndArrayIO saio;

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

    vi = AllocVec(sizeof(*vi), MEMF_CLEAR|MEMF_PRIVATE);
    vd = AllocVec(sizeof(*vd), MEMF_CLEAR|MEMF_PRIVATE);
    vb = AllocVec(sizeof(*vb), MEMF_CLEAR|MEMF_PRIVATE);
    os = AllocVec(sizeof(*os), MEMF_CLEAR|MEMF_PRIVATE);
    opg = AllocVec(sizeof(*opg), MEMF_CLEAR|MEMF_PRIVATE);
    opk = AllocVec(sizeof(*opk), MEMF_CLEAR|MEMF_PRIVATE);
    inbuffer = AllocVec(bufsiz, MEMF_PRIVATE);
    if (!vi || !vb || !vd || !inbuffer) {
        error = ERROR_NO_FREE_STORE;
        goto out;
    }

    vorbis_info_init(vi);

    vorbis_encode_init_vbr(vi, num_channels, frequency, 0.3);

    vorbis_analysis_init(vd, vi);
    vorbis_block_init(vd, vb);

    ogg_stream_init(os, rand());

    {
        ogg_packet header_main;
        ogg_packet header_comments;
        ogg_packet header_codebooks;
        vorbis_comment vc;

        vc.user_comments = NULL;
        vc.comment_lengths = NULL;
        vc.comments = 0;

        vorbis_analysis_headerout(vd, &vc, &header_main, &header_comments, &header_codebooks);

        ogg_stream_packetin(os, &header_main);
        ogg_stream_packetin(os, &header_comments);
        ogg_stream_packetin(os, &header_codebooks);

        while ((status = ogg_stream_flush(os, opg))) {
            if (!status) break;
            error = write_page(opg, file);
            if (error) goto out;
        }
    }

    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ArrayPtr = inbuffer;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = 4;
    offset = 0;
    orig_length = length;
    bar = ISoundED->CreateProgressBar(project, TRUE);
    while (length > 0) {
        getlen = MIN(buflen, length);
        outbuffer = vorbis_analysis_buffer(vd, getlen);

        saio.Offset = offset;
        saio.Length = getlen;
        saio.ChanMask = 0x01;
        DoMethodA(sound, (Msg)&saio);
        src = inbuffer;
        dst = outbuffer[0];
        frames_left = getlen;
        while (frames_left--) {
            *dst++ = *src++ / 2147483647.0;
        }
        if (num_channels == 2) {
            saio.ChanMask = 0x02;
            DoMethodA(sound, (Msg)&saio);
            src = inbuffer;
            dst = outbuffer[1];
            frames_left = getlen;
            while (frames_left--) {
                *dst++ = *src++ / 2147483647.0;
            }
        }

        vorbis_analysis_wrote(vd, getlen);
        while (!error && vorbis_analysis_blockout(vd, vb) == 1) {
            vorbis_analysis(vb, opk);
            vorbis_bitrate_addblock(vb);

            while (!error && vorbis_bitrate_flushpacket(vd, opk)) {
                ogg_stream_packetin(os, opk);
                for (;;) {
                    status = ogg_stream_pageout(os, opg);
                    if (!status) break;

                    error = write_page(opg, file);
                    if (error) break;

                    if (ogg_page_eos(opg)) break;
                }
            }
        }
        if (error) break;

        offset += getlen;
        length -= getlen;
        ISoundED->SetProgressBarAttrs(bar,
            FUELGAUGE_Percent,  TRUE,
            FUELGAUGE_Max,      orig_length,
            FUELGAUGE_Level,    offset,
            TAG_END);
    }
    while (!error && vorbis_analysis_blockout(vd, vb) == 1) {
        vorbis_analysis(vb, opk);
        vorbis_bitrate_addblock(vb);

        while (!error && vorbis_bitrate_flushpacket(vd, opk)) {
            ogg_stream_packetin(os, opk);
            for (;;) {
                status = ogg_stream_pageout(os, opg);
                if (!status) break;

                error = write_page(opg, file);
                if (error) break;

                if (ogg_page_eos(opg)) break;
            }
        }
    }

out:
    ISoundED->DeleteProgressBar(bar);
    if (os) ogg_stream_clear(os);
    if (vb) vorbis_block_clear(vb);
    if (vd) vorbis_dsp_clear(vd);
    if (vi) vorbis_info_clear(vi);
    FreeVec(vi);
    FreeVec(vd);
    FreeVec(vb);
    FreeVec(os);
    FreeVec(opg);
    FreeVec(opk);
    FreeVec(inbuffer);
    Close(file);

    return error;
}
