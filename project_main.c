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
#include "requesters.h"
#include <string.h>
#include "prefs.h"
#include "debug.h"
#include "path.h"
#include <classes/requester.h>
#include <libraries/application.h>
#include <proto/requester.h>
#include <proto/application.h>

int project_main () {
    struct Process *proc;
    struct StartupMsg *startmsg;
    struct MsgPort *port;
    Project *project;

    proc = (struct Process *)FindTask(NULL);
    WaitPort(&proc->pr_MsgPort);
    startmsg = (struct StartupMsg *)GetMsg(&proc->pr_MsgPort);

    if (startmsg->Msg.Class != MSG_STARTUP ||
        startmsg->Msg.Node.mn_Length != sizeof(*startmsg))
    {
        DebugPrintF(PROGNAME": Bad startup message!\n");
        return 0;
    }

    port = startmsg->PrPort;
    port->mp_SigBit = AllocSignal(-1);
    port->mp_SigTask = &proc->pr_Task;
    if (port->mp_SigBit == (uint8)-1) goto out;
    port->mp_Flags = PA_SIGNAL;

    project = Project_Init(startmsg);
    if (project) {
        Project_ShowGUI(project);
        if (startmsg->Filename) {
            LoadFromFilename(project, startmsg->Filename);
            startmsg->Filename = NULL;
        }

        while (!(project->Flags & PRFLG_QUIT))
            Project_HandleEvents(project, Wait(Project_SigMask(project)));

        Project_Free(project);
    }

out:
    port->mp_Flags = PA_IGNORE;
    port->mp_SigTask = NULL;
    FreeSignal((int8)port->mp_SigBit);
    port->mp_SigBit = -1;

#ifndef __amigaos4__
    Forbid();
#endif
    ReplyMsg(&startmsg->Msg.Node);
    return 0;
}

void Project_WindowTitle (Project *project) {
    int32 numchan, freq, samplesize, length;
    int32 seconds, minutes, hours;
    char *str;
    GetAttrs(project->Sound,
        SOUND_NumChannels,      &numchan,
        SOUND_SamplesPerSec,    &freq,
        SOUND_SampleSize,       &samplesize,
        SOUND_Length,           &length,
        TAG_END);
    seconds = length / freq;
    minutes = seconds / 60;
    seconds %= 60;
    hours = minutes / 60;
    minutes %= 60;
    str = ASPrintf(PROGNAME" - %s (%ld-bit, %ld Hz, %02ld:%02ld:%02ld)",
        GetProjectName(project), samplesize << 3, freq, hours, minutes, seconds);
    SetAttrs(project->Window, WA_Title, str ? str : GetString(&LocaleInfo, MSG_NOMEM), TAG_END);
    FreeVec(project->WindowTitle);
    project->WindowTitle = str;
}

void Project_IconTitle (Project *project) {
    char *title = Strdup(GetProjectName(project));
    SetAttrs(project->Window,
        WINDOW_IconTitle,   title ? title : GetString(&LocaleInfo, MSG_NOMEM),
        TAG_END);
    FreeVec(project->IconTitle);
    project->IconTitle = title;
}

void Project_UpdateProp (Project *project) {
    uint32 total, top, visible;
    GetAttrs(project->Sound,
        SOUND_HPropTotal,   &total,
        SOUND_HPropTop,     &top,
        SOUND_HPropVisible, &visible,
        TAG_END);
    SetGadgetAttrs((struct Gadget *)project->HProp, project->IWindow, NULL,
        SCROLLER_Total,     total,
        SCROLLER_Visible,   visible,
        SCROLLER_Top,       top,
        TAG_END);
}

void PerformAction (Project *project, uint32 action) {
    struct LoadPlugin *plugin;
    switch (action) {
        case ACTION_DUMMY:
            break;

        case ACTION_NEW:
            {
                struct StartupMsg *msg;
                msg = AllocMsg(sizeof(*msg), NULL);
                if (msg) {
                    msg->Msg.PrID = project->PrID;
                    msg->Msg.Class = MSG_NEWPROJECT;
                    msg->Filename = NULL;
                    PutMsg(project->MainPort, &msg->Msg.Node);
                }
            }
            break;

        case ACTION_OPEN:
            if (UnsavedChanges(project)) {
                break;
            }
            if (AslRequestTags(project->OpenReq,
                ASLFR_Window,   project->IWindow,
                TAG_END))
            {
                LoadFromFilename(project, GetFullPath1(project->OpenReq->fr_Drawer,
                    project->OpenReq->fr_File));
            }
            break;

        case ACTION_CLOSE:
            if (UnsavedChanges(project)) {
                break;
            }
            /* fall through */
        case ACTION_FORCEQUIT:
            project->Flags |= PRFLG_QUIT;
            /* fall through */
        case ACTION_STOP:
            project->Flags &= ~PRFLG_PLAYING;
            break;

        case ACTION_CLOSEALL:
            {
                struct MsgNode *msg;
                msg = AllocMsg(sizeof(*msg), NULL);
                if (msg) {
                    msg->PrID = project->PrID;
                    msg->Class = MSG_QUIT;
                    PutMsg(project->MainPort, &msg->Node);
                }
            }
            break;

        case ACTION_SAVE:
        case ACTION_SAVEAS:
            if (GetSoundLength(project->Sound) == 0) {
                break;
            }
            if (action == ACTION_SAVE && project->Filename && project->SavePlugin) {
                SaveSound(project, project->Filename);
                break;
            }
            plugin = OutputFormatRequester(project);
            if (plugin && AslRequestTags(project->SaveReq,
                ASLFR_Window,   project->IWindow,
                TAG_END))
            {
                char *filename;
                filename = GetFullPath1(project->SaveReq->fr_Drawer, project->SaveReq->fr_File);
                if (filename) {
                    BOOL res;

                    project->SavePlugin = plugin;
                    res = SaveSound(project, filename);
                    if (res) {
                        FreeVec(project->Filename);
                        project->Filename = filename;
                    } else {
                        FreeVec(filename);
                    }

                    Project_WindowTitle(project);
                    Project_IconTitle(project);
                } else {
                    DisplayBeep(project->Screen);
                }
            }
            break;

        case ACTION_ZOOMOUTFULL:
            DoGadgetMethod((struct Gadget *)project->Sound, project->IWindow, NULL,
                SNDM_ZOOM, NULL, SOUND_ZOOMOUTFULL);
            break;

        case ACTION_ZOOMSELECTION:
            DoGadgetMethod((struct Gadget *)project->Sound, project->IWindow, NULL,
                SNDM_ZOOM, NULL, SOUND_ZOOMSELECTION);
            break;

        case ACTION_ZOOMIN:
            DoGadgetMethod((struct Gadget *)project->Sound, project->IWindow, NULL,
                SNDM_ZOOM, NULL, SOUND_ZOOMIN2X);
            break;

        case ACTION_ZOOMOUT:
            DoGadgetMethod((struct Gadget *)project->Sound, project->IWindow, NULL,
                SNDM_ZOOM, NULL, SOUND_ZOOMOUT2X);
            break;

        case ACTION_DOICONIFY:
            RA_Iconify(project->Window);
            project->IWindow = NULL;
            GetAttr(WINDOW_SigMask, project->Window, &project->Signals[SIGNAL_WINDOW]);
            break;

        case ACTION_UNICONIFY:
            Project_ShowGUI(project);
            break;

        case ACTION_PLAYSOUND:
            PlaySound(project, FALSE);
            break;

        case ACTION_PLAYSELECTION:
            PlaySound(project, TRUE);
            break;

        case ACTION_CUT:
            CutSoundData(project);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;

        case ACTION_COPY:
            CopySoundData(project);
            break;

        case ACTION_PASTE:
            PasteSoundData(project);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;

        case ACTION_DELETE:
            DeleteSoundData(project);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;

        case ACTION_MIX_AVG:
            MixSoundData(project, MIX_AVG);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;
            
        case ACTION_MIX_ADD:
            MixSoundData(project, MIX_ADD);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;
            
        case ACTION_MIX_SUB:
            MixSoundData(project, MIX_SUB);
            Project_UpdateGUI(project);
            Project_WindowTitle(project);
            break;

        case ACTION_ABOUT:
            AboutRequester(project->Screen, project->Window);
            break;

        case ACTION_RECORD:
            if (UnsavedChanges(project)) {
                break;
            }
            RecordSound(project);
            Project_WindowTitle(project);
            break;

        case ACTION_VOLUME:
            AdjustVolume(project);
            break;

        case ACTION_RESAMPLE:
            Resample(project);
            break;

        case ACTION_OPENPREFS:
            {
                struct MsgNode *msg;
                msg = AllocMsg(sizeof(*msg), NULL);
                if (msg) {
                    msg->PrID = project->PrID;
                    msg->Class = MSG_OPENPREFS;
                    PutMsg(project->MainPort, &msg->Node);
                }
            }
            break;

        case ACTION_SAVESETTINGS:
            WriteProgramPrefs(TRUE, TRUE);
            break;

        default:
            DebugPrintF("Unimplemented action: %ld\n", action);
            break;
    }
}

BOOL UnsavedChanges (Project *project) {
    Object *sound = project->Sound;
    uint32 length;
    uint32 has_changed;
    BOOL ret = FALSE;
    GetAttrs(sound,
        SOUND_Length,       &length,
        SOUND_HasChanged,   &has_changed,
        TAG_END);
    if (length && has_changed) {
        struct Screen *screen = project->Screen;
        struct Window *window = project->IWindow;
        const char *gadget_str;
        Object *req;
        const char *filename = GetProjectName(project);

        ret = TRUE;
                
        gadget_str = ASPrintf("%s|%s|%s",
            GetString(&LocaleInfo, MSG_SAVE_GAD),
            GetString(&LocaleInfo, MSG_CONTINUE_GAD),
            GetString(&LocaleInfo, MSG_CANCEL_GAD));
            
        if (!gadget_str) {
            return ret;
        }
        
        req = RequesterObject,
            REQ_Image,      REQIMAGE_QUESTION,
            REQ_CharSet,    LocaleInfo.CodeSet,
            REQ_TitleText,  progname,
            REQ_BodyText,   GetString(&LocaleInfo, MSG_UNSAVED_CHANGES_REQ),
            REQ_VarArgs,    &filename,
            REQ_GadgetText, gadget_str,
        End;
        
        switch (DoReq(screen, project->Window, req)) {
            
            case 1:
                PerformAction(project, ACTION_SAVEAS);
                GetAttr(SOUND_HasChanged, sound, &has_changed);
                if (!has_changed) ret = FALSE;
                break;
                
            case 2:
                ret = FALSE;
                break;
            
        }
        
        FreeVec((APTR)gadget_str);
    }
    return ret;
}

void LoadFromFilename (Project *project, char *filename) {
    BOOL res;
    if (!filename) {
        DisplayBeep(project->Screen);
        return;
    }
    res = LoadSound(project, filename);
    FreeVec(project->Filename);
    if (res) {
        project->Filename = filename;
        SetApplicationAttrs(AppID,
            APPATTR_AppOpenedDocument,  filename,
            TAG_END);
    } else {
        project->Filename = NULL;
        FreeVec((APTR)filename);
    }
    Project_WindowTitle(project);
    Project_IconTitle(project);
    Project_UpdateGUI(project);
}

BOOL GetArea (Object *sound, uint32 applyto, int32 *offset, int32 *length, uint32 *chanmask) {
    uint32 range[2], num_channels;
    if (GetSoundLength(sound) == 0) return FALSE;
    switch (applyto) {
        case APPLY_SELECTION:
            GetAttrs(sound, 
                SOUND_Selection,    range,
                SOUND_ChanMask,     chanmask,
                SOUND_NumChannels,  &num_channels,
                TAG_END);
            if (range[1] == ~0) return FALSE;
            *offset = range[0];
            *length = 0;
            if (range[0] == range[1]) return FALSE;
            *length = range[1] - range[0] + 1;
            break;

        case APPLY_VIEW:
            GetAttrs(sound,
                SOUND_View,         range,
                SOUND_NumChannels,  &num_channels,
                TAG_END);
            *offset = range[0];
            *length = range[1] - range[0] + 1;
            break;

        case APPLY_SOUND:
            *offset = 0;
            GetAttrs(sound,
                SOUND_Length,       length,
                SOUND_NumChannels,  &num_channels,
                TAG_END);
            break;
    }
    *chanmask &= (1 << num_channels) - 1;
    return TRUE;
}

const char *GetProjectName (Project *project) {
    return project->Filename ? FilePart(project->Filename) : GetString(&LocaleInfo, MSG_UNTITLED);
}
