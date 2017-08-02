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

#include "iffparse_support.h"
#include <proto/iffparse.h>

struct IFFHandle *OpenIFF_File (BPTR file, int32 rwmode) {
    struct IFFHandle *iff;
    iff = AllocIFF();
    if (iff) {
        iff->iff_Stream = file;
        InitIFFasDOS(iff);
        if (!OpenIFF(iff, rwmode)) {
            return iff;
        }
        FreeIFF(iff);
    }
    return NULL;
}

void CloseIFF_File (struct IFFHandle *iff) {
    if (iff) {
        CloseIFF(iff);
        FreeIFF(iff);
    }
}
