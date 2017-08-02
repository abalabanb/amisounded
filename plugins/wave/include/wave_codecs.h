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

#ifndef WAVE_CODECS_H
#define WAVE_CODECS_H

#ifndef PLUGINS_H
#include "plugins.h"
#endif

#ifndef RIFF_WAVE_H
#include "riff-wave.h"
#endif

struct WaveCodecData {
    const struct WaveCodec *Codec;
    struct RIFFChunk Chunk;
    struct WaveFormatEx *Format;
    int32 BlockFrames;
    int32 SampleSize;
    APTR InitData;
};

struct WaveCodec {
    uint32 FormatTag;
    int32 (*Init)(struct WaveCodecData *data);
    void (*Exit)(struct WaveCodecData *data);
    int32 (*Decode)(struct WaveCodecData *data, uint8 *src, uint8 *dst,
        int32 num_blocks, int32 num_frames);
    int32 (*DecodeFrames)(struct WaveCodecData *data, uint8 *src, uint8 *dst,
        int32 num_blocks, int32 num_frames);
};

/* wave_codecs.c */
const struct WaveCodec *GetCodec (uint16 format_tag);

#endif
