VERSION = 0
REVISION = 1

.macro DATE
.ascii "9.9.2008"
.endm

.macro VERS
.ascii "OGG 0.1"
.endm

.macro VSTRING
.ascii "OGG 0.1 (9.9.2008)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: OGG 0.1 (9.9.2008)"
.byte 0
.endm
