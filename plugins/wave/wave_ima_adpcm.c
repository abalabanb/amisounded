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

#include "wave_ima_adpcm.h"
#include "bitpack.h"
#include "endian.h"

struct IMA_ADPCM_Format {
    uint16 FormatTag;
    int16 NumChannels;
    int32 SamplesPerSec;
    int32 AvgBytesPerSec;
    int16 BlockAlign; /* amount to read for each block */
    int16 BitsPerSample; /* 3 or 4 */

    int16 ExtraSize; /* 2 */
    int16 SamplesPerBlock;
};

int32 InitIMA_ADPCM (struct WaveCodecData *data) {
    struct IMA_ADPCM_Format *fmt = (APTR)data->Format;

    /* check BitsPerSample */
    if (fmt->BitsPerSample != 3 && fmt->BitsPerSample != 4) {
        return DTERROR_UNKNOWN_COMPRESSION;
    }
    if (data->Chunk.Size != sizeof(*fmt)) {
        return ERROR_BAD_NUMBER;
    }
    write_le16(&fmt->SamplesPerBlock, fmt->SamplesPerBlock);
    data->BlockFrames = fmt->SamplesPerBlock;
    data->SampleSize = 2;
    return OK;
}

static const int16 steptab[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

/* 3 bps */
static const int8 indextab3[8] = {
    -1, -1, 1, 2,
    -1, -1, 1, 2
};

/* 4 bps */
static const int8 indextab4[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* Each block has nchannels of these */
struct IMA_BlockHeader {
    uint8 isamp0[2]; /* Prev sample ro start decoding with (int) */
    int8 stepTableIndex; /* Current index in steptable array (0-88) */
    uint8 reserved;
};

int32 DecodeIMA_ADPCM (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    struct IMA_ADPCM_Format *fmt = (APTR)data->Format;
    int32 index[MAX_CHANNELS], diff, value[MAX_CHANNELS];
    int16 code;
    int32 num_channels = fmt->NumChannels;
    int32 sample_size, frame, frame2, chan;
    int32 bframes, skip = 0, t;
    const int8 *indextab;
    BitPack_buffer b;
    int16 *dst = (int16 *)dst_ptr;
    int16 *out;

    sample_size = fmt->BitsPerSample;

    if (sample_size == 3) {
        indextab = indextab3;
        bframes = 32;
    } else {
        indextab = indextab4;
        bframes = 8;
    }

    for (chan = 0; chan < num_channels; chan++) {
        value[chan] = (int16)read_le16(src); src += 2;
        index[chan] = *src; src += 2;
        *dst++ = value[chan];
    }

    bitpack_init_lsb(&b, src, 0);

    for (frame = 1; frame <num_frames; frame += bframes) {
        t = num_frames - frame;
        if (t < bframes) {
            skip = t * sample_size;
            bframes -= t;
        }
        for (chan = 0; chan < num_channels; chan++) {
            out = dst;
            for (frame2 = 0; frame2 < bframes; frame2++) {

                /* Step 1 - Get delta value */

                code = bitpack_read_lsb(&b, sample_size);

                /* Step 2 - Calculate difference and new expected value */

                {
                    int32 step, mask;
                    step = steptab[index[chan]];
                    diff = 0;
                    for (mask = 1 << (sample_size-2); mask; mask >>= 1, step >>= 1) {
                        if (code & mask) {
                            diff += step;
                        }
                    }
                    diff += step;
                    if (code & (1 << (sample_size - 1))) {
                        value[chan] -= diff;
                        if (value[chan] < -32678) value[chan] = -32678;
                    } else {
                        value[chan] += diff;
                        if (value[chan] > 32767) value[chan] = 32767;
                    }
                }

                /* Step 3 - Find next index value */

                index[chan] += indextab[code];

                if (index[chan] < 0) index[chan] = 0;
                if (index[chan] > 88) index[chan] = 88;

                /* Step 4 - Output value */

                *out = value[chan];
                out += num_channels;
            }
            if (skip) {
                bitpack_seek_lsb(&b, OFFSET_CURRENT, skip);
            }
            dst++;
        }
        dst += ((bframes-1) * num_channels);
    }
    return num_frames;
}
