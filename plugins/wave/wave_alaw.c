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

#include "wave_alaw.h"

static int16 decode_alaw (uint8 a_val);
static int16 decode_mulaw(uint8 u_val);

int32 InitALaw (struct WaveCodecData *data) {
    struct WaveFormatEx *fmt = data->Format;
    if (fmt->ExtraSize != 0 || fmt->BitsPerSample != 8) {
        return ERROR_BAD_NUMBER;
    }
    data->BlockFrames = 1;
    data->SampleSize = 2;
    return OK;
}

int32 DecodeALaw (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    int32 num_channels = data->Format->NumChannels;
    int32 frame, chan;
    int16 *dst = (int16 *)dst_ptr;
    for (frame = 0; frame < num_frames; frame++) {
        for (chan = 0; chan < num_channels; chan++) {
            *dst++ = decode_alaw(*src++);
        }
    }
    return num_frames;
}

int32 DecodeMuLaw (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    int32 num_channels = data->Format->NumChannels;
    int32 frame, chan;
    int16 *dst = (int16 *)dst_ptr;
    for (frame = 0; frame < num_frames; frame++) {
        for (chan = 0; chan < num_channels; chan++) {
            *dst++ = decode_mulaw(*src++);
        }
    }
    return num_frames;
}

#define SIGN_BIT    (0x80)      /* Sign bit for a A-law byte. */
#define QUANT_MASK  (0xf)       /* Quantization field mask. */
#define SEG_SHIFT   (4)     /* Left shift for segment number. */
#define SEG_MASK    (0x70)      /* Segment field mask. */
#define BIAS        (0x84)      /* Bias for linear code. */

static int16 decode_alaw (uint8 a_val) {
    int16 t;
    int16 seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}

static int16 decode_mulaw(uint8 u_val) {
    int16 t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}
