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

#include <proto/exec.h>
#include <proto/locale.h>

#include "os4types.h"
#define CATCOMP_BLOCK
#include "locale_support.h"

#ifdef __amigaos4__
struct Library     *LocaleBase;
struct LocaleIFace *ILocale;
#else
struct LocaleBase  *LocaleBase;
#define ILocale LocaleBase
#endif

void OpenLocaleCatalog (struct LocaleInfo *li, const char *catalog) {
    LocaleBase = (APTR)OpenLibrary("locale.library", 0);
#ifdef __amigaos4__
    ILocale = (APTR)GetInterface(LocaleBase, "main", 1, NULL);
    if (ILocale) {
        li->Catalog = OpenCatalog(NULL, (char *)catalog,
            OC_BuiltInCodeSet,  4,
            TAG_END);
    }
    li->CodeSet = li->Catalog ? li->Catalog->cat_CodeSet : 4;
#else
    if (LocaleBase) li->Catalog = OpenCatalogA(NULL, (char *)catalog, NULL);
#endif
}

void CloseLocaleCatalog (struct LocaleInfo *li) {
#ifdef __amigaos4__
    if (ILocale) {
        CloseCatalog(li->Catalog);
        DropInterface((APTR)ILocale);
    }
    CloseLibrary((APTR)LocaleBase);
#else
    if (LocaleBase) {
        CloseCatalog(li->Catalog);
        CloseLibrary((APTR)LocaleBase);
    }
#endif
}

const char *GetString (struct LocaleInfo *li, LONG stringNum) {
    LONG         *l;
    UWORD        *w;
    CONST_STRPTR  builtIn;

    l = (LONG *)CatCompBlock;

    while (*l != stringNum)
    {
        w = (UWORD *)((ULONG)l + 4);
        l = (LONG *)((ULONG)l + (ULONG)*w + 6);
    }
    builtIn = (CONST_STRPTR)((ULONG)l + 6);

    if (ILocale) {
        return GetCatalogStr(li->Catalog, stringNum, (char *)builtIn);
    }
    return builtIn;
}

void TranslateHints (struct LocaleInfo *li, struct HintInfo *hi) {
    while (hi->hi_GadgetID != -1) {
        hi->hi_Text = (char *)GetString(li, (LONG)hi->hi_Text);
        hi++;
    }
}

void TranslateMenus (struct LocaleInfo *li, struct NewMenu *nm) {
    while (nm->nm_Type != NM_END) {
        if (nm->nm_Label && nm->nm_Label != NM_BARLABEL)
            nm->nm_Label = (char *)GetString(li, (LONG)nm->nm_Label);
        nm++;
    }
}

void TranslateLabelArray (struct LocaleInfo *li, const char **array) {
    while (*array != STR_ID(-1)) {
        *array = GetString(li, (LONG)*array);
        array++;
    }
    *array = NULL;
}

void TranslateColumnTitles (struct LocaleInfo *li, struct ColumnInfo *ci) {
    while (ci->ci_Width > 0) {
        if ((int32)ci->ci_Title != -1)
            ci->ci_Title = (char *)GetString(li, (int32)ci->ci_Title);
        ci++;
    }
}
