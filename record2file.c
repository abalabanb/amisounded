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

#include "record2file.h"
#include "aiff_support.h"
#include <proto/exec.h>
#include <proto/dos.h>

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

struct AIFFHeader {
    struct {
        uint32 ID;
        uint32 Size;
        uint32 Type;
    } FormHeader;
    struct {
        uint32 ID;
        uint32 Size;
    } CommHeader;
    struct Common Comm;
    struct {
        uint32 ID;
        uint32 Size;
    } SSNDHeader;
    struct SampledSoundHeader SSND;
};

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

struct Record2File *StartRecord2File (const char *filename, uint32 frequency,
    uint32 num_channels, uint32 sample_size)
{
    struct Record2File *rec;
    rec = AllocVecTags(sizeof(*rec), AVT_Type, MEMF_PRIVATE, TAG_END);
    if (rec) {
        rec->Filename = filename;
        rec->File = Open(filename, MODE_NEWFILE);
        rec->Frequency = frequency;
        rec->NumChannels = num_channels;
        rec->SampleSize = sample_size;
        rec->FrameSize = sample_size * num_channels;
        rec->FramesRecorded = 0;
        if (rec->File) {
            struct AIFFHeader aiff;
            aiff.FormHeader.ID = ID_FORM;
            aiff.FormHeader.Size = 0;
            aiff.FormHeader.Type = ID_AIFF;
            aiff.CommHeader.ID = ID_COMM;
            aiff.CommHeader.Size = sizeof(struct Common);
            aiff.Comm.NumChannels = num_channels;
            aiff.Comm.NumFrames = 0;
            aiff.Comm.BitsPerSample = sample_size << 3;
            long2extended(frequency, aiff.Comm.FramesPerSec);
            aiff.SSNDHeader.ID = ID_SSND;
            aiff.SSNDHeader.Size = 0;
            aiff.SSND.DataOffset = 0;
            aiff.SSND.BlockSize = 0;
            if (FWrite(rec->File, &aiff, 1, sizeof(aiff)) == sizeof(aiff)) {
                return rec;
            }
        }
        StopRecord2File(rec, TRUE);
    }
    return NULL;
}

void Record2File (struct Record2File *rec, void *buffer, uint32 num_frames) {
    if (rec) {
        int32 frames_written;
        if (rec->FrameSize == 3) {
            int32 samples = num_frames * rec->NumChannels;
            uint8 *src = buffer;
            uint8 *dst = buffer;
            while (samples--) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                src++;
            }
        }
        frames_written = FWrite(rec->File, buffer, rec->FrameSize, num_frames);
        if (frames_written != -1) {
            rec->FramesRecorded += frames_written;
        }
    }
}

void StopRecord2File (struct Record2File *rec, BOOL delete_file) {
    if (rec) {
        if (rec->File) {
            if (rec->FramesRecorded) {
                struct AIFFHeader aiff;
                uint32 ssnd_size = rec->FramesRecorded * rec->FrameSize;
                aiff.FormHeader.ID = ID_FORM;
                aiff.FormHeader.Size = sizeof(aiff) - 8 + ssnd_size;
                aiff.FormHeader.Type = ID_AIFF;
                aiff.CommHeader.ID = ID_COMM;
                aiff.CommHeader.Size = sizeof(struct Common);
                aiff.Comm.NumChannels = rec->NumChannels;
                aiff.Comm.NumFrames = rec->FramesRecorded;
                aiff.Comm.BitsPerSample = rec->SampleSize << 3;
                long2extended(rec->Frequency, aiff.Comm.FramesPerSec);
                aiff.SSNDHeader.ID = ID_SSND;
                aiff.SSNDHeader.Size = sizeof(aiff.SSND) + ssnd_size;
                aiff.SSND.DataOffset = 0;
                aiff.SSND.BlockSize = 0;
                ChangeFilePosition(rec->File, 0, OFFSET_BEGINNING);
                FWrite(rec->File, &aiff, 1, sizeof(aiff));
            }
            Close(rec->File);
            if (delete_file) {
                Delete(rec->Filename);
            }
        }
        FreeVec(rec);
    }
}
