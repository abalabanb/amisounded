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
#include <devices/ahi.h>
#include "project.h"
#include "record2file.h"
#include "requesters.h"
#include <classes/requester.h>
#include <gadgets/integer.h>
#include <gadgets/chooser.h>
#include <gadgets/checkbox.h>
#include <gadgets/getfile.h>
#include <images/label.h>
#include <proto/integer.h>
#include <proto/chooser.h>
#include <proto/checkbox.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/graphics.h>
#include <proto/requester.h>
#include "debug.h"
#include "path.h"

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

enum {
    GID_OK = 1,
    GID_CANCEL,
    GID_DESTINATION,
    GID_FILENAME
};

enum {
    DEST_RAM = 0,
    DEST_FILE
};

typedef struct {
    uint32 Destination;
    const char *Filename;
    uint32 Frequency; // in Hz
    uint32 NumChannels;
    uint32 SampleSize; // in bytes
    uint32 MaxLength; // in seconds
} RecordSettings;

static BOOL GetRecordSettings (Project *project, RecordSettings *rec);

void RecordSound (Project *project) {
    struct Screen *screen = project->Screen;
    Object *sound = project->Sound;
    int32 destination;
    const char *filename = NULL;
    APTR file = NULL;
    int32 offset, maxlength, chanmask, buflen, reclen;
    uint32 orig_sample_size, sample_size, frame_size;
    uint32 frequency;
    uint32 num_channels;
    uint32 ahi_fmt;
    uint32 buffer_size;
    void *buf = NULL, *buf2 = NULL;
    struct MsgPort *mp;
    struct AHIRequest *io, *io2, *join;
    struct sndArrayIO saio;
    APTR tmp;
    uint32 signals, sig_ahi;
    struct Task *task;
    int8 oldpri;

    if (!OpenAHI(project)) {
        Object *req;
        req = RequesterObject,
            REQ_CharSet,    LocaleInfo.CodeSet,
            REQ_Image,      REQIMAGE_ERROR,
            REQ_TitleText,  progname,
            REQ_BodyText,   GetString(&LocaleInfo, MSG_NOAHI_REQ),
            REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
            TAG_END);
        DoReq(screen, project->Window, req);
        goto out;
    }

    {
        RecordSettings rec;

        rec.Destination = DEST_RAM;
        rec.Filename = NULL;
        rec.Frequency = 44100;
        rec.NumChannels = 1;
        rec.SampleSize = 2;
        rec.MaxLength = 0;

        if (!GetRecordSettings(project, &rec)) {
            dbug(("recording cancelled\n"));
            filename = rec.Filename;
            goto out;
        }

        destination = rec.Destination;
        filename = rec.Filename;
        frequency = rec.Frequency;
        num_channels = rec.NumChannels;
        chanmask = (1 << num_channels) - 1;
        sample_size = rec.SampleSize;
        maxlength = rec.MaxLength;
    }

    orig_sample_size = sample_size;
    switch (sample_size) {
        case 0:
            dbug(("unsupported sample size: %ld\n", sample_size));
            goto out;
        case 1:
            ahi_fmt = (num_channels == 1) ? AHIST_M8S : AHIST_S8S;
            break;
        case 2:
            ahi_fmt = (num_channels == 1) ? AHIST_M16S : AHIST_S16S;
            break;
        default:
            ahi_fmt = (num_channels == 1) ? AHIST_M32S : AHIST_S32S;
            sample_size = 4;
            break;
    }

    buflen = frequency;
    frame_size = num_channels * sample_size;
    buffer_size = buflen * frame_size;

    mp = project->AHImp;
    io = project->AHIio;
    io2 = project->AHIio2;
    join = NULL;

    buf = AllocVec(buffer_size, MEMF_SHARED);
    buf2 = AllocVec(buffer_size, MEMF_SHARED);
    if (!buf || !buf2) {
        dbug(("failed to allocate buffers (size = %ld)\n", buffer_size));
        goto out;
    }

    switch (destination) {

        case DEST_RAM:
            SetAttrs(sound,
                SOUND_Length,       0,
                SOUND_Frequency,    frequency,
                SOUND_NumChannels,  num_channels,
                SOUND_SampleSize,   orig_sample_size,
                TAG_END);
            break;
            
        case DEST_FILE:
            file = StartRecord2File(filename, frequency, num_channels, orig_sample_size);
            if (!file) goto out;
            break;
            
        default:
            goto out;
            
    }

    project->Flags |= PRFLG_PLAYING;
    sig_ahi = 1 << project->AHImp->mp_SigBit;
    project->Signals[SIGNAL_AHI] = sig_ahi;
    Project_UpdateGUI(project);
    Project_WindowTitle(project);

    task = FindTask(NULL);
    oldpri = SetTaskPri(task, 10);

    offset = 0;
    saio.MethodID = SNDM_WRITESAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ChanMask = chanmask;
    saio.Mode = SNDWM_INSERT;
    saio.BytesPerSample = sample_size;
    saio.BytesPerFrame = frame_size;
    
    io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
    io->ahir_Std.io_Command = CMD_READ;
    io->ahir_Std.io_Data = buf;
    io->ahir_Std.io_Length = buffer_size;
    io->ahir_Std.io_Offset = 0;
    io->ahir_Frequency = frequency;
    io->ahir_Type = ahi_fmt;
    io->ahir_Volume = 0x10000;
    io->ahir_Position = 0x8000;
    io->ahir_Link = NULL;
    
    SendIO((APTR)io);
    
    join = io;
    tmp = buf; buf = buf2; buf2 = tmp;
    tmp = io; io = io2; io2 = tmp;
    while (!maxlength || --maxlength) {
        io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
        io->ahir_Std.io_Command = CMD_READ;
        io->ahir_Std.io_Data = buf;
        io->ahir_Std.io_Length = buffer_size;
        io->ahir_Std.io_Offset = 0;
        io->ahir_Frequency = frequency;
        io->ahir_Type = ahi_fmt;
        io->ahir_Volume = 0x10000;
        io->ahir_Position = 0x8000;
        io->ahir_Link = NULL;
        
        for (;;) {
            signals = Wait(Project_SigMask(project));
            
            Project_HandleEvents(project, signals);
            
            if (!(project->Flags & PRFLG_PLAYING)) {
                AbortIO((APTR)io);
                maxlength = -1;
                break;
            }

            if (GetMsg(mp) == (APTR)join) break;
        }
        if (maxlength == -1) break;

        SendIO((APTR)io);

        reclen = join->ahir_Std.io_Actual / frame_size;
        switch (destination) {
        
            case DEST_RAM:
                saio.ArrayPtr = buf2;
                saio.Offset = offset;
                saio.Length = reclen;
                DoMethodA(sound, (Msg)&saio);
                if (project->IWindow && AttemptLockLayerRom(screen->BarLayer)) {
                    UnlockLayerRom(screen->BarLayer);
                    DoGadgetMethod(G(sound), project->IWindow, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
                    Project_WindowTitle(project);
                }
                break;
                
            case DEST_FILE:
                Record2File(file, buf2, reclen);
                break;
                
        }
        offset += reclen;
        
        join = io;
        tmp = buf; buf = buf2; buf2 = tmp;
        tmp = io; io = io2; io2 = tmp;
    }

    WaitIO((APTR)join);
    reclen = join->ahir_Std.io_Actual / frame_size;
    switch (destination) {
        
        case DEST_RAM:
            saio.ArrayPtr = buf2;
            saio.Offset = offset;
            saio.Length = reclen;
            DoMethodA(sound, (Msg)&saio);
            break;
        
        case DEST_FILE:
            Record2File(file, buf2, reclen);
            break;
            
    }

    SetTaskPri(task, oldpri);

    project->Flags &= ~PRFLG_PLAYING;
    project->Signals[SIGNAL_AHI] = 0;
    Project_UpdateGUI(project);

    DoGadgetMethod(G(sound), project->IWindow, NULL, SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
out:
    StopRecord2File(file, FALSE);
    FreeVec(buf);
    FreeVec(buf2);
    FreeVec((APTR)filename);
    CloseAHI(project);
}

static BOOL GetRecordSettings (Project *project, RecordSettings *rec) {
    struct Window *main_window = project->IWindow;
    Object *window;
    Object *destination;
    Object *filename;
    Object *frequency;
    Object *channels;
    Object *samplesize;
    Object *maxlength;
    struct Window *win;
    const char *chan_labels[3] = {
        STR_ID(MSG_MONO), STR_ID(MSG_STEREO), STR_ID(-1)
    };
    static const char * const samplesizes[5] = {
        "8-bit", "16-bit", "24-bit", "32-bit", NULL
    };
    BOOL done = FALSE;

    SetAttrs(project->Window, WA_BusyPointer, TRUE, TAG_END);

    TranslateLabelArray(&LocaleInfo, chan_labels);

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
                LAYOUT_Label,           GetString(&LocaleInfo, MSG_RECORDSETTINGS_LBL),
                LAYOUT_SpaceOuter,      TRUE,
                LAYOUT_AddChild,        destination = CheckBoxObject,
                    GA_ID,              GID_DESTINATION,
                    GA_RelVerify,       TRUE,
                    GA_Selected,        rec->Destination == DEST_FILE,
                End,
                CHILD_WeightedWidth,    0,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_RECORDTOFILE_GAD),
                End,
                LAYOUT_AddChild,        filename = GetFileObject,
                    GA_ID,              GID_FILENAME,
                    GA_RelVerify,       TRUE,
                    GA_Disabled,        rec->Destination != DEST_FILE,
                    GETFILE_DoSaveMode, TRUE,
                    GETFILE_FullFile,   rec->Filename,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_FILENAME_GAD),
                End,
                LAYOUT_AddChild,        frequency = IntegerObject,
                    INTEGER_Number,     rec->Frequency,
                    INTEGER_Minimum,    4000,
                    INTEGER_Maximum,    96000,
                    INTEGER_MinVisible, 6,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_FREQUENCY_GAD),
                End,
                LAYOUT_AddChild,        channels = ChooserObject,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, chan_labels,
                    CHOOSER_Selected,   rec->NumChannels-1,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_CHANNELS_GAD),
                End,
                LAYOUT_AddChild,        samplesize = ChooserObject,
                    CHOOSER_PopUp,      TRUE,
                    CHOOSER_LabelArray, samplesizes,
                    CHOOSER_Selected,   rec->SampleSize-1,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_SAMPLESIZE_GAD),
                End,
                LAYOUT_AddChild,        maxlength = IntegerObject,
                    INTEGER_Number,     rec->MaxLength,
                    INTEGER_Minimum,    0,
                    INTEGER_Maximum,    32000,
                    INTEGER_MinVisible, 4,
                End,
                CHILD_Label,            LabelObject,
                    LABEL_Text,         GetString(&LocaleInfo, MSG_RECORDLENGTH_GAD),
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

                            case GID_DESTINATION:
                                SetGadgetAttrs(G(filename), win, NULL,
                                    GA_Disabled,    !code,
                                    TAG_END);
                                break;

                            case GID_FILENAME:
                                if (DoMethod(filename, GFILE_REQUEST, win)) {
                                    FreeVec((APTR)rec->Filename);
                                    GetAttr(GETFILE_FullFile, filename, (ULONG*)&rec->Filename);
                                    rec->Filename = Strdup(rec->Filename);
                                }
                                break;

                        }
                        break;

                }
            }
        }
        if (done == GID_OK) {
            GetAttr(GA_Selected, destination, &rec->Destination);
            rec->Destination = rec->Destination ? DEST_FILE : DEST_RAM;
            GetAttr(INTEGER_Number, frequency, &rec->Frequency);
            GetAttr(CHOOSER_Selected, channels, &rec->NumChannels);
            rec->NumChannels++;
            GetAttr(CHOOSER_Selected, samplesize, &rec->SampleSize);
            rec->SampleSize++;
            GetAttr(INTEGER_Number, maxlength, &rec->MaxLength);
        }
    }

    DisposeObject(window);

    SetAttrs(project->Window, WA_BusyPointer, FALSE, TAG_END);

    dbug(("done: %ld\n", done));
    return (done == GID_OK) ? TRUE : FALSE;
}
