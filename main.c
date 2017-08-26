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
#include "plugins_private.h"
#include "prefs.h"
#include <classes/requester.h>
#include <proto/layers.h>
#include <proto/requester.h>
#include "ext_window_class.h"
#include "amiupdate.h"
#include "checkaiss.h"
#include "requesters.h"
#include "path.h"
#ifdef __amigaos4__
#include <libraries/application.h>
#include <proto/application.h>
#endif

#define AISS_VERSION 4
#define AISS_REVISION 4

const char progname[] = PROGNAME;
const char verstag[] = VERSTAG;
const char stack[] = "$STACK: 500000";
extern uint32 AppID;

struct ProjectNode {
    struct MinNode Node;
    struct MsgPort *Port;
};

typedef struct {
    struct MsgPort *Port, *AppPort;
    struct List *Projects;
    struct StartupParams Params;
    struct ProjectNode *Prefs;
} GlobalData;

struct LocaleInfo LocaleInfo;
Class *ExtWindowClass;

static BOOL OpenLibs ();
static void CloseLibs ();

static GlobalData *InitGlobals ();
static void FreeGlobals (GlobalData *gd);

static BOOL NewProject (GlobalData *gd, char *filename);
static BOOL OpenPrefs (GlobalData *gd);
static void FreeProject (struct ProjectNode *pr);
static void SendQuitMsg (GlobalData *gd, BOOL force);
static void SendMsg (GlobalData *gd, uint32 class);

int main (int argc, char **argv) {
    GlobalData *gd = NULL;
    BOOL done = FALSE;
    struct MsgNode *msg;
    struct ApplicationMsg *amsg;
    int32 num_projects = 0;
    uint32 sigmask;

    if (!OpenLibs()) goto out;
    if (!LoadProgramPrefs(argc, argv)) goto out;

    gd = InitGlobals();
    if (!gd) goto out;

    SetAmiUpdateENVVariable(progname);

    OpenLocaleCatalog(&LocaleInfo, PROGNAME".catalog");

    switch (CheckAISS(AISS_VERSION, AISS_REVISION)) {
    
        case NOAISS:
            {
                Object *req;
                req = RequesterObject,
                    REQ_CharSet,    LocaleInfo.CodeSet,
                    REQ_Image,      REQIMAGE_ERROR,
                    REQ_TitleText,  progname,
                    REQ_BodyText,   GetString(&LocaleInfo, MSG_NOAISS_REQ),
                    REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
                    TAG_END);
                DoReq(gd->Params.Screen, NULL, req);
            }
            goto out;

        case OLDAISS:
            {
                Object *req;
                int32 args[2] = { AISS_VERSION, AISS_REVISION };
                req = RequesterObject,
                    REQ_CharSet,    LocaleInfo.CodeSet,
                    REQ_Image,      REQIMAGE_ERROR,
                    REQ_TitleText,  progname,
                    REQ_BodyText,   GetString(&LocaleInfo, MSG_OLDAISS_REQ),
                    REQ_VarArgs,    args,
                    REQ_GadgetText, GetString(&LocaleInfo, MSG_OK_GAD),
                    TAG_END);
                DoReq(gd->Params.Screen, NULL, req);
            }
            goto out;

    }

    if (argc == 0) {
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        char *filename;
        int32 i;
        for (i = 1; i < wbmsg->sm_NumArgs; i++) {
            filename = GetFullPath2(wbmsg->sm_ArgList[i].wa_Lock, wbmsg->sm_ArgList[i].wa_Name);
            if (filename) {
                if (NewProject(gd, filename)) {
                    num_projects++;
                } else {
                    FreeVec((APTR)filename);
                }
            }
        }
    } else {
        char *filename;
        int32 i;
        for (i = 1; i < argc; i++) {
            filename = GetFullPath1("", argv[i]);
            if (filename) {
                if (NewProject(gd, filename)) {
                    num_projects++;
                } else {
                    FreeVec((APTR)filename);
                }
            }
        }
    }
    if (!num_projects && !NewProject(gd, NULL)) {
        goto out;
    }
    sigmask = (1 << gd->Port->mp_SigBit)|(1 << gd->AppPort->mp_SigBit);
    while (!done) {
        Wait(sigmask);
        while (msg = (struct MsgNode *)GetMsg(gd->Port)) {
            switch (msg->Node.mn_Node.ln_Type) {
                case NT_MESSAGE:
                    switch (msg->Class) {
                        case MSG_NEWPROJECT:
                            NewProject(gd, ((struct StartupMsg *)msg)->Filename);
                            break;

                        case MSG_OPENPREFS:
                            OpenPrefs(gd);
                            break;
                            
                        case MSG_QUIT:
                            SendQuitMsg(gd, FALSE);
                            break;
                            
                        case MSG_NEWPREFS:
                            SendMsg(gd, MSG_NEWPREFS);
                            break;
                    }

                    if (msg->Node.mn_ReplyPort) {
                        ReplyMsg(&msg->Node);
                    } else {
                        FreeMsg(&msg->Node);
                    }
                    break;

                case NT_REPLYMSG:
                    switch (msg->Class) {
                        case MSG_STARTUP:
                            if (msg->PrID == gd->Prefs)
                                gd->Prefs = NULL;
                            FreeProject(msg->PrID);
                            if (!GetHead(gd->Projects))
                                done = TRUE;
                            break;
                    }

                    FreeMsg(&msg->Node);
                    break;

            }
        }
        while (amsg = (struct ApplicationMsg *)GetMsg(gd->AppPort)) {
            switch (amsg->type) {

                case APPLIBMT_Quit:
                case APPLIBMT_ForceQuit:
                    SendQuitMsg(gd, amsg->type == APPLIBMT_ForceQuit);
                    break;

                case APPLIBMT_Hide:
                    SendMsg(gd, MSG_HIDE);
                    break;
                    
                case APPLIBMT_Unhide:
                    SendMsg(gd, MSG_UNHIDE);
                    break;
                    
                case APPLIBMT_ToFront:
                    SendMsg(gd, MSG_TOFRONT);
                    break;

                case APPLIBMT_NewBlankDoc:
                    if (NewProject(gd, NULL)) {
                        done = FALSE;
                    }
                    break;

                case APPLIBMT_OpenDoc:
                    {
                        char *filename;
                        filename = GetFullPath1("", ((struct ApplicationOpenPrintDocMsg *)amsg)->fileName);
                        if (filename) {
                            if (NewProject(gd, filename)) {
                                done = FALSE;
                            } else {
                                FreeVec((APTR)filename);
                            }
                        }
                    }
                    break;

                case APPLIBMT_OpenPrefs:
                    OpenPrefs(gd);
                    break;

            }
            ReplyMsg(&amsg->msg);
        }
    }

out:
    CloseLocaleCatalog(&LocaleInfo);

    FreeGlobals(gd);
    
    FreeProgramPrefs();
    CloseLibs();

    return 0;
}

struct Library *LayersBase;
struct LayersIFace *ILayers;
struct Library *ApplicationBase;
struct ApplicationIFace *IApplication;
struct PrefsObjectsIFace *IPrefsObjects;

static BOOL OpenLibs () {
    LayersBase = OpenLibrary("layers.library", 52);
    ILayers = (APTR)GetInterface(LayersBase, "main", 1, NULL);
    if (!ILayers) return FALSE;
    ApplicationBase = OpenLibrary("application.library", 52);
    IApplication = (APTR)GetInterface(ApplicationBase, "application", 2, NULL);
    if (!IApplication) return FALSE;
    IPrefsObjects = (APTR)GetInterface(ApplicationBase, "prefsobjects", 2, NULL);
    if (!IPrefsObjects) return FALSE;
    return TRUE;
}

static void CloseLibs () {
    DropInterface((APTR)IPrefsObjects);
    DropInterface((APTR)IApplication);
    CloseLibrary(ApplicationBase);
    DropInterface((APTR)ILayers);
    CloseLibrary(LayersBase);
}

static GlobalData *InitGlobals () {
    GlobalData *gd = NULL;

    gd = AllocVecTags(sizeof(*gd),  AVT_Type, MEMF_PRIVATE,
                                    AVT_ClearWithValue, 0,
                                    TAG_END);
    if (!gd) goto out;

    gd->Port = AllocPort();
    if (!gd->Port) goto out;
    GetApplicationAttrs(AppID, APPATTR_Port, &gd->AppPort, TAG_END);

    gd->Projects = AllocList();
    if (!gd->Projects) goto out;
    gd->Params.Lock = AllocSemaphore();
    if (!gd->Params.Lock) goto out;
    gd->Params.Screen = LockPubScreen(NULL);
    if (!gd->Params.Screen) goto out;
    gd->Params.WaveClass = SOUNDEDITOR_MakeClass();
    if (!gd->Params.WaveClass) goto out;
    gd->Params.Plugins = AllocList();
    if (!gd->Params.Plugins) goto out;
    ExtWindowClass = InitExtWindowClass();
    if (!ExtWindowClass) goto out;
    
    RestoreSoundEditorColors(gd->Params.WaveClass);
    LoadPlugins(gd->Params.Plugins);
    return gd;

out:
    FreeGlobals(gd);
    return NULL;
}

static void FreeGlobals (GlobalData *gd) {
    if (gd) {
        FreePlugins(gd->Params.Plugins);
        FreeClass(ExtWindowClass);
        FreeList(gd->Params.Plugins);
        SOUNDEDITOR_FreeClass(gd->Params.WaveClass);
        UnlockPubScreen(NULL, gd->Params.Screen);
        FreeSemaphore(gd->Params.Lock);
        FreeList(gd->Projects);
        FreePort(gd->Port);
        FreeVec(gd);
    }
}

static BOOL NewProject (GlobalData *gd, char *filename) {
    extern int project_main ();
    struct ProjectNode *pr;
    pr = AllocVecTags(sizeof(*pr),  AVT_Type, MEMF_PRIVATE,
                                    AVT_ClearWithValue, 0,
                                    TAG_END);
    if (pr) {
        struct StartupMsg *msg;
        msg = AllocMsg(sizeof(*msg), gd->Port);
        if (msg) {
            msg->Msg.PrID = pr;
            msg->Msg.Class = MSG_STARTUP;
            pr->Port = AllocPortNoSignal();
            msg->PrParams = &gd->Params;
            msg->Filename = filename;
            if (msg->PrPort = pr->Port) {
                struct Process *proc;
                proc = CreateNewProcTags(
                    NP_Name,    PROGNAME": New Project",
                    NP_Entry,   project_main,
                    NP_Child,   TRUE,
                    TAG_END);
                if (proc) {
                    PutMsg(&proc->pr_MsgPort, &msg->Msg.Node);
                    AddTail(gd->Projects, (struct Node *)pr);
                    return TRUE;
                }
            }
            FreeMsg(msg);
        }
        FreeProject(pr);
    }
    return FALSE;
}

static BOOL OpenPrefs (GlobalData *gd) {
    extern int prefs_main ();
    struct ProjectNode *pr;
    if (!(pr = gd->Prefs)) {
        pr = AllocVecTags(sizeof(*pr),  AVT_Type, MEMF_PRIVATE,
                                        AVT_ClearWithValue, 0,
                                        TAG_END);
        if (pr) {
            struct StartupMsg *msg;
            msg = AllocMsg(sizeof(*msg), gd->Port);
            if (msg) {
                msg->Msg.PrID = pr;
                msg->Msg.Class = MSG_STARTUP;
                pr->Port = AllocPortNoSignal();
                msg->PrParams = &gd->Params;
                msg->Filename = NULL;
                if (msg->PrPort = pr->Port) {
                    struct Process *proc;
                    proc = CreateNewProcTags(
                        NP_Name,    PROGNAME": Prefs",
                        NP_Entry,   prefs_main,
                        NP_Child,   TRUE,
                        TAG_END);
                    if (proc) {
                        PutMsg(&proc->pr_MsgPort, &msg->Msg.Node);
                        AddTail(gd->Projects, (struct Node *)pr);
                        gd->Prefs = pr;
                        return TRUE;
                    }
                }
                FreeMsg(msg);
            }
            FreeProject(pr);
        }
        return FALSE;
    } else {
        struct MsgNode *msg;
        msg = AllocMsg(sizeof(*msg), gd->Port);
        if (msg) {
            msg->PrID = pr;
            msg->Class = MSG_TOFRONT;
            PutMsg(pr->Port, &msg->Node);
        }
        return TRUE;
    }
}

static void FreeProject (struct ProjectNode *pr) {
    if (pr) {
        if (pr->Port) {
            struct Message *msg;
            while (msg = GetMsg(pr->Port)) FreeMsg(msg);
            FreePort(pr->Port);
        }
        if (pr->Node.mln_Pred && pr->Node.mln_Succ)
            Remove((struct Node *)pr);
        FreeVec(pr);
    }
}

static void SendQuitMsg (GlobalData *gd, BOOL force) {
    struct ProjectNode *pr;
    struct QuitMsg *msg;
    pr = (struct ProjectNode *)GetHead(gd->Projects);
    while (pr) {
        msg = AllocMsg(sizeof(*msg), gd->Port);
        if (msg) {
            msg->Msg.PrID = pr;
            msg->Msg.Class = MSG_QUIT;
            msg->Force = force;
            PutMsg(pr->Port, &msg->Msg.Node);
        }
        pr = (struct ProjectNode *)GetSucc((struct Node *)pr);
    }
}

static void SendMsg (GlobalData *gd, uint32 class) {
    struct ProjectNode *pr;
    struct MsgNode *msg;
    pr = (struct ProjectNode *)GetHead(gd->Projects);
    while (pr) {
        msg = AllocMsg(sizeof(*msg), gd->Port);
        if (msg) {
            msg->PrID = pr;
            msg->Class = class;
            PutMsg(pr->Port, &msg->Node);
        }
        pr = (struct ProjectNode *)GetSucc((struct Node *)pr);
    }
}
