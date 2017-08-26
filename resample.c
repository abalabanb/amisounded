/*
 * AmiSoundED - Sound Editor
 * Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
 * Copyright (C) 2017 Alexandre Balaban <github@balaban.fr>
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

#include "sounded.h"
#include "project.h"
#include <gadgets/integer.h>
#include <gadgets/chooser.h>
#include <images/label.h>
#include <proto/integer.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include "debug.h"
#include <math.h>

#define G(o) ((struct Gadget *)(o))
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#define SCALE(val,newmax,oldmax) (((uint64)(val))*((uint64)(newmax))/((uint64)(oldmax)))

enum {
    GID_OK = 1,
    GID_CANCEL
};

typedef struct {
    uint32 Frequency;
    uint32 SampleSize;
} ResampleSettings;

static BOOL GetResampleSettings (Project *project, ResampleSettings *resample);

void Resample (Project *project) {
    struct StartupParams *params = project->Params;
    struct Window *window = project->IWindow;
    Object *old_sound = project->Sound;
    Object *new_sound = NULL;
    int32 *inbuffer = NULL;
    int32 *outbuffer = NULL;
    int32 *in, *out;
    int32 num_channels, chan;
    int32 old_sample_size, sample_size;
    uint32 old_frequency, new_frequency;
    int32 old_length, new_length, getlen, i;
    int32 outoffset;
    const uint32 buflen = 1024;
    const uint32 bufsiz = 1024 << 2;
    struct sndArrayIO sain, saout;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    {
        ResampleSettings resample;

        GetAttrs(old_sound,
            SOUND_Frequency,    &resample.Frequency,
            SOUND_SampleSize,   &resample.SampleSize,
            SOUND_NumChannels,  &num_channels,
            SOUND_Length,       &old_length,
            SOUND_SampleSize,   &old_sample_size,
            TAG_END);
        old_frequency = resample.Frequency;

        if (!GetResampleSettings(project, &resample)) {
            dbug(("canceled\n"));
            goto out;
        }

        new_frequency = resample.Frequency;
        sample_size = resample.SampleSize;
        new_length = SCALE(old_length, new_frequency, old_frequency);
    }

    if (new_length == old_length && sample_size == old_sample_size) {
        dbug(("no change\n"));
        goto out;
    }

    new_sound = ClassObject(params->WaveClass),
        GA_ID,          GID_SOUNDGC,
        GA_RelVerify,   TRUE,
    End;
    if (!new_sound) {
        goto out;
    }
    if (new_length == old_length) {
        inbuffer = AllocVecTags(bufsiz*num_channels, AVT_Type, MEMF_PRIVATE, TAG_END);
        if (!inbuffer) {
            DisposeObject(new_sound);
            new_sound = NULL;
            goto out;
        }
    } else {
        inbuffer = AllocVecTags(num_channels << 3, AVT_Type, MEMF_PRIVATE, TAG_END);
        outbuffer = AllocVecTags(bufsiz*num_channels, AVT_Type, MEMF_PRIVATE, TAG_END);
        if (!inbuffer || !outbuffer) {
            DisposeObject(new_sound);
            new_sound = NULL;
            goto out;
        }
    }
    SetAttrs(new_sound,
        SOUND_Length,       new_length,
        SOUND_Frequency,    new_frequency,
        SOUND_SampleSize,   sample_size,
        SOUND_NumChannels,  num_channels,
        TAG_END);
    if (GetSoundLength(new_sound) != new_length) {
        DisposeObject(new_sound);
        goto out;
    }
    SetAttrs(project->HProp, ICA_TARGET, NULL, TAG_END);
    SetAttrs(project->Sound, ICA_TARGET, NULL, TAG_END);
    if (SetGadgetAttrs(G(project->Layout), window, NULL,
        LAYOUT_ModifyChild,     old_sound,
        CHILD_ReplaceObject,    new_sound,
        CHILD_NoDispose,        TRUE,
        TAG_END) == 1)
    {
        RethinkLayout(G(project->Layout), window, NULL, TRUE);
    }
    project->Sound = new_sound;
    SetAttrs(project->HProp, ICA_TARGET, project->Sound, TAG_END);
    SetAttrs(project->Sound, ICA_TARGET, project->HProp, TAG_END);
    Project_WindowTitle(project);

    if (new_length == old_length) {
        sain.ArrayPtr = inbuffer;
        sain.Offset = 0;
        sain.ChanMask = (1 << num_channels) - 1;
        sain.Mode = SNDRM_DEFAULT;
        sain.BytesPerSample = 4;
        sain.BytesPerFrame = num_channels << 2;
        while (old_length > 0) {
            getlen = MIN(buflen, old_length);
            sain.MethodID = SNDM_READSAMPLEARRAY;
            sain.GInfo = NULL;
            sain.Length = getlen;
            DoMethodA(old_sound, (Msg)&sain);

            sain.MethodID = SNDM_WRITESAMPLEARRAY;
            sain.GInfo = NULL;
            DoGadgetMethodA(G(new_sound), window, NULL, (Msg)&sain);

            sain.Offset += getlen;
            old_length -= getlen;
        }
    } else {
        float64 ratio = (float64)(old_length - 1) / (float64)(new_length - 1);
        float64 inoffset, x1, x2, y1, y2;
        sain.MethodID = SNDM_READSAMPLEARRAY;
        sain.GInfo = NULL;
        sain.ArrayPtr = inbuffer;
        sain.ChanMask = (1 << num_channels) - 1;
        sain.Mode = SNDRM_DEFAULT;
        sain.BytesPerSample = 4;
        sain.BytesPerFrame = num_channels << 2;
        saout.MethodID = SNDM_WRITESAMPLEARRAY;
        saout.ArrayPtr = outbuffer;
        saout.ChanMask = (1 << num_channels) - 1;
        saout.Mode = SNDWM_REPLACE;
        saout.BytesPerSample = 4;
        saout.BytesPerFrame = num_channels << 2;
        outoffset = 0;
        while (new_length > 0) {
            getlen = MIN(buflen, new_length);
            
            out = outbuffer;
            for (i = 0; i < getlen; i++) {
                inoffset = (float64)outoffset * ratio;
                x1 = inoffset - floor(inoffset);
                x2 = 1.0 - x1;
            
                sain.Offset = (int32)inoffset;
                sain.Length = MIN(2, old_length - sain.Offset);
                DoMethodA(old_sound, (Msg)&sain);
                
                in = inbuffer;
                for (chan = 0; chan < num_channels; chan++) {
                    y1 = in[0];
                    y2 = in[num_channels];
                    *out++ = (int32)(x1*y2 + x2*y1);
                    in++;
                }
                outoffset++;
            }
            
            saout.GInfo = NULL;
            saout.Offset = outoffset;
            saout.Length = getlen;
            DoGadgetMethodA(G(new_sound), window, NULL, (Msg)&saout);
            
            new_length -= getlen;
        }
    }

out:
    if (new_sound) {
        DisposeObject(old_sound);
        DoGadgetMethod(G(new_sound), project->IWindow, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
        Project_UpdateGUI(project);
    }
    FreeVec(inbuffer);
    FreeVec(outbuffer);

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
}

static BOOL GetResampleSettings (Project *project, ResampleSettings *resample) {
    struct Window *main_window = project->IWindow;
    struct StartupParams *params = project->Params;
    Object *window;
    Object *frequency;
    Object *samplesize;
    struct Window *win;
    static const char * const samplesizes[5] = {
        "8-bit", "16-bit", "24-bit", "32-bit", NULL
    };
    BOOL done = FALSE;

    window = WindowObject,
        WA_PubScreen,       project->Screen,
        WA_Flags,           WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET
                            |WFLG_NOCAREREFRESH|WFLG_ACTIVATE,
        WA_IDCMP,           IDCMP_CLOSEWINDOW|IDCMP_GADGETUP,
        WA_Title,           progname,
        WINDOW_Position,    main_window ? WPOS_CENTERWINDOW : WPOS_CENTERSCREEN,
        WINDOW_RefWindow,   main_window,
        WINDOW_CharSet,     LocaleInfo.CodeSet,
        WINDOW_Layout,      VLayoutObject,
            LAYOUT_AddChild,    VLayoutObject,
                LAYOUT_BevelStyle,      BVS_SBAR_VERT,
                LAYOUT_Label,           GetString(&LocaleInfo, MSG_RESAMPLE_LBL),
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_AddChild,        frequency = IntegerObject,
                    INTEGER_Number,     resample->Frequency,
                    INTEGER_Minimum,    4000,
                    INTEGER_Maximum,    96000,
                    INTEGER_MinVisible, 6,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_FREQUENCY_GAD),
                End,
                LAYOUT_AddChild,        samplesize = ChooserObject,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, samplesizes,
                    CHOOSER_Selected,   resample->SampleSize-1,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_SAMPLESIZE_GAD),
                End,
            End,
            LAYOUT_AddChild,    HLayoutObject,
                LAYOUT_BevelStyle,      BVS_SBAR_VERT,
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_BottomSpacing,   0,
                LAYOUT_EvenSize,        TRUE,
                LAYOUT_AddChild,        ButtonObject,
                    GA_ID,              GID_OK,
                    GA_RelVerify,       TRUE,
                    GA_Text,            GetString(&LocaleInfo, MSG_OK_GAD),
                End,
                CHILD_WeightedWidth,    0,
                LAYOUT_AddChild,        ButtonObject,
                    GA_ID,              GID_CANCEL,
                    GA_RelVerify,       TRUE,
                    GA_Text,            GetString(&LocaleInfo, MSG_CANCEL_GAD),
                End,
                CHILD_WeightedWidth,    0,
            End,
        End,
    End;

    if (window && (win = RA_OpenWindow(window))) {
        uint32 sigmask;
        uint32 res;
        uint16 code;
        GetAttr(WINDOW_SigMask, window, &sigmask);
        while (!done) {
            Wait(sigmask);
            while (WMHI_LASTMSG != (res = RA_HandleInput(window, &code))) {
                switch (res & WMHI_CLASSMASK) {

                    case WMHI_CLOSEWINDOW:
                        done = GID_CANCEL;
                        break;

                    case WMHI_GADGETUP:
                        switch (res & WMHI_GADGETMASK) {

                            case GID_OK:
                                done = GID_OK;
                                break;

                            case GID_CANCEL:
                                done = GID_CANCEL;
                                break;

                        }
                        break;

                }
            }
        }
        if (done == GID_OK) {
            GetAttr(INTEGER_Number, frequency, &resample->Frequency);
            GetAttr(CHOOSER_Selected, samplesize, &resample->SampleSize);
            resample->SampleSize++;
        }
    }

    DisposeObject(window);

    return (done == GID_OK) ? TRUE : FALSE;
}
