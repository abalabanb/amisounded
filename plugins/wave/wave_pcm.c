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

#include "wave_pcm.h"

int32 InitPCM (struct WaveCodecData *data) {
    struct WaveFormatEx *fmt = data->Format;

    /* check bitsPerSample */
    if (fmt->BitsPerSample < 1) {
        return ERROR_BAD_NUMBER;
    }
    data->SampleSize = (fmt->BitsPerSample + 7) >> 3;
    fmt->BlockAlign = data->SampleSize * fmt->NumChannels;
    data->BlockFrames = 1;
    return OK;
}

int32 DecodePCM (struct WaveCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames)
{
    struct WaveFormatEx *fmt = data->Format;
    int32 num_channels = fmt->NumChannels;
    int32 sample_size = data->SampleSize;
    int32 frames_left = num_frames;
    int32 chan;
    int32 bytes_left;

    if (sample_size == 1) {
        while (frames_left--) {
            for (chan = 0; chan < num_channels; chan++) {
                *dst++ = *src++ ^ 0x80; /* convert unsigned -> signed */
            }
        }
    } else {
        while (frames_left--) {
            for (chan = 0; chan < num_channels; chan++) {
                bytes_left = sample_size;
                dst += sample_size;
                while (bytes_left--) {
                    *--dst = *src++;
                }
                dst += sample_size;
            }
        }
    }
    return num_frames;
}
