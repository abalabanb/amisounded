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
#include "prefs.h"
#include "prefs_gui.h"

int prefs_main () {
    struct Process *proc;
    struct StartupMsg *startmsg;
    struct MsgPort *port;
    PrefsGUI *prefs;
    
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
    
    prefs = PrefsGUI_Init(startmsg);
    if (prefs) {
        PrefsGUI_ShowGUI(prefs);
    
        while (!(prefs->Flags & PRFLG_QUIT))
            PrefsGUI_HandleEvents(prefs, Wait(PrefsGUI_SigMask(prefs)));
            
        PrefsGUI_Free(prefs);
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
