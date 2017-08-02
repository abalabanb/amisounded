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

#ifndef LOCALE_SUPPORT_H
#define LOCALE_SUPPORT_H

#define CATCOMP_NUMBERS
#include "locale.h"

#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif

#ifndef LIBRARIES_GADTOOLS_H
#include <libraries/gadtools.h>
#endif

#ifndef GADGETS_LISTBROWSER_H
#include <gadgets/listbrowser.h>
#endif

#ifndef STR_ID
#define STR_ID(x) ((char *)(x))
#endif

struct LocaleInfo {
    struct Catalog     *Catalog;
#ifdef __amigaos4__
    uint32             CodeSet;
#endif
};

void OpenLocaleCatalog (struct LocaleInfo *li, const char *catalog);
void CloseLocaleCatalog (struct LocaleInfo *li);
const char *GetString (struct LocaleInfo *li, LONG stringNum);
void TranslateHints (struct LocaleInfo *li, struct HintInfo *hi);
void TranslateMenus (struct LocaleInfo *li, struct NewMenu *nm);
void TranslateLabelArray (struct LocaleInfo *li, const char **array);
void TranslateColumnTitles (struct LocaleInfo *li, struct ColumnInfo *ci);

#endif
