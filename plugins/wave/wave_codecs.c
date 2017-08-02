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

#include "riff-wave.h"
#include "wave_codecs.h"
#include "wave_pcm.h"
#include "wave_ima_adpcm.h"
#include "wave_ms_adpcm.h"
#include "wave_ieee_float.h"
#include "wave_alaw.h"
#include "wave_gsm610.h"
#include "wave_g72x.h"

static int32 DecodeBlocks (struct WaveCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames);

static const struct WaveCodec wave_codecs[] = {
    { WAVE_FORMAT_PCM,          InitPCM,        NULL,       DecodePCM,          NULL            },
    { WAVE_FORMAT_IMA_ADPCM,    InitIMA_ADPCM,  NULL,       DecodeBlocks,       DecodeIMA_ADPCM },
    { WAVE_FORMAT_ADPCM,        InitMS_ADPCM,   NULL,       DecodeBlocks,       DecodeMS_ADPCM  },
    { WAVE_FORMAT_IEEE_FLOAT,   InitIEEE_Float, NULL,       DecodeIEEE_Float,   NULL            },
    { WAVE_FORMAT_ALAW,         InitALaw,       NULL,       DecodeALaw,         NULL            },
    { WAVE_FORMAT_MULAW,        InitALaw,       NULL,       DecodeMuLaw,        NULL            },
    { WAVE_FORMAT_GSM610,       InitGSM610,     ExitGSM610, DecodeBlocks,       DecodeGSM610    },
//  { WAVE_FORMAT_G721_ADPCM,   InitG72x,       NULL,       DecodeBlocks,       DecodeG72x      },
//  { WAVE_FORMAT_G723_ADPCM,   InitG72x,       NULL,       DecodeBlocks,       DecodeG72x      },
    { 0 }
};

const struct WaveCodec *GetCodec (uint16 format_tag) {
    const struct WaveCodec *codec;
    for (codec = wave_codecs; codec->FormatTag; codec++) {
        if (codec->FormatTag == format_tag) {
            return codec;
        }
    }
    return NULL;
}

static int32 DecodeBlocks (struct WaveCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames)
{
    const struct WaveCodec *codec = data->Codec;
    struct WaveFormatEx *fmt = data->Format;
    int32 status;
    int32 frames_left;
    int32 read_frames;
    int32 block_size;
    int32 frame_size;
    frames_left = num_frames;
    read_frames = data->BlockFrames;
    block_size = fmt->BlockAlign;
    frame_size = data->SampleSize * fmt->NumChannels;
    while (frames_left > 0) {
        if (frames_left < read_frames) {
            read_frames = frames_left;
        }
        status = codec->DecodeFrames(data, src, dst, 1, read_frames);
        if (status != read_frames) {
            return num_frames - frames_left + status;
        }
        src += block_size;
        dst += read_frames * frame_size;
        frames_left -= read_frames;
    }
    return num_frames;
}
