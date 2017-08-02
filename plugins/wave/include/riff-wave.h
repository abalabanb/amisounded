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
 *  wave.datatype
 *  (c) Fredrik Wikstrom
 */

#ifndef RIFF_WAVE_H
#define RIFF_WAVE_H 1

#ifndef ENDIAN_H
#include "endian.h"
#endif

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

/* Chunk IDs */

#define ID_RIFF MAKE_ID('R','I','F','F')
#define ID_WAVE MAKE_ID('W','A','V','E')
#define ID_fmt  MAKE_ID('f','m','t',' ')
#define ID_fact MAKE_ID('f','a','c','t')
#define ID_data MAKE_ID('d','a','t','a')

struct RIFFHeader {
    uint32 ID;
    uint32 Size;
    uint32 Type;
};

struct RIFFChunk {
    uint32 ID;
    uint32 Size;
};

/* chunksize=16 */
struct WaveFormat {
    uint16 FormatTag;
    uint16 NumChannels;
    uint32 SamplesPerSec;
    uint32 AvgBytesPerSec;
    uint16 BlockAlign;
    uint16 BitsPerSample;
};

/* chunksize>=18 */
struct WaveFormatEx {
    uint16 FormatTag;
    uint16 NumChannels; /* 1 mono, 2 stereo */
    uint32 SamplesPerSec;
    uint32 AvgBytesPerSec;
    uint16 BlockAlign;
    uint16 BitsPerSample;
    uint16 ExtraSize;
};

/* formatTag values */

#include "wave_formats.h"

/* WAVE_FORMAT_EXTENSIBLE format (unsupported as of yet): */

typedef struct {
    uint32 f1;
    uint16 f2;
    uint16 f3;
    uint8 f4[8];
} _GUID;

/* chunksize>=40 */
struct WaveFormatExtensible {
    uint16 formatTag; // WAVE_FORMAT_EXTENSIBLE
    uint16 numChannels; /* 1 mono, 2 stereo */
    uint32 samplesPerSec;
    uint32 avgBytesPerSec;
    uint16 blockAlign;
    uint16 bitsPerSample;
    uint16 extraSize; /* >=22 */

    /* I have no idea how the following is to be read!!?? */
    union {
        uint16 validBitsPerSample;
        uint16 samplesPerBlock;
        uint16 reserved;
    } Samples;
    /* channel mask */
    uint32 channelMask;
    _GUID SubFmt;
};

/* GUID SubFormat IDs */

#ifndef _DATAFORMAT_SUBTYPE_PCM_
#define _DATAFORMAT_SUBTYPE_PCM_ {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}}
//static const _GUID DATAFORMAT_SUBTYPE_PCM = {0xE923AABF, 0xCB58, 0x4471, {0xA1, 0x19, 0xFF, 0xFA, 0x01, 0xE4, 0xCE, 0x62}};
#endif

#ifndef _DATAFORMAT_SUBTYPE_UNKNOWN_
#define _DATAFORMAT_SUBTYPE_UNKNOWN_ {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
//static const _GUID DATAFORMAT_SUBTYPE_UNKNOWN = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#endif

/* Microsoft speaker definitions */
#define WAVE_SPEAKER_FRONT_LEFT             0x1
#define WAVE_SPEAKER_FRONT_RIGHT            0x2
#define WAVE_SPEAKER_FRONT_CENTER           0x4
#define WAVE_SPEAKER_LOW_FREQUENCY          0x8
#define WAVE_SPEAKER_BACK_LEFT              0x10
#define WAVE_SPEAKER_BACK_RIGHT             0x20
#define WAVE_SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define WAVE_SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define WAVE_SPEAKER_BACK_CENTER            0x100
#define WAVE_SPEAKER_SIDE_LEFT              0x200
#define WAVE_SPEAKER_SIDE_RIGHT             0x400
#define WAVE_SPEAKER_TOP_CENTER             0x800
#define WAVE_SPEAKER_TOP_FRONT_LEFT         0x1000
#define WAVE_SPEAKER_TOP_FRONT_CENTER       0x2000
#define WAVE_SPEAKER_TOP_FRONT_RIGHT        0x4000
#define WAVE_SPEAKER_TOP_BACK_LEFT          0x8000
#define WAVE_SPEAKER_TOP_BACK_CENTER        0x10000
#define WAVE_SPEAKER_TOP_BACK_RIGHT         0x20000
#define WAVE_SPEAKER_RESERVED               0x80000000

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

#endif /* RIFF_WAVE_H */
