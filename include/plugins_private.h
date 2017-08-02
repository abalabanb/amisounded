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

#ifndef PLUGINS_PRIVATE_H
#define PLUGINS_PRIVATE_H

#define PLUGIN_HAS_SAVE_OPTIONS

#ifndef PLUGINS_H
#include "plugins.h"
#endif

#define TEST_SIZE(list) ((list)->l_pad)

extern const char **output_formats;
extern struct LoadPlugin **output_plugins;

void LoadPlugins (struct List *list);
void FreePlugins (struct List *list);
struct LoadPlugin *FindLoader (struct List *list, BPTR file, const char *filename);

#endif
