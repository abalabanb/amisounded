VERSION		EQU	0
REVISION	EQU	4

DATE	MACRO
		dc.b '31.10.2008'
		ENDM

VERS	MACRO
		dc.b 'MP3 0.4'
		ENDM

VSTRING	MACRO
		dc.b 'MP3 0.4 (31.10.2008)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: MP3 0.4 (31.10.2008)',0
		ENDM
