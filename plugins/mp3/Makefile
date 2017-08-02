CC = gcc -mcrt=newlib
CP = cp
RM = rm
CFLAGS = -O2 -Wall -Iinclude -D__USE_INLINE__
LINK = -nostartfiles
LIBS = -lmad -Llame -lmp3lame
TARGET = MP3
VERSION = 0
OBJS = mp3.o

$(TARGET): $(OBJS)
	$(CC) $(LINK) -o $@ $^ $(LIBS)
	strip -R.comment $@

mp3.o: $(TARGET)_rev.h include/plugins.h include/soundeditor_gc.h

include/plugins.h: ../../include/plugins.h
	$(CP) -f $< $@

include/soundeditor_gc.h: ../../include/soundeditor_gc.h
	$(CP) -f $< $@

.PHONY: distclean
distclean:
	$(RM) $(OBJS)

.PHONY: install
install: $(TARGET)
	$(CP) -f $< ../../AmiSoundED/Plugins/$(TARGET)

.PHONY: uninstall
uninstall:
	$(RM) ../../AmiSoundED/Plugins/$(TARGET)

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)
