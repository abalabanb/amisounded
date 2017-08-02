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

#ifndef AU_FORMAT_H
#define AU_FORMAT_H

/*
Links:
http://astronomy.swin.edu.au/~pbourke/dataformats/au/
http://www.cnpbagwell.com/AudioFormats-11.html
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef ENDIAN_H
#include "endian.h"
#endif

struct AUHeader {
    uint32 Magic; // magic number
    uint32 DataOffset; // offset (in file of data)
    uint32 DataSize; // length of data (optional)
    uint32 Encoding; // data encoding format
    uint32 SampleRate; // samples/sec
    uint32 NumChannels; // number of interleaved channels
};

#define AU_MAGIC MAKE_ID('.','s','n','d')

enum {
    AU_FMT_ULAW = 1,    // 8-bit mu-law samples
    AU_FMT_8BIT_PCM,    // 8-bit linear samples
    AU_FMT_16BIT_PCM,   // 16-bit linear samples
    AU_FMT_24BIT_PCM,   // 24-bit linear samples
    AU_FMT_32BIT_PCM,   // 32-bit linear samples
    AU_FMT_32BIT_IEEE_FLOAT,    // floating-point samples
    AU_FMT_64BIT_IEEE_FLOAT,    // double-precision float samples
    AU_FMT_FRAGMENT,    // fragmented sampled data
    AU_FMT_NESTED,      // ?
    AU_FMT_DSP_CORE,    // DSP program
    AU_FMT_8BIT_FIXED,  // 8-bit fixed-point samples
    AU_FMT_16BIT_FIXED, // 16-bit fixed-point samples
    AU_FMT_24BIT_FIXED, // 24-bit fixed-point samples
    AU_FMT_32BIT_FIXED, // 32-bit fixed-point samples
    AU_FMT_DISPLAY = 16,    // non-audio display data
    AU_FMT_ULAW_SQUELCH,    // ?
    AU_FMT_EMPHASIZED,  // 16-bit linear with emphasis
    AU_FMT_COMPRESSED,  // 16-bit linear with compression
    AU_FMT_EMP_COMP, // A combination of the two above
    AU_FMT_DSP_COMMANDS,    // Music Kit DSP commands
    AU_FMT_DSP_COMMANDS_SAMPLES, // ?
    AU_FMT_4BIT_G721_ADPCM,
    AU_FMT_G722_ADPCM,  // ?
    AU_FMT_3BIT_G723_ADPCM,
    AU_FMT_5BIT_G723_ADPCM,
    AU_FMT_ALAW
};

#endif
