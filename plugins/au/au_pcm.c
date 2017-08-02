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
#include "au_pcm.h"
#include <string.h>

int32 InitPCM (struct AUCodecData *data) {
    const struct AUCodec *codec = data->Codec;
    struct AUHeader *header = data->Header;
    switch (codec->Encoding) {
        case AU_FMT_8BIT_PCM:
            data->SampleSize = 1;
            break;
        case AU_FMT_16BIT_PCM:
            data->SampleSize = 2;
            break;
        case AU_FMT_24BIT_PCM:
            data->SampleSize = 3;
            break;
        case AU_FMT_32BIT_PCM:
            data->SampleSize = 4;
            break;
    }
    data->BlockSize = data->SampleSize * header->NumChannels;
    data->BlockFrames = 1;
    return OK;
}

int32 DecodePCM (struct AUCodecData *data, uint8 *src, uint8 *dst,
    int32 num_blocks, int32 num_frames)
{
    struct AUHeader *header = data->Header;
    memcpy(src, dst, num_frames * header->NumChannels * data->SampleSize);
    return num_frames;
}
