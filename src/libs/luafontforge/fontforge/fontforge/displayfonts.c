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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>
#include <ustring.h>

typedef struct di {
    int done;
    GWindow gw;
    GTimer *sizechanged;

    /* current setting */
    SplineFont *sf;
    enum sftf_fonttype fonttype;
    int pixelsize;
    int antialias;
} DI;

#define CID_Font	1001
#define CID_AA		1002
#define CID_SizeLab	1003
#define CID_Size	1004
#define CID_pfb		1005
#define CID_ttf		1006
#define CID_otf		1007
#define CID_bitmap	1009
#define CID_pfaedit	1010
#define CID_SampleText	1011
#define CID_Done	1021
#define CID_Group	1022

static GTextInfo *FontNames(SplineFont *cur_sf) {
    int cnt;
    FontView *fv;
    SplineFont *sf;
    GTextInfo *ti;

    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next )
	if ( fv->nextsame==NULL )
	    ++cnt;
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( fv=fv_list, cnt=0; fv!=NULL; fv=fv->next )
	if ( fv->nextsame==NULL ) {
	    sf = fv->sf;
	    if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	    ti[cnt].text = uc_copy(sf->fontname);
	    ti[cnt].userdata = sf;
	    if ( sf==cur_sf )
		ti[cnt].selected = true;
	    ++cnt;
	}
return( ti );
}

static BDFFont *DSP_BestMatch(SplineFont *sf,int aa,int size) {
    BDFFont *bdf, *sizem=NULL;
    int a;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	if ( bdf->clut==NULL && !aa )
	    a = 4;
	else if ( bdf->clut!=NULL && aa ) {
	    if ( bdf->clut->clut_len==256 )
		a = 4;
	    else if ( bdf->clut->clut_len==16 )
		a = 3;
	    else
		a = 2;
	} else
	    a = 1;
	if ( bdf->pixelsize==size && a==4 )
return( bdf );
	if ( sizem==NULL )
	    sizem = bdf;
	else {
	    int sdnew = bdf->pixelsize-size, sdold = sizem->pixelsize-size;
	    if ( sdnew<0 ) sdnew = -sdnew;
	    if ( sdold<0 ) sdold = -sdold;
	    if ( sdnew<sdold )
		sizem = bdf;
	    else if ( sdnew==sdold ) {
		int olda;
		if ( sizem->clut==NULL && !aa )
		    olda = 4;
		else if ( sizem->clut!=NULL && aa ) {
		    if ( sizem->clut->clut_len==256 )
			olda = 4;
		    else if ( sizem->clut->clut_len==16 )
			olda = 3;
		    else
			olda = 2;
		} else
		    olda = 1;
		if ( a>olda )
		    sizem = bdf;
	    }
	}
    }
return( sizem );	
}

static BDFFont *DSP_BestMatchDlg(DI *di) {
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int val;
    unichar_t *end;

    if ( sel==NULL )
return( NULL );
    sf = sel->userdata;
    val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    if ( *end!='\0' || val<4 )
return( NULL );

return( DSP_BestMatch(sf,GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA)),val) );
}

static void DSP_SetFont(DI *di,int doall) {
    unichar_t *end;
    int size = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),&end,10);
    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
    SplineFont *sf;
    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
    int type;

    if ( sel==NULL || *end )
return;
    sf = sel->userdata;

    type = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfb))? sftf_pfb :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_ttf))? sftf_ttf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_otf))? sftf_otf :
	   GGadgetIsChecked(GWidgetGetControl(di->gw,CID_pfaedit))? sftf_pfaedit :
	   sftf_bitmap;
    if ( di->sf==sf && di->fonttype==type && di->pixelsize==size && di->antialias==aa && !doall )
return;
    if ( !SFTFSetFont(GWidgetGetControl(di->gw,CID_SampleText),doall?0:-1,-1,
	    sf,type,size,aa))
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_error(_("Bad Font"),_("Bad Font"));
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Font"),_("Bad Font"));
#endif
    di->sf = sf;
    di->fonttype = type;
    di->pixelsize = size;
    di->antialias = aa;
}

static void DSP_ChangeFontCallback(void *context,SplineFont *sf,enum sftf_fonttype type,
	int size, int aa) {
    DI *di = context;
    char buf[12]; unichar_t ubuf[12];

    if ( di->antialias!=aa ) {
	GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),aa);
	di->antialias = aa;
    }
    if ( di->pixelsize!=size ) {
	sprintf(buf,"%d",size);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),ubuf);
	di->pixelsize = size;
    }
    if ( di->sf!=sf ) {
	GTextInfo **ti;
	int i,set; int32 len;
	ti = GGadgetGetList(GWidgetGetControl(di->gw,CID_Font),&len);
	for ( i=0; i<len; ++i )
	    if ( ti[i]->userdata == sf )
	break;
	if ( i<len ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(di->gw,CID_Font),i);
	    /*GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Font),ti[i]->text);*/
	}
	set = hasFreeType() && !sf->onlybitmaps && sf->subfontcnt==0;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfb),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_ttf),set);
	set = hasFreeType() && !sf->onlybitmaps;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),set);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_pfaedit),!sf->onlybitmaps);
	di->sf = sf;
    }
    if ( di->fonttype!=type ) {
	if ( type==sftf_pfb )
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
	else if ( type==sftf_ttf )
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_ttf),true);
	else if ( type==sftf_otf )
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
	else if ( type==sftf_pfaedit )
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfaedit),true);
	else
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);
	di->fonttype = type;
    }
}

static int DSP_AAState(SplineFont *sf,BDFFont *bestbdf) {
    /* What should AntiAlias look like given the current set of bitmaps */
    int anyaa=0, anybit=0;
    BDFFont *bdf;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	if ( bdf->clut==NULL )
	    anybit = true;
	else
	    anyaa = true;
    if ( anybit && anyaa )
return( gg_visible | gg_enabled | (bestbdf!=NULL && bestbdf->clut!=NULL ? gg_cb_on : 0) );
    else if ( anybit )
return( gg_visible );
    else if ( anyaa )
return( gg_visible | gg_cb_on );

return( gg_visible );
}

static int DSP_FontChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	GTextInfo *sel = GGadgetGetListItemSelected(g);
	SplineFont *sf;
	BDFFont *best;
	int flags, pick = 0, i;
	char size[12]; unichar_t usize[12];

	if ( sel==NULL )
return( true );
	sf = sel->userdata;
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) && sf->bitmaps==NULL )
	    pick = true;
	GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_bitmap),sf->bitmaps!=NULL);
	if ( !hasFreeType() || sf->onlybitmaps ) {
	    best = DSP_BestMatchDlg(di);
	    flags = DSP_AAState(sf,best);
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_bitmap),true);
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
	    if ( best!=NULL ) {
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	} else if ( sf->subfontcnt!=0 ) {
	    for ( i=CID_pfb; i<CID_otf; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),false);
		if ( GGadgetIsChecked(GWidgetGetControl(di->gw,i)) )
		    pick = true;
	    }
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_otf),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_otf),true);
	} else {
	    for ( i=CID_pfb; i<=CID_otf; ++i )
		GGadgetSetEnabled(GWidgetGetControl(di->gw,i),true);
	    if ( pick )
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_pfb),true);
	}
	DSP_SetFont(di,false);
    }
return( true );
}

static int DSP_AAChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    int val = u_strtol(_GGadgetGetTitle(GWidgetGetControl(di->gw,CID_Size)),NULL,10);
	    int bestdiff = 8000, bdfdiff;
	    BDFFont *bdf, *best=NULL;
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( (aa && bdf->clut) || (!aa && bdf->clut==NULL)) {
		    if ((bdfdiff = bdf->pixelsize-val)<0 ) bdfdiff = -bdfdiff;
		    if ( bdfdiff<bestdiff ) {
			best = bdf;
			bestdiff = bdfdiff;
			if ( bdfdiff==0 )
	    break;
		    }
		}
	    }
	    if ( best!=NULL ) {
		char size[12]; unichar_t usize[12];
		sprintf( size, "%d", best->pixelsize );
		uc_strcpy(usize,size);
		GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
	    }
	}
	DSP_SetFont(di,false);
    }
return( true );
}

static int DSP_SizeChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textfocuschanged &&
	    !e->u.control.u.tf_focus.gained_focus ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	int err=false;
	int size = GetInt8(di->gw,CID_Size,_("_Size:"),&err);
	if ( err || size<4 )
return( true );
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    BDFFont *bdf, *best=NULL;
	    int aa = GGadgetIsChecked(GWidgetGetControl(di->gw,CID_AA));
	    if ( sel==NULL )
return( true );
	    sf = sel->userdata;
	    for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( bdf->pixelsize==size ) {
		    if (( bdf->clut && aa ) || ( bdf->clut==NULL && !aa )) {
			best = bdf;
	    break;
		    }
		    best = bdf;
		}
	    }
	    if ( best==NULL ) {
		char buf[100], *pt=buf, *end=buf+sizeof(buf)-10;
		unichar_t ubuf[12];
		int lastsize = 0;
		for ( bdf=sf->bitmaps; bdf!=NULL && pt<end; bdf=bdf->next ) {
		    if ( bdf->pixelsize!=lastsize ) {
			sprintf( pt, "%d,", bdf->pixelsize );
			lastsize = bdf->pixelsize;
			pt += strlen(pt);
		    }
		}
		if ( pt!=buf )
		    pt[-1] = '\0';
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_post_error(_("Bad Size"),_("Requested bitmap size not available in font. Font supports %s"),buf);
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Bad Size"),_("Requested bitmap size not available in font. Font supports %s"),buf);
#endif
		best = DSP_BestMatchDlg(di);
		if ( best!=NULL ) {
		    sprintf( buf, "%d", best->pixelsize);
		    uc_strcpy(ubuf,buf);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),ubuf);
		}
	    }
	    if ( best==NULL )
return(true);
	    GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),best->clut!=NULL );
	}
	DSP_SetFont(di,false);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	/* Don't change the font on each change to the text field, that might */
	/*  look rather odd. But wait until we think they may have finished */
	/*  typing. Either when the change the focus (above) or when they */
	/*  just don't do anything for a while */
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( di->sizechanged!=NULL )
	    GDrawCancelTimer(di->sizechanged);
	di->sizechanged = GDrawRequestTimer(di->gw,600,0,NULL);
    }
return( true );
}

static void DSP_SizeChangedTimer(DI *di) {
    GEvent e;

    di->sizechanged = NULL;
    memset(&e,0,sizeof(e));
    e.type = et_controlevent;
    e.u.control.g = GWidgetGetControl(di->gw,CID_Size);
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.u.tf_focus.gained_focus = false;
    DSP_SizeChanged(e.u.control.g,&e);
}

static int DSP_RadioSet(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	DI *di = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(di->gw,CID_bitmap)) ) {
	    BDFFont *best = DSP_BestMatchDlg(di);
	    GTextInfo *sel = GGadgetGetListItemSelected(GWidgetGetControl(di->gw,CID_Font));
	    SplineFont *sf;
	    int flags;
	    char size[12]; unichar_t usize[12];

	    if ( sel!=NULL ) {
		sf = sel->userdata;
		flags = DSP_AAState(sf,best);
		GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),flags&gg_enabled);
		GGadgetSetChecked(GWidgetGetControl(di->gw,CID_AA),flags&gg_cb_on);
		if ( best!=NULL ) {
		    sprintf( size, "%d", best->pixelsize );
		    uc_strcpy(usize,size);
		    GGadgetSetTitle(GWidgetGetControl(di->gw,CID_Size),usize);
		}
	    }
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(di->gw,CID_AA),true);
	}
	DSP_SetFont(di,false);
    }
return( true );
}

static int DSP_TextChanged(GGadget *g, GEvent *e) {
return( true );
}

static int DSP_Done(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	DI *di = GDrawGetUserData(gw);
	di->done = true;
    }
return( true );
}

static int dsp_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	DI *di = GDrawGetUserData(gw);
	di->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("display.html");
return( true );
	}
return( false );
    } else if ( event->type==et_timer ) {
	DI *di = GDrawGetUserData(gw);
	if ( event->u.timer.timer==di->sizechanged )
	    DSP_SizeChangedTimer(di);
    }
return( true );
}
    
void DisplayDlg(SplineFont *sf) {
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[15], boxes[5], *harray[8], *farray[7], *barray[4], *varray[9];
    GTextInfo label[15];
    DI di;
    char buf[10];
    int hasfreetype = hasFreeType();
    BDFFont *bestbdf=DSP_BestMatch(sf,true,12);
    int twobyte;

    if ( sf->cidmaster )
	sf = sf->cidmaster;

    memset(&di,0,sizeof(di));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Display...");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,410));
    pos.height = GDrawPointsToPixels(NULL,330);
    di.gw = GDrawCreateTopWindow(NULL,&pos,dsp_e_h,&di,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));
    memset(&boxes,0,sizeof(boxes));

    label[0].text = (unichar_t *) sf->fontname;
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.popup_msg = (unichar_t *) _("Select some text, then use this list to change the\nfont in which those characters are displayed.");
    gcd[0].gd.pos.x = 12; gcd[0].gd.pos.y = 6; 
    gcd[0].gd.pos.width = 150;
    gcd[0].gd.flags = (fv_list==NULL || fv_list->next==NULL) ?
	    (gg_visible | gg_utf8_popup):
	    (gg_visible | gg_enabled | gg_utf8_popup);
    gcd[0].gd.cid = CID_Font;
    gcd[0].gd.u.list = FontNames(sf);
    gcd[0].gd.handle_controlevent = DSP_FontChanged;
    gcd[0].creator = GListButtonCreate;
    harray[0] = &gcd[0]; harray[1] = GCD_HPad10;

    label[1].text = (unichar_t *) _("_AA");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.mnemonic = 'A';
    gcd[1].gd.pos.x = 170; gcd[1].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup;
    if ( sf->bitmaps!=NULL && ( !hasfreetype || sf->onlybitmaps ))
	gcd[1].gd.flags = DSP_AAState(sf,bestbdf);
    gcd[1].gd.popup_msg = (unichar_t *) _("Select some text, this controls whether those characters will be\nAntiAlias (greymap) characters, or bitmap characters");
    gcd[1].gd.handle_controlevent = DSP_AAChange;
    gcd[1].gd.cid = CID_AA;
    gcd[1].creator = GCheckBoxCreate;
    harray[2] = &gcd[1]; harray[3] = GCD_HPad10;

    label[2].text = (unichar_t *) _("_Size:");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'S';
    gcd[2].gd.pos.x = 210; gcd[2].gd.pos.y = gcd[0].gd.pos.y+6; 
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[2].gd.popup_msg = (unichar_t *) _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[2].gd.cid = CID_SizeLab;
    gcd[2].creator = GLabelCreate;
    harray[4] = &gcd[2];

    if ( bestbdf !=NULL && ( !hasfreetype || sf->onlybitmaps ))
	sprintf( buf, "%d", bestbdf->pixelsize );
    else
	strcpy(buf,"17");
    label[3].text = (unichar_t *) buf;
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 240; gcd[3].gd.pos.y = gcd[0].gd.pos.y+3; 
    gcd[3].gd.pos.width = 40;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[3].gd.cid = CID_Size;
    gcd[3].gd.handle_controlevent = DSP_SizeChanged;
    gcd[3].gd.popup_msg = (unichar_t *) _("Select some text, this specifies the pixel\nsize of those characters");
    gcd[3].creator = GTextFieldCreate;
    harray[5] = &gcd[3]; harray[6] = GCD_Glue; harray[7] = NULL;

    boxes[2].gd.flags = gg_enabled|gg_visible;
    boxes[2].gd.u.boxelements = harray;
    boxes[2].creator = GHBoxCreate;
    varray[0] = &boxes[2]; varray[1] = NULL;

    label[4].text = (unichar_t *) "pfb";
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.mnemonic = 'p';
    gcd[4].gd.pos.x = gcd[0].gd.pos.x; gcd[4].gd.pos.y = 24+gcd[3].gd.pos.y; 
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_cb_on | gg_utf8_popup;
    gcd[4].gd.cid = CID_pfb;
    gcd[4].gd.handle_controlevent = DSP_RadioSet;
    gcd[4].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[4].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps ) gcd[4].gd.flags = gg_visible;
    farray[0] = &gcd[4];

    label[5].text = (unichar_t *) "ttf";
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.mnemonic = 't';
    gcd[5].gd.pos.x = 46; gcd[5].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[5].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[5].gd.cid = CID_ttf;
    gcd[5].gd.handle_controlevent = DSP_RadioSet;
    gcd[5].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[5].creator = GRadioCreate;
    if ( sf->subfontcnt!=0 || !hasfreetype || sf->onlybitmaps ) gcd[5].gd.flags = gg_visible;
    farray[1] = &gcd[5];

    label[6].text = (unichar_t *) "otf";
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.mnemonic = 'o';
    gcd[6].gd.pos.x = 114; gcd[6].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[6].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[6].gd.cid = CID_otf;
    gcd[6].gd.handle_controlevent = DSP_RadioSet;
    gcd[6].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[6].creator = GRadioCreate;
    if ( !hasfreetype || sf->onlybitmaps ) gcd[6].gd.flags = gg_visible;
    else if ( sf->subfontcnt!=0 ) gcd[6].gd.flags |= gg_cb_on;
    farray[2] = &gcd[6];

    label[7].text = (unichar_t *) "bitmap";
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.mnemonic = 'b';
    gcd[7].gd.pos.x = 148; gcd[7].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[7].gd.cid = CID_bitmap;
    gcd[7].gd.handle_controlevent = DSP_RadioSet;
    gcd[7].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[7].creator = GRadioCreate;
    if ( sf->bitmaps==NULL ) gcd[7].gd.flags = gg_visible;
    else if ( !hasfreetype || sf->onlybitmaps ) gcd[7].gd.flags |= gg_cb_on;
    farray[3] = &gcd[7];

    label[8].text = (unichar_t *) "FontForge";
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.mnemonic = 'p';
    gcd[8].gd.pos.x = 200; gcd[8].gd.pos.y = gcd[4].gd.pos.y; 
    gcd[8].gd.flags = gg_visible | gg_enabled | gg_utf8_popup;
    gcd[8].gd.cid = CID_pfaedit;
    gcd[8].gd.handle_controlevent = DSP_RadioSet;
    gcd[8].gd.popup_msg = (unichar_t *) _("Specifies file format used to pass the font to freetype\n  pfb -- is the standard postscript type1\n  ttf -- is truetype\n  otf -- is opentype\n  bitmap -- not passed to freetype for rendering\n    bitmap fonts must already be generated\n  FontForge -- uses FontForge's own rasterizer, not\n    freetype's. Only as last resort");
    gcd[8].creator = GRadioCreate;
    if ( !hasfreetype && sf->bitmaps==NULL ) gcd[8].gd.flags |= gg_cb_on;
    else if ( sf->onlybitmaps ) gcd[8].gd.flags = gg_visible;
    farray[4] = &gcd[8]; farray[5] = GCD_Glue; farray[6] = NULL;

    boxes[3].gd.flags = gg_enabled|gg_visible;
    boxes[3].gd.u.boxelements = farray;
    boxes[3].creator = GHBoxCreate;
    varray[2] = &boxes[3]; varray[3] = NULL;


    twobyte = sf->fv->map->enc->has_2byte;
    label[9].text = PrtBuildDef(sf,twobyte);
    gcd[9].gd.label = &label[9];
    gcd[9].gd.mnemonic = 'T';
    gcd[9].gd.pos.x = 5; gcd[9].gd.pos.y = 20+gcd[9].gd.pos.y; 
    gcd[9].gd.pos.width = 400; gcd[9].gd.pos.height = 236; 
    gcd[9].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    gcd[9].gd.handle_controlevent = DSP_TextChanged;
    gcd[9].gd.cid = CID_SampleText;
    gcd[9].creator = SFTextAreaCreate;
    varray[4] = &gcd[9]; varray[5] = NULL;


    gcd[10].gd.pos.x = (410-GIntGetResource(_NUM_Buttonsize))/2; gcd[10].gd.pos.y = gcd[9].gd.pos.y+gcd[10].gd.pos.height+6;
    gcd[10].gd.pos.width = -1; gcd[10].gd.pos.height = 0;
    gcd[10].gd.flags = gg_visible | gg_enabled | gg_but_default | gg_but_cancel;
    label[10].text = (unichar_t *) _("_Done");
    label[10].text_is_1byte = true;
    label[10].text_in_resource = true;
    gcd[10].gd.mnemonic = 'D';
    gcd[10].gd.label = &label[10];
    gcd[10].gd.cid = CID_Done;
    gcd[10].gd.handle_controlevent = DSP_Done;
    gcd[10].creator = GButtonCreate;
    barray[0] = GCD_Glue; barray[1] = &gcd[10]; barray[2] = GCD_Glue; barray[3] = NULL;

    boxes[4].gd.flags = gg_enabled|gg_visible;
    boxes[4].gd.u.boxelements = barray;
    boxes[4].creator = GHBoxCreate;
    varray[6] = &boxes[4]; varray[7] = NULL;
    varray[8] = NULL;

    boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
    boxes[0].gd.flags = gg_enabled|gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GHVGroupCreate;

    GGadgetsCreate(di.gw,boxes);

    GTextInfoListFree(gcd[0].gd.u.list);
    free( label[9].text );
    DSP_SetFont(&di,true);
    SFTFRegisterCallback(gcd[9].ret,&di,DSP_ChangeFontCallback);

    GHVBoxSetExpandableRow(boxes[0].ret,2);
    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[3].ret,gb_expandglue);
    GHVBoxSetExpandableCol(boxes[4].ret,gb_expandglue);
    GHVBoxFitWindow(boxes[0].ret);
    
    GDrawSetVisible(di.gw,true);
    while ( !di.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(di.gw);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
