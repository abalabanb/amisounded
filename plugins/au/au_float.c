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

/*
 *  sun_au.datatype by Fredrik Wikstrom
 *
 */

#include "au_codecs.h"
#include "au_float.h"
#include "endian.h"
#include <math.h>

int32 InitFloat (struct AUCodecData *data) {
    const struct AUCodec *codec = data->Codec;
    struct AUHeader *header = data->Header;
    if (codec->Encoding == AU_FMT_32BIT_IEEE_FLOAT) {
        data->BlockSize = header->NumChannels << 2;
    } else {
        data->BlockSize = header->NumChannels << 3;
    }
    data->BlockFrames = 1;
    data->SampleSize = 4;
    return OK;
}

int32 DecodeFloat32 (struct AUCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames)
{
    struct AUHeader *header = data->Header;
    int32 fr, ch;
    float32 sample;
    for (fr = 0; fr < num_frames; fr++) {
        for (ch = 0; ch < header->NumChannels; ch++) {
            sample = *(float32 *)src;
            if (sample >= 1.0)
                *(int32 *)dst = 2147483647;
            else if (sample <= -1.0)
                *(int32 *)dst = -2147483648;
            else
                *(int32 *)dst = floor(*src * 2147483648.0);
            src += 4;
            dst += 4;
        }
    }
    return num_frames;
}

int32 DecodeFloat64 (struct AUCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames)
{
    struct AUHeader *header = data->Header;
    int32 fr, ch;
    float64 sample;
    for (fr = 0; fr < num_frames; fr++) {
        for (ch = 0; ch < header->NumChannels; ch++) {
            sample = *(float64 *)src;
            if (sample >= 1.0)
                *(int32 *)dst = 2147483647;
            else if (sample <= -1.0)
                *(int32 *)dst = -2147483648;
            else
                *(int32 *)dst = floor(*src * 2147483648.0);
            src += 8;
            dst += 4;
        }
    }
    return num_frames;
}
