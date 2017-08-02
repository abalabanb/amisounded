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

#ifndef OS4TYPES_H
#define OS4TYPES_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef __amigaos4__

typedef signed long long int64;
typedef unsigned long long uint64;
typedef LONG int32;
typedef ULONG uint32;
typedef WORD int16;
typedef UWORD uint16;
typedef BYTE int8;
typedef UBYTE uint8;
typedef float float32;
typedef double float64;

#define VARARGS68K
#define APICALL

#endif

#endif

