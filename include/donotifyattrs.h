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

#ifndef DONOTIFYATTRS_H
#define DONOTIFYATTRS_H

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

uint32 VARARGS68K DoNotifyAttrs(Class *cl, Object *o, struct GadgetInfo *ginfo,
    uint32 flags, ...);
uint32 DoNotifyAttrsA(Class *cl, Object *o, struct GadgetInfo *ginfo,
    uint32 flags, struct TagItem *tags);

#endif
