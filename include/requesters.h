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

#ifndef REQUESTERS_H
#define REQUESTERS_H

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

int32 DoReq (struct Screen *screen, Object *window, Object *req);
void AboutRequester (struct Screen *screen, Object *window);
void ErrorRequester (struct Screen *screen, Object *window, int32 error, const char *filename);

#endif
