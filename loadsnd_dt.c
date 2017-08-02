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
#include <datatypes/soundclass.h>
#include <proto/datatypes.h>

#ifndef OK
#define OK 0
#endif

int32 LoadSoundDT (struct Window *window, Object *sound, const char *filename) {
    Object *dt;
    int32 error = OK;
    dt = NewDTObject((char*)filename,
        DTA_GroupID,    GID_SOUND,
        TAG_END);
    if (dt) {
        uint32 length = 0, freq = 0;
        void *sample = NULL, *chan[2] = { NULL, NULL };

        GetDTAttrs(dt,
            SDTA_SampleLength,  &length,
            SDTA_SamplesPerSec, &freq,
            SDTA_Sample,        &sample,
            SDTA_LeftSample,    &chan[0],
            SDTA_RightSample,   &chan[1],
            TAG_END);

        if (length && freq && (sample || chan[0] || chan[1])) {
            unsigned int numchan, i;
            struct sndArrayIO sndio;

            numchan = (chan[0] && chan[1]) ? 2 : 1;
            if (numchan == 1) {
                if (chan[0] == NULL)
                    chan[0] = chan[1] ? chan[1] : sample;
            }

            SetAttrs(sound,
                SOUND_Length,           length,
                SOUND_NumChannels,      numchan,
                SOUND_SampleSize,       1,
                SOUND_SamplesPerSec,    freq,
                TAG_END);

            for (i = 0; i < numchan; i++) {
                sndio.MethodID = SNDM_WRITESAMPLEARRAY;
                sndio.GInfo = NULL;
                sndio.ArrayPtr = chan[i];
                sndio.Offset = 0;
                sndio.Length = length;
                sndio.ChanMask = 1 << i;
                sndio.Mode = SNDWM_REPLACE;
                sndio.BytesPerSample = 1;
                sndio.BytesPerFrame = 1;
                DoGadgetMethodA((struct Gadget *)sound, window, NULL, (Msg)&sndio);
            }
        } else {
            error = ERROR_REQUIRED_ARG_MISSING;
        }

        DisposeDTObject(dt);
    } else {
        error = IoErr();
    }
    return error;
}
