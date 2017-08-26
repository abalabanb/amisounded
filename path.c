/*
 * AmiSoundED - Sound Editor
 * Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
 * Copyright (C) 2017 Alexandre Balaban <github@balaban.fr>
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

#include "os4types.h"
#include "path.h"
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

char *Strdup (const char *src) {
    char *dst = NULL;
    if (src) {
        int32 size = strlen(src)+1;
        dst = AllocVecTags(size, AVT_Type, MEMF_SHARED, TAG_END);
        if (dst) {
            CopyMem(src, dst, size);
        }
    }
    return dst;
}

/*char *Strndup (const char *src, int32 max_len) {
    char *dst = NULL;
    if (src) {
        int32 size = strlen(src);
        size = MIN(size, max_len)+1;
        dst = AllocVec(size, MEMF_SHARED);
        if (dst) {
            CopyMem(src, dst, size);
            dst[size-1] = '\0';
        }
    }
    return dst;
}*/

char *GetFullPath1 (const char *drawer, const char *file) {
    BPTR dir;
    char *path = NULL;
    dir = Lock(drawer, ACCESS_READ);
    if (dir) {
        path = GetFullPath2(dir, file);
        UnLock(dir);
    }
    return path;
}

char *GetFullPath2 (BPTR dir, const char *file) {
    char *drawer;
    char *path = NULL;
    int32 size = 256;
    while (drawer = AllocVecTags(size, AVT_Type, MEMF_SHARED, TAG_END)) {
        if (DevNameFromLock(dir, drawer, size, DN_FULLPATH)) break;
        FreeVec(drawer);
        drawer = NULL;
        if (IoErr() != ERROR_LINE_TOO_LONG) break;
        size += 256;
    }
    if (drawer) {
        path = MakePath(drawer, file);
        FreeVec(drawer);
    }
    return path;
}

char *MakePath (const char *drawer, const char *file) {
    char *path;
    int32 path_size;
    path_size = strlen(drawer) + strlen(file) + 8;
    path = AllocVecTags(path_size, AVT_Type, MEMF_SHARED, TAG_END);
    if (path) {
        strcpy(path, drawer);
        AddPart(path, file, path_size);
    }
    return path;
}

/*void SplitPath (const char *path, const char **drawer, const char **file) {
    *drawer = Strndup(path, PathPart(path) - path);
    *file = Strdup(FilePart(path));
}*/
