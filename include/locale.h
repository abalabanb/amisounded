/*
 * AmiSoundED - Sound Editor
 * Copyright (C) 2008-2009 Fredrik Wikstrom <fredrik@a500.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LOCALE_H
#define LOCALE_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_BLOCK
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_ABOUT_REQ 0
#define MSG_ERROR 1
#define MSG_OK_GAD 2
#define MSG_CANCEL_GAD 3
#define MSG_UNTITLED 4
#define MSG_NOMEM 5
#define MSG_OUTPUTFORMAT_LBL 6
#define MSG_RECORDSETTINGS_LBL 7
#define MSG_FREQUENCY_GAD 8
#define MSG_CHANNELS_GAD 9
#define MSG_MONO 10
#define MSG_STEREO 11
#define MSG_SAMPLESIZE_GAD 12
#define MSG_RECORDLENGTH_GAD 13
#define MSG_NOAISS_REQ 14
#define MSG_APPLYTO_GAD 15
#define MSG_SELECTION 16
#define MSG_WHOLESOUND 17
#define MSG_VIEWAREA 18
#define MSG_START_GAD 19
#define MSG_END_GAD 20
#define MSG_EFFECT_GAD 21
#define MSG_CUSTOM 22
#define MSG_FADEIN 23
#define MSG_FADEOUT 24
#define MSG_MAXIMISE 25
#define MSG_ADJUSTVOLUME_LBL 26
#define MSG_LOADING 27
#define MSG_SAVING 28
#define MSG_LEFT_CHANNEL 29
#define MSG_RIGHT_CHANNEL 30
#define MSG_BOTH_CHANNELS 31
#define MSG_RESAMPLE_LBL 32
#define MSG_SILENCE 33
#define MSG_UNSAVED_CHANGES_REQ 34
#define MSG_CONTINUE_GAD 35
#define MSG_NOCLIP_REQ 36
#define MSG_OLDAISS_REQ 37
#define MSG_RECORDTOFILE_GAD 38
#define MSG_FILENAME_GAD 39
#define MSG_NOAHI_REQ 40
#define MSG_PROJECT_MENU 200
#define MSG_PROJECT_NEW 201
#define MSG_PROJECT_OPEN 202
#define MSG_PROJECT_SAVE 203
#define MSG_PROJECT_SAVEAS 204
#define MSG_PROJECT_ABOUT 205
#define MSG_PROJECT_CLOSE 206
#define MSG_EDIT_MENU 207
#define MSG_EDIT_UNDO 208
#define MSG_EDIT_REDO 209
#define MSG_EDIT_CUT 210
#define MSG_EDIT_COPY 211
#define MSG_EDIT_PASTE 212
#define MSG_EDIT_DELETE 213
#define MSG_PROJECT_RECORD 214
#define MSG_EDIT_VOLUME 215
#define MSG_EDIT_RESAMPLE 216
#define MSG_EFFECTS_MENU 217
#define MSG_EFFECTS_ECHO 218
#define MSG_EDIT_MIX 219
#define MSG_EDIT_MIX_AVG 220
#define MSG_EDIT_MIX_ADD 221
#define MSG_EDIT_MIX_SUB 222
#define MSG_PREFS_MENU 223
#define MSG_PREFS_SETTINGS 224
#define MSG_PREFS_CREATEICONS 225
#define MSG_PREFS_SAVESETTINGS 226
#define MSG_PROJECT_QUIT 227
#define MSG_LOADINGUSINGDT 228
#define MSG_NEW_GAD 400
#define MSG_OPEN_GAD 401
#define MSG_SAVE_GAD 402
#define MSG_SAVEAS_GAD 403
#define MSG_PLAYSOUND_GAD 404
#define MSG_PLAYSELECTION_GAD 405
#define MSG_STOP_GAD 406
#define MSG_CUT_GAD 407
#define MSG_COPY_GAD 408
#define MSG_PASTE_GAD 409
#define MSG_DELETE_GAD 410
#define MSG_ZOOMSELECTION_GAD 411
#define MSG_ZOOMIN2X_GAD 412
#define MSG_ZOOMOUT2X_GAD 413
#define MSG_RECORD_GAD 414
#define MSG_SETTINGS_WIN 415
#define MSG_USE_GAD 416
#define MSG_RED_GAD 417
#define MSG_GREEN_GAD 418
#define MSG_BLUE_GAD 419
#define MSG_COLORS_GAD 420
#define MSG_PEN 421
#define MSG_BKG1_PEN 422
#define MSG_BKG2_PEN 423
#define MSG_ZEROLINES_PEN 424
#define MSG_WAVEFORM_PEN 425
#define MSG_SELBKG1_PEN 426
#define MSG_SELBKG2_PEN 427
#define MSG_SELWAVEFORM_PEN 428

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_ABOUT_REQ_STR "%s version %ld.%ld\n\nCopyright © 2008-2009\nFredrik Wikstrom\nhttp://a500.org"
#define MSG_ERROR_STR "Error"
#define MSG_OK_GAD_STR "_Ok"
#define MSG_CANCEL_GAD_STR "_Cancel"
#define MSG_UNTITLED_STR "Untitled"
#define MSG_NOMEM_STR "Not enough memory!"
#define MSG_OUTPUTFORMAT_LBL_STR "Output format"
#define MSG_RECORDSETTINGS_LBL_STR "Record Settings"
#define MSG_FREQUENCY_GAD_STR "Frequency (Hz)"
#define MSG_CHANNELS_GAD_STR "Channels"
#define MSG_MONO_STR "Mono (1)"
#define MSG_STEREO_STR "Stereo (2)"
#define MSG_SAMPLESIZE_GAD_STR "Sample Size"
#define MSG_RECORDLENGTH_GAD_STR "Length (s)"
#define MSG_NOAISS_REQ_STR "AISS assign not found.\n\nDownload and install the latest version\nfrom here: http://masonicons.de"
#define MSG_APPLYTO_GAD_STR "Apply to"
#define MSG_SELECTION_STR "Selection"
#define MSG_WHOLESOUND_STR "Whole sound"
#define MSG_VIEWAREA_STR "Viewed Area"
#define MSG_START_GAD_STR "Start (%)"
#define MSG_END_GAD_STR "End (%)"
#define MSG_EFFECT_GAD_STR "Effect"
#define MSG_CUSTOM_STR "Custom"
#define MSG_FADEIN_STR "Fade in"
#define MSG_FADEOUT_STR "Fade out"
#define MSG_MAXIMISE_STR "Maximise"
#define MSG_ADJUSTVOLUME_LBL_STR "Adjust Volume"
#define MSG_LOADING_STR "Loading..."
#define MSG_SAVING_STR "Saving..."
#define MSG_LEFT_CHANNEL_STR "Left"
#define MSG_RIGHT_CHANNEL_STR "Right"
#define MSG_BOTH_CHANNELS_STR "Both"
#define MSG_RESAMPLE_LBL_STR "Resample"
#define MSG_SILENCE_STR "Silence"
#define MSG_UNSAVED_CHANGES_REQ_STR "All unsaved changes to '%s' will be lost!"
#define MSG_CONTINUE_GAD_STR "Continue"
#define MSG_NOCLIP_REQ_STR "Clipboard does not contain sound data."
#define MSG_OLDAISS_REQ_STR "AISS v%ld.%ld or newer required.\n\nDownload and install the latest version\nfrom here: http://masonicons.de"
#define MSG_RECORDTOFILE_GAD_STR "Record to File"
#define MSG_FILENAME_GAD_STR "Filename"
#define MSG_NOAHI_REQ_STR "Could not open 'ahi.device'."
#define MSG_PROJECT_MENU_STR "Project"
#define MSG_PROJECT_NEW_STR "New"
#define MSG_PROJECT_OPEN_STR "Open..."
#define MSG_PROJECT_SAVE_STR "Save"
#define MSG_PROJECT_SAVEAS_STR "Save As..."
#define MSG_PROJECT_ABOUT_STR "About..."
#define MSG_PROJECT_CLOSE_STR "Close"
#define MSG_EDIT_MENU_STR "Edit"
#define MSG_EDIT_UNDO_STR "Undo"
#define MSG_EDIT_REDO_STR "Redo"
#define MSG_EDIT_CUT_STR "Cut"
#define MSG_EDIT_COPY_STR "Copy"
#define MSG_EDIT_PASTE_STR "Paste"
#define MSG_EDIT_DELETE_STR "Delete"
#define MSG_PROJECT_RECORD_STR "Record..."
#define MSG_EDIT_VOLUME_STR "Volume..."
#define MSG_EDIT_RESAMPLE_STR "Resample..."
#define MSG_EFFECTS_MENU_STR "Effects"
#define MSG_EFFECTS_ECHO_STR "Echo..."
#define MSG_EDIT_MIX_STR "Mix"
#define MSG_EDIT_MIX_AVG_STR "Average"
#define MSG_EDIT_MIX_ADD_STR "Add"
#define MSG_EDIT_MIX_SUB_STR "Subtract"
#define MSG_PREFS_MENU_STR "Preferences"
#define MSG_PREFS_SETTINGS_STR "Program Settings..."
#define MSG_PREFS_CREATEICONS_STR "Create Icons"
#define MSG_PREFS_SAVESETTINGS_STR "Save Settings"
#define MSG_PROJECT_QUIT_STR "Quit"
#define MSG_LOADINGUSINGDT_STR "Loading sound using datatypes..."
#define MSG_NEW_GAD_STR "New"
#define MSG_OPEN_GAD_STR "Open"
#define MSG_SAVE_GAD_STR "Save"
#define MSG_SAVEAS_GAD_STR "Save As"
#define MSG_PLAYSOUND_GAD_STR "Play"
#define MSG_PLAYSELECTION_GAD_STR "Play Selection"
#define MSG_STOP_GAD_STR "Stop"
#define MSG_CUT_GAD_STR "Cut"
#define MSG_COPY_GAD_STR "Copy"
#define MSG_PASTE_GAD_STR "Paste"
#define MSG_DELETE_GAD_STR "Delete"
#define MSG_ZOOMSELECTION_GAD_STR "Zoom to Selection"
#define MSG_ZOOMIN2X_GAD_STR "Zoom In (2x)"
#define MSG_ZOOMOUT2X_GAD_STR "Zoom Out (2x)"
#define MSG_RECORD_GAD_STR "Record"
#define MSG_SETTINGS_WIN_STR "Program Settings"
#define MSG_USE_GAD_STR "Use"
#define MSG_RED_GAD_STR "Red:"
#define MSG_GREEN_GAD_STR "Green:"
#define MSG_BLUE_GAD_STR "Blue:"
#define MSG_COLORS_GAD_STR "Colors"
#define MSG_PEN_STR "Pen"
#define MSG_BKG1_PEN_STR "Background gradient - Outer"
#define MSG_BKG2_PEN_STR "Background gradient - Inner"
#define MSG_ZEROLINES_PEN_STR "Zero point lines"
#define MSG_WAVEFORM_PEN_STR "Waveform"
#define MSG_SELBKG1_PEN_STR "Selection - Background gradient - Outer"
#define MSG_SELBKG2_PEN_STR "Selection - Background gradient - Inner"
#define MSG_SELWAVEFORM_PEN_STR "Selection - Waveform"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
    LONG         cca_ID;
    CONST_STRPTR cca_Str;
};

STATIC CONST struct CatCompArrayType CatCompArray[] =
{
    {MSG_ABOUT_REQ,(CONST_STRPTR)MSG_ABOUT_REQ_STR},
    {MSG_ERROR,(CONST_STRPTR)MSG_ERROR_STR},
    {MSG_OK_GAD,(CONST_STRPTR)MSG_OK_GAD_STR},
    {MSG_CANCEL_GAD,(CONST_STRPTR)MSG_CANCEL_GAD_STR},
    {MSG_UNTITLED,(CONST_STRPTR)MSG_UNTITLED_STR},
    {MSG_NOMEM,(CONST_STRPTR)MSG_NOMEM_STR},
    {MSG_OUTPUTFORMAT_LBL,(CONST_STRPTR)MSG_OUTPUTFORMAT_LBL_STR},
    {MSG_RECORDSETTINGS_LBL,(CONST_STRPTR)MSG_RECORDSETTINGS_LBL_STR},
    {MSG_FREQUENCY_GAD,(CONST_STRPTR)MSG_FREQUENCY_GAD_STR},
    {MSG_CHANNELS_GAD,(CONST_STRPTR)MSG_CHANNELS_GAD_STR},
    {MSG_MONO,(CONST_STRPTR)MSG_MONO_STR},
    {MSG_STEREO,(CONST_STRPTR)MSG_STEREO_STR},
    {MSG_SAMPLESIZE_GAD,(CONST_STRPTR)MSG_SAMPLESIZE_GAD_STR},
    {MSG_RECORDLENGTH_GAD,(CONST_STRPTR)MSG_RECORDLENGTH_GAD_STR},
    {MSG_NOAISS_REQ,(CONST_STRPTR)MSG_NOAISS_REQ_STR},
    {MSG_APPLYTO_GAD,(CONST_STRPTR)MSG_APPLYTO_GAD_STR},
    {MSG_SELECTION,(CONST_STRPTR)MSG_SELECTION_STR},
    {MSG_WHOLESOUND,(CONST_STRPTR)MSG_WHOLESOUND_STR},
    {MSG_VIEWAREA,(CONST_STRPTR)MSG_VIEWAREA_STR},
    {MSG_START_GAD,(CONST_STRPTR)MSG_START_GAD_STR},
    {MSG_END_GAD,(CONST_STRPTR)MSG_END_GAD_STR},
    {MSG_EFFECT_GAD,(CONST_STRPTR)MSG_EFFECT_GAD_STR},
    {MSG_CUSTOM,(CONST_STRPTR)MSG_CUSTOM_STR},
    {MSG_FADEIN,(CONST_STRPTR)MSG_FADEIN_STR},
    {MSG_FADEOUT,(CONST_STRPTR)MSG_FADEOUT_STR},
    {MSG_MAXIMISE,(CONST_STRPTR)MSG_MAXIMISE_STR},
    {MSG_ADJUSTVOLUME_LBL,(CONST_STRPTR)MSG_ADJUSTVOLUME_LBL_STR},
    {MSG_LOADING,(CONST_STRPTR)MSG_LOADING_STR},
    {MSG_SAVING,(CONST_STRPTR)MSG_SAVING_STR},
    {MSG_LEFT_CHANNEL,(CONST_STRPTR)MSG_LEFT_CHANNEL_STR},
    {MSG_RIGHT_CHANNEL,(CONST_STRPTR)MSG_RIGHT_CHANNEL_STR},
    {MSG_BOTH_CHANNELS,(CONST_STRPTR)MSG_BOTH_CHANNELS_STR},
    {MSG_RESAMPLE_LBL,(CONST_STRPTR)MSG_RESAMPLE_LBL_STR},
    {MSG_SILENCE,(CONST_STRPTR)MSG_SILENCE_STR},
    {MSG_UNSAVED_CHANGES_REQ,(CONST_STRPTR)MSG_UNSAVED_CHANGES_REQ_STR},
    {MSG_CONTINUE_GAD,(CONST_STRPTR)MSG_CONTINUE_GAD_STR},
    {MSG_NOCLIP_REQ,(CONST_STRPTR)MSG_NOCLIP_REQ_STR},
    {MSG_OLDAISS_REQ,(CONST_STRPTR)MSG_OLDAISS_REQ_STR},
    {MSG_RECORDTOFILE_GAD,(CONST_STRPTR)MSG_RECORDTOFILE_GAD_STR},
    {MSG_FILENAME_GAD,(CONST_STRPTR)MSG_FILENAME_GAD_STR},
    {MSG_NOAHI_REQ,(CONST_STRPTR)MSG_NOAHI_REQ_STR},
    {MSG_PROJECT_MENU,(CONST_STRPTR)MSG_PROJECT_MENU_STR},
    {MSG_PROJECT_NEW,(CONST_STRPTR)MSG_PROJECT_NEW_STR},
    {MSG_PROJECT_OPEN,(CONST_STRPTR)MSG_PROJECT_OPEN_STR},
    {MSG_PROJECT_SAVE,(CONST_STRPTR)MSG_PROJECT_SAVE_STR},
    {MSG_PROJECT_SAVEAS,(CONST_STRPTR)MSG_PROJECT_SAVEAS_STR},
    {MSG_PROJECT_ABOUT,(CONST_STRPTR)MSG_PROJECT_ABOUT_STR},
    {MSG_PROJECT_CLOSE,(CONST_STRPTR)MSG_PROJECT_CLOSE_STR},
    {MSG_EDIT_MENU,(CONST_STRPTR)MSG_EDIT_MENU_STR},
    {MSG_EDIT_UNDO,(CONST_STRPTR)MSG_EDIT_UNDO_STR},
    {MSG_EDIT_REDO,(CONST_STRPTR)MSG_EDIT_REDO_STR},
    {MSG_EDIT_CUT,(CONST_STRPTR)MSG_EDIT_CUT_STR},
    {MSG_EDIT_COPY,(CONST_STRPTR)MSG_EDIT_COPY_STR},
    {MSG_EDIT_PASTE,(CONST_STRPTR)MSG_EDIT_PASTE_STR},
    {MSG_EDIT_DELETE,(CONST_STRPTR)MSG_EDIT_DELETE_STR},
    {MSG_PROJECT_RECORD,(CONST_STRPTR)MSG_PROJECT_RECORD_STR},
    {MSG_EDIT_VOLUME,(CONST_STRPTR)MSG_EDIT_VOLUME_STR},
    {MSG_EDIT_RESAMPLE,(CONST_STRPTR)MSG_EDIT_RESAMPLE_STR},
    {MSG_EFFECTS_MENU,(CONST_STRPTR)MSG_EFFECTS_MENU_STR},
    {MSG_EFFECTS_ECHO,(CONST_STRPTR)MSG_EFFECTS_ECHO_STR},
    {MSG_EDIT_MIX,(CONST_STRPTR)MSG_EDIT_MIX_STR},
    {MSG_EDIT_MIX_AVG,(CONST_STRPTR)MSG_EDIT_MIX_AVG_STR},
    {MSG_EDIT_MIX_ADD,(CONST_STRPTR)MSG_EDIT_MIX_ADD_STR},
    {MSG_EDIT_MIX_SUB,(CONST_STRPTR)MSG_EDIT_MIX_SUB_STR},
    {MSG_PREFS_MENU,(CONST_STRPTR)MSG_PREFS_MENU_STR},
    {MSG_PREFS_SETTINGS,(CONST_STRPTR)MSG_PREFS_SETTINGS_STR},
    {MSG_PREFS_CREATEICONS,(CONST_STRPTR)MSG_PREFS_CREATEICONS_STR},
    {MSG_PREFS_SAVESETTINGS,(CONST_STRPTR)MSG_PREFS_SAVESETTINGS_STR},
    {MSG_PROJECT_QUIT,(CONST_STRPTR)MSG_PROJECT_QUIT_STR},
    {MSG_LOADINGUSINGDT,(CONST_STRPTR)MSG_LOADINGUSINGDT_STR},
    {MSG_NEW_GAD,(CONST_STRPTR)MSG_NEW_GAD_STR},
    {MSG_OPEN_GAD,(CONST_STRPTR)MSG_OPEN_GAD_STR},
    {MSG_SAVE_GAD,(CONST_STRPTR)MSG_SAVE_GAD_STR},
    {MSG_SAVEAS_GAD,(CONST_STRPTR)MSG_SAVEAS_GAD_STR},
    {MSG_PLAYSOUND_GAD,(CONST_STRPTR)MSG_PLAYSOUND_GAD_STR},
    {MSG_PLAYSELECTION_GAD,(CONST_STRPTR)MSG_PLAYSELECTION_GAD_STR},
    {MSG_STOP_GAD,(CONST_STRPTR)MSG_STOP_GAD_STR},
    {MSG_CUT_GAD,(CONST_STRPTR)MSG_CUT_GAD_STR},
    {MSG_COPY_GAD,(CONST_STRPTR)MSG_COPY_GAD_STR},
    {MSG_PASTE_GAD,(CONST_STRPTR)MSG_PASTE_GAD_STR},
    {MSG_DELETE_GAD,(CONST_STRPTR)MSG_DELETE_GAD_STR},
    {MSG_ZOOMSELECTION_GAD,(CONST_STRPTR)MSG_ZOOMSELECTION_GAD_STR},
    {MSG_ZOOMIN2X_GAD,(CONST_STRPTR)MSG_ZOOMIN2X_GAD_STR},
    {MSG_ZOOMOUT2X_GAD,(CONST_STRPTR)MSG_ZOOMOUT2X_GAD_STR},
    {MSG_RECORD_GAD,(CONST_STRPTR)MSG_RECORD_GAD_STR},
    {MSG_SETTINGS_WIN,(CONST_STRPTR)MSG_SETTINGS_WIN_STR},
    {MSG_USE_GAD,(CONST_STRPTR)MSG_USE_GAD_STR},
    {MSG_RED_GAD,(CONST_STRPTR)MSG_RED_GAD_STR},
    {MSG_GREEN_GAD,(CONST_STRPTR)MSG_GREEN_GAD_STR},
    {MSG_BLUE_GAD,(CONST_STRPTR)MSG_BLUE_GAD_STR},
    {MSG_COLORS_GAD,(CONST_STRPTR)MSG_COLORS_GAD_STR},
    {MSG_PEN,(CONST_STRPTR)MSG_PEN_STR},
    {MSG_BKG1_PEN,(CONST_STRPTR)MSG_BKG1_PEN_STR},
    {MSG_BKG2_PEN,(CONST_STRPTR)MSG_BKG2_PEN_STR},
    {MSG_ZEROLINES_PEN,(CONST_STRPTR)MSG_ZEROLINES_PEN_STR},
    {MSG_WAVEFORM_PEN,(CONST_STRPTR)MSG_WAVEFORM_PEN_STR},
    {MSG_SELBKG1_PEN,(CONST_STRPTR)MSG_SELBKG1_PEN_STR},
    {MSG_SELBKG2_PEN,(CONST_STRPTR)MSG_SELBKG2_PEN_STR},
    {MSG_SELWAVEFORM_PEN,(CONST_STRPTR)MSG_SELWAVEFORM_PEN_STR},
};

#endif /* CATCOMP_ARRAY */


/****************************************************************************/


#ifdef CATCOMP_BLOCK

STATIC CONST UBYTE CatCompBlock[] =
{
    "\x00\x00\x00\x00\x00\x4C"
    MSG_ABOUT_REQ_STR "\x00\x00"
    "\x00\x00\x00\x01\x00\x06"
    MSG_ERROR_STR "\x00"
    "\x00\x00\x00\x02\x00\x04"
    MSG_OK_GAD_STR "\x00"
    "\x00\x00\x00\x03\x00\x08"
    MSG_CANCEL_GAD_STR "\x00"
    "\x00\x00\x00\x04\x00\x0A"
    MSG_UNTITLED_STR "\x00\x00"
    "\x00\x00\x00\x05\x00\x14"
    MSG_NOMEM_STR "\x00\x00"
    "\x00\x00\x00\x06\x00\x0E"
    MSG_OUTPUTFORMAT_LBL_STR "\x00"
    "\x00\x00\x00\x07\x00\x10"
    MSG_RECORDSETTINGS_LBL_STR "\x00"
    "\x00\x00\x00\x08\x00\x10"
    MSG_FREQUENCY_GAD_STR "\x00\x00"
    "\x00\x00\x00\x09\x00\x0A"
    MSG_CHANNELS_GAD_STR "\x00\x00"
    "\x00\x00\x00\x0A\x00\x0A"
    MSG_MONO_STR "\x00\x00"
    "\x00\x00\x00\x0B\x00\x0C"
    MSG_STEREO_STR "\x00\x00"
    "\x00\x00\x00\x0C\x00\x0C"
    MSG_SAMPLESIZE_GAD_STR "\x00"
    "\x00\x00\x00\x0D\x00\x0C"
    MSG_RECORDLENGTH_GAD_STR "\x00\x00"
    "\x00\x00\x00\x0E\x00\x60"
    MSG_NOAISS_REQ_STR "\x00"
    "\x00\x00\x00\x0F\x00\x0A"
    MSG_APPLYTO_GAD_STR "\x00\x00"
    "\x00\x00\x00\x10\x00\x0A"
    MSG_SELECTION_STR "\x00"
    "\x00\x00\x00\x11\x00\x0C"
    MSG_WHOLESOUND_STR "\x00"
    "\x00\x00\x00\x12\x00\x0C"
    MSG_VIEWAREA_STR "\x00"
    "\x00\x00\x00\x13\x00\x0A"
    MSG_START_GAD_STR "\x00"
    "\x00\x00\x00\x14\x00\x08"
    MSG_END_GAD_STR "\x00"
    "\x00\x00\x00\x15\x00\x08"
    MSG_EFFECT_GAD_STR "\x00\x00"
    "\x00\x00\x00\x16\x00\x08"
    MSG_CUSTOM_STR "\x00\x00"
    "\x00\x00\x00\x17\x00\x08"
    MSG_FADEIN_STR "\x00"
    "\x00\x00\x00\x18\x00\x0A"
    MSG_FADEOUT_STR "\x00\x00"
    "\x00\x00\x00\x19\x00\x0A"
    MSG_MAXIMISE_STR "\x00\x00"
    "\x00\x00\x00\x1A\x00\x0E"
    MSG_ADJUSTVOLUME_LBL_STR "\x00"
    "\x00\x00\x00\x1B\x00\x0C"
    MSG_LOADING_STR "\x00\x00"
    "\x00\x00\x00\x1C\x00\x0A"
    MSG_SAVING_STR "\x00"
    "\x00\x00\x00\x1D\x00\x06"
    MSG_LEFT_CHANNEL_STR "\x00\x00"
    "\x00\x00\x00\x1E\x00\x06"
    MSG_RIGHT_CHANNEL_STR "\x00"
    "\x00\x00\x00\x1F\x00\x06"
    MSG_BOTH_CHANNELS_STR "\x00\x00"
    "\x00\x00\x00\x20\x00\x0A"
    MSG_RESAMPLE_LBL_STR "\x00\x00"
    "\x00\x00\x00\x21\x00\x08"
    MSG_SILENCE_STR "\x00"
    "\x00\x00\x00\x22\x00\x2A"
    MSG_UNSAVED_CHANGES_REQ_STR "\x00"
    "\x00\x00\x00\x23\x00\x0A"
    MSG_CONTINUE_GAD_STR "\x00\x00"
    "\x00\x00\x00\x24\x00\x28"
    MSG_NOCLIP_REQ_STR "\x00\x00"
    "\x00\x00\x00\x25\x00\x6A"
    MSG_OLDAISS_REQ_STR "\x00"
    "\x00\x00\x00\x26\x00\x10"
    MSG_RECORDTOFILE_GAD_STR "\x00\x00"
    "\x00\x00\x00\x27\x00\x0A"
    MSG_FILENAME_GAD_STR "\x00\x00"
    "\x00\x00\x00\x28\x00\x1E"
    MSG_NOAHI_REQ_STR "\x00\x00"
    "\x00\x00\x00\xC8\x00\x08"
    MSG_PROJECT_MENU_STR "\x00"
    "\x00\x00\x00\xC9\x00\x04"
    MSG_PROJECT_NEW_STR "\x00"
    "\x00\x00\x00\xCA\x00\x08"
    MSG_PROJECT_OPEN_STR "\x00"
    "\x00\x00\x00\xCB\x00\x06"
    MSG_PROJECT_SAVE_STR "\x00\x00"
    "\x00\x00\x00\xCC\x00\x0C"
    MSG_PROJECT_SAVEAS_STR "\x00\x00"
    "\x00\x00\x00\xCD\x00\x0A"
    MSG_PROJECT_ABOUT_STR "\x00\x00"
    "\x00\x00\x00\xCE\x00\x06"
    MSG_PROJECT_CLOSE_STR "\x00"
    "\x00\x00\x00\xCF\x00\x06"
    MSG_EDIT_MENU_STR "\x00\x00"
    "\x00\x00\x00\xD0\x00\x06"
    MSG_EDIT_UNDO_STR "\x00\x00"
    "\x00\x00\x00\xD1\x00\x06"
    MSG_EDIT_REDO_STR "\x00\x00"
    "\x00\x00\x00\xD2\x00\x04"
    MSG_EDIT_CUT_STR "\x00"
    "\x00\x00\x00\xD3\x00\x06"
    MSG_EDIT_COPY_STR "\x00\x00"
    "\x00\x00\x00\xD4\x00\x06"
    MSG_EDIT_PASTE_STR "\x00"
    "\x00\x00\x00\xD5\x00\x08"
    MSG_EDIT_DELETE_STR "\x00\x00"
    "\x00\x00\x00\xD6\x00\x0A"
    MSG_PROJECT_RECORD_STR "\x00"
    "\x00\x00\x00\xD7\x00\x0A"
    MSG_EDIT_VOLUME_STR "\x00"
    "\x00\x00\x00\xD8\x00\x0C"
    MSG_EDIT_RESAMPLE_STR "\x00"
    "\x00\x00\x00\xD9\x00\x08"
    MSG_EFFECTS_MENU_STR "\x00"
    "\x00\x00\x00\xDA\x00\x08"
    MSG_EFFECTS_ECHO_STR "\x00"
    "\x00\x00\x00\xDB\x00\x04"
    MSG_EDIT_MIX_STR "\x00"
    "\x00\x00\x00\xDC\x00\x08"
    MSG_EDIT_MIX_AVG_STR "\x00"
    "\x00\x00\x00\xDD\x00\x04"
    MSG_EDIT_MIX_ADD_STR "\x00"
    "\x00\x00\x00\xDE\x00\x0A"
    MSG_EDIT_MIX_SUB_STR "\x00\x00"
    "\x00\x00\x00\xDF\x00\x0C"
    MSG_PREFS_MENU_STR "\x00"
    "\x00\x00\x00\xE0\x00\x14"
    MSG_PREFS_SETTINGS_STR "\x00"
    "\x00\x00\x00\xE1\x00\x0E"
    MSG_PREFS_CREATEICONS_STR "\x00\x00"
    "\x00\x00\x00\xE2\x00\x0E"
    MSG_PREFS_SAVESETTINGS_STR "\x00"
    "\x00\x00\x00\xE3\x00\x06"
    MSG_PROJECT_QUIT_STR "\x00\x00"
    "\x00\x00\x00\xE4\x00\x22"
    MSG_LOADINGUSINGDT_STR "\x00\x00"
    "\x00\x00\x01\x90\x00\x04"
    MSG_NEW_GAD_STR "\x00"
    "\x00\x00\x01\x91\x00\x06"
    MSG_OPEN_GAD_STR "\x00\x00"
    "\x00\x00\x01\x92\x00\x06"
    MSG_SAVE_GAD_STR "\x00\x00"
    "\x00\x00\x01\x93\x00\x08"
    MSG_SAVEAS_GAD_STR "\x00"
    "\x00\x00\x01\x94\x00\x06"
    MSG_PLAYSOUND_GAD_STR "\x00\x00"
    "\x00\x00\x01\x95\x00\x10"
    MSG_PLAYSELECTION_GAD_STR "\x00\x00"
    "\x00\x00\x01\x96\x00\x06"
    MSG_STOP_GAD_STR "\x00\x00"
    "\x00\x00\x01\x97\x00\x04"
    MSG_CUT_GAD_STR "\x00"
    "\x00\x00\x01\x98\x00\x06"
    MSG_COPY_GAD_STR "\x00\x00"
    "\x00\x00\x01\x99\x00\x06"
    MSG_PASTE_GAD_STR "\x00"
    "\x00\x00\x01\x9A\x00\x08"
    MSG_DELETE_GAD_STR "\x00\x00"
    "\x00\x00\x01\x9B\x00\x12"
    MSG_ZOOMSELECTION_GAD_STR "\x00"
    "\x00\x00\x01\x9C\x00\x0E"
    MSG_ZOOMIN2X_GAD_STR "\x00\x00"
    "\x00\x00\x01\x9D\x00\x0E"
    MSG_ZOOMOUT2X_GAD_STR "\x00"
    "\x00\x00\x01\x9E\x00\x08"
    MSG_RECORD_GAD_STR "\x00\x00"
    "\x00\x00\x01\x9F\x00\x12"
    MSG_SETTINGS_WIN_STR "\x00\x00"
    "\x00\x00\x01\xA0\x00\x04"
    MSG_USE_GAD_STR "\x00"
    "\x00\x00\x01\xA1\x00\x06"
    MSG_RED_GAD_STR "\x00\x00"
    "\x00\x00\x01\xA2\x00\x08"
    MSG_GREEN_GAD_STR "\x00\x00"
    "\x00\x00\x01\xA3\x00\x06"
    MSG_BLUE_GAD_STR "\x00"
    "\x00\x00\x01\xA4\x00\x08"
    MSG_COLORS_GAD_STR "\x00\x00"
    "\x00\x00\x01\xA5\x00\x04"
    MSG_PEN_STR "\x00"
    "\x00\x00\x01\xA6\x00\x1C"
    MSG_BKG1_PEN_STR "\x00"
    "\x00\x00\x01\xA7\x00\x1C"
    MSG_BKG2_PEN_STR "\x00"
    "\x00\x00\x01\xA8\x00\x12"
    MSG_ZEROLINES_PEN_STR "\x00\x00"
    "\x00\x00\x01\xA9\x00\x0A"
    MSG_WAVEFORM_PEN_STR "\x00\x00"
    "\x00\x00\x01\xAA\x00\x28"
    MSG_SELBKG1_PEN_STR "\x00"
    "\x00\x00\x01\xAB\x00\x28"
    MSG_SELBKG2_PEN_STR "\x00"
    "\x00\x00\x01\xAC\x00\x16"
    MSG_SELWAVEFORM_PEN_STR "\x00\x00"
};

#endif /* CATCOMP_BLOCK */


/****************************************************************************/


#ifdef CATCOMP_CODE

#ifndef PROTO_LOCALE_H
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/locale.h>
#endif

struct LocaleInfo
{
#ifndef __amigaos4__
    struct Library     *li_LocaleBase;
#else
    struct LocaleIFace *li_ILocale;
#endif
    struct Catalog     *li_Catalog;
};


CONST_STRPTR GetString(struct LocaleInfo *li, LONG stringNum);

CONST_STRPTR GetString(struct LocaleInfo *li, LONG stringNum)
{
#ifndef __amigaos4__
    struct Library     *LocaleBase = li->li_LocaleBase;
#else
    struct LocaleIFace *ILocale    = li->li_ILocale;
#endif
    LONG         *l;
    UWORD        *w;
    CONST_STRPTR  builtIn;

    l = (LONG *)CatCompBlock;

    while (*l != stringNum)
    {
        w = (UWORD *)((ULONG)l + 4);
        l = (LONG *)((ULONG)l + (ULONG)*w + 6);
    }
    builtIn = (CONST_STRPTR)((ULONG)l + 6);

#ifndef __amigaos4__
    if (LocaleBase)
    {
        return GetCatalogStr(li->li_Catalog, stringNum, builtIn);
    }
#else
    if (ILocale)
    {
#ifdef __USE_INLINE__
        return GetCatalogStr(li->li_Catalog, stringNum, builtIn);
#else
        return ILocale->GetCatalogStr(li->li_Catalog, stringNum, builtIn);
#endif
    }
#endif
    return builtIn;
}


#endif /* CATCOMP_CODE */


/****************************************************************************/


#endif /* LOCALE_H */
