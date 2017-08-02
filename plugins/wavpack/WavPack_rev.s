VERSION = 0
REVISION = 1

.macro DATE
.ascii "12.11.2008"
.endm

.macro VERS
.ascii "WavPack 0.1"
.endm

.macro VSTRING
.ascii "WavPack 0.1 (12.11.2008)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: WavPack 0.1 (12.11.2008)"
.byte 0
.endm
