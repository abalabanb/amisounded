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
#include "debug.h"
#include "requesters.h"
#include <classes/requester.h>
#include <proto/graphics.h>
#include <proto/requester.h>

#define G(o) ((struct Gadget *)(o))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

BOOL OpenAHI (Project *project) {
    project->AHImp = CreateMsgPort();
    project->AHIio = (APTR)CreateIORequest(project->AHImp, sizeof(struct AHIRequest));
    project->AHIio2 = (APTR)CreateIORequest(project->AHImp, sizeof(struct AHIRequest));
    if (!project->AHIio || !project->AHIio2) return FALSE;
    project->AHIio->ahir_Version = 4;
    if (!OpenDevice("ahi.device", 0, (APTR)project->AHIio, 0)) {
        CopyMem(project->AHIio, project->AHIio2, sizeof(struct AHIRequest));
        return TRUE;
    } else {
        project->AHIio->ahir_Std.io_Device = NULL;
        return FALSE;
    }
}

void CloseAHI (Project *project) {
    if (project->AHIio && project->AHIio2) {
        struct IORequest *io = (APTR)project->AHIio;
        if (io->io_Device) CloseDevice(io);
    }
    DeleteIORequest((APTR)project->AHIio);
    DeleteIORequest((APTR)project->AHIio2);
    project->AHIio = project->AHIio2 = NULL;
    DeleteMsgPort(project->AHImp);
    project->AHImp = NULL;
}

void PlaySound (Project *project, BOOL selection) {
    struct Screen *screen = project->Params->Screen;
    Object *sound = project->Sound;
    uint32 offset, length;
    uint32 chanmask, buflen, playlen;
    uint32 sample_size, frame_size;
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

    offset = length = chanmask = 0;
    if (selection) {
        if (!GetSelection(sound, &offset, &length, &chanmask, TRUE)) {
            goto out;
        }
    }

    if (!selection || length <= 1) {
        GetAttr(SOUND_Length, sound, &length);
        if (length == 0 || length <= offset) {
            dbug(("bad length: %ld\n", length));
            goto out;
        }
        length -= offset;
    }

    num_channels = sample_size = frequency = 0;
    GetAttrs(sound,
        SOUND_NumChannels,      &num_channels,
        SOUND_SampleSize,       &sample_size,
        SOUND_SamplesPerSec,    &frequency,
        TAG_END);
    if (chanmask) {
        uint32 tmp;
        num_channels = 0;
        for (tmp = chanmask; tmp; tmp >>= 1) num_channels++;
    } else {
        chanmask = (1 << num_channels) - 1;
    }
    switch (sample_size) {
        case 0:
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
    if (num_channels != 1 && num_channels != 2) goto out;

    buflen = MIN(frequency/10, length);
    frame_size = num_channels * sample_size;
    buffer_size = buflen * frame_size;

    mp = project->AHImp;
    io = project->AHIio;
    io2 = project->AHIio2;
    join = NULL;

    buf = AllocVec(buffer_size, MEMF_SHARED);
    if (buflen < length) {
        buf2 = AllocVec(buffer_size, MEMF_SHARED);
        if (!buf || !buf2) goto out;
    } else {
        buf2 = NULL;
        if (!buf) goto out;
    }

    project->Flags |= PRFLG_PLAYING;
    sig_ahi = 1 << project->AHImp->mp_SigBit;
    project->Signals[SIGNAL_AHI] = sig_ahi;
    Project_UpdateGUI(project);

    task = FindTask(NULL);
    oldpri = SetTaskPri(task, 10);

    saio.MethodID = SNDM_READSAMPLEARRAY;
    saio.GInfo = NULL;
    saio.ChanMask = (1 << num_channels) - 1;
    saio.Mode = SNDRM_DEFAULT;
    saio.BytesPerSample = sample_size;
    saio.BytesPerFrame = frame_size;
    while (length > 0) {
        playlen = MIN(buflen, length);

        saio.ArrayPtr = buf;
        saio.Offset = offset;
        saio.Length = playlen;
        DoMethodA(sound, (Msg)&saio);

        io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
        io->ahir_Std.io_Command = CMD_WRITE;
        io->ahir_Std.io_Data = buf;
        io->ahir_Std.io_Length = playlen * frame_size;
        io->ahir_Std.io_Offset = 0;
        io->ahir_Frequency = frequency;
        io->ahir_Type = ahi_fmt;
        io->ahir_Volume = 0x10000;
        io->ahir_Position = 0x8000;
        io->ahir_Link = join;
        SendIO((APTR)io);

        if (project->IWindow && AttemptLockLayerRom(screen->BarLayer)) {
            UnlockLayerRom(screen->BarLayer);
            SetGadgetAttrs(G(sound), project->IWindow, NULL,
                SOUND_PlayMarker,   offset,
                TAG_END);
        } else if (!project->IWindow) {
            SetAttrs(G(sound),
                SOUND_PlayMarker,   offset,
                TAG_END);
        }

        for (;;) {
            signals = Wait(Project_SigMask(project));

            Project_HandleEvents(project, signals);

            if (!(project->Flags & PRFLG_PLAYING)) {
                AbortIO((APTR)io);
                if (join) AbortIO((APTR)join);
                length = playlen;
                break;
            }

            if (GetMsg(mp) == (APTR)join) break;
        }
        
        if (join) WaitIO((APTR)join);
        join = io;
        tmp = buf; buf = buf2; buf2 = tmp;
        tmp = io; io = io2; io2 = tmp;

        offset += playlen;
        length -= playlen;
    }
    if (join) WaitIO((APTR)join);

    SetTaskPri(task, oldpri);

    SetGadgetAttrs(G(sound), project->IWindow, NULL,
        SOUND_PlayMarker,   ~0,
        TAG_END);

    project->Flags &= ~PRFLG_PLAYING;
    project->Signals[SIGNAL_AHI] = 0;
    Project_UpdateGUI(project);

out:
    FreeVec(buf);
    FreeVec(buf2);
    CloseAHI(project);
}
