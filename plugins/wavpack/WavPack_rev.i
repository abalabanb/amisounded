VERSION		EQU	0
REVISION	EQU	1

DATE	MACRO
		dc.b '12.11.2008'
		ENDM

VERS	MACRO
		dc.b 'WavPack 0.1'
		ENDM

VSTRING	MACRO
		dc.b 'WavPack 0.1 (12.11.2008)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: WavPack 0.1 (12.11.2008)',0
		ENDM