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

#ifndef PROJECT_H
#define PROJECT_H

enum {
    GID_DUMMY = 0,
    GID_SPEEDBAR,
    GID_SOUNDGC,
    GID_SBLEFT,
    GID_SBRIGHT
};

enum {
    SIGNAL_MSGPORT = 0,
    SIGNAL_WINDOW,
    SIGNAL_AHI,
    NUM_SIGNALS
};

#define PRFLG_PLAYING   0x00000002
#define PRFLG_QUIT      0x00000001

typedef struct {
    void *PrID;
    struct StartupParams *Params;
    struct MsgPort *MainPort, *SubPort;

    struct Screen *Screen;
    struct List *SpeedButtons;
    struct MsgPort *AppPort;
    struct Window *IWindow;
    Object *Window;
    Object *Layout;
    Object *Sound;
    Object *SpeedBar;
    Object *HProp;
    Object *SBLeft;
    Object *SBRight;

    char *Filename;
    char *WindowTitle;
    char *IconTitle;
    void *SavePlugin;

    struct FileRequester *OpenReq;
    struct FileRequester *SaveReq;

    uint32 Flags;
    uint32 Signals[NUM_SIGNALS];

    struct AHIRequest *AHIio, *AHIio2;
    struct MsgPort *AHImp;
} Project;

enum {
    ACTION_DUMMY = 0,
    ACTION_NEW,
    ACTION_OPEN,
    ACTION_SAVE,
    ACTION_SAVEAS,
    ACTION_SAVESELECTION,
    ACTION_UNDO,
    ACTION_REDO,
    ACTION_CUT,
    ACTION_COPY,
    ACTION_PASTE,
    ACTION_DELETE,
    ACTION_CLOSE,
    ACTION_CLOSEALL,
    ACTION_ZOOMIN,
    ACTION_ZOOMOUT,
    ACTION_ZOOMSELECTION,
    ACTION_ABOUT,
    ACTION_DOICONIFY,
    ACTION_UNICONIFY,
    ACTION_CLEARSELECTION,
    ACTION_SETTINGS,
    ACTION_PLAYSOUND,
    ACTION_PLAYSELECTION,
    ACTION_STOP,
    ACTION_TOGGLELEFTCHANNEL,
    ACTION_TOGGLERIGHTCHANNEL,
    ACTION_RECORD,
    ACTION_VOLUME,
    ACTION_RESAMPLE,
    ACTION_ECHO,
    ACTION_ZOOMOUTFULL,
    ACTION_MIX_AVG,
    ACTION_MIX_ADD,
    ACTION_MIX_SUB,
    ACTION_FORCEQUIT,
    ACTION_OPENPREFS,
    ACTION_CREATEICONS,
    ACTION_SAVESETTINGS
};

enum {
    APPLY_SELECTION = 0,
    APPLY_VIEW,
    APPLY_SOUND
};

enum {
    MIX_AVG = 0,
    MIX_ADD,
    MIX_SUB
};

/* project_main.c */
int project_main ();
void Project_WindowTitle (Project *project);
void Project_IconTitle (Project *project);
void Project_UpdateProp (Project *project);
void PerformAction (Project *project, uint32 action);
BOOL UnsavedChanges (Project *project);
void LoadFromFilename (Project *project, char *filename);
void LoadFromWBArg (Project *project, struct WBArg *wbarg);
BOOL GetArea (Object *sound, uint32 applyto, int32 *offset, int32 *length, uint32 *chanmask);
const char *GetProjectName (Project *project);

/* project_gui.c */
Project *Project_Init (struct StartupMsg *startmsg);
void Project_Free (Project *project);
void Project_ShowGUI (Project *project);
uint32 Project_SigMask (Project *project);
void Project_HandleEvents (Project *project, uint32 sigmask);
void Project_UpdateGUI (Project *project);

/* clipboard.c */
BOOL CutSoundData (Project *project);
BOOL CopySoundData (Project *project);
BOOL PasteSoundData (Project *project);
BOOL DeleteSoundData (Project *project);
BOOL MixSoundData (Project *project, int32 mix_type);
BOOL GetSelection (Object *sound, uint32 *offset, uint32 *length, uint32 *chanmask, BOOL allow_empty);

/* playsnd.c */
BOOL OpenAHI (Project *project);
void CloseAHI (Project *project);
void PlaySound (Project *project, BOOL selection);

/* loadsnd.c */
BOOL LoadSound (Project *project, const char *filename);
uint32 GetSoundLength (Object *sound);

/* loadsnd_dt.c */
int32 LoadSoundDT (struct Window *window, Object *sound, const char *filename);

/* savesnd.c */
BOOL SaveSound (Project *project, const char *filename);
struct LoadPlugin *OutputFormatRequester (Project *project);

/* recordsnd.c */
void RecordSound (Project *project);

/* volumeadjust.c */
void AdjustVolume (Project *project);

/* resample.c */
void Resample (Project *project);

#ifndef OK
#define OK 0
#endif

#endif
