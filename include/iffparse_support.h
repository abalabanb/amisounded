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

#ifndef IFFPARSE_SUPPORT_H
#define IFFPARSE_SUPPORT_H

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

struct IFFHandle *OpenIFF_File (BPTR file, int32 rwmode);
void CloseIFF_File (struct IFFHandle *iff);
struct IFFHandle *OpenClip (uint32 unit, int32 rwmode);
void CloseClip (struct IFFHandle *iff);

#define IFFErr(error) (((error) < 0) ? (error) : DTERROR_NOT_ENOUGH_DATA)

#endif
