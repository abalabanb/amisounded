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

#ifndef EXT_WINDOW_CLASS_H
#define EXT_WINDOW_CLASS_H

#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif

#define EXTWINDOW_Dummy (0x80025000L)
#define WINDOW_SnapshotGadget (EXTWINDOW_Dummy+1)

#define GID_SNAPSHOT (0x7FFE)

#define WMHI_SNAPSHOT (256L<<16)

extern Class *ExtWindowClass;

Class * InitExtWindowClass ();

#define ExtWindowObject NewObject(ExtWindowClass, NULL

#endif
