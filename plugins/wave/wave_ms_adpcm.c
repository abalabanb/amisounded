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

#include "wave_ms_adpcm.h"
#include "bitpack.h"
#include "endian.h"

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

struct coefset {
    int16 coef1;
    int16 coef2;
};

struct MS_ADPCM_Format {
    uint16 FormatTag;
    int16 NumChannels;
    int32 SamplesPerSec;
    int32 AvgBytesPerSec;
    int16 BlockAlign; /* amount to read for each block */
    int16 BitsPerSample; /* 4 */

    int16 ExtraSize; /* 4+4*numCoefs */
    int16 SamplesPerBlock;
    int16 NumCoefs; // number of coef sets in file
    struct coefset aCoef[]; // numCoef coef sets
};

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

// Note: Coefs are fixed point 8.8 signed numbers.

/* block header:
 *
 * struct MS_ADPCM_Block {
 *      int8 bpredictor[nchannels];
 *      int16 newdelta[nchannels];
 *      int16 isamp1[nchannels];
 *      int16 isamp2[nchannels];
 * };
 */

int32 InitMS_ADPCM (struct WaveCodecData *data) {
    struct MS_ADPCM_Format * fmt = (APTR)data->Format;
    int32 i;

    if (fmt->BitsPerSample != 4) {
        return DTERROR_UNKNOWN_COMPRESSION;
    }
    write_le16(&fmt->NumCoefs, fmt->NumCoefs);
    if (4 + (fmt->NumCoefs << 2) != fmt->ExtraSize) {
        return ERROR_BAD_NUMBER;
    }
    /* change endianness of coefsets */
    for (i = 0; i < fmt->NumCoefs; i++) {
        write_le16(&fmt->aCoef[i].coef1, fmt->aCoef[i].coef1);
        write_le16(&fmt->aCoef[i].coef2, fmt->aCoef[i].coef2);
    }
    write_le16(&fmt->SamplesPerBlock, fmt->SamplesPerBlock);
    data->BlockFrames = fmt->SamplesPerBlock;
    data->SampleSize = 2;
    return OK;
}

static const int16 adaptiontable[16] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230
};

struct ms_adpcm_stat {
    int32 nstep;
    int32 samp1, samp2;
    int32 coef1, coef2;
};

static int32 DecodeAdpcmSample (struct ms_adpcm_stat * stat, int32 err) {
    int32 step, nsamp;
    step = stat->nstep;
    stat->nstep = (adaptiontable[err] * step) >> 8;
    if (stat->nstep < 16) stat->nstep = 16;
    if (err & 8) err -= 16;
    nsamp = ((stat->samp1 * stat->coef1) + (stat->samp2 * stat->coef2)) >> 8;
    nsamp += (err * step);
    if (nsamp > 0x7FFF) nsamp = 0x7FFF;
    if (nsamp < -0x8000) nsamp = -0x8000;
    stat->samp2 = stat->samp1;
    stat->samp1 = nsamp;
    return nsamp;
}

int32 DecodeMS_ADPCM (struct WaveCodecData *data, uint8 *src, uint8 *dst_ptr,
    int32 num_blocks, int32 num_frames)
{
    struct ms_adpcm_stat stat[MAX_CHANNELS];
    struct MS_ADPCM_Format *fmt = (APTR)data->Format;
    int32 num_channels = fmt->NumChannels;
    int32 num_samples, sample, chan;
    int16 *dst = (int16 *)dst_ptr;

    for (chan = 0; chan < num_channels; chan++) {
        int32 bpred;
        bpred = *src++;
        if (bpred >= fmt->NumCoefs) bpred = 0;
        stat[chan].coef1 = fmt->aCoef[bpred].coef1;
        stat[chan].coef2 = fmt->aCoef[bpred].coef2;
    }
    for (chan = 0; chan < num_channels; chan++) {
        stat[chan].nstep = (int16)read_le16(src);
        src += 2;
    }
    for (chan = 0; chan < num_channels; chan++) {
        stat[chan].samp1 = (int16)read_le16(src);
        src += 2;
    }
    for (chan = 0; chan < num_channels; chan++) {
        stat[chan].samp2 = (int16)read_le16(src);
        src += 2;
        switch (num_frames) {
            default:
                dst[num_channels] = stat[chan].samp1;
            case 1:
                dst[0] = stat[chan].samp2;
            case 0:
                break;
        }
        dst++;
    }
    dst += num_channels;

    if (num_frames <= 2) return num_frames;
    num_samples = (num_frames - 2) * num_channels;
    chan = 0;
    {
        int32 tmp = num_samples >> 1;
        for (sample = 0; sample < tmp; sample++) {
            *dst++ = DecodeAdpcmSample(&stat[chan], *src >> 4);
            if (++chan == num_channels) chan = 0;
            *dst++ = DecodeAdpcmSample(&stat[chan], *src & 0xF);
            if (++chan == num_channels) chan = 0;
            src++;
        }
    }
    if (num_samples & 1) {
        *dst++ = DecodeAdpcmSample(&stat[chan], *src >> 4);
    }

    return num_frames;
}
