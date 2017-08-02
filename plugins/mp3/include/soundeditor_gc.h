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

#ifndef SOUNDEDITOR_GC_H
#define SOUNDEDITOR_GC_H

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#ifndef GADGETS_SCROLLER_H
#include <gadgets/scroller.h>
#endif

void InitGradSpec(struct GradientSpec *grad, int angle, uint32 argb1, uint32 argb2);
Class *SOUNDEDITOR_MakeClass ();
void SOUNDEDITOR_FreeClass (Class *cl);

#define SOUND_HPropTotal    SCROLLER_Total
#define SOUND_HPropTop      SCROLLER_Top
#define SOUND_HPropVisible  SCROLLER_Visible

enum {
    SOUND_Dummy = 0x81010000,
    SOUND_PreserveData,
    SOUND_Length,
    SOUND_NumChannels,
    SOUND_SampleSize,
    SOUND_SamplesPerSec,
    SOUND_Selection,
    SOUND_ChanMask,
    SOUND_PlayMarker,
    SOUND_AreaSelected,
    SOUND_View,
    SOUND_HasSelection,
    SOUND_HasChanged
};
#define SOUND_Frequency     SOUND_SamplesPerSec
#define SOUND_FramesPerSec  SOUND_SamplesPerSec

struct SoundSelection {
    uint32 FirstSample, LastSample;
};

enum {
    SNDM_DUMMY = 0x81010000,
    SNDM_WRITESAMPLEARRAY,
    SNDM_READSAMPLEARRAY,
    SNDM_ZOOM,
    SNDM_DELETESAMPLES
};

struct sndArrayIO {
    uint32 MethodID;
    struct GadgetInfo *GInfo;
    void *ArrayPtr;
    uint32 Offset;
    uint32 Length;
    uint8 ChanMask;
    uint8 Mode;
    uint8 BytesPerSample;
    uint8 BytesPerFrame;
};

enum {
    SNDRM_DEFAULT = 0
};

enum {
    SNDWM_REPLACE = 0,
    SNDWM_INSERT
};

struct sndZoom {
    uint32 MethodID;
    struct GadgetInfo *GInfo;
    uint32 ZoomType;
};

enum {
    SOUND_ZOOMOUTFULL = 0,
    SOUND_ZOOMIN2X,
    SOUND_ZOOMOUT2X,
    SOUND_ZOOMSELECTION
};

struct sndDeleteSamples {
    uint32 MethodID;
    struct GadgetInfo *GInfo;
    uint32 Offset;
    uint32 Length;
};

#endif /* SOUNDEDITOR_GC_H */
