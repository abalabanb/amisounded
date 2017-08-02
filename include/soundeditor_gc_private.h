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

#ifndef SOUNDEDITOR_GC_PRIVATE_H
#define SOUNDEDITOR_GC_PRIVATE_H

#ifndef SOUNDEDITOR_GC_H
#include "soundeditor_gc.h"
#endif /* SOUNDEDITOR_GC_H */

#ifndef ASOT_MACROS_H
#include "asot_macros.h"
#endif

enum {
    COL_BKG1 = 0,
    COL_BKG2,
    COL_LINES,
    COL_WAVEFORM,
    COL_ALTBKG1,
    COL_ALTBKG2,
    COL_ALTWAVEFORM,
    NUM_COLORS
};

enum {
    PEN_BKG = 0,
    PEN_LINES,
    PEN_WAVEFORM,
    PEN_ALTBKG,
    PEN_ALTWAVEFORM,
    NUM_PENS
};

#define GLOB_DATA(cl) ((GlobalData *)(cl)->cl_UserData)

typedef struct {
    uint32 Colors[NUM_COLORS];
    uint8 Pen2Col[NUM_PENS];
    uint8 Pen2Dri[NUM_PENS];
    struct GradientSpec GradSpec[4];
} GlobalData;

#define MAX_CHANNELS 2

struct Damage {
    struct Damage *NextDamage;
    uint16 StartX, EndX;
    uint8 Channel;
    uint8 Pad[3];
};
#define MAX_DAMAGE (MAX_CHANNELS*4)

struct Sample {
    struct MinNode Link;
    uint32 Length;
    void *Data[MAX_CHANNELS];
};
#define SAMPLE_LENGTH (64 << 10)

typedef struct {
    struct SignalSemaphore *Lock;

    struct List *Samples;
    uint32 Length;
    uint32 SamplesPerSec;
    uint8 NumChannels, SampleSize;
    uint8 ChanMask;
    uint8 Flags;

    struct {
        uint16 Total, Top, Visible;
        uint16 Factor;
    } HProp;
    struct {
        uint32 StartSample;
        uint32 EndSample;
    } View;

    struct {
        uint32 StartSample;
        uint32 EndSample;
    } Selection;
    struct {
        uint32 Sample;
    } PlayMarker;

    struct Screen *Screen;
    struct DrawInfo *DrawInfo;
    struct Window *Window;
    struct Requester *Requester;
    struct Gadget *Gadget;

    struct ColorMap *ColorMap;
    BOOL Truecolor;
    uint16 Pens[NUM_PENS];

    struct Damage *FirstDamage, *LastDamage;
    struct Damage DamageList[MAX_DAMAGE];
} InstanceData;

#define SNDFLG_SCROLLED     1
#define SNDFLG_HASCHANGED   2
#define SNDFLG_ISACTIVE     4
#define SNDFLG_DMGOVERFLOW  8

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#define CLAMP(x,y,z) MAX(MIN(x,z),y)
#define SCALE(val,newmax,oldmax) (((uint64)(val))*((uint64)(newmax))/((uint64)(oldmax)))

/* soundeditor_samples.c */
struct Sample *GetSampleData(struct List *list, int32 *offset_ptr);
void FreeSample(struct Sample *sample);
struct Sample *AllocSample(int32 num_channels, uint32 sample_size);
int32 AddSampleData(InstanceData *id, int32 offset, int32 length, BOOL clear);
int32 RemoveSampleData(InstanceData *id, int32 offset, int32 length);

#endif /* SOUNDEDITOR_GC_PRIVATE_H */
