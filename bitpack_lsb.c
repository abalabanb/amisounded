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

/*
 *  wave.datatype
 *  (c) Fredrik Wikstrom
 */

#include <exec/types.h>
#include <dos/dos.h>

#include "endian.h"
#include "bitpack.h"

// LSb style functions:

extern const uint32 bp_mask[33];

uint32 bitpack_read1_lsb (BitPack_buffer *b) {
    uint32 ret=0;
    if (b->ptr[0]&(1 << b->endbit))
        ret=1;
    b->endbit++;
    if (b->endbit==8) {
        b->endbit=0;
        b->ptr++;
        b->endbyte++;
    }
    return (ret);
}

uint32 bitpack_read_lsb (BitPack_buffer *b, int32 bits) {
    uint32 ret;

    bits+=b->endbit;

    ret=b->ptr[0]>>b->endbit;
    if (bits>8) {
        ret|=b->ptr[1]<<(8-b->endbit);
        if (bits>16) {
            ret|=b->ptr[2]<<(16-b->endbit);
            if (bits>24) {
                ret|=b->ptr[3]<<(24-b->endbit);
                if (bits>32) {
                    ret|=b->ptr[4]<<(32-b->endbit);
                }
            }
        }
    }
    ret&=bp_mask[bits];

    b->ptr+=(bits >> 3);
    b->endbyte+=(bits >> 3);
    b->endbit=bits&7;

    return (ret);
}

#if BP_WRITE_CMDS

void bitpack_write1_lsb (BitPack_buffer *b, uint32 val) {
    if (val & 1) {
        b->ptr[0]|=(1 << b->endbit);
    } else {
        b->ptr[0]&=~(1 << b->endbit);
    }
    b->endbit++;
    if (b->endbit==8) {
        b->endbit=0;
        b->ptr++;
        b->endbyte++;
    }
}

void bitpack_write_lsb (BitPack_buffer *b, uint32 val, int32 bits) {
    val&=bp_mask[bits];
    bits+=b->endbit;

    b->ptr[0]|=val<<b->endbit;
    if (bits>=8) {
        b->ptr[1]=val>>(8-b->endbit);
        if (bits>=16) {
            b->ptr[2]=val>>(16-b->endbit);
            if (bits>=32) {
                if (b->endbit)
                    b->ptr[4]=val>>(32-b->endbit);
                else
                    b->ptr[4]=0;
            }
        }
    }

    b->endbyte+=(bits>>3);
    b->ptr+=(bits>>3);
    b->endbit=bits&7;
}

#endif

#if BP_FAST_CMDS

void bitpack_iobits_lsb (ExtBitPack_buffer *b, int32 bits) {
}

uint32 bitpack_fastread_lsb (ExtBitPack_buffer *b) {
}

#if BP_WRITE_CMDS

void bitpack_fastwrite_lsb (ExtBitPack_buffer *b, uint32 val) {
}

#endif

#endif
