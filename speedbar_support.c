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

#include <exec/exec.h>
#include <gadgets/speedbar.h>
#include <images/bitmap.h>
#include <reaction/reaction_macros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/speedbar.h>
#include <proto/bitmap.h>

#include "speedbar_support.h"
#include "asot_macros.h"

struct List *MakeSpeedButtonList (struct Screen *scr, const struct SpeedButtonSpec *spec) {
    struct List *list;
    Object *image;
    struct Node *node;
    list = AllocList();
    if (list) {
        while (spec->ButtonID) {
            image = BitMapObject,
                BITMAP_Screen,              scr,
                BITMAP_SourceFile,          spec->Normal,
                BITMAP_SelectSourceFile,    spec->Selected,
                BITMAP_DisabledSourceFile,  spec->Disabled,
                BITMAP_Masking,             TRUE,
            End;

            if (image)
                node = AllocSpeedButtonNode(spec->ButtonID,
                    SBNA_Image,     image,
                    SBNA_Highlight, SBH_IMAGE,
                    SBNA_Toggle,    (spec->Flags == SBNA_Toggle),
                    TAG_END);
            else
                node = NULL;

            if (!node) {
                DisposeObject(image);
                FreeSpeedButtonList(list);
                return NULL;
            }
            AddTail(list, node);
            spec++;
        }
    }
    return list;
}

void FreeSpeedButtonList (struct List *list) {
    struct Node *node;
    Object *image;
    if (list) {
        while (node = RemHead(list)) {
            GetSpeedButtonNodeAttrs(node, SBNA_Image, &image, TAG_END);
            FreeSpeedButtonNode(node);
            DisposeObject(image);
        }
        FreeList(list);
    }
}

struct List *RemapSpeedButtons (struct List *list, struct Screen *scr) {
    struct Node *node;
    Object *image, *newimg;
    char *normal, *selected, *disabled;
    if (list == NULL) return NULL;
    for (node = GetHead(list); node; node = GetSucc(node)) {
        image = NULL;
        GetSpeedButtonNodeAttrs(node,
            SBNA_Image,     &image,
            TAG_END);
        if (image) {
            GetAttrs(image,
                BITMAP_SourceFile,          &normal,
                BITMAP_SelectSourceFile,    &selected,
                BITMAP_DisabledSourceFile,  &disabled,
                TAG_END);

            newimg = BitMapObject,
                BITMAP_Screen,              scr,
                BITMAP_SourceFile,          normal,
                BITMAP_SelectSourceFile,    selected,
                BITMAP_DisabledSourceFile,  disabled,
                BITMAP_Masking,             TRUE,
            End;

            if (!newimg) {
                FreeSpeedButtonList(list);
                return NULL;
            }

            SetAttrs(node, SBNA_Image, newimg, TAG_END);
            DisposeObject(image);
        }
    }
    return list;
}
