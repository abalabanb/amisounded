# amisounded
A 32-bit mono/stereo sound editor for AmigaOS 4

Features:

 - ReAction GUI
 - Project window can be snapshotted ("snapshot" button in titlebar)
 - Edit mono/stereo sounds in 8-, 16-, 24- or 32-bit
 - Sound playback/recording through AHI's device interface
 - Clipboard support (uses 8svx format for 8-bit data and otherwise aiff)
 - Locale support
 - Drag and drop support
 - Record to disk (uncompressed aiff format)

Loads:
 - IFF-8SVX (.8svx, .iff)
   - uncompressed
   - Fibonacci delta compression
   - Exponential delta compression
 - IFF-AIFF / IFF-AIFC (.aiff)
   - uncompressed
 - Sun AU (.au)
   - PCM, a-law, u-law, float
 - RIFF-WAVE (.wav)
   - PCM, MS/IMA ADPCM, GSM610, a-law, u-law, float, ...
 - Datatypes (8-bit only)

Saves:
 - IFF-8SVX (.8svx, .iff)
 - IFF-AIFF (.aiff)
 - RIFF-WAVE (.wav)

