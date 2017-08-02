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

#include "wave_ieee_float.h"

int32 InitIEEE_Float (struct WaveCodecData *data) {
    struct WaveFormatEx *fmt = data->Format;
    if (fmt->ExtraSize != 0) {
        return ERROR_BAD_NUMBER;
    }
    switch (fmt->BitsPerSample) {
        case 32:
            break;
        case 64:
            break;
        default:
            return DTERROR_UNKNOWN_COMPRESSION;
    }
    data->BlockFrames = 1;
    data->SampleSize = 4;
    return OK;
}

int32 DecodeIEEE_Float (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    int32 num_channels = data->Format->NumChannels;
    int32 frame, chan;
    int32 *dst = (int32 *)dst_ptr;
    if (data->Format->BitsPerSample == 32) {
        for (frame = 0; frame < num_frames; frame++) {
            for (chan = 0; chan < num_channels; chan++) {
                union {
                    float32 f;
                    uint32 i;
                } samp;
                samp.i = read_le32(src); src += 4;
                *dst++ = (int32)(samp.f * 2147483647.0);
            }
        }
    } else {
        for (frame = 0; frame < num_frames; frame++) {
            for (chan = 0; chan < num_channels; chan++) {
                union {
                    float64 f;
                    uint64 i;
                } samp;
                samp.i = read_le64(src); src += 8;
                *dst++ = (int32)(samp.f * 2147483647.0);
            }
        }
    }
    return num_frames;
}
