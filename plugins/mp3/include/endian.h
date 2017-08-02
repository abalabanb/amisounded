/*
�*�AmiSoundED�-�Sound�Editor
�*�Copyright�(C)�2008-2009�Fredrik�Wikstrom�<fredrik@a500.org>
�*
�*�This�program�is�free�software;�you�can�redistribute�it�and/or�modify
�*�it�under�the�terms�of�the�GNU�General�Public�License�as�published�by
�*�the�Free�Software�Foundation,�either�version�2�of�the�License,�or
�*�(at�your�option)�any�later�version.
�*
�*�This�program�is�distributed�in�the�hope�that�it�will�be�useful,
�*�but�WITHOUT�ANY�WARRANTY;�without�even�the�implied�warranty�of
�*�MERCHANTABILITY�or�FITNESS�FOR�A�PARTICULAR�PURPOSE.��See�the
�*�GNU�General�Public�License�for�more�details.
�*
�*�You�should�have�received�a�copy�of�the�GNU�General�Public�License�along
�*�with�this�program;�if�not,�write�to�the�Free�Software�Foundation,�Inc.,
�*�51�Franklin�Street,�Fifth�Floor,�Boston,�MA�02110-1301�USA.
�*/

#ifndef ENDIAN_H
#define ENDIAN_H

#if defined(__X86__)
#define CPU_IS_LITTLE_ENDIAN 1
#else
#undef CPU_IS_LITTLE_ENDIAN
#endif

// generic C routines

#ifndef IFF_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

// override with endian aware version
#undef MAKE_ID
#ifndef CPU_IS_LITTLE_ENDIAN
#define MAKE_ID(a,b,c,d)    \
    ((uint32) (a)<<24 | (uint32) (b)<<16 | (uint32) (c)<<8 | (uint32) (d))
#else
#define MAKE_ID(a,b,c,d)    \
    ((uint32) (d)<<24 | (uint32) (c)<<16 | (uint32) (b)<<8 | (uint32) (a))
#endif

#define read_le16(P) le2nat16(*(uint16 *)(P))
#define read_le32(P) le2nat32(*(uint32 *)(P))
#define read_le64(P) le2nat64(*(uint64 *)(P))

#define write_le16(P,V) *(uint16 *)(P)=nat2le16(V)
#define write_le32(P,V) *(uint32 *)(P)=nat2le32(V)
#define write_le64(P,V) *(uint64 *)(P)=nat2le64(V)

#define read_be16(P) be2nat16(*(uint16 *)(P))
#define read_be32(P) be2nat32(*(uint32 *)(P))
#define read_be64(P) be2nat64(*(uint64 *)(P))

#define write_be16(P,V) *(uint16 *)(P)=nat2be16(V)
#define write_be32(P,V) *(uint32 *)(P)=nat2be32(V)
#define write_be64(P,V) *(uint64 *)(P)=nat2be64(V)

#define endian64(A) \
endian32((uint64)(A) >> 32)+((uint64)endian32(A) << 32)

#define endian32(A) \
( ( ((uint32)(A)&0xFF000000)>>24 )+( ((uint32)(A)&0xFF0000)>>8 )+( ((uint32)(A)&0xFF00)<<8 ) \
+( ((uint32)(A)&0xFF)<<24 ) )

#define endian16(A) \
( ( ((uint16)(A)&0xFF00)>>8 )+( ((uint16)(A)&0xFF)<<8 ) )

#if defined(__M68K__) && defined(__VBCC__) // use VBCC/m68k inline asm

#undef endian32
#undef endian16

int32 endian32(__reg("d0") int32) =
    "\trol.w\t#8,d0\n"
    "\tswap\td0\n"
    "\trol.w\t#8,d0";

int16 endian16(__reg("d0") int16) =
    "\trol.w\t#8,d0";

#elif defined(__PPC__) && defined(__VBCC__) // use VBCC/PPC inline asm

#undef endian64
#undef endian32
#undef endian16

int64 endian64(__reg("r5/r6") int64) =
    "\trlwinm\t4,5,24,0,31\n"
    "\trlwinm\t3,6,24,0,31\n"
    "\trlwimi\t4,5,8,8,15\n"
    "\trlwimi\t3,6,8,8,15\n"
    "\trlwimi\t4,5,8,24,31\n"
    "\trlwimi\t3,6,8,24,31";

int32 endian32(__reg("r4") int32) =
    "\trlwinm\t3,4,24,0,31\n"
    "\trlwimi\t3,4,8,8,15\n"
    "\trlwimi\t3,4,8,24,31";

int16 endian16(__reg("r4") int16) =
    "\trlwinm\t3,4,8,16,24\n"
    "\trlwimi\t3,4,24,24,31";

#undef read_le64
#undef read_le32
#undef read_le16

int64 read_le64(__reg("r3") void *) =
    "\taddi\t5,3,4\n" // r5 = r3 + 4
    "\tlwbrx\t4,0,3\n"
    "\tlwbrx\t3,0,5";

int32 read_le32(__reg("r3") void *) =
    "\tlwbrx\t3,0,3";

int16 read_le16(__reg("r3") void *) =
    "\tlhbrx\t3,0,3";

#undef write_le64
#undef write_le32
#undef write_le16

void write_le64(__reg("r3") void *, __reg("r5/r6") int64) =
    "\taddi\t4,3,4\n" // r4 = r3 + 4
    "\tstwbrx\t6,0,3\n"
    "\tstwbrx\t5,0,4";

void write_le32(__reg("r3") void *, __reg("r4") int32) =
    "\tstwbrx\t4,0,3";

void write_le16(__reg("r3") void *, __reg("r4") int16) =
    "\tsthbrx\t4,0,3";

#endif

#ifndef CPU_IS_LITTLE_ENDIAN

#define be2nat16(A) (A)
#define be2nat32(A) (A)
#define be2nat64(A) (A)

#define le2nat16(A) endian16(A)
#define le2nat32(A) endian32(A)
#define le2nat64(A) endian64(A)

#define nat2be16(A) (A)
#define nat2be32(A) (A)
#define nat2be64(A) (A)

#define nat2le16(A) endian16(A)
#define nat2le32(A) endian32(A)
#define nat2le64(A) endian64(A)

#else

#define be2nat16(A) endian16(A)
#define be2nat32(A) endian32(A)
#define be2nat64(A) endian64(A)

#define le2nat16(A) (A)
#define le2nat32(A) (A)
#define le2nat64(A) (A)

#define nat2be16(A) endian16(A)
#define nat2be32(A) endian32(A)
#define nat2be64(A) endian64(A)

#define nat2le16(A) (A)
#define nat2le32(A) (A)
#define nat2le64(A) (A)

#endif

#endif /* ENDIAN_H */
