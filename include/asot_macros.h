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

#ifndef ASOT_MACROS_H
#define ASOT_MACROS_H

#ifdef __amigaos4__

#define AllocMsg(size,port) AllocSysObjectTags(ASOT_MESSAGE, \
    ASOMSG_Size,        size, \
    ASOMSG_ReplyPort,   port, \
    TAG_END)
#define FreeMsg(msg) FreeSysObject(ASOT_MESSAGE, msg)

#define AllocPort() AllocSysObject(ASOT_PORT, NULL)
#define AllocPortNoSignal() AllocSysObjectTags(ASOT_PORT, \
    ASOPORT_AllocSig,   FALSE, \
    ASOPORT_Action,     PA_IGNORE, \
    ASOPORT_Target,     NULL, \
    TAG_END)
#define FreePort(port) FreeSysObject(ASOT_PORT, port)

#define AllocList() AllocSysObject(ASOT_LIST, NULL)
#define AllocMinList(size) AllocSysObjectTags(ASOT_LIST, \
    ASOLIST_Min,    TRUE, \
    TAG_END)
#define FreeList(list) FreeSysObject(ASOT_LIST, list)

#define AllocSemaphore() AllocSysObject(ASOT_SEMAPHORE, NULL)
#define FreeSemaphore(sem) FreeSysObject(ASOT_SEMAPHORE, sem)

#define AllocNode(size) AllocSysObjectTags(ASOT_NODE, \
    ASONODE_Size,   size, \
    TAG_END)
#define AllocMinNode(size) AllocSysObjectTags(ASOT_NODE, \
    ASONODE_Min,    TRUE, \
    ASONODE_Size,   size, \
    TAG_END)
#define FreeNode(node) FreeSysObject(ASOT_NODE, node)

#else

#ifndef CLIB_ALIB_PROTOS_H
#include <clib/alib_protos.h>
#endif

#define GetHead(list) ((list)->lh_Head->ln_Succ ? (list)->lh_Head : NULL)

static inline void *AllocMsg (int32 size, struct MsgPort *port) {
    struct Message *msg;
    msg = AllocMem(size, MEMF_CLEAR);
    if (msg) {
        msg->mn_Node.ln_Type = NT_MESSAGE;
        msg->mn_Length = size;
        msg->mn_ReplyPort = port;
    }
    return msg;
}

static inline void FreeMsg (void *msg) {
    FreeMem(msg, ((struct Message *)msg)->mn_Length);
}

#define AllocPort() CreateMsgPort()

static inline struct MsgPort *AllocPortNoSignal () {
    struct MsgPort *port;
    port = CreateMsgPort();
    if (port) {
        port->mp_Flags = PA_IGNORE;
        FreeSignal(port->mp_SigBit);
        port->mp_SigTask = NULL;
        port->mp_SigBit = -1;
    }
    return port;
}

#define FreePort(port) DeleteMsgPort(port)

static inline struct List *AllocList () {
    struct List *list;
    list = AllocVec(sizeof(*list), MEMF_CLEAR);
    if (list) NewList(list);
    return list;
}

static inline struct MinList *AllocMinList () {
    struct MinList *list;
    list = AllocVec(sizeof(*list), MEMF_CLEAR);
    if (list) NewList((struct List *)list);
    return list;
}

#define FreeList(list) FreeVec(list)

static inline struct SignalSemaphore *AllocSemaphore () {
    struct SignalSemaphore *sem;
    sem = AllocVec(sizeof(*sem), MEMF_CLEAR);
    if (sem) InitSemaphore(sem);
    return sem;
}

#define FreeSemaphore(sem) FreeVec(sem)

#define AllocNode(size) AllocVec(size, MEMF_CLEAR)
#define AllocMinNode(size) AllocVec(size, MEMF_CLEAR)
#define FreeNode(node) FreeVec(node)

#endif

#endif /* MESSAGE_SUPPORT_H */
