#
# AmiSoundED - Sound Editor
# Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

CC = vc +aosppc
RM = delete
CP = copy

# Change these as required
OPTIMIZE = -O3 -speed
DEBUG =
CFLAGS = $(OPTIMIZE) $(DEBUG) -I. -Iinclude -Iplugins/wave/include -Iplugins/au/include \
	-D__USE_INLINE__ #-DDEBUG

# Flags passed to gcc during linking
LINK = $(OPTIMIZE) $(DEBUG)

# Name of the "thing" to build
TARGET  = AmiSoundED
VERSION = 0

# Additional linker libraries
LIBS = -lauto -lraauto -lm

# Source code files used in this project
# Add any additional files to this line

OBJS = locale_support.o main.o project_main.o project_gui.o soundeditor_gc.o \
	soundeditor_samples.o speedbar_support.o loadsnd.o loadsnd_dt.o \
	playsnd.o clipboard.o iffparse_clipboard.o plugins.o aiff_support.o \
	requesters.o prefs.o ext_window_class.o savesnd.o recordsnd.o \
	amiupdate.o checkaiss.o donotifyattrs.o interfaces/sounded.o \
	progressbar.o volumeadjust.o resample.o record2file.o path.o prefs_main.o \
	prefs_gui.o

PLUGINS = \
	AmiSoundED/Plugins/8SVX \
	AmiSoundED/Plugins/AIFF \
	AmiSoundED/Plugins/WAVE \
	AmiSoundED/Plugins/AU

# -------------------------------------------------------------
# Nothing should need changing below this line

# Rules for building
all: AmiSoundED/$(TARGET) $(PLUGINS)

AmiSoundED/$(TARGET): $(OBJS)
	$(CC) $(LINK) -o $@ $^ $(LIBS)
	C:Protect $@ +e

main.o requesters.o interfaces/sounded.o: $(TARGET)_rev.h
locale_support.o: include/locale.h

include/locale.h: $(TARGET).cd
	catcomp $< CFILE $@

AmiSoundED/Plugins/8SVX: plugins/8svx/8svx.o iffparse_support.o
	$(CC) $(LINK) -nostdlib -o $@ $^ -lvc

AmiSoundED/Plugins/AIFF: plugins/aiff/aiff.o aiff_support.o iffparse_support.o
	$(CC) $(LINK) -nostdlib -o $@ $^ -lvc

AmiSoundED/Plugins/WAVE: plugins/wave/wave.o plugins/wave/wave_codecs.o plugins/wave/wave_pcm.o \
	plugins/wave/wave_ima_adpcm.o plugins/wave/wave_ms_adpcm.o plugins/wave/wave_ieee_float.o \
	plugins/wave/wave_alaw.o plugins/wave/wave_gsm610.o bitpack.o bitpack_lsb.o
	$(CC) $(LINK) -nostdlib -o $@ $^ -Lplugins/wave/gsm -lgsm

AmiSoundED/Plugins/AU: plugins/au/au.o plugins/au/au_codecs.o plugins/au/au_pcm.o \
	plugins/au/au_alaw.o plugins/au/au_float.o
	$(CC) $(LINK) -nostdlib -o $@ $^ -lvc -lm

.PHONY: clean
clean:
	$(RM) $(OBJS) plugins/#?/#?.o

.PHONY: dist
dist: all
	$(CP) $(TARGET).cd AmiSoundED/Catalogs
	lha u -r amisounded.lha AmiSoundED AutoInstall

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)
