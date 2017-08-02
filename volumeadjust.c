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

#include "sounded.h"
#include "project.h"
#include <gadgets/integer.h>
#include <gadgets/chooser.h>
#include <images/label.h>
#include <proto/integer.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include "debug.h"

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ABS(x) (((x) >= 0) ? (x) : -(x))

enum {
    GID_OK = 1,
    GID_CANCEL,
    GID_APPLYTO,
    GID_EFFECT
};

enum {
    EFFECT_CUSTOM,
    EFFECT_FADEIN,
    EFFECT_FADEOUT,
    EFFECT_MAXIMISE,
    EFFECT_SILENCE
};

typedef struct {
    uint32 ApplyTo;
    uint32 Effect;
    uint32 StartVolume; // in %
    uint32 EndVolume; // in %
    uint32 ChanMask;
} VolumeSettings;

static BOOL GetVolumeSettings (Project *project, VolumeSettings *vol, BOOL area_selected);

void AdjustVolume (Project *project) {
    Object *sound = project->Sound;
    int32 offset, length;
    int32 effect;
    int32 startvolume, endvolume;
    uint32 chanmask;
    int32 *buffer = NULL, *ptr;
    int32 num_channels, frame_size;
    const uint32 buflen = 1024;
    uint32 bufsiz, getlen;
    struct sndArrayIO saio;
    int32 chan, frames;
    float64 scale, delta;
    float64 sample;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    {
        VolumeSettings vol;
        uint32 area_selected;

        GetAttr(SOUND_AreaSelected, sound, &area_selected);
        vol.ApplyTo = area_selected ? APPLY_SELECTION : APPLY_SOUND;
        vol.Effect = EFFECT_CUSTOM;
        vol.StartVolume = 100;
        vol.EndVolume = 100;
        vol.ChanMask = 0x03;

        if (!GetVolumeSettings(project, &vol, area_selected)) {
            dbug(("canceled\n"));
            goto out;
        }

        effect = vol.Effect;
        startvolume = vol.StartVolume;
        endvolume = vol.EndVolume;
        chanmask = vol.ChanMask;
        dbug(("apply to: %ld\n", vol.ApplyTo));
        dbug(("start: %ld\n", startvolume));
        dbug(("end: %ld\n", endvolume));
        dbug(("chanmask: %08lx\n", chanmask));
        if (!GetArea(sound, vol.ApplyTo, &offset, &length, &chanmask)) {
            dbug(("no area to apply to\n"));
            goto out;
        }
        dbug(("chanmask: %08lx\n", chanmask));
    }

    {
        uint32 tmp;
        num_channels = 0;
        for (tmp = chanmask; tmp; tmp >>= 1) {
            if (tmp & 1) num_channels++;
        }
    }


    dbug(("num channels: %ld\n", num_channels));
    dbug(("offset: %ld\n", offset));
    dbug(("length: %ld\n", length));
    frame_size = num_channels << 2;
    bufsiz = buflen * frame_size;
    buffer = AllocVec(bufsiz, MEMF_PRIVATE);
    if (!buffer) {
        dbug(("no memory\n"));
        goto out;
    }

    saio.ArrayPtr = buffer;
    saio.ChanMask = chanmask;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = 4;
    saio.BytesPerFrame = frame_size;
    if (effect == EFFECT_MAXIMISE) {
        int32 orig_offset = offset;
        int32 orig_length = length;
        uint32 max = 0, current;
        saio.MethodID = SNDM_READSAMPLEARRAY;
        saio.GInfo = NULL;
        while (length > 0) {
            getlen = MIN(buflen, length);
            saio.Offset = offset;
            saio.Length = getlen;
            DoMethodA(sound, (Msg)&saio);

            frames = getlen;
            ptr = buffer;
            while (frames--) {
                for (chan = 0; chan < num_channels; chan++) {
                    current = ABS(*ptr);
                    if (current > max) max = current;
                    ptr++;
                }
            }
            if (max >= 0x7fffffffUL) break;
            offset += getlen;
            length -= getlen;
        }
        if (max == 0 || max >= 0x7fffffffUL) {
            scale = 1.0;
            delta = 0.0;
        } else {
            scale = 2147483647.0 / (float64)max;
            delta = 0.0;
        }
        offset = orig_offset;
        length = orig_length;
    } else {
        scale = (float64)startvolume / 100.0;
        delta = (float64)(endvolume - startvolume) / ((float64)length * 100.0);
    }
    while (length > 0) {
        getlen = MIN(buflen, length);
        saio.MethodID = SNDM_READSAMPLEARRAY;
        saio.GInfo = NULL;
        saio.Offset = offset;
        saio.Length = getlen;
        DoMethodA(sound, (Msg)&saio);

        frames = getlen;
        ptr = buffer;
        while (frames--) {
            for (chan = 0; chan < num_channels; chan++) {
                sample = (float64)(*ptr) * scale;
                if (sample > 2147483647.0) {
                    *ptr++ = 0x7fffffff;
                } else if (sample < -2147483648.0) {
                    *ptr++ = -0x80000000;
                } else {
                    *ptr++ = sample;
                }
            }
            scale += delta;
        }

        saio.MethodID = SNDM_WRITESAMPLEARRAY;
        saio.GInfo = NULL;
        DoGadgetMethodA(G(sound), project->IWindow, NULL, (Msg)&saio);

        offset += getlen;
        length -= getlen;
        dbug(("offset: %ld\n", offset));
        dbug(("length: %ld\n", length));
    }

out:
    FreeVec(buffer);

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);
}

static BOOL GetVolumeSettings (Project *project, VolumeSettings *vol, BOOL area_selected) {
    struct Window *main_window = project->IWindow;
    struct StartupParams *params = project->Params;
    Object *sound = project->Sound;
    uint32 num_channels;
    Object *window;
    Object *applyto;
    Object *channels;
    Object *effect;
    Object *startvolume;
    Object *endvolume;
    struct Window *win;
    BOOL done = FALSE;
    BOOL no_selection = !area_selected;
    const char *apply_labels[4] = {
        STR_ID(MSG_SELECTION), STR_ID(MSG_VIEWAREA), STR_ID(MSG_WHOLESOUND), STR_ID(-1)
    };
    const char *chan_labels[4] = {
        STR_ID(MSG_LEFT_CHANNEL), STR_ID(MSG_RIGHT_CHANNEL), STR_ID(MSG_BOTH_CHANNELS), STR_ID(-1)
    };
    const char *effect_labels[6] = {
        STR_ID(MSG_CUSTOM), STR_ID(MSG_FADEIN), STR_ID(MSG_FADEOUT), STR_ID(MSG_MAXIMISE),
        STR_ID(MSG_SILENCE), STR_ID(-1)
    };

    GetAttr(SOUND_NumChannels, sound, &num_channels);

    TranslateLabelArray(&LocaleInfo, apply_labels);
    TranslateLabelArray(&LocaleInfo, chan_labels);
    TranslateLabelArray(&LocaleInfo, effect_labels);

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
                LAYOUT_Label,           GetString(&LocaleInfo, MSG_ADJUSTVOLUME_LBL),
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_AddChild,        applyto = ChooserObject,
                    GA_ID,              GID_APPLYTO,
                    GA_RelVerify,       TRUE,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, apply_labels + no_selection,
                    CHOOSER_Selected,   MAX(vol->ApplyTo - 1, no_selection) - no_selection,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_APPLYTO_GAD),
                End,
                LAYOUT_AddChild,        channels = ChooserObject,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, chan_labels,
                    CHOOSER_Selected,   vol->ChanMask - 1,
                    GA_Disabled,        (num_channels == 1 || vol->ApplyTo == APPLY_SELECTION) ? TRUE : FALSE,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_CHANNELS_GAD),
                End,
                LAYOUT_AddChild,        effect = ChooserObject,
                    GA_ID,              GID_EFFECT,
                    GA_RelVerify,       TRUE,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, effect_labels,
                    CHOOSER_Selected,   vol->Effect - 1,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_EFFECT_GAD),
                End,
                LAYOUT_AddChild,        startvolume = IntegerObject,
                    INTEGER_Minimum,    0,
                    INTEGER_Maximum,    1000000000,
                    INTEGER_Number,     vol->StartVolume,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_START_GAD),
                End,
                LAYOUT_AddChild,        endvolume = IntegerObject,
                    INTEGER_Minimum,    0,
                    INTEGER_Maximum,    1000000000,
                    INTEGER_Number,     vol->EndVolume,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_END_GAD),
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

                            case GID_APPLYTO:
                                if (num_channels == 1) break;
                                SetGadgetAttrs(G(channels), win, NULL,
                                    GA_Disabled,    ((code + no_selection) == APPLY_SELECTION) ?
                                                    TRUE : FALSE,
                                    TAG_END);
                                break;

                            case GID_EFFECT:
                                switch (code) {
                                    case EFFECT_FADEIN:
                                        vol->StartVolume = 0;
                                        vol->EndVolume = 100;
                                        break;
                                    case EFFECT_FADEOUT:
                                        vol->StartVolume = 100;
                                        vol->EndVolume = 0;
                                        break;
                                    case EFFECT_MAXIMISE:
                                        break;
                                    case EFFECT_SILENCE:
                                        vol->StartVolume = 0;
                                        vol->EndVolume = 0;
                                        break;
                                }
                                SetGadgetAttrs(G(startvolume), win, NULL,
                                    GA_Disabled,    (code != EFFECT_CUSTOM) ? TRUE : FALSE,
                                    INTEGER_Number, vol->StartVolume,
                                    TAG_END);
                                SetGadgetAttrs(G(endvolume), win, NULL,
                                    GA_Disabled,    (code != EFFECT_CUSTOM) ? TRUE : FALSE,
                                    INTEGER_Number, vol->EndVolume,
                                    TAG_END);
                                break;

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
            GetAttr(CHOOSER_Selected, applyto, &vol->ApplyTo);
            vol->ApplyTo += no_selection;
            GetAttr(CHOOSER_Selected, channels, &vol->ChanMask);
            vol->ChanMask++;
            GetAttr(CHOOSER_Selected, effect, &vol->Effect);
            GetAttr(INTEGER_Number, startvolume, &vol->StartVolume);
            GetAttr(INTEGER_Number, endvolume, &vol->EndVolume);
        }
    }

    DisposeObject(window);

    return (done == GID_OK) ? TRUE : FALSE;
}
