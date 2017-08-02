VERSION = 0
REVISION = 4

.macro DATE
.ascii "31.10.2008"
.endm

.macro VERS
.ascii "MP3 0.4"
.endm

.macro VSTRING
.ascii "MP3 0.4 (31.10.2008)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: MP3 0.4 (31.10.2008)"
.byte 0
.endm
