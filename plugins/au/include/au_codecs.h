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

#ifndef AU_CODECS_H
#define AU_CODECS_H

#ifndef PLUGINS_H
#include "plugins.h"
#endif

#ifndef AU_FORMAT_H
#include "au_format.h"
#endif

struct AUCodecData {
    const struct AUCodec *Codec;
    struct AUHeader *Header;
    int32 BlockSize;
    int32 BlockFrames;
    int32 SampleSize;
    APTR InitData;
};

struct AUCodec {
    uint32 Encoding;
    int32 (*Init)(struct AUCodecData *data);
    void (*Exit)(struct AUCodecData *data);
    int32 (*Decode)(struct AUCodecData *data, uint8 *src, uint8 *dst,
        int32 num_blocks, int32 num_frames);
    int32 (*DecodeFrames)(struct AUCodecData *data, uint8 *src, uint8 *dst,
        int32 num_blocks, int32 num_frames);
};

const struct AUCodec *GetCodec (uint32 encoding);

#endif
