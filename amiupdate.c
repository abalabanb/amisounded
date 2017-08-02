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

#include <dos/dos.h>
#include <proto/dos.h>
#include "amiupdate.h"

/**********************************************************
**
** The following function saves the variable name passed in 
** 'varname' to the ENV(ARC) system so that the application 
** can become AmiUpdate aware.
**
**********************************************************/

void SetAmiUpdateENVVariable (const char *varname) {
    /* AmiUpdate support code */
    BPTR lock;
  
    /* obtain the lock to the home directory */
    if (lock = GetProgramDir()) {
        TEXT progpath[2048];
        TEXT varpath[1024] = "AppPaths";

        /*
        get a unique name for the lock, 
        this call uses device names,
        as there can be multiple volumes 
        with the same name on the system
        */

        if (DevNameFromLock(lock, progpath, sizeof(progpath), DN_FULLPATH)) {
            APTR oldwin;

            /* stop any "Insert volume..." type requesters */
            oldwin = SetProcWindow((APTR)-1);
        
            /* 
            finally set the variable to the 
            path the executable was run from 
            don't forget to supply the variable 
            name to suit your application 
            */

            AddPart(varpath, varname, 1024);
            SetVar(varpath, progpath, -1, GVF_GLOBAL_ONLY|GVF_SAVE_VAR);

            /* turn requesters back on */
            SetProcWindow(oldwin);
        }
    }
}
