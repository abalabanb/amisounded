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

#include <exec/exec.h>
#include <utility/utility.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <graphics/rpattr.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <string.h>

#include "soundeditor_gc.h"
#include "soundeditor_gc_private.h"
#include "donotifyattrs.h"
#include "debug.h"

#define MAX_HPROP_TOTAL 32000

#define G(o) ((struct Gadget *)(o))
#define EXTG(o) ((struct ExtGadget *)(o))

#define MINIMUM_WIDTH   80
#define MINIMUM_HEIGHT  48
#define NOMINAL_WIDTH   320
#define NOMINAL_HEIGHT  192

static uint32 ClassDispatch (Class *cl, Object *o, Msg msg);

void InitGradSpec(struct GradientSpec *grad, int angle, uint32 argb1, uint32 argb2) {
    grad->Direction = DirectionVector(angle);
    grad->Mode = GRADMODE_COLOR;
    grad->Specs.Abs.RGBStart[0] = ((argb1 >> 16) & 0xff) * 0x01010101;
    grad->Specs.Abs.RGBStart[1] = ((argb1 >> 8) & 0xff) * 0x01010101;
    grad->Specs.Abs.RGBStart[2] = (argb1 & 0xff) * 0x01010101;
    grad->Specs.Abs.RGBEnd[0] = ((argb2 >> 16) & 0xff) * 0x01010101;
    grad->Specs.Abs.RGBEnd[1] = ((argb2 >> 8) & 0xff) * 0x01010101;
    grad->Specs.Abs.RGBEnd[2] = (argb2 & 0xff) * 0x01010101;
}

Class *SOUNDEDITOR_MakeClass () {
    Class *cl;
    cl = MakeClass("soundeditor.gadget", "gadgetclass", NULL, sizeof(InstanceData), 0);
    if (cl) {
        GlobalData *gd = AllocMem(sizeof(*gd), MEMF_SHARED|MEMF_CLEAR);
        if (gd) {
            gd->Colors[COL_BKG1] = 0x000000;
            gd->Colors[COL_BKG2] = 0x5f5f5f;
            gd->Colors[COL_LINES] = 0xffffff;
            gd->Colors[COL_WAVEFORM] = 0x00ff00;
            gd->Colors[COL_ALTBKG1] = 0x7f00ff;
            gd->Colors[COL_ALTBKG2] = 0x220044;
            gd->Colors[COL_ALTWAVEFORM] = 0xAA66ff;

            gd->Pen2Col[PEN_BKG] = COL_BKG1;
            gd->Pen2Col[PEN_LINES] = COL_LINES;
            gd->Pen2Col[PEN_WAVEFORM] = COL_WAVEFORM;
            gd->Pen2Col[PEN_ALTBKG] = COL_ALTBKG1;
            gd->Pen2Col[PEN_ALTWAVEFORM] = COL_ALTWAVEFORM;

            gd->Pen2Dri[PEN_BKG] = TEXTPEN;
            gd->Pen2Dri[PEN_LINES] = HIGHLIGHTTEXTPEN;
            gd->Pen2Dri[PEN_WAVEFORM] = HIGHLIGHTTEXTPEN;
            gd->Pen2Dri[PEN_ALTBKG] = HIGHLIGHTTEXTPEN;
            gd->Pen2Dri[PEN_ALTWAVEFORM] = TEXTPEN;

            InitGradSpec(&gd->GradSpec[0], 0, gd->Colors[COL_BKG1],
                gd->Colors[COL_BKG2]);
            InitGradSpec(&gd->GradSpec[1], 180, gd->Colors[COL_BKG1],
                gd->Colors[COL_BKG2]);
            InitGradSpec(&gd->GradSpec[2], 0, gd->Colors[COL_ALTBKG1],
                gd->Colors[COL_ALTBKG2]);
            InitGradSpec(&gd->GradSpec[3], 180, gd->Colors[COL_ALTBKG1],
                gd->Colors[COL_ALTBKG2]);

            cl->cl_Dispatcher.h_Entry = (HOOKFUNC)ClassDispatch;
            cl->cl_UserData = (uint32)gd;
        } else {
            SOUNDEDITOR_FreeClass(cl);
            cl = NULL;
        }
    }
    return cl;
}

void SOUNDEDITOR_FreeClass (Class *cl) {
    if (cl) {
        if (cl->cl_UserData) FreeMem((APTR)cl->cl_UserData, sizeof(GlobalData));
        FreeClass(cl);
    }
}

static void FreeSampleData (InstanceData *id, BOOL init);
static void ObtainPens (InstanceData *id, GlobalData *gd);
static void ReleasePens (InstanceData *id);
static void UpdatePropValues (InstanceData *id);

static uint32 SOUNDEDITOR_New (Class *cl, Object *o, struct opSet *ops);
static uint32 SOUNDEDITOR_Dispose (Class *cl, Object *o, Msg msg);
static uint32 SOUNDEDITOR_Get (Class *cl, Object *o, struct opGet *opg);
static uint32 SOUNDEDITOR_Set (Class *cl, Object *o, struct opSet *ops);
static uint32 SOUNDEDITOR_Domain (Class *cl, Object *o, struct gpDomain *gpd);
static uint32 SOUNDEDITOR_Layout (Class *cl, Object *o, struct gpLayout *gpl);
static uint32 SOUNDEDITOR_Render (Class *cl, Object *o, struct gpRender *gpr);
static uint32 SOUNDEDITOR_GoActive (Class *cl, Object *o, struct gpInput *gpi);
static uint32 SOUNDEDITOR_HandleInput (Class *cl, Object *o, struct gpInput *gpi);
static uint32 SOUNDEDITOR_GoInactive (Class *cl, Object *o, struct gpGoInactive *gpgi);
static uint32 SOUNDEDITOR_SampleArrayIO (Class *cl, Object *o, struct sndArrayIO *sndio);
static uint32 SOUNDEDITOR_DeleteSamples (Class *cl, Object *o, struct sndDeleteSamples *sndio);
static uint32 SOUNDEDITOR_Zoom (Class *cl, Object *o, struct sndZoom *sndzm);

static void RefreshDisplay (InstanceData *id, GlobalData *gd, struct RastPort *rp,
    struct GadgetInfo *ginfo, struct IBox *gbox, uint16 minx, uint16 maxx, uint8 chanmask,
    BOOL draw_wave);
static void DrawWaveform (InstanceData *id, GlobalData *gd, struct RastPort *rp,
    struct DrawInfo *dri, struct IBox *gbox, uint16 minx, uint16 maxx, int chan,
    uint32 left, uint32 top, uint32 width, uint32 height,
    BOOL highlight);

void InitDmgList (InstanceData *id);
void OptimiseDmgList (InstanceData *id);
void RepairDmg (InstanceData *id, GlobalData *gd, struct GadgetInfo *gi, struct IBox *gbox);
void RegisterDmg (InstanceData *id, uint16 x1, uint16 x2, uint8 chanmask, uint8 method);

enum {
    DMG_OR = 0,
    DMG_XOR
};

static uint32 ClassDispatch (Class *cl, Object *o, Msg msg) {
    uint32 ret;
    switch (msg->MethodID) {
        case OM_NEW:
            ret = DoSuperMethodA(cl, o, msg);
            if (ret) {
                if (!SOUNDEDITOR_New(cl, (Object *)ret, (struct opSet *)msg)) {
                    CoerceMethod(cl, (Object *)ret, OM_DISPOSE);
                    ret = 0;
                }
            }
            break;

        case OM_DISPOSE:
            ret = SOUNDEDITOR_Dispose(cl, o, msg);
            break;

        case OM_GET:
            ret = SOUNDEDITOR_Get(cl, o, (struct opGet *)msg);
            break;

        case OM_UPDATE:
        case OM_SET:
            ret = SOUNDEDITOR_Set(cl, o, (struct opSet *)msg);
            break;

        case GM_DOMAIN:
            ret = SOUNDEDITOR_Domain(cl, o, (struct gpDomain *)msg);
            break;

        case GM_EXTENT:
            ret = GMR_FULLBBOX;
            break;

        case GM_LAYOUT:
            ret = SOUNDEDITOR_Layout (cl, o, (struct gpLayout *)msg);
            break;

        case GM_RENDER:
            ret = SOUNDEDITOR_Render(cl, o, (struct gpRender *)msg);
            break;

        case GM_HITTEST:
            ret = GMR_GADGETHIT;
            break;

        case GM_GOACTIVE:
            ret = SOUNDEDITOR_GoActive(cl, o, (struct gpInput *)msg);
            break;

        case GM_HANDLEINPUT:
            ret = SOUNDEDITOR_HandleInput(cl, o, (struct gpInput *)msg);
            break;

        case GM_GOINACTIVE:
            ret = SOUNDEDITOR_GoInactive(cl, o, (struct gpGoInactive *)msg);
            break;

        case SNDM_WRITESAMPLEARRAY:
        case SNDM_READSAMPLEARRAY:
            ret = SOUNDEDITOR_SampleArrayIO(cl, o, (struct sndArrayIO *)msg);
            break;

        case SNDM_DELETESAMPLES:
            ret = SOUNDEDITOR_DeleteSamples(cl, o, (struct sndDeleteSamples *)msg);
            break;

        case SNDM_ZOOM:
            ret = SOUNDEDITOR_Zoom(cl, o, (struct sndZoom *)msg);
            break;

        default:
            ret = DoSuperMethodA(cl, o, msg);
            break;
    }
    return ret;
}

static void FreeSampleData (InstanceData *id, BOOL init) {
    struct Sample *sample;
    while (sample = (APTR)RemHead(id->Samples)) {
        FreeSample(sample);
    }
    if (init) {
        id->Length = 0;
        id->NumChannels = 1;
        id->ChanMask = (1 << id->NumChannels)-1;
        id->SamplesPerSec = 44100;
        id->SampleSize = 2;
        id->View.StartSample = id->View.EndSample = 0;
        id->Selection.StartSample = id->Selection.EndSample = ~0;
        id->PlayMarker.Sample = ~0;
    }
}

static void ObtainPens (InstanceData *id, GlobalData *gd) {
    struct ColorMap *cmap;
    uint16 *pens;
    uint8 *rgb;
    int i;
    ReleasePens(id);
    if (cmap = id->ColorMap) {
        pens = id->Pens;
        for (i = 0; i < NUM_PENS; i++) {
            rgb = (uint8 *)&gd->Colors[gd->Pen2Col[i]];
            *pens++ = ObtainBestPenA(cmap,
                rgb[1]*0x01010101, rgb[2]*0x01010101, rgb[3]*0x01010101, NULL);
        }
    }
}

static void ReleasePens (InstanceData *id) {
    struct ColorMap *cmap;
    int i;
    if (cmap = id->ColorMap) {
        for (i = 0; i < NUM_PENS; i++) {
            ReleasePen(cmap, (int16)id->Pens[i]);
            id->Pens[i] = ~0;
        }
    }
}

static void UpdatePropValues (InstanceData *id) {
    uint32 length = id->Length;
    uint32 top = id->View.StartSample;
    uint32 visible = id->View.EndSample - top + 1;
    if (length > MAX_HPROP_TOTAL) {
        uint32 factor;
        factor = length / MAX_HPROP_TOTAL;
        if (length % MAX_HPROP_TOTAL) factor++;
        id->HProp.Factor = factor;
        id->HProp.Total = length / factor;
        if (length % factor) id->HProp.Total++;
        id->HProp.Top = top / factor;
        id->HProp.Visible = visible / factor;
        if (visible % factor) id->HProp.Visible++;
        id->View.StartSample -= top % factor;
        id->View.EndSample += factor - (visible % factor);
        if (id->View.EndSample >= length) {
            id->View.EndSample = length - 1;
        }
    } else {
        id->HProp.Factor = 1;
        id->HProp.Total = length;
        id->HProp.Top = top;
        id->HProp.Visible = visible;
    }
}

static uint32 SOUNDEDITOR_New (Class *cl, Object *o, struct opSet *ops) {
    InstanceData *id = INST_DATA(cl, o);

    SetSuperAttrs(cl, o, GA_RelSpecial, TRUE, TAG_END);
    G(o)->Flags |= GFLG_EXTENDED;
    EXTG(o)->MoreFlags |= GMORE_SCROLLRASTER;

    SetMem(id->Pens, 0xff, NUM_PENS*2);

    id->Samples = AllocList();
    id->Lock = AllocSemaphore();

    if (id->Samples && id->Lock) {
        FreeSampleData(id, TRUE);
        UpdatePropValues(id);
        return TRUE;
    }
    return FALSE;
}

static uint32 SOUNDEDITOR_Dispose (Class *cl, Object *o, Msg msg) {
    InstanceData *id = INST_DATA(cl, o);

    ObtainSemaphore(id->Lock);

    FreeSampleData(id, FALSE);

    FreeSemaphore(id->Lock);
    FreeList(id->Samples);

    ReleasePens(id);

    return DoSuperMethodA(cl, o, msg);
}

static uint32 SOUNDEDITOR_Get (Class *cl, Object *o, struct opGet *opg) {
    InstanceData *id = INST_DATA(cl, o);
    uint32 *data, ret = TRUE;

    ObtainSemaphoreShared(id->Lock);

    data = (uint32 *)opg->opg_Storage;

    switch (opg->opg_AttrID) {
        case SOUND_Length:
            *data = id->Length;
            break;

        case SOUND_NumChannels:
            *data = id->NumChannels;
            break;

        case SOUND_SampleSize:
            *data = id->SampleSize;
            break;

        case SOUND_SamplesPerSec:
            *data = id->SamplesPerSec;
            break;

        case SOUND_Selection:
            data[0] = MIN(id->Selection.StartSample, id->Selection.EndSample);
            data[1] = MAX(id->Selection.StartSample, id->Selection.EndSample);
            break;

        case SOUND_ChanMask:
            *data = id->ChanMask;
            break;

        case PGA_Total:
        case SOUND_HPropTotal:
            *data = id->HProp.Total;
            break;

        case PGA_Top:
        case SOUND_HPropTop:
            *data = id->HProp.Top;
            break;

        case PGA_Visible:
        case SOUND_HPropVisible:
            *data = id->HProp.Visible;
            break;

        case SOUND_PlayMarker:
            *data = id->PlayMarker.Sample;
            break;

        case SOUND_HasSelection:
            *data = (id->Selection.EndSample != ~0) ?
                TRUE : FALSE;
            break;

        case SOUND_AreaSelected:
            *data = (id->Selection.EndSample != ~0 &&
                id->Selection.StartSample != id->Selection.EndSample) ?
                TRUE : FALSE;
            break;

        case SOUND_View:
            data[0] = id->View.StartSample;
            data[1] = id->View.EndSample;
            break;
            
        case SOUND_HasChanged:
            *data = (id->Flags & SNDFLG_HASCHANGED) ? TRUE : FALSE;
            break;
    }

    if (!ret) {
        ret = DoSuperMethodA(cl, o, (Msg)opg);
    }

    ReleaseSemaphore(id->Lock);

    return ret;
}

static uint32 SOUNDEDITOR_Set (Class *cl, Object *o, struct opSet *ops) {
    InstanceData *id = INST_DATA(cl, o);
    struct TagItem *ti, *tstate;
    uint32 data;
    uint32 ret, refresh;

    BOOL scroll = FALSE;
    uint32 oldstart, oldend;

    uint32 preserve = FALSE;
    uint32 length = ~0, numchan = ~0, samplesize = ~0;
    uint32 new_marker = ~0, old_marker = ~0;

    ObtainSemaphore(id->Lock);

    ret = refresh = DoSuperMethodA(cl, o, (Msg)ops);

    tstate = ops->ops_AttrList;
    while (ti = NextTagItem(&tstate)) {
        data = ti->ti_Data;
        ret++;
        switch (ti->ti_Tag) {
            case SOUND_PreserveData:
                preserve = data;
                break;

            case SOUND_Length:
                length = data;
                break;

            case SOUND_NumChannels:
                numchan = CLAMP(data, 1, 2);
                break;

            case SOUND_SampleSize:
                samplesize = CLAMP(data, 1, 4);
                break;

            case SOUND_SamplesPerSec:
                id->SamplesPerSec = data;
                break;

            case SOUND_Selection:
                if (((uint32 *)data)[0] >= id->Length ||
                    ((uint32 *)data)[1] >= id->Length)
                {
                    id->Selection.StartSample =
                    id->Selection.EndSample = ~0;
                    break;
                }
                id->Selection.StartSample = ((uint32 *)data)[0];
                id->Selection.EndSample = ((uint32 *)data)[1];
                break;

            case SOUND_ChanMask:
                id->ChanMask = data & ((1 << id->NumChannels)-1);
                break;

            case PGA_Top:
            case SOUND_HPropTop:
                id->HProp.Top = data;
                oldstart = id->View.StartSample;
                oldend = id->View.EndSample;
                id->View.StartSample = id->HProp.Top*id->HProp.Factor;
                id->View.EndSample = MIN(id->View.StartSample +
                    id->HProp.Visible*id->HProp.Factor, id->Length)-1;
                if (ops->MethodID == OM_UPDATE &&
                    (((struct opUpdate *)ops)->opu_Flags & OPUF_INTERIM))
                {
                    if (id->View.StartSample != oldstart) {
                        if ((oldstart > id->View.StartSample &&
                            oldstart <= id->View.EndSample) ||
                            (oldend >= id->View.StartSample &&
                            oldend < id->View.EndSample))
                        {
                            scroll = TRUE;
                        } else
                            refresh++;
                    }
                } else if ((id->Flags & SNDFLG_SCROLLED) || id->View.StartSample != oldstart) {
                    refresh++;
                    id->Flags &= ~SNDFLG_SCROLLED;
                }
                break;

            case SOUND_PlayMarker:
                old_marker = id->PlayMarker.Sample;
                new_marker = data;
                break;

            case SOUND_HasChanged:
                if (data) {
                    id->Flags |= SNDFLG_HASCHANGED;
                } else {
                    id->Flags &= ~SNDFLG_HASCHANGED;
                }
                break;

            default:
                ret--;
                break;
        }
    }

    if (length != ~0 || numchan != ~0 || samplesize != ~0) {
        id->Flags |= SNDFLG_HASCHANGED;
        preserve = FALSE; /* not yet supported */
        if (preserve) {
            /* not implemented */
        } else {
            if (length == ~0) length = id->Length;
            if (numchan == ~0) numchan = MAX(id->NumChannels, 1);
            if (samplesize == ~0) samplesize = id->SampleSize;

            if (!numchan) length = 0;

            FreeSampleData(id, FALSE);

            id->Length = 0;
            id->NumChannels = numchan;
            id->SampleSize = samplesize;
            id->ChanMask = (1 << id->NumChannels)-1;
            length = AddSampleData(id, 0, length, TRUE);
            id->View.StartSample = 0;
            id->View.EndSample = MAX(length, 1) - 1;
            id->Selection.StartSample = id->Selection.EndSample = ~0;
            id->PlayMarker.Sample = ~0;

            UpdatePropValues(id);
            DoNotifyAttrs(cl, o, ops->ops_GInfo, 0,
                SOUND_HPropTotal,   id->HProp.Total,
                SOUND_HPropVisible, id->HProp.Visible,
                SOUND_HPropTop,     id->HProp.Top,
                TAG_END);
        }
        refresh++;
    }

    if (ops->ops_GInfo) {
        if (refresh) {
            struct GadgetInfo *ginfo = ops->ops_GInfo;
            struct RastPort *rp;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);

                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, gbox.Width-1, 0xff, TRUE);

                ReleaseGIRPort(rp);
            }
        } else if (scroll) {
            struct GadgetInfo *ginfo = ops->ops_GInfo;
            struct RastPort *rp;
            id->Flags |= SNDFLG_SCROLLED;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;
                uint32 start, end, vlen, pxlen;
                uint32 cplen, uplen;
                struct Hook *backfill;

                start = id->View.StartSample;
                end = id->View.EndSample;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);
                vlen = end-start;
                pxlen = gbox.Width-1;

                id->View.StartSample = oldstart;
                id->View.EndSample = oldend;
                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, gbox.Width-1, 0xff, FALSE);
                id->View.StartSample = start;
                id->View.EndSample = end;

                backfill = InstallLayerHook(rp->Layer, LAYERS_NOBACKFILL);

                if (start < oldstart) {
                    uplen = MIN(SCALE(oldstart-start, pxlen, vlen)+1, gbox.Width);
                    cplen = MIN(SCALE(end-oldstart, pxlen, vlen)+1, gbox.Width);

                    ScrollRasterBF(rp, cplen-gbox.Width, 0, gbox.Left, gbox.Top,
                        gbox.Left+gbox.Width-1, gbox.Top+gbox.Height-1);

                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        0, gbox.Width-1, 0xff, FALSE);

                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        0, uplen-1, 0xff, TRUE);
                } else if (start > oldstart) {
                    uplen = MIN(SCALE(end-oldend, pxlen, vlen)+1, gbox.Width);
                    cplen = MIN(SCALE(oldend-start, pxlen, vlen)+1, gbox.Width);

                    ScrollRasterBF(rp, gbox.Width-cplen, 0, gbox.Left, gbox.Top,
                        gbox.Left+gbox.Width-1, gbox.Top+gbox.Height-1);

                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        0, gbox.Width-1, 0xff, FALSE);

                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        gbox.Width-uplen, gbox.Width-1, 0xff, TRUE);
                }

                InstallLayerHook(rp->Layer, backfill);

                if ((rp->Layer->Flags & LAYERSIMPLE) && (rp->Layer->Flags & LAYERREFRESH)) {
                    int32 succ;
                    succ = BeginUpdate(rp->Layer);
                    if (succ) {
                        RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                            0, gbox.Width-1, 0xff, TRUE);
                    }
                    EndUpdate(rp->Layer, succ);
                }

                ReleaseGIRPort(rp);
            }
        } else if (old_marker != new_marker) {
            struct GadgetInfo *ginfo = ops->ops_GInfo;
            struct RastPort *rp;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);

                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, gbox.Width-1, 0xff, FALSE);

                id->PlayMarker.Sample = new_marker;

                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, gbox.Width-1, 0xff, FALSE);

                ReleaseGIRPort(rp);
            }
        }
    }

    ReleaseSemaphore(id->Lock);

    return ret;
}

static uint32 SOUNDEDITOR_Domain (Class *cl, Object *o, struct gpDomain *gpd) {
    struct IBox *gbox = &gpd->gpd_Domain;
    GadgetBox(G(o), gpd->gpd_GInfo, GBD_GINFO, GBF_BOUNDS, gbox);
    switch (gpd->gpd_Which) {
        case GDOMAIN_MINIMUM:
            gbox->Width = MINIMUM_WIDTH;
            gbox->Height = MINIMUM_HEIGHT;
            break;

        case GDOMAIN_NOMINAL:
            gbox->Width = NOMINAL_WIDTH;
            gbox->Height = NOMINAL_HEIGHT;
            break;

        case GDOMAIN_MAXIMUM:
            gbox->Width = gbox->Height = ~0;
            break;
    }
    return TRUE;
}

static uint32 SOUNDEDITOR_Layout (Class *cl, Object *o, struct gpLayout *gpl) {
    InstanceData *id = INST_DATA(cl, o);
    struct GadgetInfo *gi = gpl->gpl_GInfo;

    if (gpl->gpl_Initial) {
        int32 depth = 0;

        id->Window = gi->gi_Window;
        id->Requester = gi->gi_Requester;
        id->Gadget = G(o);
        id->ColorMap = gi->gi_Screen->ViewPort.ColorMap;

        if (!GetScreenAttr(gi->gi_Screen, SA_Depth, &depth, sizeof(depth)) || depth <= 8) {
            id->Truecolor = FALSE;
            ObtainPens(id, GLOB_DATA(cl));
        } else {
            id->Truecolor = TRUE;
        }
    }

    return TRUE;
}

static uint32 SOUNDEDITOR_Render (Class *cl, Object *o, struct gpRender *gpr) {
    InstanceData *id = INST_DATA(cl, o);
    struct GadgetInfo *ginfo = gpr->gpr_GInfo;
    struct IBox gbox;

    ObtainSemaphoreShared(id->Lock);

    GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);

    RefreshDisplay(id, GLOB_DATA(cl), gpr->gpr_RPort, ginfo, &gbox, 0, gbox.Width-1, 0xff, TRUE);

    ReleaseSemaphore(id->Lock);

    return TRUE;
}

static uint32 SOUNDEDITOR_GoActive (Class *cl, Object *o, struct gpInput *gpi) {
    InstanceData *id = INST_DATA(cl, o);
    struct InputEvent *event;

    if (id->Length == 0) {
        return GMR_NOREUSE;
    }

    G(o)->Flags |= GFLG_SELECTED;
    id->Flags |= SNDFLG_ISACTIVE;

    for (event = gpi->gpi_IEvent; event; event = event->ie_NextEvent) {
        if (event->ie_Class == IECLASS_RAWMOUSE && event->ie_Code == IECODE_LBUTTON) {
            struct IBox gbox;
            uint32 endsample;
            uint32 endx;
            uint32 sample;

            GadgetBox(G(o), gpi->gpi_GInfo, GBD_GINFO, GBF_BOUNDS, &gbox);
            endsample = id->View.EndSample-id->View.StartSample;
            endx = gbox.Width-1;

            sample = id->View.StartSample + SCALE(CLAMP((int)gpi->gpi_Mouse.X,
                0, (int)endx), endsample, endx);
            if (sample >= id->Length) sample = id->Length-1;

            InitDmgList(id);
            if (id->Selection.StartSample <= id->View.EndSample &&
                id->Selection.EndSample >= id->View.StartSample)
            {
                uint32 vstart = id->View.StartSample;
                uint32 vend = id->View.EndSample;
                RegisterDmg(id,
                    SCALE(CLAMP(id->Selection.StartSample, vstart, vend)-vstart,
                    endx, endsample),
                    SCALE(CLAMP(id->Selection.EndSample, vstart, vend)-vstart,
                    endx, endsample),
                    id->ChanMask, DMG_OR);
            }

            if (event->ie_Qualifier & IEQUALIFIER_LSHIFT) {
                if (id->Selection.StartSample != ~0) {
                    id->Selection.StartSample = id->Selection.EndSample;
                    return SOUNDEDITOR_HandleInput(cl, o, gpi);
                }
            } else
            if (event->ie_Qualifier & IEQUALIFIER_RSHIFT) {
                if (id->Selection.StartSample != ~0) {
                    id->Selection.EndSample = id->Selection.StartSample;
                    return SOUNDEDITOR_HandleInput(cl, o, gpi);
                }
            } else {
                id->Selection.StartSample = id->Selection.EndSample = sample;
                return SOUNDEDITOR_HandleInput(cl, o, gpi);
            }
        }
    }

    G(o)->Flags &= ~GFLG_SELECTED;
    id->Flags &= ~SNDFLG_ISACTIVE;

    return GMR_NOREUSE;
}

static uint32 SOUNDEDITOR_HandleInput (Class *cl, Object *o, struct gpInput *gpi) {
    InstanceData *id = INST_DATA(cl, o);
    uint32 x;
    uint32 sample;
    uint32 ret = GMR_MEACTIVE;
    struct IBox gbox;
    uint32 endsample;
    uint32 endx;
    struct RastPort *rp;
    struct InputEvent *ie = gpi->gpi_IEvent;
    struct GadgetInfo *ginfo = gpi->gpi_GInfo;
    uint32 vstart = id->View.StartSample;
    uint32 vend = id->View.EndSample;
    uint32 new_chanmask = id->ChanMask;

    ObtainSemaphoreShared(id->Lock);

    GadgetBox(G(o), gpi->gpi_GInfo, GBD_GINFO, GBF_BOUNDS, &gbox);
    endsample = vend-vstart;
    endx = gbox.Width-1;

    x = gpi->gpi_Mouse.X;
    sample = vstart + SCALE(CLAMP((int)x, 0, (int)endx), endsample, endx);
    if (sample >= id->Length) sample = id->Length-1;

    if (id->NumChannels == 2) {
        uint32 y, starty, endy;
        y = gpi->gpi_Mouse.Y;
        starty = gbox.Height >> 2;
        endy = (gbox.Height * 3) >> 2;
        if (y <= starty) {
            new_chanmask = 1;
        } else
        if (y >= endy) {
            new_chanmask = 2;
        } else {
            new_chanmask = 3;
        }
        /*if (new_chanmask != id->ChanMask) {
            RegisterDmg(id, 0, endx, 0xff, DMG_OR);
            id->ChanMask = new_chanmask;
        }*/
    }

    /*if (sample != id->Selection.EndSample)*/ {
        uint32 start, end;

        if (id->Selection.EndSample >= id->Selection.StartSample) {
            start = id->Selection.StartSample;
            end = id->Selection.EndSample;
        } else {
            start = id->Selection.EndSample;
            end = id->Selection.StartSample;
        }
        if (start <= vend && end >= vstart) {
            start = MAX(start, vstart)-vstart;
            end = MIN(end, vend)-vstart;
            RegisterDmg(id,
                SCALE(start, endx, endsample),
                SCALE(end, endx, endsample),
                id->ChanMask, DMG_OR);
        }

        id->Selection.EndSample = sample;
        id->ChanMask = new_chanmask;

        if (id->Selection.EndSample >= id->Selection.StartSample) {
            start = id->Selection.StartSample;
            end = id->Selection.EndSample;
        } else {
            start = id->Selection.EndSample;
            end = id->Selection.StartSample;
        }
        if (start <= vend && end >= vstart) {
            start = MAX(start, vstart)-vstart;
            end = MIN(end, vend)-vstart;
            RegisterDmg(id,
                SCALE(start, endx, endsample),
                SCALE(end, endx, endsample),
                id->ChanMask, DMG_XOR);
        }
    }
    RepairDmg(id, GLOB_DATA(cl), ginfo, &gbox);

    if (ie->ie_Code == (IECODE_LBUTTON|IECODE_UP_PREFIX)) {
        *gpi->gpi_Termination = 0;
        ret = GMR_NOREUSE|GMR_VERIFY;
    }

    ReleaseSemaphore(id->Lock);

    return ret;
}

static uint32 SOUNDEDITOR_GoInactive (Class *cl, Object *o, struct gpGoInactive *gpgi) {
    InstanceData *id = INST_DATA(cl, o);

    ObtainSemaphoreShared(id->Lock);

    uint32 startsample = id->Selection.StartSample;
    uint32 endsample = id->Selection.EndSample;

    if (startsample > endsample) {
        id->Selection.StartSample = endsample;
        id->Selection.EndSample = startsample;
    }

    G(o)->Flags &= ~GFLG_SELECTED;
    if (id->Flags & SNDFLG_ISACTIVE) {
        id->Flags &= ~SNDFLG_ISACTIVE;
    }

    ReleaseSemaphore(id->Lock);

    return 0;
}

static uint32 SOUNDEDITOR_SampleArrayIO (Class *cl, Object *o, struct sndArrayIO *sndio) {
    InstanceData *id = INST_DATA(cl, o);
    int32 i;
    int32 length, offset, blklen, s;
    int32 sample_size = id->SampleSize;
    int32 src_sample_size = sndio->BytesPerSample;
    uint8 mask;
    int32 copy, skip, skip1, skip2;
    uint8 *base, *ptr;
    struct Sample *sample;
    uint32 ret = TRUE;
    uint8 *src, *dst;

    if (sndio->MethodID == SNDM_WRITESAMPLEARRAY) {
        ObtainSemaphore(id->Lock);
        offset = sndio->Offset;
        length = MIN(sndio->Length, id->Length-offset);
        switch (sndio->Mode) {
            case SNDWM_REPLACE:
                break;
            case SNDWM_INSERT:
                length = sndio->Length;
                if (length = AddSampleData(id, offset, length, TRUE)) {
                    if (offset < id->View.StartSample) {
                        id->View.StartSample += length;
                        id->View.EndSample += length;
                    }
                    UpdatePropValues(id);
                    id->Selection.StartSample =
                    id->Selection.EndSample = ~0;
                    DoNotifyAttrs(cl, o, sndio->GInfo, 0,
                        SOUND_HPropTotal,   id->HProp.Total,
                        SOUND_HPropVisible, id->HProp.Visible,
                        SOUND_HPropTop,     id->HProp.Top,
                        TAG_END);
                    break;
                }
                /* fall through */
            default:
                ReleaseSemaphore(id->Lock);
                return FALSE;
        }
    } else {
        ObtainSemaphoreShared(id->Lock);
        offset = sndio->Offset;
        length = MIN(sndio->Length, id->Length-offset);
        switch (sndio->Mode) {
            case SNDRM_DEFAULT:
                break;
            default:
                ReleaseSemaphore(id->Lock);
                return FALSE;
        }
    }

    base = sndio->ArrayPtr;
    sample = GetSampleData(id->Samples, &offset);
    if (sample) {
        skip = sample_size-src_sample_size;
        if (skip <= 0) {
            copy = sample_size;
            skip2 = 0;
            skip = -skip;
        } else {
            copy = src_sample_size;
            skip2 = skip;
            skip = 0;
        }
        skip1 = sndio->BytesPerFrame-copy;
    } else {
        ret = FALSE;
        goto out;
    }

    if (sndio->MethodID == SNDM_WRITESAMPLEARRAY) {
        id->Flags |= SNDFLG_HASCHANGED;
        while (length > 0) {
            blklen = MIN(sample->Length - offset, length);
            ptr = base;
            for (i = 0, mask = 1; i < id->NumChannels; i++, mask <<= 1) {
                if (sndio->ChanMask & mask) {
                    src = ptr;
                    dst = (uint8 *)sample->Data[i] + offset * sample_size;

                    for (s = 0; s < blklen; s++) {
                        memcpy(dst, src, copy);
                        if (skip2) memset(dst+copy, 0, skip2);
                        dst += copy+skip2;
                        src += copy+skip1;
                    }

                    ptr += src_sample_size;
                }
            }
            length -= blklen;
            if (length <= 0) break;
            base += blklen * sndio->BytesPerFrame;
            sample = (APTR)GetSucc((APTR)sample);
            offset = 0;
            if (!sample) {
                ret = FALSE;
                break;
            }
        }
    } else if (sndio->MethodID == SNDM_READSAMPLEARRAY) {
        while (length > 0) {
            blklen = MIN(sample->Length - offset, length);
            ptr = base;
            for (i = 0, mask = 1; i < id->NumChannels; i++, mask <<= 1) {
                if (sndio->ChanMask & mask) {
                    src = ptr;
                    dst = (uint8 *)sample->Data[i] + offset * sample_size;

                    for (s = 0; s < blklen; s++) {
                        memcpy(src, dst, copy);
                        if (skip) memset(src+copy, 0, skip);
                        dst += copy+skip2;
                        src += copy+skip1;
                    }

                    ptr += src_sample_size;
                }
            }
            length -= blklen;
            if (length <= 0) break;
            base += blklen * sndio->BytesPerFrame;
            sample = (APTR)GetSucc((APTR)sample);
            offset = 0;
            if (!sample) {
                ret = FALSE;
                break;
            }
        }
    } else {
        ret = FALSE;
    }

out:
    if (sndio->MethodID == SNDM_WRITESAMPLEARRAY && sndio->GInfo) {
        uint32 start, end, vstart, vend;
        start = sndio->Offset;
        end = sndio->Offset+sndio->Length-1;
        vstart = id->View.StartSample;
        vend = id->View.EndSample;
        if (start <= vend || end >= vstart) {
            struct GadgetInfo *ginfo = sndio->GInfo;
            struct RastPort *rp;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;
                uint32 pxlen;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);
                pxlen = gbox.Width-1;

                if (sndio->Mode == SNDWM_REPLACE) {
                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        SCALE(start, pxlen, vend),
                        SCALE(end, pxlen, vend),
                        sndio->ChanMask, TRUE);
                } else {
                    RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                        SCALE(start, pxlen, vend), pxlen,
                        sndio->ChanMask, TRUE);
                }

                ReleaseGIRPort(rp);
            }
        }
    }

    ReleaseSemaphore(id->Lock);

    return ret;
}

static uint32 SOUNDEDITOR_DeleteSamples (Class *cl, Object *o, struct sndDeleteSamples *sndio) {
    InstanceData *id = INST_DATA(cl, o);
    int32 length, offset;
    uint32 ret = TRUE;

    ObtainSemaphore(id->Lock);

    offset = sndio->Offset;
    length = MIN(sndio->Length, id->Length-offset);
    if (length = RemoveSampleData(id, offset, length)) {
        id->Flags |= SNDFLG_HASCHANGED;
        if (offset <= id->View.EndSample) {
            id->View.StartSample = MAX(id->View.StartSample, length)-length;
            id->View.EndSample = MAX(id->View.EndSample, length)-length;
        }
        if (id->View.EndSample >= id->Length) {
            id->View.EndSample = id->Length - 1;
        }
        UpdatePropValues(id);
        id->Selection.StartSample =
        id->Selection.EndSample = ~0;
        DoNotifyAttrs(cl, o, sndio->GInfo, 0,
            SOUND_HPropTotal,   id->HProp.Total,
            SOUND_HPropVisible, id->HProp.Visible,
            SOUND_HPropTop,     id->HProp.Top,
            TAG_END);
    } else {
        ReleaseSemaphore(id->Lock);
        return FALSE;
    }

    if (sndio->GInfo) {
        uint32 start, end, vstart, vend;
        start = sndio->Offset;
        end = sndio->Offset+sndio->Length-1;
        vstart = id->View.StartSample;
        vend = id->View.EndSample;
        if (start <= vend || end >= vstart) {
            struct GadgetInfo *ginfo = sndio->GInfo;
            struct RastPort *rp;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;
                uint32 pxlen;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);
                pxlen = gbox.Width-1;

                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, pxlen, (1 << id->NumChannels) - 1, TRUE);

                ReleaseGIRPort(rp);
            }
        }
    }

    ReleaseSemaphore(id->Lock);

    return ret;
}

static uint32 SOUNDEDITOR_Zoom (Class *cl, Object *o, struct sndZoom *sndzm) {
    InstanceData *id = INST_DATA(cl, o);
    uint32 fac = id->HProp.Factor;
    uint32 sndend = MAX(id->Length, 1)-1;
    uint32 start, end, len;
    uint32 res = TRUE;

    if (id->Length == 0) return FALSE;

    ObtainSemaphoreShared(id->Lock);

    switch (sndzm->ZoomType) {
        case SOUND_ZOOMOUTFULL:
            id->View.StartSample = 0;
            id->View.EndSample = sndend;
            UpdatePropValues(id);
            break;

        case SOUND_ZOOMSELECTION:
            if (id->Selection.StartSample != id->Selection.EndSample) {
                start = MIN(id->Selection.StartSample, id->Selection.EndSample);
                end = MAX(id->Selection.StartSample, id->Selection.EndSample);

                start -= start%fac;
                end += fac-((end+1)%fac);

                id->View.StartSample = start;
                id->View.EndSample = MIN(end, sndend);

                len = end-start+1;
                id->HProp.Top = start/fac;
                id->HProp.Visible = len/fac;
            } else
                res = FALSE;
            break;

        case SOUND_ZOOMIN2X:
            start = id->View.StartSample;
            end = id->View.EndSample;
            len = end-start+1;
            if (len != fac) {
                uint32 oldlen = len;
                len >>= 1;

                len += fac-(len%fac);
                start += (oldlen-len) >> 1;
                start -= start%fac;
                end = start+len-1;

                id->View.StartSample = start;
                id->View.EndSample = MIN(end, sndend);

                id->HProp.Top = start/fac;
                id->HProp.Visible = len/fac;
            } else
                res = FALSE;
            break;

        case SOUND_ZOOMOUT2X:
            start = id->View.StartSample;
            end = id->View.EndSample;
            len = end-start+1;
            if (len != id->Length) {
                uint32 oldlen = len, tmp;
                len <<= 1;

                len += fac-(len%fac);
                tmp = (len-oldlen) >> 1;
                start = (tmp < start) ? (start-tmp) : 0;
                start -= start%fac;
                end = start+len-1;

                id->View.StartSample = start;
                id->View.EndSample = MIN(end, sndend);

                id->HProp.Top = start/fac;
                id->HProp.Visible = len/fac;
            } else
                res = FALSE;
            break;
    }
    if (res) {
        if (sndzm->GInfo) {
            struct GadgetInfo *ginfo = sndzm->GInfo;
            struct RastPort *rp;
            rp = ObtainGIRPort(ginfo);
            if (rp) {
                struct IBox gbox;

                GadgetBox(G(o), ginfo, GBD_GINFO, GBF_BOUNDS, &gbox);

                RefreshDisplay(id, GLOB_DATA(cl), rp, ginfo, &gbox,
                    0, gbox.Width-1, 0xff, TRUE);

                ReleaseGIRPort(rp);
            }
        }
        DoNotifyAttrs(cl, o, sndzm->GInfo, 0,
            SOUND_HPropTotal,   id->HProp.Total,
            SOUND_HPropVisible, id->HProp.Visible,
            SOUND_HPropTop,     id->HProp.Top,
            TAG_END);
    }

    ReleaseSemaphore(id->Lock);

    return res;
}

static void RefreshDisplay (InstanceData *id, GlobalData *gd, struct RastPort *rp,
    struct GadgetInfo *ginfo, struct IBox *gbox, uint16 minx, uint16 maxx, uint8 chanmask,
    BOOL draw_wave)
{
    struct DrawInfo *dri = ginfo->gi_DrInfo;
    uint32 left, top, width, height, mod;
    uint32 numchan = id->NumChannels;
    int chan;
    uint8 mask;

    BOOL draw_sel = FALSE;
    BOOL draw_play = FALSE;
    uint32 selx1, selx2, playx;

    left = gbox->Left; top = gbox->Top;
    width = maxx-minx+1;
    height = gbox->Height / numchan;
    mod = gbox->Height % numchan;

    if (id->Selection.StartSample != ~0) {
        uint32 sstart, send;
        uint32 vstart, vend;
        if (id->Selection.EndSample >= id->Selection.StartSample) {
            sstart = id->Selection.StartSample;
            send = id->Selection.EndSample;
        } else {
            sstart = id->Selection.EndSample;
            send = id->Selection.StartSample;
        }
        vstart = id->View.StartSample;
        vend = id->View.EndSample;
        if (sstart <= vend && send >= vstart) {
            uint32 xmax = gbox->Width-1;
            uint32 smax = vend-vstart;
            selx1 = SCALE(MAX(sstart, vstart)-vstart, xmax, smax);
            selx2 = SCALE(MIN(send, vend)-vstart, xmax, smax);
            if (selx1 <= maxx && selx2 >= minx) {
                draw_sel = TRUE;
                if (selx1 < minx) selx1 = minx;
                if (selx2 > maxx) selx2 = maxx;
            }
        }
    }

    if (id->PlayMarker.Sample != ~0) {
        uint32 pos;
        uint32 vstart, vend;
        pos = id->PlayMarker.Sample;
        vstart = id->View.StartSample;
        vend = id->View.EndSample;
        if (pos <= vend && pos >= vstart) {
            uint32 xmax = gbox->Width-1;
            uint32 smax = vend-vstart;
            playx = SCALE(pos-vstart, xmax, smax);
            if (playx <= maxx && playx >= minx) {
                draw_play = TRUE;
            }
        }
    }

    for (chan = 0, mask = 1; chan < numchan; chan++, mask <<= 1) {
        if (chanmask & mask) {
            if (draw_wave) {
                if (draw_sel && (id->ChanMask & mask)) {
                    if (selx1 > minx) {
                        DrawWaveform(id, gd, rp, dri, gbox, minx, selx1, chan,
                            left+minx, top, selx1-minx+1, height, FALSE);
                    }
                    if (selx2 < maxx) {
                        DrawWaveform(id, gd, rp, dri, gbox, selx2, maxx, chan,
                            left+selx2, top, maxx-selx2+1, height, FALSE);
                    }
                    DrawWaveform(id, gd, rp, dri, gbox, selx1, selx2, chan,
                        left+selx1, top, selx2-selx1+1, height, TRUE);
                } else {
                    DrawWaveform(id, gd, rp, dri, gbox, minx, maxx, chan,
                        left+minx, top, width, height, FALSE);
                }
            }
            if (draw_play) {
                SetDrMd(rp, COMPLEMENT);
                Move(rp, left+playx, top);
                Draw(rp, left+playx, top+height-1);
                SetDrMd(rp, JAM1);
            }
        }
        top += height+1;
        if (mod == 0) height--;
        mod--;
    }
}

static void DrawWaveform (InstanceData *id, GlobalData *gd, struct RastPort *rp,
    struct DrawInfo *dri, struct IBox *gbox, uint16 minx, uint16 maxx, int chan,
    uint32 left, uint32 top, uint32 width, uint32 height,
    BOOL highlight)
{
    uint16 *pens = dri->dri_Pens;

    if (id->Truecolor)
        SetRPAttrs(rp, RPTAG_APenColor, gd->Colors[COL_LINES], TAG_END);
    else
        SetAPen(rp, (id->Pens[PEN_LINES] != (uint16)~0) ? id->Pens[PEN_LINES] :
            pens[gd->Pen2Dri[PEN_LINES]]);

    {
        uint32 h, m;
        h = height >> 1;
        m = height & 1;

        if (id->Truecolor) {
            uint32 grad = highlight ? 2 : 0;
            DrawGradient(rp, left, top, width, h+m, NULL, 0, &gd->GradSpec[grad], dri);
            DrawGradient(rp, left, top+h, width, h+m, NULL, 0, &gd->GradSpec[grad+1], dri);
            SetRPAttrs(rp, RPTAG_APenColor, gd->Colors[COL_LINES], TAG_END);
        } else {
            uint32 pen = highlight ? PEN_ALTBKG : PEN_BKG;
            SetAPen(rp, (id->Pens[pen] != (uint16)~0) ? id->Pens[pen] :
                pens[gd->Pen2Dri[pen]]);
            RectFill(rp, left, top, left+width-1, top+height-1);
            SetAPen(rp, (id->Pens[PEN_LINES] != (uint16)~0) ? id->Pens[PEN_LINES] :
                pens[gd->Pen2Dri[PEN_LINES]]);
        }
        Move(rp, left, top+h);
        Draw(rp, left+width-1, top+h);

        if (id->Truecolor) {
            uint32 col = highlight ? COL_ALTWAVEFORM : COL_WAVEFORM;
            SetRPAttrs(rp, RPTAG_APenColor, gd->Colors[col], TAG_END);
        } else {
            uint32 pen = highlight ? PEN_ALTWAVEFORM : PEN_WAVEFORM;
            SetAPen(rp, (id->Pens[pen] != (uint16)~0) ? id->Pens[pen] :
                pens[gd->Pen2Dri[pen]]);
        }

        if (id->Length && GetHead(id->Samples)) {
            uint32 start, end, vlen;
            int32 offs;
            uint32 pxlen, samsize, x;
            struct Sample *sample;
            int8 *src, *srcend;
            float64 samperpix;

            vlen = id->View.EndSample-id->View.StartSample;
            pxlen = gbox->Width-1;
            start = id->View.StartSample + SCALE(minx, vlen, pxlen);
            end = id->View.StartSample + SCALE(maxx, vlen, pxlen);
            end = MIN(end+1, id->Length)-start;

            samsize = id->SampleSize;
            offs = start;
            sample = GetSampleData(id->Samples, &offs);
            src = (int8 *)sample->Data[chan] + offs*samsize;
            srcend = (int8 *)sample->Data[chan] + sample->Length*samsize;

            samperpix = (vlen+1) / (pxlen+1);

            if (samperpix < 16.0) {
                uint32 y;
                if (samsize == 1) {
                    int8 val;
                    for (offs = 0; offs < end; offs++) {
                        val = *src++;
                        x = SCALE(offs, pxlen, vlen);
                        y = (val >= 0) ? (h-(val*h/127)) : (h-(val*(h+m)/128));
                        if (offs == 0)
                            Move(rp, left+x, top+y);
                        else
                            Draw(rp, left+x, top+y);
                        if (src == srcend) {
                            sample = (APTR)GetSucc((APTR)sample);
                            if (!sample) break;
                            src = (int8 *)sample->Data[chan];
                            srcend = (int8 *)sample->Data[chan] + sample->Length;
                        }
                    }
                } else {
                    int16 val;
                    for (offs = 0; offs < end; offs++) {
                        val = *(int16 *)src;
                        src += samsize;
                        x = SCALE(offs, pxlen, vlen);
                        y = (val >= 0) ? (h-(val*h/32767)) : (h-(val*(h+m)/32768));
                        if (offs == 0)
                            Move(rp, left+x, top+y);
                        else
                            Draw(rp, left+x, top+y);
                        if (src == srcend) {
                            sample = (APTR)GetSucc((APTR)sample);
                            if (!sample) break;
                            src = (int8 *)sample->Data[chan];
                            srcend = (int8 *)sample->Data[chan] + sample->Length*samsize;
                        }
                    }
                }
            } else {
                float64 flend = 0.0;
                uint32 thisend;
                uint32 offsinc = 1;
                uint32 srcinc = samsize;
                uint32 miny, maxy, mx;
                end = id->Length-start;
                x = 0; mx = maxx-minx;
                {
                    uint32 tmp = samperpix;
                    for (tmp >>= 7; tmp; tmp >>= 1) offsinc <<= 1;
                    srcinc = offsinc * samsize;
                }
                if (samsize == 1) {
                    int8 val;
                    int8 minval, maxval;
                    for (offs = 0; sample && offs < end && x <= mx; x++) {
                        flend += samperpix;
                        thisend = MIN((int)flend, end);

                        for (minval = 127, maxval = -128; offs < thisend;
                            offs+=offsinc)
                        {
                            val = *src;
                            src += srcinc;
                            if (val < minval) minval = val;
                            if (val > maxval) maxval = val;
                            while (src >= srcend) {
                                sample = (APTR)GetSucc((APTR)sample);
                                if (!sample) break;
                                src = (int8 *)sample->Data[chan] + (src - srcend);
                                srcend = (int8 *)sample->Data[chan] + sample->Length*samsize;
                            }
                        }

                        miny = (minval >= 0) ? (h-(minval*h/127)) : (h-(minval*(h+m)/128));
                        maxy = (maxval >= 0) ? (h-(maxval*h/127)) : (h-(maxval*(h+m)/128));

                        Move(rp, left+x, top+miny);
                        Draw(rp, left+x, top+maxy);
                    }
                } else {
                    int16 val;
                    int16 minval, maxval;
                    for (offs = 0; sample && offs < end && x <= mx; x++) {
                        flend += samperpix;
                        thisend = MIN((int)flend, end); 

                        for (minval = 32767, maxval = -32768; offs < thisend;
                            offs+=offsinc)
                        {
                            val = *(int16 *)src;
                            src += srcinc;
                            if (val < minval) minval = val;
                            if (val > maxval) maxval = val;
                            while (src >= srcend) {
                                sample = (APTR)GetSucc((APTR)sample);
                                if (!sample) break;
                                src = (int8 *)sample->Data[chan] + (src - srcend);
                                srcend = (int8 *)sample->Data[chan] + sample->Length*samsize;
                            }
                        }

                        miny = (minval >= 0) ? (h-(minval*h/32767)) : (h-(minval*(h+m)/32768));
                        maxy = (maxval >= 0) ? (h-(maxval*h/32767)) : (h-(maxval*(h+m)/32768));

                        Move(rp, left+x, top+miny);
                        Draw(rp, left+x, top+maxy);
                    }
                }
            }
        }
    }

    chan++;
    if (chan != id->NumChannels) {
        EraseRect(rp, left, top+height, left+width-1, top+height);
    }
}

void InitDmgList (InstanceData *id) {
    id->Flags &= ~SNDFLG_DMGOVERFLOW;
    id->FirstDamage = id->LastDamage = NULL;
}

void OptimiseDmgList (InstanceData *id) {
    /* NOP */
}

void RepairDmg (InstanceData *id, GlobalData *gd, struct GadgetInfo *gi, struct IBox *gbox) {
    struct RastPort *rp;
    rp = ObtainGIRPort(gi);
    if (rp) {
        if (id->Flags & SNDFLG_DMGOVERFLOW) {
            RefreshDisplay(id, gd, rp, gi, gbox, 0, gbox->Width - 1, (1 << id->NumChannels) - 1, TRUE);
        } else {
            struct Damage *dmg;
            for (dmg = id->FirstDamage; dmg; dmg = dmg->NextDamage) {
                RefreshDisplay(id, gd, rp, gi, gbox, dmg->StartX, dmg->EndX, 1 << dmg->Channel, TRUE);
            }
        }
        ReleaseGIRPort(rp);
    }
    InitDmgList(id);
}

void RegisterDmg (InstanceData *id, uint16 ix1, uint16 ix2, uint8 chanmask, uint8 method) {
    int chan;
    uint8 msk;
    struct Damage *dmg, *lastdmg;
    uint16 x1, x2;

    if (id->Flags & SNDFLG_DMGOVERFLOW) return;

    for (chan = 0, msk = 1; chan < MAX_CHANNELS; chan++, msk <<= 1) {
        if (chanmask & msk) {
            x1 = ix1; x2 = ix2;
            for (dmg = id->FirstDamage; dmg; dmg = dmg->NextDamage) {
                if (chan == dmg->Channel &&
                    dmg->StartX <= x2 && dmg->EndX >= x1)
                {
                    switch (method) {
                        case DMG_OR:
                            if (dmg->StartX > x1) dmg->StartX = x1;
                            if (dmg->EndX < x2) dmg->EndX = x2;
                            chanmask &= ~msk;
                            break;
                        case DMG_XOR:
                            {
                                uint16 ox1, ox2;
                                ox1 = dmg->StartX; ox2 = dmg->EndX;
                                if (ox1 <= x1)
                                    dmg->EndX = x1;
                                else {
                                    dmg->StartX = x1;
                                    dmg->EndX = ox1;
                                }
                                if (x2 >= ox2)
                                    x1 = ox2;
                                else {
                                    x1 = x2;
                                    x2 = ox2;
                                }
                            }
                            break;
                    }
                }
            }

            dmg = id->LastDamage;
            lastdmg = &id->DamageList[MAX_DAMAGE];
            
            if (!dmg) {
                id->FirstDamage = dmg = &id->DamageList[0];
            } else {
                dmg->NextDamage = &dmg[1]; dmg++;
            }

            if (dmg >= lastdmg) {
                id->Flags |= SNDFLG_DMGOVERFLOW;
                return;
            }

            dmg->NextDamage = NULL;
            dmg->StartX = x1; dmg->EndX = x2;
            dmg->Channel = chan;

            id->LastDamage = dmg;
        }
    }
}
