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

#include "wave_gsm610.h"
#include "gsm/gsm.h"
#include "gsm/private.h"
#include <proto/exec.h>

struct GSM610_Format {
    uint16 FormatTag;
    int16 NumChannels;
    int32 SamplesPerSec;
    int32 AvgBytesPerSec;
    int16 BlockAlign; /* amount to read for each block */
    int16 BitsPerSample;

    int16 ExtraSize; /* 2 */
    int16 SamplesPerBlock;
};

struct gsmstate {
    gsm         handle;
    gsm_signal  *samples;
};

int32 InitGSM610 (struct WaveCodecData *data) {
    //int valP = 1;
    struct GSM610_Format *fmt = (APTR)data->Format;
    struct gsmstate *state;

    if (fmt->NumChannels != 1) {
        return ERROR_NOT_IMPLEMENTED;
    }

    if (fmt->ExtraSize != 0 && fmt->ExtraSize != 2) {
        return ERROR_BAD_NUMBER;
    }

    state = AllocVec(sizeof(*state), MEMF_PRIVATE|MEMF_CLEAR);
    data->InitData = state;
    if (!state) {
        return ERROR_NO_FREE_STORE;
    }

    // state->handle = gsm_create();
    state->handle = AllocVec(sizeof(struct gsm_state), MEMF_PRIVATE|MEMF_CLEAR);
    if (!state->handle) {
        return ERROR_NO_FREE_STORE;
    }
    state->handle->nrp=40;

    // gsm_option(state->handle, GSM_OPT_WAV49, &valP);
    state->handle->wav_fmt = TRUE;

    fmt->BlockAlign = 65;
    if (fmt->ExtraSize == 0 || fmt->SamplesPerBlock == 0) {
        data->BlockFrames = 320;
    } else
    if (fmt->ExtraSize == 2) {
        write_le16(&fmt->SamplesPerBlock, fmt->SamplesPerBlock);
        data->BlockFrames = fmt->SamplesPerBlock;
    }
    data->SampleSize = 2;

    state->samples = AllocVec(data->BlockFrames << 1, MEMF_PRIVATE);
    if (!state->samples) {
        return ERROR_NO_FREE_STORE;
    }

    return OK;
}

void ExitGSM610 (struct WaveCodecData *data) {
    struct gsmstate *state = data->InitData;
    if (state) {
        data->InitData = NULL;

        FreeVec(state->samples);

        // gsm_destroy(state->handle);
        FreeVec(state->handle);

        FreeVec(state);
    }
}

int32 DecodeGSM610 (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    struct gsmstate *state = data->InitData;
    gsm_signal *buff;
    /* int32 num_channels = data->Format->NumChannels; */
    /* int32 chan; */
    int32 frame;
    int16 *dst = (int16 *)dst_ptr;

    /* for (chan = 0; chan < num_channels; chan++) { */
        /* decode the long 33 byte half */
        if (gsm_decode(state->handle, src, state->samples) < 0) {
            return 0;
        }
        /* decode the short 32 byte half */
        if (gsm_decode(state->handle, src+33, state->samples+160) < 0) {
            return 0;
        }
        CopyMem(state->samples, dst, num_frames << 1);
        /*buff = state->samples;
        for (frame = 0; frame < num_frames; frame++) {
            *dst++ = *buff++;
        }*/
    /* } */

    return num_frames;
}
