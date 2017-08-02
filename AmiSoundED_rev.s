VERSION = 0
REVISION = 12

.macro DATE
.ascii "30.1.2009"
.endm

.macro VERS
.ascii "AmiSoundED 0.12"
.endm

.macro VSTRING
.ascii "AmiSoundED 0.12 (30.1.2009)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: AmiSoundED 0.12 (30.1.2009)"
.byte 0
.endm
