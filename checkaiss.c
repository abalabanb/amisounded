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

#include <dos/dos.h>
#include <proto/dos.h>
#include <string.h>
#include "checkaiss.h"

int32 CheckAISS (int32 min_version, int32 min_revision) {
    APTR oldwin;
    BPTR lock;
    char envvar[32];
    char *ver_str, *rev_str, *end;
    int32 version, revision;

    /* Check for AISS assign */
    oldwin = SetProcWindow((APTR)-1);
    lock = Lock("tbimages:", ACCESS_READ);
    UnLock(lock);
    SetProcWindow(oldwin);
    if (lock == ZERO) return NOAISS;

    /* Check AISS version */
    version = 0;
    revision = 0;
    if (GetVar("AISS", envvar, sizeof(envvar), GVF_GLOBAL_ONLY) == -1) {
        return OLDAISS;
    }
    ver_str = strchr(envvar, ' ');
    if (ver_str) {
        *ver_str++ = 0;
        end = strchr(ver_str, ' ');
        if (end) {
            *end = 0;
            rev_str = strchr(ver_str, '.');
            if (rev_str) {
                *rev_str++ = 0;
                end = strchr(rev_str, '.');
                if (end) *end = 0;
            }
            StrToLong(ver_str, &version);
            if (rev_str) StrToLong(rev_str, &revision);
        }
    }
    if (version > min_version || (version == min_version && revision >= min_revision)) {
        return AISSOK;
    }
    return OLDAISS;
}
