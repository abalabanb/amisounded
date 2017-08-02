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

#include "au_format.h"
#include "au_codecs.h"
#include "au_pcm.h"
#include "au_alaw.h"
#include "au_float.h"
#include "au_g72x.h"

static const struct AUCodec au_codecs[] = {
    { AU_FMT_8BIT_PCM,          InitPCM,    NULL,       DecodePCM,      NULL        },
    { AU_FMT_16BIT_PCM,         InitPCM,    NULL,       DecodePCM,      NULL        },
    { AU_FMT_24BIT_PCM,         InitPCM,    NULL,       DecodePCM,      NULL        },
    { AU_FMT_32BIT_PCM,         InitPCM,    NULL,       DecodePCM,      NULL        },
    { AU_FMT_ALAW,              InitALAW,   NULL,       DecodeALAW,     NULL        },
    { AU_FMT_ULAW,              InitALAW,   NULL,       DecodeULAW,     NULL        },
    { AU_FMT_32BIT_IEEE_FLOAT,  InitFloat,  NULL,       DecodeFloat32,  NULL        },
    { AU_FMT_64BIT_IEEE_FLOAT,  InitFloat,  NULL,       DecodeFloat64,  NULL        },
//  { AU_FMT_4BIT_G721_ADPCM,   InitG72x,   ExitG72x,   DecodeBlocks,   DecodeG72x  },
//  { AU_FMT_3BIT_G723_ADPCM,   InitG72x,   ExitG72x,   DecodeBlocks,   DecodeG72x  },
//  { AU_FMT_5BIT_G723_ADPCM,   InitG72x,   ExitG72x,   DecodeBlocks,   DecodeG72x  },
    { 0 }
};

const struct AUCodec * GetCodec (uint32 encoding) {
    const struct AUCodec *codec;
    for (codec = au_codecs; codec->Encoding; codec++) {
        if (codec->Encoding == encoding) {
            return codec;
        }
    }
    return NULL;
}
