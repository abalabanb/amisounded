VERSION		EQU	0
REVISION	EQU	1

DATE	MACRO
		dc.b '9.9.2008'
		ENDM

VERS	MACRO
		dc.b 'OGG 0.1'
		ENDM

VSTRING	MACRO
		dc.b 'OGG 0.1 (9.9.2008)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: OGG 0.1 (9.9.2008)',0
		ENDM