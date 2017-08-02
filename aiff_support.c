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

#include "aiff_support.h"
#include <proto/iffparse.h>
#include <string.h>

#define OK 0
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

int32 extended2long (uint8 *extended) {
    int32 expo;
    uint32 mant;
    int32 value;
    expo = *(int16 *)&extended[0] & 0x7fff;
    mant = *(uint32 *)&extended[2];
    expo -= 0x3fff;
    if (expo < 0) {
        value = 0;
    } else {
        expo -= 31;
        value = (expo > 0) ? 0x7fffffff : mant >> (-expo);
    }
    return (extended[0] & 0x80) ? -value : value;
}

void long2extended (int32 value, uint8 *extended) {
    uint32 expo;
    int32 i;
    memset(extended, 0, 10);
    expo = ((uint32)value) >> 2;
    for (i = 0; i < 32 && expo > 0; i++) expo >>= 1;
    extended[0] = 0x40;
    extended[1] = i;
    for (i = 32; i > 0 && value > 0; i--) value <<= 1;
    *(uint32 *)&extended[2] = value;
}

int32 ParseSSND (struct IFFHandle *iff, void *buffer, int32 bufsiz) {
    struct SampledSoundHeader ssnd;
    int32 error;
    int32 toskip;
    int32 readsiz;
    error = ReadChunkBytes(iff, &ssnd, sizeof(ssnd));
    if (error < 0) return error;
    toskip = ssnd.DataOffset;
    while (toskip > 0) {
        readsiz = MIN(bufsiz, toskip);
        error = ReadChunkBytes(iff, buffer, readsiz);
        if (error < 0) return error;
        toskip -= readsiz;
    }
    return OK;
}
