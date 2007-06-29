/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pfaeditui.h"
#include <ustring.h>
#include <math.h>
#include <utype.h>
#include <chardata.h>
#include "ttf.h"		/* For MAC_DELETED_GLYPH_NAME */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>
extern int lookup_hideunused;

typedef struct charinfo {
    CharView *cv;
    EncMap *map;
    SplineChar *sc;
    SplineChar *oldsc;		/* oldsc->charinfo will point to us. Used to keep track of that pointer */
    int enc;
    GWindow gw;
    int done, first, changed;
    struct lookup_subtable *old_sub;
    int r,c;
} CharInfo;

#define CI_Width	218
#define CI_Height	292

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Cancel	1005
#define CID_ComponentMsg	1006
#define CID_Components	1007
#define CID_Comment	1008
#define CID_Color	1009
#define CID_GClass	1010
#define CID_Tabs	1011

#define CID_TeX_Height	1012
#define CID_TeX_Depth	1013
#define CID_TeX_Sub	1014
#define CID_TeX_Super	1015

/* Offsets for repeated fields. add 100*index */
#define CID_List	1020
#define CID_New		1021
#define CID_Delete	1022
#define CID_Edit	1023
#define CID_Copy	1024
#define CID_Paste	1025

#define CID_PST		1111
#define CID_Tag		1112
#define CID_Contents	1113
#define CID_SelectResults	1114
#define CID_MergeResults	1115
#define CID_RestrictSelection	1116

#ifdef FONTFORGE_CONFIG_DEVICETABLES
#define SIM_DX		1
#define SIM_DY		3
#define SIM_DX_ADV	5
#define SIM_DY_ADV	7
#define PAIR_DX1	2
#define PAIR_DY1	4
#define PAIR_DX_ADV1	6
#define PAIR_DY_ADV1	8
#define PAIR_DX2	10
#define PAIR_DY2	12
#define PAIR_DX_ADV2	14
#define PAIR_DY_ADV2	16
#else
#define SIM_DX		1
#define SIM_DY		2
#define SIM_DX_ADV	3
#define SIM_DY_ADV	4
#define PAIR_DX1	2
#define PAIR_DY1	3
#define PAIR_DX_ADV1	4
#define PAIR_DY_ADV1	5
#define PAIR_DX2	6
#define PAIR_DY2	7
#define PAIR_DX_ADV2	8
#define PAIR_DY_ADV2	9
#endif

static GTextInfo glyphclasses[] = {
    { (unichar_t *) N_("Automatic"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("No Class"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Base Glyph"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Base Lig"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mark"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Component"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { NULL, NULL }
};

static GTextInfo std_colors[] = {
    { (unichar_t *) N_("Color|Default"), &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, false, true, false, false, false, false, true },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, false, true },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, false, true },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, false, true },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, false, true },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, false, true },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, false, true },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, false, true },
    { NULL, NULL }
};

static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };

static char *newstrings[] = { N_("New Positioning"), N_("New Pair Position"),
	N_("New Substitution Variant"),
	N_("New Alternate List"), N_("New Multiple List"), N_("New Ligature"), NULL };
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static unichar_t *CounterMaskLine(SplineChar *sc, HintMask *hm) {
    unichar_t *textmask = NULL;
    int j,k,len;
    StemInfo *h;
    char buffer[100];

    for ( j=0; j<2; ++j ) {
	len = 0;
	for ( h=sc->hstem, k=0; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "H<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	for ( h=sc->vstem; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "V<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	if ( textmask==NULL ) {
	    textmask = galloc((len+1)*sizeof(unichar_t));
	    *textmask = '\0';
	}
    }
    if ( len>1 && textmask[len-2]==',' )
	textmask[len-2] = '\0';
return( textmask );
}

#define CID_HintMask	2020
#define HI_Width	200
#define HI_Height	260

struct hi_data {
    int done, ok, empty;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    GWindow gw;
#endif
    HintMask *cur;
    SplineChar *sc;
};

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int HI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));
	int32 i, len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(hi->gw,CID_HintMask),&len);

	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected )
	break;

	memset(hi->cur,0,sizeof(HintMask));
	if ( i==len ) {
	    hi->empty = true;
	} else {
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    (*hi->cur)[i>>3] |= (0x80>>(i&7));
	}
	PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);

	hi->done = true;
	hi->ok = true;
    }
return( true );
}

static void HI_DoCancel(struct hi_data *hi) {
    hi->done = true;
    PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);
}

static int HI_HintSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));

	PI_ShowHints(hi->sc,g,true);
	/* Do I need to check for overlap here? */
    }
return( true );
}

static int HI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	HI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int hi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	HI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Counters");
return( true );
	}
return( false );
    }
return( true );
}

static void CI_AskCounters(CharInfo *ci,HintMask *old) {
    HintMask *cur = old != NULL ? old : chunkalloc(sizeof(HintMask));
    struct hi_data hi;
    GWindowAttrs wattrs;
    GGadgetCreateData hgcd[5], *varray[11], *harray[8], boxes[3];
    GTextInfo hlabel[5];
    GGadget *list = GWidgetGetControl(ci->gw,CID_List+600);
    int j,k;
    GRect pos;

    memset(&hi,0,sizeof(hi));
    hi.cur = cur;
    hi.sc = ci->sc;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = old==NULL?_("New Counter Mask"):_("Edit Counter Mask");
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,HI_Width));
	pos.height = GDrawPointsToPixels(NULL,HI_Height);
	hi.gw = GDrawCreateTopWindow(NULL,&pos,hi_e_h,&hi,&wattrs);


	memset(hgcd,0,sizeof(hgcd));
	memset(boxes,0,sizeof(boxes));
	memset(hlabel,0,sizeof(hlabel));

	j=k=0;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled;
	hlabel[j].text = (unichar_t *) _("Select hints between which counters are formed");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	hgcd[j++].creator = GLabelCreate;

	hgcd[j].gd.pos.x = 5; hgcd[j].gd.pos.y = 5;
	hgcd[j].gd.pos.width = HI_Width-10; hgcd[j].gd.pos.height = HI_Height-45;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
	hgcd[j].gd.cid = CID_HintMask;
	hgcd[j].gd.u.list = SCHintList(ci->sc,old);
	hgcd[j].gd.handle_controlevent = HI_HintSel;
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	hgcd[j++].creator = GListCreate;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	hlabel[j].text = (unichar_t *) _("_OK");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Ok;
	harray[0] = GCD_Glue; harray[1] = &hgcd[j]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;
	hgcd[j++].creator = GButtonCreate;

	hgcd[j].gd.pos.x = -20; hgcd[j].gd.pos.y = HI_Height-31;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	hlabel[j].text = (unichar_t *) _("_Cancel");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Cancel;
	harray[4] = GCD_Glue; harray[5] = &hgcd[j]; harray[6] = GCD_Glue; harray[7] = NULL;
	hgcd[j++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[k++] = &boxes[2]; varray[k++] = NULL; 
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	varray[k] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(hi.gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,1);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
	GTextInfoListFree(hgcd[0].gd.u.list);

	PI_ShowHints(hi.sc,hgcd[0].ret,true);

    GDrawSetVisible(hi.gw,true);
    while ( !hi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(hi.gw);

    if ( !hi.ok ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Cancelled */
    } else if ( old==NULL && hi.empty ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Didn't add anything new */
    } else if ( old==NULL ) {
	GListAddStr(list,CounterMaskLine(hi.sc,cur),cur);
return;
    } else if ( !hi.empty ) {
	GListReplaceStr(list,GGadgetGetFirstListSelectedItem(list),
		CounterMaskLine(hi.sc,cur),cur);
return;
    } else {
	GListDelSelected(list);
	chunkfree(cur,sizeof(HintMask));
    }
}
#endif 

static int UnicodeContainsCombiners(int uni) {
    const unichar_t *alt;

    if ( uni<0 || uni>=unicode4_size )
return( -1 );
    if ( iscombining(uni))
return( true );

    if ( !isdecompositionnormative(uni) || unicode_alternates[uni>>8]==NULL )
return( false );
    alt = unicode_alternates[uni>>8][uni&0xff];
    if ( alt==NULL )
return( false );
    while ( *alt ) {
	if ( UnicodeContainsCombiners(*alt))
return( true );
	++alt;
    }
return( false );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int CI_NewCounter(GGadget *g, GEvent *e) {
    CharInfo *ci;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_AskCounters(ci,NULL);
    }
return( true );
}

static int CI_EditCounter(GGadget *g, GEvent *e) {
    GTextInfo *ti;
    GGadget *list;
    CharInfo *ci;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+6*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	CI_AskCounters(ci,ti->userdata);
    }
return( true );
}

static int CI_DeleteCounter(GGadget *g, GEvent *e) {
    int32 len; int i,j, offset;
    GTextInfo **old, **new;
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	offset = GGadgetGetCid(g)-CID_Delete;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+offset);
	old = GGadgetGetList(list,&len);
	new = gcalloc(len+1,sizeof(GTextInfo *));
	for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	    new[j] = galloc(sizeof(GTextInfo));
	    *new[j] = *old[i];
	    new[j]->text = u_copy(new[j]->text);
	    ++j;
	}
	new[j] = gcalloc(1,sizeof(GTextInfo));
	if ( offset==600 ) {
	    for ( i=0; i<len; ++i ) if ( old[i]->selected )
		chunkfree(old[i]->userdata,sizeof(HintMask));
	}
	GGadgetSetList(list,new,false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete+offset),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit+offset),false);
    }
return( true );
}

static int CI_CounterSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	int offset = GGadgetGetCid(g)-CID_List;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Delete+offset),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Edit+offset),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Copy+offset),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int offset = GGadgetGetCid(g)-CID_List;
	e->u.control.subtype = et_buttonactivate;
	e->u.control.g = GWidgetGetControl(ci->gw,CID_Edit+offset);
	CI_EditCounter(e->u.control.g,e);
    }
return( true );
}
#endif

static int MultipleValues(char *name, int local) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif
    if ( gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this Unicode encoding\n(named %1$.40s, at local encoding %2$d).\nIs that what you want?"),name,local)==0 )
return( true );
#endif
return( false );
}

static int MultipleNames(void) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_Cancel"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif
    if ( gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this name,\ndo you want to swap names?"))==0 )
return( true );
#endif
return( false );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int ParseUValue(GWindow gw, int cid, int minusoneok, SplineFont *sf) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    unichar_t *end;
    int val;

    if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	val = u_strtoul(ret+2,&end,16);
    else if ( *ret=='#' )
	val = u_strtoul(ret+1,&end,16);
    else
	val = u_strtoul(ret,&end,16);
    if ( val==-1 && minusoneok )
return( -1 );
    if ( *end || val<0 || val>0x10ffff ) {
	Protest8( _("Unicode _Value:") );
return( -2 );
    }
return( val );
}

static void SetNameFromUnicode(GWindow gw,int cid,int val) {
    unichar_t *temp;
    char buf[100];
    CharInfo *ci = GDrawGetUserData(gw);

    temp = utf82u_copy(StdGlyphName(buf,val,ci->sc->parent->uni_interp,ci->sc->parent->for_new_glyphs));
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SCInsertPST(SplineChar *sc,PST *new) {
    new->next = sc->possub;
    sc->possub = new;
}
		
int SCSetMetaData(SplineChar *sc,char *name,int unienc,const char *comment) {
    SplineFont *sf = sc->parent;
    int i, mv=0;
    int isnotdef, samename=false;
    struct altuni *alt;

    for ( alt=sc->altuni; alt!=NULL && alt->unienc!=unienc; alt=alt->next );
    if ( (sc->unicodeenc == unienc || alt!=NULL ) && strcmp(name,sc->name)==0 ) {
	samename = true;	/* No change, it must be good */
    }
    if ( alt!=NULL || !samename ) {
	isnotdef = strcmp(name,".notdef")==0;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]!=sc ) {
	    if ( unienc!=-1 && sf->glyphs[i]->unicodeenc==unienc ) {
		if ( !mv && !MultipleValues(sf->glyphs[i]->name,i)) {
return( false );
		}
		mv = 1;
	    } else if ( !isnotdef && strcmp(name,sf->glyphs[i]->name)==0 ) {
		if ( !MultipleNames()) {
return( false );
		}
		free(sf->glyphs[i]->name);
		sf->glyphs[i]->namechanged = true;
		if ( strncmp(sc->name,"uni",3)==0 && sf->glyphs[i]->unicodeenc!=-1) {
		    char buffer[12];
		    if ( sf->glyphs[i]->unicodeenc<0x10000 )
			sprintf( buffer,"uni%04X", sf->glyphs[i]->unicodeenc);
		    else
			sprintf( buffer,"u%04X", sf->glyphs[i]->unicodeenc);
		    sf->glyphs[i]->name = copy(buffer);
		} else {
		    sf->glyphs[i]->name = sc->name;
		    sc->name = NULL;
		}
	    break;
	    }
	}
	if ( sc->unicodeenc!=unienc ) {
	    struct splinecharlist *scl;
	    int layer;
	    RefChar *ref;

	    for ( scl=sc->dependents; scl!=NULL; scl=scl->next ) {
		for ( layer=ly_fore; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			if ( ref->sc==sc )
			    ref->unicode_enc = unienc;
	    }
	}
    }
    if ( alt!=NULL )
	alt->unienc = sc->unicodeenc;
    sc->unicodeenc = unienc;
    if ( sc->name==NULL || strcmp(name,sc->name)!=0 ) {
	free(sc->name);
	sc->name = copy(name);
	sc->namechanged = true;
	GlyphHashFree(sf);
    }
    sf->changed = true;
    if ( unienc>=0xe000 && unienc<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( samename )
	/* Ok to name it itself */;
    else {
	FontView *fvs;
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    int enc = fvs->map->backmap[sc->orig_pos];
	    if ( enc!=-1 && ((fvs->map->enc->only_1byte && enc<256) ||
			(fvs->map->enc->has_2byte && enc<65535 ))) {
		fvs->map->enc = &custom;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		FVSetTitle(fvs);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	    }
	}
    }
    free(sc->comment); sc->comment = NULL;
    if ( comment!=NULL && *comment!='\0' )
	sc->comment = copy(comment);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCRefreshTitles(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}

static int CI_NameCheck(const unichar_t *name) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    int bad, questionable;
    extern int allow_utf8_glyphnames;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    if ( uc_strcmp(name,".notdef")==0 )		/* This name is a special case and doesn't follow conventions */
return( true );
    if ( u_strlen(name)>31 ) {
	gwwv_post_error(_("Bad Name"),_("Glyph names are limitted to 31 characters"));
return( false );
    } else if ( *name=='\0' ) {
	gwwv_post_error(_("Bad Name"),_("Bad Name"));
return( false );
    } else if ( isdigit(*name) || *name=='.' ) {
	gwwv_post_error(_("Bad Name"),_("A glyph name may not start with a digit nor a full stop (period)"));
return( false );
    }
    bad = questionable = false;
    while ( *name ) {
	if ( *name<=' ' || (!allow_utf8_glyphnames && *name>=0x7f) ||
		*name=='(' || *name=='[' || *name=='{' || *name=='<' ||
		*name==')' || *name==']' || *name=='}' || *name=='>' ||
		*name=='%' || *name=='/' )
	    bad=true;
	else if ( !isalnum(*name) && *name!='.' && *name!='_' )
	    questionable = true;
	++name;
    }
    if ( bad ) {
	gwwv_post_error(_("Bad Name"),_("A glyph name must be ASCII, without spaces and may not contain the characters \"([{<>}])/%%\", and should contain only alphanumerics, periods and underscores"));
return( false );
    } else if ( questionable ) {
	if ( gwwv_ask(_("Bad Name"),(const char **) buts,0,1,_("A glyph name should contain only alphanumerics, periods and underscores\nDo you want to use this name in spite of that?"))==1 )
return(false);
    }
#endif
return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void CI_ParseCounters(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);
    SplineChar *sc = ci->sc;

    free(sc->countermasks);

    sc->countermask_cnt = len;
    if ( len==0 )
	sc->countermasks = NULL;
    else {
	sc->countermasks = galloc(len*sizeof(HintMask));
	for ( i=0; i<len; ++i ) {
	    memcpy(sc->countermasks[i],ti[i]->userdata,sizeof(HintMask));
	    chunkfree(ti[i]->userdata,sizeof(HintMask));
	    ti[i]->userdata = NULL;
	}
    }
}
#endif

#ifdef FONTFORGE_CONFIG_DEVICETABLES
int DeviceTableOK(char *dvstr, int *_low, int *_high) {
    char *pt, *end;
    int low, high, pixel, cor;

    low = high = -1;
    if ( dvstr!=NULL ) {
	while ( *dvstr==' ' ) ++dvstr;
	for ( pt=dvstr; *pt; ) {
	    pixel = strtol(pt,&end,10);
	    if ( pixel<=0 || pt==end)
	break;
	    pt = end;
	    if ( *pt==':' ) ++pt;
	    cor = strtol(pt,&end,10);
	    if ( pt==end || cor<-128 || cor>127 )
	break;
	    pt = end;
	    while ( *pt==' ' ) ++pt;
	    if ( *pt==',' ) ++pt;
	    while ( *pt==' ' ) ++pt;
	    if ( low==-1 ) low = high = pixel;
	    else if ( pixel<low ) low = pixel;
	    else if ( pixel>high ) high = pixel;
	}
	if ( *pt != '\0' )
return( false );
    }
    *_low = low; *_high = high;
return( true );
}
    
DeviceTable *DeviceTableParse(DeviceTable *dv,char *dvstr) {
    char *pt, *end;
    int low, high, pixel, cor;

    DeviceTableOK(dvstr,&low,&high);
    if ( low==-1 ) {
	if ( dv!=NULL ) {
	    free(dv->corrections);
	    memset(dv,0,sizeof(*dv));
	}
return( dv );
    }
    if ( dv==NULL )
	dv = chunkalloc(sizeof(DeviceTable));
    else
	free(dv->corrections);
    dv->first_pixel_size = low;
    dv->last_pixel_size = high;
    dv->corrections = gcalloc(high-low+1,1);

    for ( pt=dvstr; *pt; ) {
	pixel = strtol(pt,&end,10);
	if ( pixel<=0 || pt==end)
    break;
	pt = end;
	if ( *pt==':' ) ++pt;
	cor = strtol(pt,&end,10);
	if ( pt==end || cor<-128 || cor>127 )
    break;
	pt = end;
	while ( *pt==' ' ) ++pt;
	if ( *pt==',' ) ++pt;
	while ( *pt==' ' ) ++pt;
	dv->corrections[pixel-low] = cor;
    }
return( dv );
}

void VRDevTabParse(struct vr *vr,struct matrix_data *md) {
    ValDevTab temp, *adjust;
    int any = false;

    if ( (adjust = vr->adjust)==NULL ) {
	adjust = &temp;
	memset(&temp,0,sizeof(temp));
    }
    any |= (DeviceTableParse(&adjust->xadjust,md[0].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->yadjust,md[2].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->xadv,md[4].u.md_str)!=NULL);
    any |= (DeviceTableParse(&adjust->yadv,md[6].u.md_str)!=NULL);
    if ( any && adjust==&temp ) {
	vr->adjust = chunkalloc(sizeof(ValDevTab));
	*vr->adjust = temp;
    } else if ( !any && vr->adjust!=NULL ) {
	ValDevFree(vr->adjust);
	vr->adjust = NULL;
    }
}

void DevTabToString(char **str,DeviceTable *adjust) {
    char *pt;
    int i;

    if ( adjust==NULL || adjust->corrections==NULL ) {
	*str = NULL;
return;
    }
    *str = pt = galloc(11*(adjust->last_pixel_size-adjust->first_pixel_size+1)+1);
    for ( i=adjust->first_pixel_size; i<=adjust->last_pixel_size; ++i ) {
	if ( adjust->corrections[i-adjust->first_pixel_size]!=0 )
	    sprintf( pt, "%d:%d, ", i, adjust->corrections[i-adjust->first_pixel_size]);
	pt += strlen(pt);
    }
    if ( pt>*str && pt[-2] == ',' )
	pt[-2] = '\0';
}
    
void ValDevTabToStrings(struct matrix_data *mds,int first_offset,ValDevTab *adjust) {
    if ( adjust==NULL )
return;
    DevTabToString(&mds[first_offset].u.md_str,&adjust->xadjust);
    DevTabToString(&mds[first_offset+2].u.md_str,&adjust->yadjust);
    DevTabToString(&mds[first_offset+4].u.md_str,&adjust->xadv);
    DevTabToString(&mds[first_offset+6].u.md_str,&adjust->yadv);
}
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
void KpMDParse(SplineFont *sf,SplineChar *sc,struct lookup_subtable *sub,
	struct matrix_data *possub,int rows,int cols,int i) {
    SplineChar *other;
    PST *pst;
    KernPair *kp;
    int isv, iskpable, offset, newv;
    char *dvstr;
    char *pt, *start;
    int ch;

    for ( start=possub[cols*i+1].u.md_str; ; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	other = SFGetChar(sc->parent,-1,start);
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == sub &&
		    strcmp(start,pst->u.pair.paired)==0 )
	break;
	}
	kp = NULL;
	if ( pst==NULL && other!=NULL ) {
	    for ( isv=0; isv<2; ++isv ) {
		for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
		    if ( kp->subtable==(void *) possub[cols*i+0].u.md_ival &&
			    kp->sc == other )
		break;
		if ( kp!=NULL )
	    break;
	    }
	}
	newv = false;
	if ( other==NULL )
	    iskpable = false;
	else if ( sub->vertical_kerning ) {
	    newv = true;
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DY1].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DY1+1].u.md_str;
#endif
	} else if ( sub->lookup->lookup_flags & pst_r2l ) {
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DX_ADV2].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DX_ADV2+1].u.md_str;
#endif
	} else {
	    iskpable = possub[cols*i+PAIR_DX1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY1].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV1].u.md_ival==0 &&
			possub[cols*i+PAIR_DX2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY2].u.md_ival==0 &&
			possub[cols*i+PAIR_DX_ADV2].u.md_ival==0 &&
			possub[cols*i+PAIR_DY_ADV2].u.md_ival==0;
	    offset = possub[cols*i+PAIR_DX_ADV1].u.md_ival;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    iskpable &= (possub[cols*i+PAIR_DX1+1].u.md_str==NULL || *possub[cols*i+PAIR_DX1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY1+1].u.md_str==0 || *possub[cols*i+PAIR_DY1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV1+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX2+1].u.md_str==0 || *possub[cols*i+PAIR_DX2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY2+1].u.md_str==0 || *possub[cols*i+PAIR_DY2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DX_ADV2+1].u.md_str==0 ) &&
			(possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 || *possub[cols*i+PAIR_DY_ADV2+1].u.md_str==0 );
	    dvstr = possub[cols*i+PAIR_DX_ADV1+1].u.md_str;
#endif
	}
	if ( iskpable ) {
	    if ( kp==NULL ) {
		/* If there's a pst, ignore it, it will not get ticked and will*/
		/*  be freed later */
		kp = chunkalloc(sizeof(KernPair));
		kp->subtable = sub;
		kp->sc = other;
		if ( newv ) {
		    kp->next = sc->vkerns;
		    sc->vkerns = kp;
		} else {
		    kp->next = sc->kerns;
		    sc->kerns = kp;
		}
	    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    DeviceTableFree(kp->adjust);
	    kp->adjust = DeviceTableParse(NULL,dvstr);
#endif
	    kp->off = offset;
	    kp->kcid = true;
	} else {
	    if ( pst == NULL ) {
		/* If there's a kp, ignore it, it will not get ticked and will*/
		/*  be freed later */
		pst = chunkalloc(sizeof(PST));
		pst->type = pst_pair;
		pst->subtable = sub;
		pst->next = sc->possub;
		sc->possub = pst;
		pst->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		pst->u.pair.paired = copy(start);
	    }
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    VRDevTabParse(&pst->u.pair.vr[0],&possub[cols*i+PAIR_DX1+1]);
	    VRDevTabParse(&pst->u.pair.vr[1],&possub[cols*i+PAIR_DX2]+1);
#endif
	    pst->u.pair.vr[0].xoff = possub[cols*i+PAIR_DX1].u.md_ival;
	    pst->u.pair.vr[0].yoff = possub[cols*i+PAIR_DY1].u.md_ival;
	    pst->u.pair.vr[0].h_adv_off = possub[cols*i+PAIR_DX_ADV1].u.md_ival;
	    pst->u.pair.vr[0].v_adv_off = possub[cols*i+PAIR_DY_ADV1].u.md_ival;
	    pst->u.pair.vr[1].xoff = possub[cols*i+PAIR_DX2].u.md_ival;
	    pst->u.pair.vr[1].yoff = possub[cols*i+PAIR_DY2].u.md_ival;
	    pst->u.pair.vr[1].h_adv_off = possub[cols*i+PAIR_DX_ADV2].u.md_ival;
	    pst->u.pair.vr[1].v_adv_off = possub[cols*i+PAIR_DY_ADV2].u.md_ival;
	    pst->ticked = true;
	}
	*pt = ch;
	start = pt;
    }
}

static int CI_ProcessPosSubs(CharInfo *ci) {
    /* Check for duplicate entries in kerning and ligatures. If we find any */
    /*  complain and return failure */
    /* Check for various other errors */
    /* Otherwise process */
    SplineChar *sc = ci->sc, *found;
    int i,j, rows, cols, isv, pstt, ch;
    char *pt;
    struct matrix_data *possub;
    char *buts[3];
    KernPair *kp, *kpprev, *kpnext;
    PST *pst, *pstprev, *pstnext;

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_ligature-1)*100) );
    for ( i=0; i<rows; ++i ) {
	for ( j=i+1; j<rows; ++j ) {
	    if ( possub[cols*i+0].u.md_ival == possub[cols*j+0].u.md_ival &&
		    strcmp(possub[cols*i+1].u.md_str,possub[cols*j+1].u.md_str)==0 ) {
		gwwv_post_error( _("Duplicate Ligature"),_("There are two ligature entries with the same components (%.80s) in the same lookup subtable (%.30s)"),
			possub[cols*j+1].u.md_str,
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	}
    }
    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    for ( i=0; i<rows; ++i ) {
	for ( j=i+1; j<rows; ++j ) {
	    if ( possub[cols*i+0].u.md_ival == possub[cols*j+0].u.md_ival &&
		    strcmp(possub[cols*i+1].u.md_str,possub[cols*j+1].u.md_str)==0 ) {
		gwwv_post_error( _("Duplicate Kern data"),_("There are two kerning entries for the same glyph (%.80s) in the same lookup subtable (%.30s)"),
			possub[cols*j+1].u.md_str,
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	}
    }

#ifdef FONTFORGE_CONFIG_DEVICETABLES
    /* Check for badly specified device tables */
    for ( pstt = pst_position; pstt<=pst_pair; ++pstt ) {
	int startc = pstt==pst_position ? SIM_DX+1 : PAIR_DX1+1;
	int low, high, c, r;
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100));
	for ( r=0; r<rows; ++r ) {
	    for ( c=startc; c<cols; c+=2 ) {
		if ( !DeviceTableOK(possub[r*cols+c].u.md_str,&low,&high) ) {
		    gwwv_post_error( _("Bad Device Table Adjustment"),_("A device table adjustment specified for %.80s is invalid"),
			    possub[cols*r+0].u.md_str );
return( true );
		}
	    }
	}
    }
#endif

    for ( pstt = pst_pair; pstt<=pst_ligature; ++pstt ) {
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100));
	for ( i=0; i<rows; ++i ) {
	    char *start = possub[cols*i+1].u.md_str;
	    while ( *start== ' ' ) ++start;
	    if ( *start=='\0' ) {
		gwwv_post_error( _("Missing glyph name"),_("You must specify a glyph name for subtable %s"),
			((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name );
return( false );
	    }
	    while ( *start ) {
		for ( pt=start; *pt!='\0' && *pt!=' '; ++pt );
		ch = *pt; *pt='\0';
		found = SFGetChar(sc->parent,-1,start);
		if ( found==NULL ) {
		    buts[0] = _("_Yes");
		    buts[1] = _("_Cancel");
		    buts[2] = NULL;
		    if ( gwwv_ask(_("Missing glyph"),(const char **) buts,0,1,_("In lookup subtable %.30s you refer to a glyph named %.80s, which is not in the font yet. Was this intentional?"),
			    ((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name,
			    start)==1 ) {
			*pt = ch;
return( false );
		    }
		} else if ( found==ci->sc && pstt!=pst_pair ) {
		    buts[0] = _("_Yes");
		    buts[1] = _("_Cancel");
		    buts[2] = NULL;
		    if ( gwwv_ask(_("Substitution generates itself"),(const char **) buts,0,1,_("In lookup subtable %.30s you replace a glyph with itself. Was this intentional?"),
			    ((struct lookup_subtable *) possub[cols*i+0].u.md_ival)->subtable_name)==1 ) {
			*pt = ch;
return( false );
		    }
		}
		*pt = ch;
		while ( *pt== ' ' ) ++pt;
		start = pt;
	    }
	}
    }

    /* If we get this far, then we didn't find any errors */
    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	pst->ticked = pst->type==pst_lcaret;
    for ( isv=0; isv<2; ++isv )
	for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    kp->kcid = 0;

    for ( pstt=pst_substitution; pstt<=pst_ligature; ++pstt ) {
	possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100), &rows );
	cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pstt-1)*100) );
	for ( i=0; i<rows; ++i ) {
	    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->subtable == (void *) possub[cols*i+0].u.md_ival &&
			!pst->ticked )
	    break;
	    }
	    if ( pst==NULL ) {
		pst = chunkalloc(sizeof(PST));
		pst->type = pstt;
		pst->subtable = (void *) possub[cols*i+0].u.md_ival;
		pst->next = sc->possub;
		sc->possub = pst;
	    } else
		free( pst->u.subs.variant );
	    pst->ticked = true;
	    pst->u.subs.variant = copy( possub[cols*i+1].u.md_str );
	    if ( pstt==pst_ligature )
		pst->u.lig.lig = sc;
	}
    }

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100));
    for ( i=0; i<rows; ++i ) {
	for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->subtable == (void *) possub[cols*i+0].u.md_ival &&
		    !pst->ticked )
	break;
	}
	if ( pst==NULL ) {
	    pst = chunkalloc(sizeof(PST));
	    pst->type = pst_position;
	    pst->subtable = (void *) possub[cols*i+0].u.md_ival;
	    pst->next = sc->possub;
	    sc->possub = pst;
	}
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	VRDevTabParse(&pst->u.pos,&possub[cols*i+SIM_DX+1]);
#endif
	pst->u.pos.xoff = possub[cols*i+SIM_DX].u.md_ival;
	pst->u.pos.yoff = possub[cols*i+SIM_DY].u.md_ival;
	pst->u.pos.h_adv_off = possub[cols*i+SIM_DX_ADV].u.md_ival;
	pst->u.pos.v_adv_off = possub[cols*i+SIM_DY_ADV].u.md_ival;
	pst->ticked = true;
    }

    possub = GMatrixEditGet(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100), &rows );
    cols = GMatrixEditGetColCnt(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    for ( i=0; i<rows; ++i ) {
	struct lookup_subtable *sub = ((struct lookup_subtable *) possub[cols*i+0].u.md_ival);
	KpMDParse(sc->parent,sc,sub,possub,rows,cols,i);
    }

    /* Now, free anything that did not get ticked */
    for ( pstprev=NULL, pst = sc->possub; pst!=NULL; pst=pstnext ) {
	pstnext = pst->next;
	if ( pst->ticked )
	    pstprev = pst;
	else {
	    if ( pstprev==NULL )
		sc->possub = pstnext;
	    else
		pstprev->next = pstnext;
	    pst->next = NULL;
	    PSTFree(pst);
	}
    }
    for ( isv=0; isv<2; ++isv ) {
	for ( kpprev=NULL, kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kpnext ) {
	    kpnext = kp->next;
	    if ( kp->kcid!=0 )
		kpprev = kp;
	    else {
		if ( kpprev!=NULL )
		    kpprev->next = kpnext;
		else if ( isv )
		    sc->vkerns = kpnext;
		else
		    sc->kerns = kpnext;
		kp->next = NULL;
		KernPairsFree(kp);
	    }
	}
    }
return( true );
}

static int gettex(GWindow gw,int cid,char *msg,int *err) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));

    if ( *ret=='\0' )
return( TEX_UNDEF );
return( GetInt8(gw,cid,msg,err));
}

static int _CI_OK(CharInfo *ci) {
    int val;
    int ret, refresh_fvdi=0;
    char *name, *comment;
    const unichar_t *nm;
    FontView *fvs;
    int err = false;
    int tex_height, tex_depth, tex_sub, tex_super;

    val = ParseUValue(ci->gw,CID_UValue,true,ci->sc->parent);
    if ( val==-2 )
return( false );
    tex_height = gettex(ci->gw,CID_TeX_Height,_("Height:"),&err);
    tex_depth  = gettex(ci->gw,CID_TeX_Depth ,_("Depth:") ,&err);
    tex_sub    = gettex(ci->gw,CID_TeX_Sub   ,_("Subscript Pos:"),&err);
    tex_super  = gettex(ci->gw,CID_TeX_Super ,_("Superscript Pos:"),&err);
    if ( err )
return( false );
    nm = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
    if ( !CI_NameCheck(nm) )
return( false );
    if ( !CI_ProcessPosSubs(ci))
return( false );
    name = u2utf8_copy( nm );
    if ( strcmp(name,ci->sc->name)!=0 || val!=ci->sc->unicodeenc )
	refresh_fvdi = 1;
    comment = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_Comment));
    SCPreserveState(ci->sc,2);
    ret = SCSetMetaData(ci->sc,name,val,comment);
    free(name); free(comment);
    if ( refresh_fvdi ) {
	for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next ) {
	    GDrawRequestExpose(fvs->gw,NULL,false);	/* Redraw info area just in case this char is selected */
	    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw character area in case this char is on screen */
	}
    }
    if ( ret ) {
	ci->sc->glyph_class = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_GClass));
	val = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color));
	if ( val!=-1 ) {
	    if ( ci->sc->color != (int) (intpt) (std_colors[val].userdata) ) {
		ci->sc->color = (intpt) (std_colors[val].userdata);
		for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
		    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw info area just in case this char is selected */
	    }
	}
	CI_ParseCounters(ci);
	ci->sc->tex_height = tex_height;
	ci->sc->tex_depth  = tex_depth;
	ci->sc->tex_sub_pos   = tex_sub;
	ci->sc->tex_super_pos = tex_super;
    }
    if ( ret )
	ci->sc->parent->changed = true;
return( ret );
}

static void CI_Finish(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}

static int CI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( _CI_OK(ci) )
	    CI_Finish(ci);
    }
return( true );
}

static char *LigDefaultStr(int uni, char *name, int alt_lig ) {
    const unichar_t *alt=NULL, *pt;
    char *components = NULL;
    int len;
    const char *uname;
    unichar_t hack[30], *upt;
    char buffer[80];

    /* If it's not (bmp) unicode we have no info on it */
    /*  Unless it looks like one of adobe's special ligature names */
    if ( uni==-1 || uni>=0x10000 )
	/* Nope */;
    else if ( isdecompositionnormative(uni) &&
		unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	if ( alt[1]=='\0' )
	    alt = NULL;		/* Single replacements aren't ligatures */
	else if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2]))) {
	    if ( alt_lig != -10 )	/* alt_lig = 10 => mac unicode decomp */
		alt = NULL;		/* Otherwise, don't treat accented letters as ligatures */
	} else if ( _UnicodeNameAnnot!=NULL &&
		(uname = _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name)!=NULL &&
		strstr(uname,"LIGATURE")==NULL &&
		strstr(uname,"VULGAR FRACTION")==NULL &&
		uni!=0x152 && uni!=0x153 &&	/* oe ligature should not be standard */
		uni!=0x132 && uni!=0x133 &&	/* nor ij */
		(uni<0xfb2a || uni>0xfb4f) &&	/* Allow hebrew precomposed chars */
		uni!=0x215f &&
		!((uni>=0x0958 && uni<=0x095f) || uni==0x929 || uni==0x931 || uni==0x934)) {
	    alt = NULL;
	} else if ( _UnicodeNameAnnot==NULL ) {
	    if ( (uni>=0xbc && uni<=0xbe ) ||		/* Latin1 fractions */
		    (uni>=0x2153 && uni<=0x215e ) ||	/* other fractions */
		    (uni>=0xfb00 && uni<=0xfb06 ) ||	/* latin ligatures */
		    (uni>=0xfb13 && uni<=0xfb17 ) ||	/* armenian ligatures */
		    uni==0xfb17 ||			/* hebrew ligature */
		    (uni>=0xfb2a && uni<=0xfb4f ) ||	/* hebrew precomposed chars */
		    (uni>=0xfbea && uni<=0xfdcf ) ||	/* arabic ligatures */
		    (uni>=0xfdf0 && uni<=0xfdfb ) ||	/* arabic ligatures */
		    (uni>=0xfef5 && uni<=0xfefc ))	/* arabic ligatures */
		;	/* These are good */
	    else
		alt = NULL;
	}
    }
    if ( alt==NULL ) {
	if ( name==NULL || alt_lig )
return( NULL );
	else
return( AdobeLigatureFormat(name));
    }

    if ( uni==0xfb03 && alt_lig==1 )
	components = copy("ff i");
    else if ( uni==0xfb04 && alt_lig==1 )
	components = copy("ff l");
    else if ( alt!=NULL ) {
	if ( alt[1]==0x2044 && (alt[2]==0 || alt[3]==0) && alt_lig==1 ) {
	    u_strcpy(hack,alt);
	    hack[1] = '/';
	    alt = hack;
	} else if ( alt_lig>0 )
return( NULL );

	if ( isarabisolated(uni) || isarabinitial(uni) || isarabmedial(uni) || isarabfinal(uni) ) {
	    /* If it is arabic, then convert from the unformed version to the formed */
	    if ( u_strlen(alt)<sizeof(hack)/sizeof(hack[0])-1 ) {
		u_strcpy(hack,alt);
		for ( upt=hack ; *upt ; ++upt ) {
		    /* Make everything medial */
		    if ( *upt>=0x600 && *upt<=0x6ff )
			*upt = ArabicForms[*upt-0x600].medial;
		}
		if ( isarabisolated(uni) || isarabfinal(uni) ) {
		    int len = upt-hack-1;
		    if ( alt[len]>=0x600 && alt[len]<=0x6ff )
			hack[len] = ArabicForms[alt[len]-0x600].final;
		}
		if ( isarabisolated(uni) || isarabinitial(uni) ) {
		    if ( alt[0]>=0x600 && alt[0]<=0x6ff )
			hack[0] = ArabicForms[alt[0]-0x600].initial;
		}
		alt = hack;
	    }
	}

	components=NULL;
	while ( 1 ) {
	    len = 0;
	    for ( pt=alt; *pt; ++pt ) {
		if ( components==NULL ) {
		    len += strlen(StdGlyphName(buffer,*pt,ui_none,(NameList *)-1))+1;
		} else {
		    const char *temp = StdGlyphName(buffer,*pt,ui_none,(NameList *)-1);
		    strcpy(components+len,temp);
		    len += strlen( temp );
		    components[len++] = ' ';
		}
	    }
	    if ( components!=NULL )
	break;
	    components = galloc(len+1);
	}
	components[len-1] = '\0';
    }
return( components );
}

char *AdobeLigatureFormat(char *name) {
    /* There are two formats for ligs: <glyph-name>_<glyph-name>{...} or */
    /*  uni<code><code>{...} (only works for BMP) */
    /* I'm not checking to see if all the components are valid */
    char *components, *pt, buffer[12];
    const char *next;
    int len = strlen(name), uni;

    if ( strncmp(name,"uni",3)==0 && (len-3)%4==0 && len>7 ) {
	pt = name+3;
	components = galloc(1); *components = '\0';
	while ( *pt ) {
	    if ( sscanf(pt,"%4x", (unsigned *) &uni )==0 ) {
		free(components); components = NULL;
	break;
	    }
	    next = StdGlyphName(buffer,uni,ui_none,(NameList *)-1);
	    components = grealloc(components,strlen(components) + strlen(next) + 2);
	    if ( *components!='\0' )
		strcat(components," ");
	    strcat(components,next);
	    pt += 4;
	}
	if ( components!=NULL )
return( components );
    }

    if ( strchr(name,'_')==NULL )
return( NULL );
    pt = components = copy(name);
    while ( (pt = strchr(pt,'_'))!=NULL )
	*pt = ' ';
return( components );
}

uint32 LigTagFromUnicode(int uni) {
    int tag = CHR('l','i','g','a');	/* standard */

    if (( uni>=0xbc && uni<=0xbe ) || (uni>=0x2153 && uni<=0x215f) )
	tag = CHR('f','r','a','c');	/* Fraction */
    /* hebrew precomposed characters */
    else if ( uni>=0xfb2a && uni<=0xfb4e )
	tag = CHR('c','c','m','p');
    else if ( uni==0xfb4f )
	tag = CHR('h','l','i','g');
    /* armenian */
    else if ( uni>=0xfb13 && uni<=0xfb17 )
	tag = CHR('l','i','g','a');
    /* devanagari ligatures */
    else if ( (uni>=0x0958 && uni<=0x095f) || uni==0x931 || uni==0x934 || uni==0x929 )
	tag = CHR('n','u','k','t');
    else switch ( uni ) {
      case 0xfb05:		/* long-s t */
	/* This should be 'liga' for long-s+t and 'hlig' for s+t */
	tag = CHR('l','i','g','a');
      break;
      case 0x00c6: case 0x00e6:		/* ae, AE */
      case 0x0152: case 0x0153:		/* oe, OE */
      case 0x0132: case 0x0133:		/* ij, IJ */
      case 0xfb06:			/* s t */
	tag = CHR('d','l','i','g');
      break;
      case 0xfefb: case 0xfefc:	/* Lam & Alef, required ligs */
	tag = CHR('r','l','i','g');
      break;
    }
#if 0
    if ( tag==CHR('l','i','g','a') && uni!=-1 && uni<0x10000 ) {
	const unichar_t *alt=NULL;
	if ( isdecompositionnormative(uni) &&
		    unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	    if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2])))
		tag = ((27<<16)|1);
	}
    }
#endif
return( tag );
}

SplineChar *SuffixCheck(SplineChar *sc,char *suffix) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[200];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL ) {
	sprintf( namebuf, "%.20s.%d.%.80s", sf->cidmaster->ordering, sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
	if ( alt==NULL ) {
	    sprintf( namebuf, "cid-%d.%.80s", sc->orig_pos, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
    if ( alt==NULL && sc->unicodeenc!=-1 ) {
	sprintf( namebuf, "uni%04X.%.80s", sc->unicodeenc, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "glyph%d.%.80s", sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "%.80s.%.80s", sc->name, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
return( alt );
}

static SplineChar *SuffixCheckCase(SplineChar *sc,char *suffix, int cvt2lc ) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[100];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL )
return( NULL );

    /* Small cap characters are sometimes named "a.sc" */
    /*  and sometimes "A.small" */
    /* So if I want a 'smcp' feature I must convert "a" to "A.small" */
    /* And if I want a 'c2sc' feature I must convert "A" to "a.sc" */
    if ( cvt2lc ) {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		isupper(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", tolower(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && isupper(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", tolower(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    } else {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		islower(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", toupper(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && islower(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", toupper(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
return( alt );
}

void SCLigCaretCheck(SplineChar *sc,int clean) {
    PST *pst, *carets=NULL, *prev_carets=NULL, *prev;
    int lig_comp_max=0, lc, i;
    char *pt;
    /* Check to see if this is a ligature character, and if so, does it have */
    /*  a ligature caret structure. If a lig but no lig caret structure then */
    /*  create a lig caret struct */

    for ( pst=sc->possub, prev=NULL; pst!=NULL; prev = pst, pst=pst->next ) {
	if ( pst->type == pst_lcaret ) {
	    if ( carets!=NULL )
		IError("Too many ligature caret structures" );
	    else {
		carets = pst;
		prev_carets = prev;
	    }
	} else if ( pst->type==pst_ligature ) {
	    for ( lc=0, pt=pst->u.lig.components; *pt; ++pt )
		if ( *pt==' ' ) ++lc;
	    if ( lc>lig_comp_max )
		lig_comp_max = lc;
	}
    }
    if ( lig_comp_max == 0 ) {
	if ( clean && carets!=NULL ) {
	    if ( prev_carets==NULL )
		sc->possub = carets->next;
	    else
		prev_carets->next = carets->next;
	    carets->next = NULL;
	    PSTFree(carets);
	}
return;
    }
    if ( carets==NULL ) {
	carets = chunkalloc(sizeof(PST));
	carets->type = pst_lcaret;
	carets->subtable = NULL;		/* Not relevant here */
	carets->next = sc->possub;
	sc->possub = carets;
    }
    if ( carets->u.lcaret.cnt>=lig_comp_max ) {
	carets->u.lcaret.cnt = lig_comp_max;
return;
    }
    if ( carets->u.lcaret.carets==NULL )
	carets->u.lcaret.carets = (int16 *) gcalloc(lig_comp_max,sizeof(int16));
    else {
	carets->u.lcaret.carets = (int16 *) grealloc(carets->u.lcaret.carets,lig_comp_max*sizeof(int16));
	for ( i=carets->u.lcaret.cnt; i<lig_comp_max; ++i )
	    carets->u.lcaret.carets[i] = 0;
    }
    carets->u.lcaret.cnt = lig_comp_max;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[40], *ctemp; unichar_t ubuf[2], *temp;
	ctemp = u2utf8_copy(ret);
	i = UniFromName(ctemp,ui_none,&custom);
	free(ctemp);
	if ( i==-1 ) {
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two (or more) 4-digit codes). */
	    /* But that names outside of BMP should be uXXXX or uXXXXX or uXXXXXX */
	    if ( ret[0]=='u' && ret[1]!='n' && u_strlen(ret)<=1+6 ) {
		unichar_t *end;
		i = u_strtol(ret+1,&end,16);
		if ( *end )
		    i = -1;
		else		/* Make sure it is properly capitalized */
		    SetNameFromUnicode(ci->gw,CID_UName,i);
	    }
	}

	sprintf(buf,"U+%04x", i);
	temp = uc_copy(i==-1?"-1":buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);

	ubuf[0] = i;
	if ( i==-1 || i>0xffff )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static int CI_SValue(GGadget *g, GEvent *e) {	/* Set From Value */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t ubuf[2];
	int val;

	val = ParseUValue(ci->gw,CID_UValue,false,ci->sc->parent);
	if ( val<0 )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

GTextInfo *TIFromName(const char *name) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = utf82u_copy(name);
    ti->fg = COLOR_DEFAULT;
    ti->bg = COLOR_DEFAULT;
return( ti );
}

static void CI_SetNameList(CharInfo *ci,int val) {
    GGadget *g = GWidgetGetControl(ci->gw,CID_UName);
    int cnt;

    if ( GGadgetGetUserData(g)==(void *) (intpt) val )
return;		/* Didn't change */
    {
	GTextInfo **list = NULL;
	char **names = AllGlyphNames(val,ci->sc->parent->for_new_glyphs,ci->sc);

	for ( cnt=0; names[cnt]!=NULL; ++cnt );
	list = galloc((cnt+1)*sizeof(GTextInfo*)); 
	for ( cnt=0; names[cnt]!=NULL; ++cnt ) {
	    list[cnt] = TIFromName(names[cnt]);
	    free(names[cnt]);
	}
	free(names);
	list[cnt] = TIFromName(NULL);
	GGadgetSetList(g,list,true);
    }
    GGadgetSetUserData(g,(void *) (intpt) val);
}

static int CI_UValChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UValue));
	unichar_t *end;
	int val;

	if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	    ret += 2;
	val = u_strtol(ret,&end,16);
	if ( *end=='\0' )
	    CI_SetNameList(ci,val);
    }
return( true );
}

static int CI_CharChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UChar));
	int val = *ret;
	unichar_t *temp, ubuf[2]; char buf[10];

	if ( ret[0]=='\0' )
return( true );
	else if ( ret[1]!='\0' ) {
	    ff_post_notice(_("Only a single character allowed"),_("Only a single character allowed"));
	    ubuf[0] = ret[0];
	    ubuf[1] = '\0';
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
return( true );
	}

	SetNameFromUnicode(ci->gw,CID_UName,val);
	CI_SetNameList(ci,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static int CI_CommentChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	/* Let's give things with comments a white color. This may not be a good idea */
	if ( ci->first && ci->sc->color==COLOR_DEFAULT &&
		0==GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color)) )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),1);
	ci->first = false;
    }
return( true );
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
struct devtab_dlg {
    int done;
    GWindow gw;
    GGadget *gme;
    DeviceTable devtab;
};

static struct col_init devtabci[2] = {
    { me_int, NULL, NULL, NULL, N_("Pixel Size") },
    { me_int, NULL, NULL, NULL, N_("Correction") },
    };

static void DevTabMatrixInit(struct matrixinit *mi,char *dvstr) {
    struct matrix_data *md;
    int k, p, cnt;
    DeviceTable devtab;

    memset(&devtab,0,sizeof(devtab));
    DeviceTableParse(&devtab,dvstr);
    cnt = 0;
    if ( devtab.corrections!=NULL ) {
	for ( k=devtab.first_pixel_size; k<=devtab.last_pixel_size; ++k )
	    if ( devtab.corrections[k-devtab.first_pixel_size]!=0 )
		++cnt;
    }

    memset(mi,0,sizeof(*mi));
    mi->col_cnt = 2;
    mi->col_init = devtabci;

    md = NULL;
    for ( k=0; k<2; ++k ) {
	cnt = 0;
	if ( devtab.corrections==NULL )
	    /* Do Nothing */;
	else for ( p=devtab.first_pixel_size; p<=devtab.last_pixel_size; ++p ) {
	    if ( devtab.corrections[p-devtab.first_pixel_size]!=0 ) {
		if ( k ) {
		    md[2*cnt+0].u.md_ival = p;
		    md[2*cnt+1].u.md_ival = devtab.corrections[p-devtab.first_pixel_size];
		}
		++cnt;
	    }
	}
	if ( md==NULL )
	    md = gcalloc(2*(cnt+10),sizeof(struct matrix_data));
    }
    mi->matrix_data = md;
    mi->initial_row_cnt = cnt;

    mi->initrow = NULL;
    mi->finishedit = NULL;
    mi->candelete = NULL;
    mi->popupmenu = NULL;
    mi->handle_key = NULL;
    mi->bigedittitle = NULL;

    free( devtab.corrections );
}

static int DevTabDlg_OK(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct devtab_dlg *dvd = GDrawGetUserData(GGadgetGetWindow(g));
	int rows, i, low, high;
	struct matrix_data *corrections = GMatrixEditGet(dvd->gme, &rows);

	low = high = -1;
	for ( i=0; i<rows; ++i ) {
	    if ( corrections[2*i+1].u.md_ival<-128 || corrections[2*i+1].u.md_ival>127 ) {
		gwwv_post_error(_("Bad correction"),_("The correction on line %d is too big.  It must be between -128 and 127"),
			i+1 );
return(true);
	    } else if ( corrections[2*i+0].u.md_ival<0 || corrections[2*i+0].u.md_ival>32767 ) {
		gwwv_post_error(_("Bad pixel size"),_("The pixel size on line %d is out of bounds."),
			i+1 );
return(true);
	    }
	    if ( corrections[2*i+1].u.md_ival!=0 ) {
		if ( low==-1 )
		    low = high = corrections[2*i+0].u.md_ival;
		else if ( corrections[2*i+0].u.md_ival<low )
		    low = corrections[2*i+0].u.md_ival;
		else if ( corrections[2*i+0].u.md_ival>high )
		    high = corrections[2*i+0].u.md_ival;
	    }
	}
	memset(&dvd->devtab,0,sizeof(DeviceTable));
	if ( low!=-1 ) {
	    dvd->devtab.first_pixel_size = low;
	    dvd->devtab.last_pixel_size = high;
	    dvd->devtab.corrections = gcalloc(high-low+1,1);
	    for ( i=0; i<rows; ++i ) {
		if ( corrections[2*i+1].u.md_ival!=0 ) {
		    dvd->devtab.corrections[ corrections[2*i+0].u.md_ival-low ] =
			    corrections[2*i+1].u.md_ival;
		}
	    }
	}
	dvd->done = 2;
    }
return( true );
}

static int DevTabDlg_Cancel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct devtab_dlg *dvd = GDrawGetUserData(GGadgetGetWindow(g));
	dvd->done = true;
    }
return( true );
}

static int devtabdlg_e_h(GWindow gw, GEvent *event) {

    if ( event->type==et_close ) {
	struct devtab_dlg *dvd = GDrawGetUserData(gw);
	dvd->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }

return( true );
}

static char *DevTab_Dlg(GGadget *g, int r, int c) {
    int rows, k, j;
    struct matrix_data *strings = GMatrixEditGet(g, &rows);
    char *dvstr = strings[2*r+c].u.md_str;
    struct devtab_dlg dvd;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[3];
    GGadgetCreateData *varray[6], *harray3[8];
    GTextInfo label[4];
    struct matrixinit mi;

    memset(&dvd,0,sizeof(dvd));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Device Table Adjustments...");
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    dvd.gw = gw = GDrawCreateTopWindow(NULL,&pos,devtabdlg_e_h,&dvd,&wattrs);

    DevTabMatrixInit(&mi,dvstr);

    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));
    memset(&label,0,sizeof(label));
    k=j=0;
    gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = gcd[1].gd.pos.y+14;
    gcd[k].gd.flags = gg_enabled | gg_visible | gg_utf8_popup;
    gcd[k].gd.u.matrix = &mi;
    gcd[k].gd.popup_msg = (unichar_t *) _(
	"At small pixel sizes (screen font sizes)\n"
	"the rounding errors that occur may be\n"
	"extremely ugly. A device table allows\n"
	"you to specify adjustments to the rounded\n"
	"Every pixel size my have its own adjustment.");
    gcd[k].creator = GMatrixEditCreate;
    varray[j++] = &gcd[k++]; varray[j++] = NULL;

    gcd[k].gd.pos.x = 30-3; 
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = DevTabDlg_OK;
    gcd[k++].creator = GButtonCreate;

    gcd[k].gd.pos.x = -30;
    gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.handle_controlevent = DevTabDlg_Cancel;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k++].creator = GButtonCreate;

    harray3[0] = harray3[2] = harray3[3] = harray3[4] = harray3[6] = GCD_Glue;
    harray3[7] = NULL;
    harray3[1] = &gcd[k-2]; harray3[5] = &gcd[k-1];

    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = harray3;
    boxes[0].creator = GHBoxCreate;
    varray[j++] = &boxes[0]; varray[j++] = NULL; varray[j] = NULL;
    
    boxes[1].gd.pos.x = boxes[1].gd.pos.y = 2;
    boxes[1].gd.flags = gg_enabled|gg_visible;
    boxes[1].gd.u.boxelements = varray;
    boxes[1].creator = GHVGroupCreate;

    GGadgetsCreate(gw,boxes+1);

    free( mi.matrix_data );

    dvd.gme = gcd[0].ret;
    GMatrixEditSetNewText(gcd[0].ret,S_("PixelSize|New"));
    GHVBoxSetExpandableRow(boxes[1].ret,1);
    GHVBoxSetExpandableCol(boxes[0].ret,gb_expandgluesame);

    GHVBoxFitWindow(boxes[1].ret);

    GDrawSetVisible(gw,true);
    while ( !dvd.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
    if ( dvd.done==2 ) {
	char *ret;
	DevTabToString(&ret,&dvd.devtab);
	free(dvd.devtab.corrections);
return( ret );
    } else
return( copy(dvstr));
}
#endif

static void finishedit(GGadget *g, int r, int c, int wasnew);
static void enable_enum(GGadget *g, GMenuItem *mi, int r, int c);

static struct col_init simplesubsci[2] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Name") }
    };
static struct col_init ligatureci[2] = {
    { me_enum , NULL, NULL, NULL, N_("Subtable") },	/* There can be multiple ligatures for a glyph */
    { me_string, NULL, NULL, NULL, N_("Source Glyph Names") }
    };
static struct col_init altsubsci[2] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Names") }
    };
static struct col_init multsubsci[2] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_string, NULL, NULL, NULL, N_("Replacement Glyph Names") }
    };
#ifdef FONTFORGE_CONFIG_DEVICETABLES
static struct col_init simpleposci[] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_int, NULL, NULL, NULL, NU_("∆x") },	/* delta-x */
/* GT: "Adjust" here means Device Table based pixel adjustments, an OpenType */
/* GT: concept which allows small corrections for small pixel sizes where */
/* GT: rounding errors (in kerning for example) may smush too glyphs together */
/* GT: or space them too far apart. Generally not a problem for big pixelsizes*/
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") }
    };
static struct col_init pairposci[] = {
    { me_enum , NULL, NULL, NULL, N_("Subtable") },	/* There can be multiple kern-pairs for a glyph */
    { me_string , DevTab_Dlg, NULL, NULL, N_("Second Glyph Name") },
    { me_int, NULL, NULL, NULL, NU_("∆x #1") },		/* delta-x */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y #1") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #1") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #1") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x #2") },		/* delta-x */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y #2") },		/* delta-y */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #2") },	/* delta-x-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") },
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #2") },	/* delta-y-adv */
    { me_funcedit, DevTab_Dlg, NULL, NULL, N_("Adjust") }
    };
#else
static struct col_init simpleposci[5] = {
    { me_enum , NULL, NULL, enable_enum, N_("Subtable") },
    { me_int, NULL, NULL, NULL, NU_("∆x") },	/* delta-x */
    { me_int, NULL, NULL, NULL, NU_("∆y") },	/* delta-y */
    { me_int, NULL, NULL, NULL, NU_("∆x_adv") },	/* delta-x-adv */
    { me_int, NULL, NULL, NULL, NU_("∆y_adv") }	/* delta-y-adv */
    };
static struct col_init pairposci[10] = {
    { me_enum , NULL, NULL, NULL, N_("Subtable") },	/* There can be multiple kern-pairs for a glyph */
    { me_string , NULL, NULL, NULL, N_("Second Glyph Name") },
    { me_int, NULL, NULL, NULL, NU_("∆x #1") },	/* delta-x */
    { me_int, NULL, NULL, NULL, NU_("∆y #1") },	/* delta-y */
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #1") },	/* delta-x-adv */
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #1") },	/* delta-y-adv */
    { me_int, NULL, NULL, NULL, NU_("∆x #2") },	/* delta-x */
    { me_int, NULL, NULL, NULL, NU_("∆y #2") },	/* delta-y */
    { me_int, NULL, NULL, NULL, NU_("∆x_adv #2") },	/* delta-x-adv */
    { me_int, NULL, NULL, NULL, NU_("∆y_adv #2") }	/* delta-y-adv */
    };
#endif
static int pst2lookuptype[] = { ot_undef, gpos_single, gpos_pair, gsub_single,
     gsub_alternate, gsub_multiple, gsub_ligature, 0 };
struct matrixinit mi[] = {
    { sizeof(simpleposci)/sizeof(struct col_init), simpleposci, 0, NULL, NULL, NULL, finishedit },
    { sizeof(pairposci)/sizeof(struct col_init), pairposci, 0, NULL, NULL, NULL, finishedit },
    { sizeof(simplesubsci)/sizeof(struct col_init), simplesubsci, 0, NULL, NULL, NULL, finishedit },
    { sizeof(altsubsci)/sizeof(struct col_init), altsubsci, 0, NULL, NULL, NULL, finishedit },
    { sizeof(multsubsci)/sizeof(struct col_init), multsubsci, 0, NULL, NULL, NULL, finishedit },
    { sizeof(ligatureci)/sizeof(struct col_init), ligatureci, 0, NULL, NULL, NULL, finishedit },
    { 0 }
    };

static void enable_enum(GGadget *g, GMenuItem *mi, int r, int c) {
    int i,rows,j;
    struct matrix_data *possub;
    CharInfo *ci;
    int sel,cols;

    if ( c!=0 )
return;
    ci = GDrawGetUserData(GGadgetGetWindow(g));
    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);

    ci->old_sub = (void *) possub[r*cols+0].u.md_ival;

    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.line; ++i ) {
	if ( mi[i].ti.line )	/* Lines, and the new entry always enabled */
	    mi[i].ti.disabled = false;
	else if ( mi[i].ti.userdata == NULL )
	    /* One of the lookup (rather than subtable) entries. leave disabled */;
	else if ( mi[i].ti.userdata == (void *) possub[r*cols+0].u.md_ival ) {
	    mi[i].ti.selected = true;		/* Current thing, they can keep on using it */
	    mi[i].ti.disabled = false;
	} else {
	    for ( j=0; j<rows; ++j )
		if ( mi[i].ti.userdata == (void *) possub[r*cols+0].u.md_ival ) {
		    mi[i].ti.selected = false;
		    mi[i].ti.disabled = true;
	    break;
		}
	    if ( j==rows ) {	/* This subtable hasn't been used yet */
		mi[i].ti.disabled = false;
	    }
	}
    }
}

void SCSubtableDefaultSubsCheck(SplineChar *sc, struct lookup_subtable *sub, struct matrix_data *possub, int col_cnt, int r) {
    FeatureScriptLangList *fl;
    int lookup_type = sub->lookup->lookup_type;
    SplineChar *alt;
    char buffer[8];
    DBounds bb;
    int i;
    static uint32 form_tags[] = { CHR('i','n','i','t'), CHR('m','e','d','i'), CHR('f','i','n','a'), CHR('i','s','o','l'), 0 };

    if ( lookup_type == gsub_single && sub->suffix != NULL ) {
	alt = SuffixCheck(sc,sub->suffix);
	if ( alt!=NULL ) {
	    possub[r*col_cnt+1].u.md_str = copy( alt->name );
return;
	}
    }

    for ( fl = sub->lookup->features; fl!=NULL; fl=fl->next ) {
	if ( lookup_type == gpos_single ) {
	    /* These too features are designed to crop off the left and right */
	    /*  side bearings respectively */
	    if ( fl->featuretag == CHR('l','f','b','d') ) {
		SplineCharFindBounds(sc,&bb);
		/* Adjust horixontal positioning and horizontal advance by */
		/*  the left side bearing */
		possub[r*col_cnt+SIM_DX].u.md_ival = -bb.minx;
		possub[r*col_cnt+SIM_DX_ADV].u.md_ival = -bb.minx;
return;
	    } else if ( fl->featuretag == CHR('r','t','b','d') ) {
		SplineCharFindBounds(sc,&bb);
		/* Adjust horizontal advance by right side bearing */
		possub[r*col_cnt+SIM_DX_ADV].u.md_ival = bb.maxx-sc->width;
return;
	    }
	} else if ( lookup_type == gsub_single ) {
	    alt = NULL;
	    if ( fl->featuretag == CHR('s','m','c','p') ) {
		alt = SuffixCheck(sc,"sc");
		if ( alt==NULL )
		    alt = SuffixCheckCase(sc,"small",false);
	    } else if ( fl->featuretag == CHR('c','2','s','c') ) {
		alt = SuffixCheck(sc,"small");
		if ( alt==NULL )
		    alt = SuffixCheckCase(sc,"sc",true);
	    } else if ( fl->featuretag == CHR('r','t','l','a') ) {
		if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && tomirror(sc->unicodeenc)!=0 )
		    alt = SFGetChar(sc->parent,tomirror(sc->unicodeenc),NULL);
	    } else if ( sc->unicodeenc==0x3c3 && fl->featuretag==CHR('f','i','n','a') ) {
		/* Greek final sigma */
		alt = SFGetChar(sc->parent,0x3c2,NULL);
	    }
	    if ( alt==NULL ) {
		buffer[0] = fl->featuretag>>24;
		buffer[1] = fl->featuretag>>16;
		buffer[2] = fl->featuretag>>8;
		buffer[3] = fl->featuretag&0xff;
		buffer[4] = 0;
		alt = SuffixCheck(sc,buffer);
	    }
	    if ( alt==NULL && sc->unicodeenc>=0x600 && sc->unicodeenc<0x700 ) {
		/* Arabic forms */
		for ( i=0; form_tags[i]!=0; ++i ) if ( form_tags[i]==fl->featuretag ) {
		    if ( (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=0 &&
			    (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=sc->unicodeenc &&
			    (alt = SFGetChar(sc->parent,(&(ArabicForms[sc->unicodeenc-0x600].initial))[i],NULL))!=NULL )
		break;
		}
	    }
	    if ( alt!=NULL ) {
		possub[r*col_cnt+1].u.md_str = copy( alt->name );
return;
	    }
	} else if ( lookup_type == gsub_ligature ) {
	    if ( fl->featuretag == LigTagFromUnicode(sc->unicodeenc) ) {
		int alt_index;
		for ( alt_index = 0; ; ++alt_index ) {
		    char *components = LigDefaultStr(sc->unicodeenc,sc->name,alt_index);
		    if ( components==NULL )
		break;
		    for ( i=0; i<r; ++i ) {
			if ( possub[i*col_cnt+0].u.md_ival == (intpt) sub &&
				strcmp(possub[i*col_cnt+1].u.md_str,components)==0 )
		    break;
		    }
		    if ( i==r ) {
			possub[r*col_cnt+1].u.md_str = components;
return;
		    }
		    free( components );
		}
	    }
	}
    }
}

static void finishedit(GGadget *g, int r, int c, int wasnew) {
    int rows;
    struct matrix_data *possub;
    CharInfo *ci;
    int sel,cols;
    struct lookup_subtable *sub;
    struct subtable_data sd;
    GTextInfo *ti;

    if ( c!=0 )
return;
    ci = GDrawGetUserData(GGadgetGetWindow(g));
    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    possub = GMatrixEditGet(g, &rows);
    cols = GMatrixEditGetColCnt(g);
    if ( possub[r*cols+0].u.md_ival!=0 ) {
	if ( wasnew )
	    SCSubtableDefaultSubsCheck(ci->sc,(struct lookup_subtable *) possub[r*cols+0].u.md_ival, possub, cols, r);
return;
    }
    /* They asked to create a new subtable */

    memset(&sd,0,sizeof(sd));
    sd.flags = sdf_dontedit;
    sub = SFNewLookupSubtableOfType(ci->sc->parent,pst2lookuptype[sel+1],&sd);
    if ( sub!=NULL ) {
	possub[r*cols+0].u.md_ival = (intpt) sub;
	ti = SFSubtableListOfType(ci->sc->parent, pst2lookuptype[sel+1], false);
	GMatrixEditSetColumnChoices(g,0,ti);
	GTextInfoListFree(ti);
	if ( wasnew )
	    SCSubtableDefaultSubsCheck(ci->sc,sub, possub, cols, r);
    } else if ( ci->old_sub!=NULL ) {
	/* Restore old value */
	possub[r*cols+0].u.md_ival = (intpt) ci->old_sub;
    } else {
	GMatrixEditDeleteRow(g,r);
    }
    ci->old_sub = NULL;
    GGadgetRedraw(g);
}

static void CI_DoHideUnusedSingle(CharInfo *ci) {
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    uint8 cols_used[20];
    int r, col, tot;

    if ( lookup_hideunused ) {
	memset(cols_used,0,sizeof(cols_used));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( r=0; r<rows; ++r ) {
	    for ( col=1; col<cols; col+=2 ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
		if ( old[cols*r+col+1].u.md_str!=NULL && *old[cols*r+col+1].u.md_str!='\0' )
		    cols_used[col+1] = true;
	    }
	}
#else
	for ( r=0; r<rows; ++r ) {
	    for ( col=1; col<cols; ++col ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
	    }
	}
#endif
	for ( col=1, tot=0; col<cols; ++col )
	    tot += cols_used[col];
	/* If no columns used (no info yet, all info is to preempt a kernclass and sets to 0) */
	/*  then show what we expect to be the default column for this kerning mode*/
	if ( tot==0 ) {
	    if ( strstr(ci->sc->name,".vert")!=NULL || strstr(ci->sc->name,".vrt2")!=NULL )
		cols_used[SIM_DY] = true;
	    else
		cols_used[SIM_DX] = true;
	}
	for ( col=1; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,cols_used[col]);
    } else {
	for ( col=1; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,true);
    }
    GWidgetToDesiredSize(ci->gw);

    GGadgetRedraw(pstk);
}

static void CI_DoHideUnusedPair(CharInfo *ci) {
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    uint8 cols_used[20];
    int r, col, tot;

    if ( lookup_hideunused ) {
	memset(cols_used,0,sizeof(cols_used));
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( r=0; r<rows; ++r ) {
	    for ( col=2; col<cols; col+=2 ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
		if ( old[cols*r+col+1].u.md_str!=NULL && *old[cols*r+col+1].u.md_str!='\0' )
		    cols_used[col+1] = true;
	    }
	}
#else
	for ( r=0; r<rows; ++r ) {
	    for ( col=2; col<cols; ++col ) {
		if ( old[cols*r+col].u.md_ival!=0 )
		    cols_used[col] = true;
	    }
	}
#endif
	for ( col=2, tot=0; col<cols; ++col )
	    tot += cols_used[col];
	/* If no columns used (no info yet, all info is to preempt a kernclass and sets to 0) */
	/*  then show what we expect to be the default column for this kerning mode*/
	if ( tot==0 ) {
	    if ( strstr(ci->sc->name,".vert")!=NULL || strstr(ci->sc->name,".vrt2")!=NULL )
		cols_used[PAIR_DY_ADV1] = true;
	    else if ( SCRightToLeft(ci->sc))
		cols_used[PAIR_DX_ADV2] = true;
	    else
		cols_used[PAIR_DX_ADV1] = true;
	}
	for ( col=2; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,cols_used[col]);
    } else {
	for ( col=2; col<cols; ++col )
	    GMatrixEditShowColumn(pstk,col,true);
    }
    GWidgetToDesiredSize(ci->gw);

    GGadgetRedraw(pstk);
}

static int CI_HideUnusedPair(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	lookup_hideunused = GGadgetIsChecked(g);
	CI_DoHideUnusedPair(ci);
	GGadgetRedraw(GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100));
    }
return( true );
}

static int CI_HideUnusedSingle(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	lookup_hideunused = GGadgetIsChecked(g);
	CI_DoHideUnusedSingle(ci);
	GGadgetRedraw(GWidgetGetControl(ci->gw,CID_List+(pst_position-1)*100));
    }
return( true );
}

static void CI_FreeKernedImage(const void *_ci, GImage *img) {
    GImageDestroy(img);
}

static const int kern_popup_size = 100;

static BDFChar *Rasterize(CharInfo *ci,SplineChar *sc) {
    void *freetypecontext=NULL;
    BDFChar *ret;

    freetypecontext = FreeTypeFontContext(sc->parent,sc,sc->parent->fv);
    if ( freetypecontext==NULL ) {
	ret = SplineCharFreeTypeRasterize(freetypecontext,sc->orig_pos,kern_popup_size,8);
	FreeTypeFreeContext(freetypecontext);
    } else
	ret = SplineCharAntiAlias(sc,kern_popup_size,4);
return( ret );
}

static GImage *CI_GetKernedImage(const void *_ci) {
    CharInfo *ci = (CharInfo *) _ci;
    GGadget *pstk = GWidgetGetControl(ci->gw,CID_List+(pst_pair-1)*100);
    int rows, cols = GMatrixEditGetColCnt(pstk);
    struct matrix_data *old = GMatrixEditGet(pstk,&rows);
    SplineChar *othersc = SFGetChar(ci->sc->parent,-1, old[cols*ci->r+1].u.md_str);
    BDFChar *me, *other;
    double scale = kern_popup_size/(double) (ci->sc->parent->ascent+ci->sc->parent->descent);
    int kern;
    int width, height, miny, maxy, minx, maxx;
    GImage *img;
    struct _GImage *base;
    Color fg, bg;
    int l,clut_scale;
    int x,y, xoffset, yoffset, coff1, coff2;
    struct lookup_subtable *sub = (struct lookup_subtable *) (old[cols*ci->r+0].u.md_ival);

    if ( othersc==NULL )
return( NULL );
    me = Rasterize(ci,ci->sc);
    other = Rasterize(ci,othersc);
    if ( sub->vertical_kerning ) {
	int vwidth = rint(ci->sc->vwidth*scale);
	kern = rint( old[cols*ci->r+PAIR_DY_ADV1].u.md_ival*scale );
	miny = me->ymin + rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	maxy = me->ymax + rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	if ( miny > vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymin )
	    miny = vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymin;
	if ( maxy < vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymax )
	    maxy = vwidth + kern + rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale) + other->ymax;
	height = maxy - miny + 2;
	minx = me->xmin + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale); maxx = me->xmax + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale);
	if ( minx>other->xmin + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ) minx = other->xmin+ rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ;
	if ( maxx<other->xmax + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ) maxx = other->xmax+ rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale) ;

	img = GImageCreate(it_index,maxx-minx+2,height);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	yoffset = 1 + maxy - vwidth - kern - rint(old[cols*ci->r+PAIR_DY1].u.md_ival*scale);
	xoffset = 1 - minx + rint(old[cols*ci->r+PAIR_DX1].u.md_ival*scale);
	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY2].u.md_ival*scale);
	xoffset = 1 - minx + rint(old[cols*ci->r+PAIR_DX2].u.md_ival*scale);
	for ( y=other->ymin; y<=other->ymax; ++y ) {
	    for ( x=other->xmin; x<=other->xmax; ++x ) {
		int n = other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)];
		if ( n>base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] )
		    base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] = n;
	    }
	}
    } else {
	coff1 = coff2 = 0;
	if ( sub->lookup->lookup_flags & pst_r2l ) {
	    BDFChar *temp = me;
	    me = other;
	    other = temp;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    coff1 = 8; coff2 = -8;
#else
	    coff1 = 4; coff2 = -4;
#endif
	}
	kern = rint( old[cols*ci->r+PAIR_DX_ADV1+coff1].u.md_ival*scale );
	minx = me->xmin + rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale);
	maxx = me->xmax + rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale);
	if ( minx > me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmin )
	    minx = me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmin;
	if ( maxx < me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmax )
	    maxx = me->width + kern + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale) + other->xmax;
	width = maxx - minx + 2;
	miny = me->ymin + rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale); maxy = me->ymax + rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale);
	if ( miny>other->ymin + rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ) miny = other->ymin+ rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ;
	if ( maxy<other->ymax + rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ) maxy = other->ymax+ rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale) ;

	img = GImageCreate(it_index,width,maxy-miny+2);
	base = img->u.image;
	memset(base->data,'\0',base->bytes_per_line*base->height);

	xoffset = rint(old[cols*ci->r+PAIR_DX1+coff1].u.md_ival*scale) + 1 - minx;
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY1+coff1].u.md_ival*scale);
	for ( y=me->ymin; y<=me->ymax; ++y ) {
	    for ( x=me->xmin; x<=me->xmax; ++x ) {
		base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] =
			me->bitmap[(me->ymax-y)*me->bytes_per_line + (x-me->xmin)];
	    }
	}
	xoffset = 1 - minx + me->width + kern - other->xmin + rint(old[cols*ci->r+PAIR_DX2+coff2].u.md_ival*scale);
	yoffset = 1 + maxy - rint(old[cols*ci->r+PAIR_DY2+coff2].u.md_ival*scale);
	for ( y=other->ymin; y<=other->ymax; ++y ) {
	    for ( x=other->xmin; x<=other->xmax; ++x ) {
		int n = other->bitmap[(other->ymax-y)*other->bytes_per_line + (x-other->xmin)];
		if ( n>base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] )
		    base->data[(yoffset-y)*base->bytes_per_line + (x+xoffset)] = n;
	    }
	}
    }
    memset(base->clut,'\0',sizeof(*base->clut));
    bg = GDrawGetDefaultBackground(NULL);
    fg = GDrawGetDefaultForeground(NULL);
    clut_scale = me->depth == 8 ? 8 : 4;
    base->clut->clut_len = 1<<clut_scale;
    for ( l=0; l<(1<<clut_scale); ++l )
	base->clut->clut[l] =
	    COLOR_CREATE(
	     COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<clut_scale)-1),
	     COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<clut_scale)-1),
	     COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<clut_scale)-1) );
return( img );
}

static void CI_KerningPopupPrepare(GGadget *g, int r, int c) {
    CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
    int rows, cols = GMatrixEditGetColCnt(g);
    struct matrix_data *old = GMatrixEditGet(g,&rows);
    if ( c<0 || c>=cols || r<0 || r>=rows || old[cols*r+1].u.md_str==NULL ||
	SFGetChar(ci->sc->parent,-1, old[cols*r+1].u.md_str)==NULL )
return;
    ci->r = r; ci->c = c;
    GGadgetPreparePopupImage(GGadgetGetWindow(g),NULL,ci,CI_GetKernedImage,CI_FreeKernedImage);
}

static void CIFillup(CharInfo *ci) {
    SplineChar *sc = ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[400];
    char buf[200];
    const unichar_t *bits;
    int i,j,gid, isv;
    struct matrix_data *mds[pst_max];
    int cnts[pst_max];
    PST *pst;
    KernPair *kp;
    unichar_t ubuf[4];
    GTextInfo **ti;

    sprintf(buf,_("Glyph Info for %.40s"),sc->name);
    GDrawSetWindowTitles8(ci->gw, buf, _("Glyph Info..."));

    if ( ci->oldsc!=NULL && ci->oldsc->charinfo==ci )
	ci->oldsc->charinfo = NULL;
    sc->charinfo = ci;
    ci->oldsc = sc;

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1), ci->enc>0 &&
	    ((gid=ci->map->map[ci->enc-1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1), ci->enc<ci->map->enccount-1 &&
	    ((gid=ci->map->map[ci->enc+1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));

    temp = utf82u_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);
    CI_SetNameList(ci,sc->unicodeenc);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = utf82u_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret )
	++cnts[pst->type];
    for ( isv=0; isv<2; ++isv ) {
	for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    ++cnts[pst_pair];
    }
    for ( i=pst_null+1; i<pst_max && i<pst_lcaret ; ++i )
	mds[i] = gcalloc((cnts[i]+1)*mi[i-1].col_cnt,sizeof(struct matrix_data));
    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) if ( pst->type!=pst_lcaret ) {
	j = (cnts[pst->type]++ * mi[pst->type-1].col_cnt);
	mds[pst->type][j+0].u.md_ival = (intpt) pst->subtable;
	if ( pst->type==pst_position ) {
	    mds[pst->type][j+SIM_DX].u.md_ival = pst->u.pos.xoff;
	    mds[pst->type][j+SIM_DY].u.md_ival = pst->u.pos.yoff;
	    mds[pst->type][j+SIM_DX_ADV].u.md_ival = pst->u.pos.h_adv_off;
	    mds[pst->type][j+SIM_DY_ADV].u.md_ival = pst->u.pos.v_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevTabToStrings(mds[pst_position],j+SIM_DX+1,pst->u.pos.adjust);
#endif
	} else if ( pst->type==pst_pair ) {
	    mds[pst->type][j+1].u.md_str = copy(pst->u.pair.paired);
	    mds[pst->type][j+PAIR_DX1].u.md_ival = pst->u.pair.vr[0].xoff;
	    mds[pst->type][j+PAIR_DY1].u.md_ival = pst->u.pair.vr[0].yoff;
	    mds[pst->type][j+PAIR_DX_ADV1].u.md_ival = pst->u.pair.vr[0].h_adv_off;
	    mds[pst->type][j+PAIR_DY_ADV1].u.md_ival = pst->u.pair.vr[0].v_adv_off;
	    mds[pst->type][j+PAIR_DX2].u.md_ival = pst->u.pair.vr[1].xoff;
	    mds[pst->type][j+PAIR_DY2].u.md_ival = pst->u.pair.vr[1].yoff;
	    mds[pst->type][j+PAIR_DX_ADV2].u.md_ival = pst->u.pair.vr[1].h_adv_off;
	    mds[pst->type][j+PAIR_DY_ADV2].u.md_ival = pst->u.pair.vr[1].v_adv_off;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevTabToStrings(mds[pst_pair],j+PAIR_DX1+1,pst->u.pair.vr[0].adjust);
	    ValDevTabToStrings(mds[pst_pair],j+PAIR_DX2+1,pst->u.pair.vr[1].adjust);
#endif
	} else {
	    mds[pst->type][j+1].u.md_str = copy(pst->u.subs.variant);
	}
    }
    for ( isv=0; isv<2; ++isv ) {
	for ( kp=isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next ) {
	    j = (cnts[pst_pair]++ * mi[pst_pair-1].col_cnt);
	    mds[pst_pair][j+0].u.md_ival = (intpt) kp->subtable;
	    mds[pst_pair][j+1].u.md_str = copy(kp->sc->name);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    if ( isv ) {
		mds[pst_pair][j+PAIR_DY_ADV1].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DY_ADV1+1].u.md_str,kp->adjust);
	    } else if ( kp->subtable->lookup->lookup_flags&pst_r2l ) {
		mds[pst_pair][j+PAIR_DX_ADV2].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DX_ADV2+1].u.md_str,kp->adjust);
	    } else {
		mds[pst_pair][j+PAIR_DX_ADV1].u.md_ival = kp->off;
		DevTabToString(&mds[pst_pair][j+PAIR_DX_ADV1+1].u.md_str,kp->adjust);
	    }
#else
	    if ( isv ) {
		mds[pst_pair][j+PAIR_DY_ADV1].u.md_ival = kp->off;
	    } else if ( kp->subtable->lookup->lookup_flags&pst_r2l ) {
		mds[pst_pair][j+PAIR_DX_ADV2].u.md_ival = kp->off;
	    } else {
		mds[pst_pair][j+PAIR_DX_ADV1].u.md_ival = kp->off;
	    }
#endif
	}
    }
    for ( i=pst_null+1; i<pst_lcaret /* == pst_max-1 */; ++i ) {
	GMatrixEditSet(GWidgetGetControl(ci->gw,CID_List+(i-1)*100),
		mds[i],cnts[i],false);
    }
    /* There's always a pane showing kerning data */
    CI_DoHideUnusedPair(ci);
    CI_DoHideUnusedSingle(ci);

    bits = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true);
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ComponentMsg),
	bits==NULL ? _("No components") :
	hascomposing(sc->parent,sc->unicodeenc,sc) ? _("Accented glyph composed of:") :
	    _("Glyph composed of:"));
    if ( bits==NULL ) {
	ubuf[0] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),ubuf);
    } else {
	unichar_t *temp = galloc(11*u_strlen(bits)*sizeof(unichar_t));
	unichar_t *upt=temp;
	while ( *bits!='\0' ) {
	    sprintf(buffer, "U+%04x ", *bits );
	    uc_strcpy(upt,buffer);
	    upt += u_strlen(upt);
	    ++bits;
	}
	upt[-1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),temp);
	free(temp);
    }

    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),0);

    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_Comment),
	    sc->comment?sc->comment:"");
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_GClass),sc->glyph_class);
    for ( i=0; std_colors[i].image!=NULL; ++i ) {
	if ( std_colors[i].userdata == (void *) (intpt) sc->color )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),i);
    }
    ci->first = sc->comment==NULL;

    ti = galloc((sc->countermask_cnt+1)*sizeof(GTextInfo *));
    ti[sc->countermask_cnt] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<sc->countermask_cnt; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = CounterMaskLine(sc,&sc->countermasks[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->userdata = chunkalloc(sizeof(HintMask));
	memcpy(ti[i]->userdata,sc->countermasks[i],sizeof(HintMask));
    }
    GGadgetSetList(GWidgetGetControl(ci->gw,CID_List+600),ti,false);

    if ( sc->tex_height!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_height);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Height),ubuf);

    if ( sc->tex_depth!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_depth);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Depth),ubuf);

    if ( sc->tex_sub_pos!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_sub_pos);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Sub),ubuf);

    if ( sc->tex_super_pos!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_super_pos);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Super),ubuf);
}

static int CI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = ci->enc + GGadgetGetCid(g);	/* cid is 1 for next, -1 for prev */
	SplineChar *new;

	if ( enc<0 || enc>=ci->map->enccount ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	if ( !_CI_OK(ci))
return( true );
	new = SFMakeChar(ci->sc->parent,ci->map,enc);
	if ( new->charinfo!=NULL && new->charinfo!=ci ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	ci->sc = new;
	ci->enc = enc;
	CIFillup(ci);
    }
return( true );
}

static void CI_DoCancel(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);

    for ( i=0; i<len; ++i )
	chunkfree(ti[i]->userdata,sizeof(HintMask));
    CI_Finish(ci);
}

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_DoCancel(ci);
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CharInfo *ci = GDrawGetUserData(gw);
	CI_DoCancel(ci);
    } else if ( event->type==et_char ) {
	CharInfo *ci = GDrawGetUserData(gw);
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html");
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		CI_DoCancel(ci);
	    else
		MenuExit(NULL,NULL,NULL);
	}
return( false );
    } else if ( event->type == et_destroy ) {
	CharInfo *ci = GDrawGetUserData(gw);
	ci->sc->charinfo = NULL;
	free(ci);
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void SCCharInfo(SplineChar *sc,EncMap *map,int enc) {
    CharInfo *ci;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData ugcd[12], cgcd[6], psgcd[7][7], cogcd[3], mgcd[9], tgcd[10];
    GTextInfo ulabel[12], clabel[6], pslabel[7][6], colabel[3], mlabel[9], tlabel[10];
    GGadgetCreateData mbox[4], *mvarray[7], *mharray1[7], *mharray2[8];
    GGadgetCreateData ubox[3], *uhvarray[19], *uharray[6];
    GGadgetCreateData cbox[3], *cvarray[5], *charray[4];
    GGadgetCreateData pstbox[7][4], *pstvarray[7][5], *pstharray1[7][8];
    GGadgetCreateData cobox[2], *covarray[4];
    GGadgetCreateData tbox[2], *thvarray[16];
    int i;
    GTabInfo aspects[13];
    static GBox smallbox = { bt_raised, bs_rect, 2, 1, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static int boxset=0;
    FontRequest rq;
    GFont *font;
    int is_math = sc->parent->texdata.type==tex_math || sc->parent->texdata.type==tex_mathext;

    CharInfoInit();

    if ( sc->charinfo!=NULL ) {
	GDrawSetVisible(sc->charinfo->gw,true);
	GDrawRaise(sc->charinfo->gw);
return;
    }

    ci = gcalloc(1,sizeof(CharInfo));
    ci->sc = sc;
    ci->done = false;
    ci->map = map;
    if ( enc==-1 )
	enc = map->backmap[sc->orig_pos];
    ci->enc = enc;

    if ( !boxset ) {
	extern GBox _ggadget_Default_Box;
	extern void GGadgetInit(void);
	GGadgetInit();
	smallbox = _ggadget_Default_Box;
	smallbox.padding = 1;
	boxset = 1;
    }

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = false;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Glyph Info...");
	wattrs.is_dlg = false;
	pos.x = pos.y = 0;
#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width));
#else
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width+65));
#endif
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	ci->gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,ci,&wattrs);

	memset(&ugcd,0,sizeof(ugcd));
	memset(&ubox,0,sizeof(ubox));
	memset(&ulabel,0,sizeof(ulabel));

	ulabel[0].text = (unichar_t *) _("U_nicode Name:");
	ulabel[0].text_is_1byte = true;
	ulabel[0].text_in_resource = true;
	ugcd[0].gd.label = &ulabel[0];
	ugcd[0].gd.pos.x = 5; ugcd[0].gd.pos.y = 5+4; 
	ugcd[0].gd.flags = gg_enabled|gg_visible;
	ugcd[0].gd.mnemonic = 'N';
	ugcd[0].creator = GLabelCreate;
	uhvarray[0] = &ugcd[0];

	ugcd[1].gd.pos.x = 85; ugcd[1].gd.pos.y = 5;
	ugcd[1].gd.flags = gg_enabled|gg_visible;
	ugcd[1].gd.mnemonic = 'N';
	ugcd[1].gd.cid = CID_UName;
	ugcd[1].creator = GListFieldCreate;
	ugcd[1].data = (void *) (-2);
	uhvarray[1] = &ugcd[1]; uhvarray[2] = NULL;

	ulabel[2].text = (unichar_t *) _("Unicode _Value:");
	ulabel[2].text_in_resource = true;
	ulabel[2].text_is_1byte = true;
	ugcd[2].gd.label = &ulabel[2];
	ugcd[2].gd.pos.x = 5; ugcd[2].gd.pos.y = 31+4; 
	ugcd[2].gd.flags = gg_enabled|gg_visible;
	ugcd[2].gd.mnemonic = 'V';
	ugcd[2].creator = GLabelCreate;
	uhvarray[3] = &ugcd[2];

	ugcd[3].gd.pos.x = 85; ugcd[3].gd.pos.y = 31;
	ugcd[3].gd.flags = gg_enabled|gg_visible;
	ugcd[3].gd.mnemonic = 'V';
	ugcd[3].gd.cid = CID_UValue;
	ugcd[3].gd.handle_controlevent = CI_UValChanged;
	ugcd[3].creator = GTextFieldCreate;
	uhvarray[4] = &ugcd[3]; uhvarray[5] = NULL;

	ulabel[4].text = (unichar_t *) _("Unicode C_har:");
	ulabel[4].text_in_resource = true;
	ulabel[4].text_is_1byte = true;
	ugcd[4].gd.label = &ulabel[4];
	ugcd[4].gd.pos.x = 5; ugcd[4].gd.pos.y = 57+4; 
	ugcd[4].gd.flags = gg_enabled|gg_visible;
	ugcd[4].gd.mnemonic = 'h';
	ugcd[4].creator = GLabelCreate;
	uhvarray[6] = &ugcd[4];

	ugcd[5].gd.pos.x = 85; ugcd[5].gd.pos.y = 57;
	ugcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	ugcd[5].gd.mnemonic = 'h';
	ugcd[5].gd.cid = CID_UChar;
	ugcd[5].gd.handle_controlevent = CI_CharChanged;
	ugcd[5].creator = GTextFieldCreate;
	uhvarray[7] = &ugcd[5]; uhvarray[8] = NULL;

	ugcd[6].gd.pos.x = 5; ugcd[6].gd.pos.y = 83+4;
	ugcd[6].gd.flags = gg_visible | gg_enabled;
	ulabel[6].text = (unichar_t *) _("OT _Glyph Class:");
	ulabel[6].text_is_1byte = true;
	ulabel[6].text_in_resource = true;
	ugcd[6].gd.label = &ulabel[6];
	ugcd[6].creator = GLabelCreate;
	uhvarray[9] = &ugcd[6];

	ugcd[7].gd.pos.x = 85; ugcd[7].gd.pos.y = 83;
	ugcd[7].gd.flags = gg_visible | gg_enabled;
	ugcd[7].gd.cid = CID_GClass;
	ugcd[7].gd.u.list = glyphclasses;
	ugcd[7].creator = GListButtonCreate;
	uhvarray[10] = &ugcd[7]; uhvarray[11] = NULL;

	ugcd[8].gd.pos.x = 12; ugcd[8].gd.pos.y = 117;
	ugcd[8].gd.flags = gg_visible | gg_enabled;
	ulabel[8].text = (unichar_t *) _("Set From N_ame");
	ulabel[8].text_is_1byte = true;
	ulabel[8].text_in_resource = true;
	ugcd[8].gd.mnemonic = 'a';
	ugcd[8].gd.label = &ulabel[8];
	ugcd[8].gd.handle_controlevent = CI_SName;
	ugcd[8].creator = GButtonCreate;
	uharray[0] = GCD_Glue; uharray[1] = &ugcd[8];

	ugcd[9].gd.pos.x = 107; ugcd[9].gd.pos.y = 117;
	ugcd[9].gd.flags = gg_visible | gg_enabled;
	ulabel[9].text = (unichar_t *) _("Set From Val_ue");
	ulabel[9].text_is_1byte = true;
	ulabel[9].text_in_resource = true;
	ugcd[9].gd.mnemonic = 'l';
	ugcd[9].gd.label = &ulabel[9];
	ugcd[9].gd.handle_controlevent = CI_SValue;
	ugcd[9].creator = GButtonCreate;
	uharray[2] = GCD_Glue; uharray[3] = &ugcd[9]; uharray[4] = GCD_Glue; uharray[5] = NULL;

	ubox[2].gd.flags = gg_enabled|gg_visible;
	ubox[2].gd.u.boxelements = uharray;
	ubox[2].creator = GHBoxCreate;
	uhvarray[12] = &ubox[2]; uhvarray[13] = GCD_ColSpan; uhvarray[14] = NULL;
	uhvarray[15] = GCD_Glue; uhvarray[16] = GCD_Glue; uhvarray[17] = NULL;
	uhvarray[18] = NULL;

	ubox[0].gd.flags = gg_enabled|gg_visible;
	ubox[0].gd.u.boxelements = uhvarray;
	ubox[0].creator = GHVBoxCreate;


	memset(&cgcd,0,sizeof(cgcd));
	memset(&cbox,0,sizeof(cbox));
	memset(&clabel,0,sizeof(clabel));

	clabel[0].text = (unichar_t *) _("Comment");
	clabel[0].text_is_1byte = true;
	cgcd[0].gd.label = &clabel[0];
	cgcd[0].gd.pos.x = 5; cgcd[0].gd.pos.y = 5; 
	cgcd[0].gd.flags = gg_enabled|gg_visible;
	cgcd[0].creator = GLabelCreate;
	cvarray[0] = &cgcd[0];

	cgcd[1].gd.pos.x = 5; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13;
	cgcd[1].gd.pos.height = 7*12+6;
	cgcd[1].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	cgcd[1].gd.cid = CID_Comment;
	cgcd[1].gd.handle_controlevent = CI_CommentChanged;
	cgcd[1].creator = GTextAreaCreate;
	cvarray[1] = &cgcd[1]; cvarray[2] = GCD_Glue;

	clabel[2].text = (unichar_t *) _("Color:");
	clabel[2].text_is_1byte = true;
	cgcd[2].gd.label = &clabel[2];
	cgcd[2].gd.pos.x = 5; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+cgcd[1].gd.pos.height+5+6; 
	cgcd[2].gd.flags = gg_enabled|gg_visible;
	cgcd[2].creator = GLabelCreate;
	charray[0] = &cgcd[2];

	cgcd[3].gd.pos.x = cgcd[3].gd.pos.x; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y-6;
	cgcd[3].gd.flags = gg_enabled|gg_visible;
	cgcd[3].gd.cid = CID_Color;
	cgcd[3].gd.u.list = std_colors;
	cgcd[3].creator = GListButtonCreate;
	charray[1] = &cgcd[3]; charray[2] = GCD_Glue; charray[3] = NULL;

	cbox[2].gd.flags = gg_enabled|gg_visible;
	cbox[2].gd.u.boxelements = charray;
	cbox[2].creator = GHBoxCreate;
	cvarray[3] = &cbox[2]; cvarray[4] = NULL;

	cbox[0].gd.flags = gg_enabled|gg_visible;
	cbox[0].gd.u.boxelements = cvarray;
	cbox[0].creator = GVBoxCreate;

	memset(&psgcd,0,sizeof(psgcd));
	memset(&pstbox,0,sizeof(pstbox));
	memset(&pslabel,0,sizeof(pslabel));

	for ( i=0; i<6; ++i ) {
	    psgcd[i][0].gd.pos.x = 5; psgcd[i][0].gd.pos.y = 5;
	    psgcd[i][0].gd.flags = gg_visible | gg_enabled;
	    psgcd[i][0].gd.cid = CID_List+i*100;
	    psgcd[i][0].gd.u.matrix = &mi[i];
	    mi[i].col_init[0].enum_vals = SFSubtableListOfType(sc->parent, pst2lookuptype[i+1], false);
	    psgcd[i][0].creator = GMatrixEditCreate;
	}
	for ( i=pst_position; i<=pst_pair; ++i ) {
	    pslabel[i-1][1].text = (unichar_t *) _("_Hide Unused Columns");
	    pslabel[i-1][1].text_is_1byte = true;
	    pslabel[i-1][1].text_in_resource = true;
	    psgcd[i-1][1].gd.label = &pslabel[i-1][1];
	    psgcd[i-1][1].gd.pos.x = 5; psgcd[i-1][1].gd.pos.y = 5+4; 
	    psgcd[i-1][1].gd.flags = lookup_hideunused ? (gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup) : (gg_enabled|gg_visible|gg_utf8_popup);
	    psgcd[i-1][1].gd.popup_msg = (unichar_t *) _("Don't display columns of 0s.\nThe OpenType lookup allows for up to 8 kinds\nof data, but almost all kerning lookups will use just one.\nOmitting the others makes the behavior clearer.");
	    psgcd[i-1][1].gd.handle_controlevent = i==pst_position ? CI_HideUnusedSingle : CI_HideUnusedPair;
	    psgcd[i-1][1].creator = GCheckBoxCreate;
	    pstvarray[i-1][0] = &psgcd[i-1][0];
	    pstvarray[i-1][1] = &psgcd[i-1][1];
	    pstvarray[i-1][2] = NULL;

	    pstbox[i-1][0].gd.flags = gg_enabled|gg_visible;
	    pstbox[i-1][0].gd.u.boxelements = pstvarray[i-1];
	    pstbox[i-1][0].creator = GVBoxCreate;
	}

	    psgcd[6][0].gd.pos.x = 5; psgcd[6][0].gd.pos.y = 5;
	    psgcd[6][0].gd.flags = gg_visible | gg_enabled;
	    psgcd[6][0].gd.cid = CID_List+6*100;
	    psgcd[6][0].gd.handle_controlevent = CI_CounterSelChanged;
	    psgcd[6][0].gd.box = &smallbox;
	    psgcd[6][0].creator = GListCreate;
	    pstvarray[6][0] = &psgcd[6][0];

	    psgcd[6][1].gd.pos.x = 10; psgcd[6][1].gd.pos.y = psgcd[6][0].gd.pos.y+psgcd[6][0].gd.pos.height+4;
	    psgcd[6][1].gd.flags = gg_visible | gg_enabled;
	    pslabel[6][1].text = (unichar_t *) S_("CounterHint|_New...");
	    pslabel[6][1].text_is_1byte = true;
	    pslabel[6][1].text_in_resource = true;
	    psgcd[6][1].gd.label = &pslabel[6][1];
	    psgcd[6][1].gd.cid = CID_New+i*100;
	    psgcd[6][1].gd.handle_controlevent = CI_NewCounter;
	    psgcd[6][1].gd.box = &smallbox;
	    psgcd[6][1].creator = GButtonCreate;
	    pstharray1[6][0] = GCD_Glue; pstharray1[6][1] = &psgcd[6][1];

	    psgcd[6][2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); psgcd[6][2].gd.pos.y = psgcd[6][1].gd.pos.y;
	    psgcd[6][2].gd.flags = gg_visible;
	    pslabel[6][2].text = (unichar_t *) _("_Delete");
	    pslabel[6][2].text_is_1byte = true;
	    pslabel[6][2].text_in_resource = true;
	    psgcd[6][2].gd.label = &pslabel[6][2];
	    psgcd[6][2].gd.cid = CID_Delete+i*100;
	    psgcd[6][2].gd.handle_controlevent = CI_DeleteCounter;
	    psgcd[6][2].gd.box = &smallbox;
	    psgcd[6][2].creator = GButtonCreate;
	    pstharray1[6][2] = GCD_Glue; pstharray1[6][3] = &psgcd[6][2];

	    psgcd[6][3].gd.pos.x = -10; psgcd[6][3].gd.pos.y = psgcd[6][1].gd.pos.y;
	    psgcd[6][3].gd.flags = gg_visible;
	    pslabel[6][3].text = (unichar_t *) _("_Edit...");
	    pslabel[6][3].text_is_1byte = true;
	    pslabel[6][3].text_in_resource = true;
	    psgcd[6][3].gd.label = &pslabel[6][3];
	    psgcd[6][3].gd.cid = CID_Edit+i*100;
	    psgcd[6][3].gd.handle_controlevent = CI_EditCounter;
	    psgcd[6][3].gd.box = &smallbox;
	    psgcd[6][3].creator = GButtonCreate;
	    pstharray1[6][4] = GCD_Glue; pstharray1[6][5] = &psgcd[6][3]; pstharray1[6][6] = GCD_Glue; pstharray1[6][7] = NULL;

	    pstbox[6][2].gd.flags = gg_enabled|gg_visible;
	    pstbox[6][2].gd.u.boxelements = pstharray1[6];
	    pstbox[6][2].creator = GHBoxCreate;
	    pstvarray[6][1] = &pstbox[6][2]; pstvarray[6][2] = NULL;

	    pstbox[6][0].gd.flags = gg_enabled|gg_visible;
	    pstbox[6][0].gd.u.boxelements = pstvarray[6];
	    pstbox[6][0].creator = GVBoxCreate;
	psgcd[6][4].gd.flags = psgcd[6][5].gd.flags = 0;	/* No copy, paste for hint masks */

	memset(&cogcd,0,sizeof(cogcd));
	memset(&cobox,0,sizeof(cobox));
	memset(&colabel,0,sizeof(colabel));

	colabel[0].text = (unichar_t *) _("Accented glyph composed of:");
	colabel[0].text_is_1byte = true;
	cogcd[0].gd.label = &colabel[0];
	cogcd[0].gd.pos.x = 5; cogcd[0].gd.pos.y = 5; 
	cogcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[0].gd.cid = CID_ComponentMsg;
	cogcd[0].creator = GLabelCreate;

	cogcd[1].gd.pos.x = 5; cogcd[1].gd.pos.y = cogcd[0].gd.pos.y+12;
	cogcd[1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[1].gd.cid = CID_Components;
	cogcd[1].creator = GLabelCreate;

	covarray[0] = &cogcd[0]; covarray[1] = &cogcd[1]; covarray[2] = GCD_Glue; covarray[3] = NULL;
	cobox[0].gd.flags = gg_enabled|gg_visible;
	cobox[0].gd.u.boxelements = covarray;
	cobox[0].creator = GVBoxCreate;
	


	memset(&tgcd,0,sizeof(tgcd));
	memset(&tbox,0,sizeof(tbox));
	memset(&tlabel,0,sizeof(tlabel));

	tlabel[0].text = (unichar_t *) _("Height:");
	tlabel[0].text_is_1byte = true;
	tgcd[0].gd.label = &tlabel[0];
	tgcd[0].gd.pos.x = 5; tgcd[0].gd.pos.y = 5+4; 
	tgcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[0].gd.popup_msg = (unichar_t *) _("These fields are the metrics fields used by TeX\nThe height and depth are pretty self-explanatory,\nexcept that they are corrected for optical distortion.\nSo 'x' and 'o' probably have the same height.\nSubscript and Superscript positions\nare only used in math fonts and should be left blank elsewhere");
	tgcd[0].creator = GLabelCreate;
	thvarray[0] = &tgcd[0];

	tgcd[1].gd.pos.x = 85; tgcd[1].gd.pos.y = 5;
	tgcd[1].gd.flags = gg_enabled|gg_visible;
	tgcd[1].gd.cid = CID_TeX_Height;
	tgcd[1].creator = GTextFieldCreate;
	thvarray[1] = &tgcd[1]; thvarray[2] = NULL;

	tlabel[2].text = (unichar_t *) _("Depth:");
	tlabel[2].text_is_1byte = true;
	tgcd[2].gd.label = &tlabel[2];
	tgcd[2].gd.pos.x = 5; tgcd[2].gd.pos.y = 31+4; 
	tgcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[2].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[2].creator = GLabelCreate;
	thvarray[3] = &tgcd[2];

	tgcd[3].gd.pos.x = 85; tgcd[3].gd.pos.y = 31;
	tgcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[3].gd.cid = CID_TeX_Depth;
	tgcd[3].creator = GTextFieldCreate;
	thvarray[4] = &tgcd[3]; thvarray[5] = NULL;

	tlabel[4].text = (unichar_t *) _("Subscript Position:");
	tlabel[4].text_is_1byte = true;
	tgcd[4].gd.label = &tlabel[4];
	tgcd[4].gd.pos.x = 5; tgcd[4].gd.pos.y = 57+4; 
	tgcd[4].gd.flags = is_math ? (gg_enabled|gg_visible|gg_utf8_popup) :
		(gg_visible|gg_utf8_popup);
	tgcd[4].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[4].creator = GLabelCreate;
	thvarray[6] = &tgcd[4];

	tgcd[5].gd.pos.x = 85; tgcd[5].gd.pos.y = 57; tgcd[5].gd.pos.width = 50;
	tgcd[5].gd.flags = is_math ? (gg_enabled|gg_visible|gg_text_xim) :
		(gg_visible);
	tgcd[5].gd.cid = CID_TeX_Sub;
	tgcd[5].creator = GTextFieldCreate;
	thvarray[7] = &tgcd[5]; thvarray[8] = NULL;

	tgcd[6].gd.pos.x = 5; tgcd[6].gd.pos.y = 83+4;
	tgcd[6].gd.flags = tgcd[4].gd.flags;
	tlabel[6].text = (unichar_t *) _("Superscript Pos:");
	tlabel[6].text_is_1byte = true;
	tgcd[6].gd.label = &tlabel[6];
	tgcd[6].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[6].creator = GLabelCreate;
	thvarray[9] = &tgcd[6];

	tgcd[7].gd.pos.x = 85; tgcd[7].gd.pos.y = 83; tgcd[7].gd.pos.width = 50;
	tgcd[7].gd.flags = tgcd[5].gd.flags;
	tgcd[7].gd.cid = CID_TeX_Super;
	tgcd[7].creator = GTextFieldCreate;
	thvarray[10] = &tgcd[7]; thvarray[11] = NULL;

	thvarray[12] = GCD_Glue; thvarray[13] = GCD_Glue; thvarray[14] = NULL;
	thvarray[15] = NULL;

	tbox[0].gd.flags = gg_enabled|gg_visible;
	tbox[0].gd.u.boxelements = thvarray;
	tbox[0].creator = GHVBoxCreate;

	memset(&mgcd,0,sizeof(mgcd));
	memset(&mbox,0,sizeof(mbox));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,'\0',sizeof(aspects));

	i = 0;
	aspects[i].text = (unichar_t *) _("Unicode");
	aspects[i].text_is_1byte = true;
	aspects[i].selected = true;
	aspects[i++].gcd = ubox;

	aspects[i].text = (unichar_t *) _("Comment");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = cbox;

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Pos");
#else
	aspects[i].text = (unichar_t *) _("Positionings");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[pst_position-1];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Pair");
#else
	aspects[i].text = (unichar_t *) _("Pairwise Pos");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[pst_pair-1];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Subs");
#else
	aspects[i].text = (unichar_t *) _("Substitutions");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[2];

	aspects[i].text = (unichar_t *) _("Alt Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[3];

	aspects[i].text = (unichar_t *) _("Mult Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[4];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Ligature");
#else
	aspects[i].text = (unichar_t *) _("Ligatures");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = psgcd[5];

	aspects[i].text = (unichar_t *) _("Components");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = cobox;

	aspects[i].text = (unichar_t *) _("Counters");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[6];

	aspects[i].text = (unichar_t *) U_("ΤεΧ");		/* TeX */
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = tbox;

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.u.tabs = aspects;
#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	mgcd[0].gd.flags = gg_visible | gg_enabled;
#else
	mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
#endif
	mgcd[0].gd.cid = CID_Tabs;
	mgcd[0].creator = GTabSetCreate;
	mvarray[0] = &mgcd[0]; mvarray[1] = NULL;

	mgcd[1].gd.pos.x = 40; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+3;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _("< _Prev");
	mlabel[1].text_is_1byte = true;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.mnemonic = 'P';
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = CI_NextPrev;
	mgcd[1].gd.cid = -1;
	mharray1[0] = GCD_Glue; mharray1[1] = &mgcd[1]; mharray1[2] = GCD_Glue;
	mgcd[1].creator = GButtonCreate;

	mgcd[2].gd.pos.x = -40; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y;
	mgcd[2].gd.flags = gg_visible | gg_enabled ;
	mlabel[2].text = (unichar_t *) _("_Next >");
	mlabel[2].text_is_1byte = true;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.mnemonic = 'N';
	mgcd[2].gd.handle_controlevent = CI_NextPrev;
	mgcd[2].gd.cid = 1;
	mharray1[3] = GCD_Glue; mharray1[4] = &mgcd[2]; mharray1[5] = GCD_Glue; mharray1[6] = NULL;
	mgcd[2].creator = GButtonCreate;

	mbox[2].gd.flags = gg_enabled|gg_visible;
	mbox[2].gd.u.boxelements = mharray1;
	mbox[2].creator = GHBoxCreate;
	mvarray[2] = &mbox[2]; mvarray[3] = NULL;

	mgcd[3].gd.pos.x = 25-3; mgcd[3].gd.pos.y = CI_Height-31-3;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[3].text = (unichar_t *) _("_OK");
	mlabel[3].text_is_1byte = true;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.mnemonic = 'O';
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.handle_controlevent = CI_OK;
	mharray2[0] = GCD_Glue; mharray2[1] = &mgcd[3]; mharray2[2] = GCD_Glue; mharray2[3] = GCD_Glue;
	mgcd[3].creator = GButtonCreate;

	mgcd[4].gd.pos.x = -25; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y+3;
	mgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[4].text = (unichar_t *) _("_Done");
	mlabel[4].text_is_1byte = true;
	mlabel[4].text_in_resource = true;
	mgcd[4].gd.label = &mlabel[4];
	mgcd[4].gd.mnemonic = 'C';
	mgcd[4].gd.handle_controlevent = CI_Cancel;
	mgcd[4].gd.cid = CID_Cancel;
	mharray2[4] = GCD_Glue; mharray2[5] = &mgcd[4]; mharray2[6] = GCD_Glue; mharray2[7] = NULL;
	mgcd[4].creator = GButtonCreate;

	mbox[3].gd.flags = gg_enabled|gg_visible;
	mbox[3].gd.u.boxelements = mharray2;
	mbox[3].creator = GHBoxCreate;
	mvarray[4] = &mbox[3]; mvarray[5] = NULL;
	mvarray[6] = NULL;

	mbox[0].gd.pos.x = mbox[0].gd.pos.y = 2;
	mbox[0].gd.flags = gg_enabled|gg_visible;
	mbox[0].gd.u.boxelements = mvarray;
	mbox[0].creator = GHVGroupCreate;

	GGadgetsCreate(ci->gw,mbox);
	GHVBoxSetExpandableRow(mbox[0].ret,0);
	GHVBoxSetExpandableCol(mbox[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(mbox[3].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(ubox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(ubox[0].ret,1);
	GHVBoxSetExpandableCol(ubox[2].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(cbox[0].ret,1);
	GHVBoxSetExpandableCol(cbox[2].ret,gb_expandglue);

	for ( i=0; i<6; ++i ) {
	    GMatrixEditSetNewText(GWidgetGetControl(ci->gw,CID_List+i*100),
		    newstrings[i]);
	}
	GHVBoxSetExpandableRow(pstbox[pst_pair-1][0].ret,0);
	GMatrixEditSetMouseMoveReporter(psgcd[pst_pair-1][0].ret,CI_KerningPopupPrepare);
	for ( i=6; i<7; ++i ) {
	    GHVBoxSetExpandableRow(pstbox[i][0].ret,0);
	    GHVBoxSetExpandableCol(pstbox[i][2].ret,gb_expandgluesame);
	}

	GHVBoxSetExpandableRow(cobox[0].ret,gb_expandglue);
	GHVBoxSetExpandableRow(tbox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(tbox[0].ret,1);
	GHVBoxSetPadding(tbox[0].ret,6,4);

	GHVBoxFitWindow(mbox[0].ret);
	
	memset(&rq,0,sizeof(rq));
	rq.family_name = monospace;
	rq.point_size = 12;
	rq.weight = 400;
	font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(ci->gw),&rq);
	for ( i=0; i<5; ++i )
	    GGadgetSetFont(psgcd[i][0].ret,font);

    CIFillup(ci);

    GWidgetHidePalettes();
    GDrawSetVisible(ci->gw,true);
}

void CharInfoDestroy(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}

struct sel_dlg {
    int done;
    int ok;
    FontView *fv;
};

#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */


static int tester(SplineChar *sc, struct lookup_subtable *sub) {
    PST *pst;
    KernPair *kp;
    int isv;
    AnchorPoint *ap;

    if ( sc==NULL )
return( false );

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next )
	if ( ap->anchor->subtable == sub )
return( true );
    for ( pst=sc->possub; pst!=NULL; pst=pst->next )
	if ( pst->subtable==sub )
return( true );
    for ( isv=0; isv<2; ++isv )
	for ( kp = isv ? sc->vkerns : sc->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->subtable == sub )
return( true );

return( false );
}

int FVParseSelectByPST(FontView *fv,struct lookup_subtable *sub,
	int search_type) {
    int i;
    SplineFont *sf;
    int first;
    int gid;

    sf = fv->sf;
    first = -1;
    if ( search_type==1 ) {	/* Select results */
	for ( i=0; i<fv->map->enccount; ++i ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],sub)) && first==-1 )
		first = i;
	}
    } else if ( search_type==2) {/* merge results */
	for ( i=0; i<fv->map->enccount; ++i ) if ( !fv->selected[i] ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],sub)) && first==-1 )
		first = i;
	}
    } else {			/* restrict selection */
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],sub)) && first==-1 )
		first = i;
	}
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( first!=-1 )
	FVScrollToChar(fv,first);
    else if ( !no_windowing_ui )
	ff_post_notice(_("Select By ATT..."),_("No glyphs matched"));
    if (  !no_windowing_ui )
	GDrawRequestExpose(fv->v,NULL,false);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}
    
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int SelectStuff(struct sel_dlg *sld,GWindow gw) {
    struct lookup_subtable *sub = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PST))->userdata;
    int search_type = GGadgetIsChecked(GWidgetGetControl(gw,CID_SelectResults)) ? 1 :
	    GGadgetIsChecked(GWidgetGetControl(gw,CID_MergeResults)) ? 2 :
		3;
return( FVParseSelectByPST(sld->fv, sub, search_type));
}

static int selpst_e_h(GWindow gw, GEvent *event) {
    struct sel_dlg *sld = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	sld->done = true;
	sld->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("selectbyatt.html");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	sld->ok = GGadgetGetCid(event->u.control.g);
	if ( !sld->ok || SelectStuff(sld,gw))
	    sld->done = true;
    }
return( true );
}

void FVSelectByPST(FontView *fv) {
    struct sel_dlg sld;
    GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    GGadgetCreateData *varray[20], *harray[8];
    int i,j,isgpos, cnt;
    OTLookup *otl;
    struct lookup_subtable *sub;
    GTextInfo *ti;
    SplineFont *sf = fv->sf;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    ti = NULL;
    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( isgpos=0; isgpos<2; ++isgpos ) {
	    for ( otl = isgpos ? sf->gpos_lookups : sf->gsub_lookups ; otl!=NULL; otl=otl->next ) {
		if ( otl->lookup_type== gsub_single ||
			otl->lookup_type== gsub_multiple ||
			otl->lookup_type== gsub_alternate ||
			otl->lookup_type== gsub_ligature ||
			otl->lookup_type== gpos_single ||
			otl->lookup_type== gpos_pair ||
			otl->lookup_type== gpos_cursive ||
			otl->lookup_type== gpos_mark2base ||
			otl->lookup_type== gpos_mark2ligature ||
			otl->lookup_type== gpos_mark2mark )
		    for ( sub=otl->subtables; sub!=NULL; sub=sub->next )
			if ( sub->kc==NULL ) {
			    if ( ti!=NULL ) {
				ti[cnt].text = (unichar_t *) copy(sub->subtable_name);
			        ti[cnt].text_is_1byte = true;
			        ti[cnt].userdata = sub;
			        ti[cnt].selected = cnt==0;
			    }
			    ++cnt;
			}
	    }
	}
	if ( cnt==0 ) {
	    gwwv_post_notice(_("No Lookups"), _("No applicable lookup subtables"));
return;
	}
	if ( ti==NULL )
	    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    }

    CharInfoInit();

    memset(&sld,0,sizeof(sld));
    sld.fv = fv;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Select By Lookup Subtable...");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,204);
	gw = GDrawCreateTopWindow(NULL,&pos,selpst_e_h,&sld,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	i=j=0;

	label[i].text = (unichar_t *) _("Select Glyphs in lookup subtable");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	gcd[i].gd.label = &ti[0];
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible/*|gg_list_exactlyone*/;
	gcd[i].gd.u.list = ti;
	gcd[i].gd.cid = CID_PST;
	gcd[i++].creator = GListButtonCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;
	varray[j++] = GCD_Glue; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Select Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to the glyphs\nfound by this search");
	gcd[i].gd.cid = CID_SelectResults;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Merge Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Expand the selection of the font view to include\nall the glyphs found by this search");
	gcd[i].gd.cid = CID_MergeResults;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;

	label[i].text = (unichar_t *) _("Restrict Selection");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Only search the selected glyphs, and unselect\nany characters which do not match this search");
	gcd[i].gd.cid = CID_RestrictSelection;
	gcd[i++].creator = GRadioCreate;
	varray[j++] = &gcd[i-1]; varray[j++] = NULL;
	varray[j++] = GCD_Glue; varray[j++] = NULL;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+22;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	gcd[i++].creator = GButtonCreate;
	harray[0] = GCD_Glue; harray[1] = &gcd[i-1]; harray[2] = GCD_Glue;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = false;
	gcd[i++].creator = GButtonCreate;
	harray[3] = GCD_Glue; harray[4] = &gcd[i-1]; harray[5] = GCD_Glue;
	harray[6] = NULL;

	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = harray;
	gcd[i].creator = GHBoxCreate;
	varray[j++] = &gcd[i++]; varray[j++] = NULL; varray[j++] = NULL;

	gcd[i].gd.pos.x = gcd[i].gd.pos.y = 2;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.boxelements = varray;
	gcd[i].creator = GHVGroupCreate;

	GGadgetsCreate(gw,gcd+i);
	GTextInfoListFree(ti);
	GHVBoxSetExpandableRow(gcd[i].ret,gb_expandglue);
	GHVBoxSetExpandableCol(gcd[i-1].ret,gb_expandgluesame);
	GHVBoxFitWindow(gcd[i].ret);

    GDrawSetVisible(gw,true);
    while ( !sld.done )
	GDrawProcessOneEvent(NULL);
    if ( sld.ok ) {
    }
    GDrawDestroyWindow(gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void CharInfoInit(void) {
    static GTextInfo *lists[] = { glyphclasses, std_colors, NULL };
    static int done = 0;
    int i, j;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    static char **cnames[] = { newstrings, NULL };
#endif

    if ( done )
return;
    done = true;
    for ( i=0; lists[i]!=NULL; ++i )
	for ( j=0; lists[i][j].text!=NULL; ++j )
	    lists[i][j].text = (unichar_t *) S_((char *) lists[i][j].text);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    for ( i=0; cnames[i]!=NULL; ++i )
	for ( j=0; cnames[i][j]!=NULL; ++j )
	    cnames[i][j] = _(cnames[i][j]);
#endif
}    
#endif /* FONTFORGE_CONFIG_NO_WINDOWING_UI */
