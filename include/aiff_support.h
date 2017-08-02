#ifndef AIFF_SUPPORT_H
#define AIFF_SUPPORT_H

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

#define ID_AIFF	MAKE_ID('A','I','F','F')
#define ID_AIFC	MAKE_ID('A','I','F','C')
#define ID_FVER	MAKE_ID('F','V','E','R')
#define ID_COMM	MAKE_ID('C','O','M','M')
#define ID_SSND	MAKE_ID('S','S','N','D')

struct FormatVersionHeader {
	int32 timeStamp;
};

#define AIFCVersion1 (0xA2805140UL)

struct Common {
	int16	NumChannels;
	int32	NumFrames;
	int16	BitsPerSample;
	uint8	FramesPerSec[10];
};

struct ExtCommon {
	int16	NumChannels;
	int32	NumFrames;
	int16	BitsPerSample;
	uint8	FramesPerSec[10];
	uint32	CompType;
};

#define COMP_NONE	MAKE_ID('N','O','N','E')
#define COMP_fl32	MAKE_ID('f','l','3','2')
#define COMP_FL32	MAKE_ID('F','L','3','2')
#define COMP_fl64	MAKE_ID('f','l','6','4')
#define COMP_FL64	MAKE_ID('F','L','6','4')
#define COMP_alaw	MAKE_ID('a','l','a','w')
#define COMP_ALAW	MAKE_ID('A','L','A','W')
#define COMP_ulaw	MAKE_ID('u','l','a','w')
#define COMP_ULAW	MAKE_ID('U','L','A','W')
#define COMP_GSM	MAKE_ID('G','S','M',' ')
#define COMP_ima4	MAKE_ID('i','m','a','4')
#define COMP_MAC3	MAKE_ID('M','A','C','3')
#define COMP_MAC4	MAKE_ID('M','A','C','4')
#define COMP_ADP4	MAKE_ID('A','D','P','4')

struct SampledSoundHeader {
	int32	DataOffset;
	int32	BlockSize;
};

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

int32 extended2long (uint8 *extended);
void long2extended (int32 value, uint8 *extended);
int32 ParseSSND (struct IFFHandle *iff, void *buffer, int32 bufsiz);

#endif
