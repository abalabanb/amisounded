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

#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

#include "soundeditor_gc.h"
#include "soundeditor_gc_private.h"
#include "debug.h"

struct Sample *GetSampleData(struct List *list, int32 *offset_ptr) {
    int32 offset = *offset_ptr;
    struct Sample *sample;
    sample = (APTR)GetHead(list);
    while (sample) {
        if (offset < sample->Length) {
            *offset_ptr = offset;
            return sample;
        }
        offset -= sample->Length;
        sample = (APTR)GetSucc((APTR)sample);
    }
    return NULL;
}

void FreeSample(struct Sample *sample) {
    if (sample) {
        int32 chan;
        for (chan = 0; chan < MAX_CHANNELS; chan++) {
            FreeVec(sample->Data[chan]);
        }
        FreeNode(sample);
    }
}

struct Sample *AllocSample(int32 num_channels, uint32 sample_size) {
    struct Sample *sample;
    int32 chan;
    sample = AllocMinNode(sizeof(struct Sample));
    if (sample) {
        ClearMem(&sample->Link + 1, sizeof(*sample)-sizeof(sample->Link));
        for (chan = 0; chan < num_channels; chan++) {
            sample->Data[chan] = AllocVec(SAMPLE_LENGTH * sample_size, MEMF_VIRTUAL|MEMF_SHARED);
            if (!sample->Data[chan]) {
                FreeSample(sample);
                return NULL;
            }
        }
    }
    return sample;
}

int32 AddSampleData(InstanceData *id, int32 offset, int32 length, BOOL clear) {
    struct List *list = id->Samples;
    struct Sample *sample, *sample2;
    int32 num_channels = id->NumChannels;
    int32 sample_size = id->SampleSize;
    int32 chan;
    int32 space, tomove;
    uint8 *src, *dst;
    int32 orig_length = length;

    if (length == 0) return 0;

    if (offset == id->Length) {
        sample = (APTR)GetTail(list);
        offset = sample ? sample->Length : 0;
    } else {
        sample = GetSampleData(list, &offset);
    }
    if (offset == 0) {
        sample2 = AllocSample(num_channels, sample_size);
        if (!sample2) return 0;
        Insert(list, (APTR)sample2, GetPred((APTR)sample));
        sample = sample2;
    }
    if (!sample) return 0;

    space = SAMPLE_LENGTH - sample->Length;
    tomove = sample->Length - offset;

    if (length <= space) {
        sample->Length += length;
        id->Length += length;

        for (chan = 0; chan < num_channels; chan++) {
            src = (uint8 *)sample->Data[chan] + offset * sample_size;
            if (tomove) MoveMem(src, src + length * sample_size, tomove * sample_size);
            if (clear) ClearMem(src, length * sample_size);
        }
        return length;
    }
    if (tomove > 0) {
        sample2 = AllocSample(num_channels, sample_size);
        if (!sample2) return 0;
        Insert(list, (APTR)sample2, (APTR)sample);

        sample->Length -= tomove;
        sample2->Length = tomove;

        for (chan = 0; chan < num_channels; chan++) {
            src = (uint8 *)sample->Data[chan] + offset * sample_size;
            dst = (uint8 *)sample2->Data[chan];
            CopyMem(src, dst, tomove * sample_size);
        }
        space += tomove;
    }

    space = MIN(space, length);
    sample->Length += space;
    id->Length += space;
    if (clear) {
        for (chan = 0; chan < num_channels; chan++) {
            dst = (uint8 *)sample->Data[chan] + offset * sample_size;
            ClearMem(dst, space * sample_size);
        }
    }
    length -= space;
    while (length > 0) {
        sample2 = AllocSample(num_channels, sample_size);
        if (!sample2) return orig_length - length;
        Insert(list, (APTR)sample2, (APTR)sample);
        sample = sample2;

        space = MIN(SAMPLE_LENGTH, length);
        sample->Length = space;
        id->Length += space;
        if (clear) {
            for (chan = 0; chan < num_channels; chan++) {
                dst = (uint8 *)sample->Data[chan];
                ClearMem(dst, space * sample_size);
            }
        }
        length -= space;
    }

    return orig_length;
}

int32 RemoveSampleData(InstanceData *id, int32 offset, int32 length) {
    struct List *list = id->Samples;
    struct Sample *sample, *sample2;
    int32 num_channels = id->NumChannels;
    int32 sample_size = id->SampleSize;
    int32 chan;
    int32 remove;
    int32 tomove;
    uint8 *dst;
    int32 orig_length = length;

    sample = GetSampleData(list, &offset);
    if (!sample) return 0;
    remove = MIN(sample->Length - offset, length);
    tomove = sample->Length - offset - remove;
    sample->Length -= remove;
    id->Length -= remove;
    if (tomove > 0) {
        for (chan = 0; chan < num_channels; chan++) {
            dst = (uint8 *)sample->Data[chan] + offset * sample_size;
            MoveMem(dst + remove * sample_size, dst, tomove * sample_size);
        }
        return remove;
    }
    sample2 = (APTR)GetSucc((APTR)sample);
    if (sample->Length == 0) {
        Remove((APTR)sample);
        FreeSample(sample);
    }
    sample = sample2;
    length -= remove;
    while (length > 0) {
        if (!sample) return orig_length - length;
        remove = MIN(sample->Length, length);
        tomove = sample->Length - remove;
        sample->Length -= remove;
        id->Length -= remove;
        if (tomove > 0) {
            for (chan = 0; chan < num_channels; chan++) {
                dst = (uint8 *)sample->Data[chan];
                MoveMem(dst + remove * sample_size, dst, tomove * sample_size);
            }
            return orig_length;
        }
        sample2 = (APTR)GetSucc((APTR)sample);
        if (sample->Length == 0) {
            Remove((APTR)sample);
            FreeSample(sample);
        }
        sample = sample2;
        length -= remove;
    }
    return orig_length;
}
