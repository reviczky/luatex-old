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
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <math.h>

static uint32 simple_stdfeatures[] = { CHR('c','c','m','p'), CHR('l','o','c','a'), CHR('k','e','r','n'), CHR('l','i','g','a'), CHR('c','a','l','t'), CHR('m','a','r','k'), CHR('m','k','m','k'), REQUIRED_FEATURE, 0 };
static uint32 arab_stdfeatures[] = { CHR('c','c','m','p'), CHR('l','o','c','a'), CHR('i','s','o','l'), CHR('i','n','i','t'), CHR('m','e','d','i'),CHR('f','i','n','a'), CHR('r','l','i','g'), CHR('l','i','g','a'), CHR('c','a','l','t'), CHR('k','e','r','n'), CHR('c','u','r','s'), CHR('m','a','r','k'), CHR('m','k','m','k'), REQUIRED_FEATURE, 0 };
static uint32 hebrew_stdfeatures[] = { CHR('c','c','m','p'), CHR('l','o','c','a'), CHR('l','i','g','a'), CHR('c','a','l','t'), CHR('k','e','r','n'), CHR('m','a','r','k'), CHR('m','k','m','k'), REQUIRED_FEATURE, 0 };
static struct { uint32 script, *stdfeatures; } script_2_std[] = {
    { CHR('l','a','t','n'), simple_stdfeatures },
    { CHR('D','F','L','T'), simple_stdfeatures },
    { CHR('c','y','r','l'), simple_stdfeatures },
    { CHR('g','r','e','k'), simple_stdfeatures },
    { CHR('a','r','a','b'), arab_stdfeatures },
    { CHR('h','e','b','r'), hebrew_stdfeatures },
    { 0 }
};

static int mv_antialias = true;
static double mv_scales[] = { 2.0, 1.5, 1.0, 2.0/3.0, .5, 1.0/3.0, .25, .2, 1.0/6.0, .125, .1 };

static int MVSetVSb(MetricsView *mv);

static uint32 *StdFeaturesOfScript(uint32 script) {
    int i;

    for ( i=0; script_2_std[i].script!=0; ++i )
	if ( script_2_std[i].script==script )
return( script_2_std[i].stdfeatures );

return( simple_stdfeatures );
}

#if 0
static void MVDrawAnchorPoint(GWindow pixmap,MetricsView *mv,int i,struct aplist *apl) {
    SplineFont *sf = mv->sf;
    int emsize = sf->ascent+sf->descent;
    double scale = mv->pixelsize / (double) emsize;
    double scaleas = mv->pixelsize / (double) (mv_scales[mv->scale_index]*emsize);
    AnchorPoint *ap = apl->ap;
    int x,y;

    if ( mv->bdf!=NULL )
return;

    y = mv->topend + 2 + scaleas * sf->ascent - mv->perchar[i].yoff - ap->me.y*scale - mv->yoff;
    x = mv->perchar[i].dx-mv->xoff+mv->perchar[i].xoff;
    if ( mv->perchar[i].selected )
	x += mv->activeoff;
    if ( mv->right_to_left )
	x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
    x += ap->me.x*scale;
    DrawAnchorPoint(pixmap,x,y,apl->selected);
}
#endif

static void MVVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    /* Expose routine for vertical metrics */
    GRect *clip, r, old2;
    int xbase, y, si, i, x, width, height;
    int as = rint(mv->pixelsize*mv->sf->ascent/(double) (mv->sf->ascent+mv->sf->descent));
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;

    clip = &event->u.expose.rect;

    xbase = (mv->dwidth-mv->xstart)/2 + mv->xstart;
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,xbase,mv->topend,xbase,mv->displayend,0x808080);

    r.x = clip->x; r.width = clip->width;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    if ( r.x<=mv->xstart ) {
	r.width -= (mv->xstart-r.x);
	r.x = mv->xstart;
    }
    GDrawPushClip(pixmap,&r,&old2);
    if ( mv->bdf==NULL && mv->showgrid ) {
	y = mv->perchar[0].dy-mv->yoff;
	GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x808080);
    }

    si = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	y = mv->perchar[i].dy-mv->yoff;
	if ( mv->bdf==NULL && mv->showgrid ) {
	    int yp = y+mv->perchar[i].dheight+mv->perchar[i].kernafter;
	    GDrawDrawLine(pixmap,0, yp,mv->dwidth,yp,0x808080);
	}
	y += mv->perchar[i].yoff;
	bdfc = mv->bdf==NULL ?	BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos) :
				mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos];
	if ( bdfc==NULL )
    continue;
	y += as-bdfc->ymax;
	if ( mv->perchar[i].selected )
	    y += mv->activeoff;
	x = xbase - mv->pixelsize/2 + bdfc->xmin - mv->perchar[i].xoff;
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( clip->y+clip->height<y )
    break;
	if ( y+height>=clip->y && x<clip->x+clip->width && x+width >= clip->x ) {
	    memset(&gi,'\0',sizeof(gi));
	    memset(&base,'\0',sizeof(base));
	    memset(&clut,'\0',sizeof(clut));
	    gi.u.image = &base;
	    base.clut = &clut;
	    if ( !bdfc->byte_data ) {
		base.image_type = it_mono;
		clut.clut_len = 2;
		clut.clut[0] = 0xffffff;
		if ( mv->perchar[i].selected )
		    clut.clut[1] = 0x808080;
	    } else {
		int scale, l;
		Color fg, bg;
		if ( mv->bdf!=NULL )
		    scale = BDFDepth(mv->bdf);
		else
		    scale = BDFDepth(mv->show);
		base.image_type = it_index;
		clut.clut_len = 1<<scale;
		bg = default_background;
		fg = ( mv->perchar[i].selected ) ? 0x808080 : 0x000000;
		for ( l=0; l<(1<<scale); ++l )
		    clut.clut[l] =
			COLOR_CREATE(
			 COLOR_RED(bg) + (l*(COLOR_RED(fg)-COLOR_RED(bg)))/((1<<scale)-1),
			 COLOR_GREEN(bg) + (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg)))/((1<<scale)-1),
			 COLOR_BLUE(bg) + (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg)))/((1<<scale)-1) );
	    }
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = width;
	    base.height = height;
	    GDrawDrawImage(pixmap,&gi,NULL,x,y);
	}
    }
    if ( si!=-1 && mv->bdf==NULL && mv->showgrid ) {
	y = mv->perchar[si].dy-mv->yoff;
	if ( si!=0 )
	    GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x008000);
	y += mv->perchar[si].dheight+mv->perchar[si].kernafter;
	GDrawDrawLine(pixmap,0,y,mv->dwidth,y,0x000080);
    }
    GDrawPopClip(pixmap,&old2);
}

static void MVExpose(MetricsView *mv, GWindow pixmap, GEvent *event) {
    GRect old, *clip, r, old2;
    int x,y,ybase, width,height, i;
    SplineFont *sf = mv->sf;
    BDFChar *bdfc;
    struct _GImage base;
    GImage gi;
    GClut clut;
    int si;
    int ke = mv->height-mv->sbh-(mv->fh+4);
    double s = sin(-mv->sf->italicangle*3.1415926535897932/180.);
    int x_iaoffh = 0, x_iaoffl = 0;

    clip = &event->u.expose.rect;
    if ( clip->y+clip->height < mv->topend )
return;
    GDrawPushClip(pixmap,clip,&old);
    GDrawSetLineWidth(pixmap,0);
    for ( x=mv->mwidth; x<mv->width; x+=mv->mwidth ) {
	GDrawDrawLine(pixmap,x,mv->displayend,x,ke,0x000000);
	GDrawDrawLine(pixmap,x+mv->mwidth/2,ke,x+mv->mwidth/2,mv->height-mv->sbh,0x000000);
    }
    GDrawDrawLine(pixmap,0,mv->topend,mv->width,mv->topend,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend,mv->width,mv->displayend,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+mv->fh+4,mv->width,mv->displayend+mv->fh+4,0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+2*(mv->fh+4),mv->width,mv->displayend+2*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+3*(mv->fh+4),mv->width,mv->displayend+3*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+4*(mv->fh+4),mv->width,mv->displayend+4*(mv->fh+4),0x000000);
    GDrawDrawLine(pixmap,0,mv->displayend+5*(mv->fh+4),mv->width,mv->displayend+5*(mv->fh+4),0x000000);
    if ( clip->y >= mv->displayend ) {
	GDrawPopClip(pixmap,&old);
return;
    }

    if ( mv->vertical ) {
	MVVExpose(mv,pixmap,event);
	GDrawPopClip(pixmap,&old);
return;
    }

    ybase = mv->topend + 2 + (mv->pixelsize/mv_scales[mv->scale_index] * sf->ascent / (sf->ascent+sf->descent)) - mv->yoff;
    if ( mv->showgrid )
	GDrawDrawLine(pixmap,0,ybase,mv->dwidth,ybase,0x808080);


    r.x = clip->x; r.width = clip->width;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    if ( r.x<=mv->xstart ) {
	r.width -= (mv->xstart-r.x);
	r.x = mv->xstart;
    }
    GDrawPushClip(pixmap,&r,&old2);
    if ( mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[0].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[0].dwidth - mv->perchar[0].kernafter;
	GDrawDrawLine(pixmap,x,mv->topend,x,mv->displayend,0x808080);
	x_iaoffh = rint((ybase-mv->topend)*s), x_iaoffl = rint((mv->displayend-ybase)*s);
	if ( ItalicConstrained && x_iaoffh!=0 ) {
	    GDrawDrawLine(pixmap,x+x_iaoffh,mv->topend,x-x_iaoffl,mv->displayend,0x909090);
	}
    }
    si = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) si = i;
	x = mv->perchar[i].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	if ( mv->bdf==NULL && mv->showgrid ) {
	    int xp = x+mv->perchar[i].dwidth+mv->perchar[i].kernafter;
	    GDrawDrawLine(pixmap,xp, mv->topend,xp,mv->displayend,0x808080);
	    if ( ItalicConstrained && x_iaoffh!=0 ) {
		GDrawDrawLine(pixmap,xp+x_iaoffh,mv->topend,xp-x_iaoffl,mv->displayend,0x909090);
	    }
	}
	if ( mv->right_to_left )
	    x += mv->perchar[i].kernafter-mv->perchar[i].xoff;
	else
	    x += mv->perchar[i].xoff;
	bdfc = mv->bdf==NULL ?	BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos) :
				mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos];
	if ( bdfc==NULL )
    continue;
	x += bdfc->xmin;
	if ( mv->perchar[i].selected )
	    x += mv->activeoff;
	y = ybase - bdfc->ymax - mv->perchar[i].yoff;
	width = bdfc->xmax-bdfc->xmin+1; height = bdfc->ymax-bdfc->ymin+1;
	if ( !mv->right_to_left && clip->x+clip->width<x )
    break;
	if ( x+width>=clip->x && y<clip->y+clip->height && y+height >= clip->y ) {
	    memset(&gi,'\0',sizeof(gi));
	    memset(&base,'\0',sizeof(base));
	    memset(&clut,'\0',sizeof(clut));
	    gi.u.image = &base;
	    base.clut = &clut;
	    if ( !bdfc->byte_data ) {
		base.image_type = it_mono;
		clut.clut_len = 2;
		clut.clut[0] = 0xffffff;
		if ( mv->perchar[i].selected )
		    clut.clut[1] = 0x808080;
	    } else {
		int lscale = 3000/mv->pixelsize, l;
		Color fg, bg;
		int scale;
		if ( lscale>4 ) lscale = 4; else if ( lscale==3 ) lscale= 2;
		if ( mv->bdf!=NULL && mv->bdf->clut!=NULL )
		    lscale = BDFDepth(mv->bdf);
		base.image_type = it_index;
		scale = lscale*lscale;
		clut.clut_len = scale;
		bg = default_background;
		fg = ( mv->perchar[i].selected ) ? 0x808080 : 0x000000;
		for ( l=0; l<scale; ++l )
		    clut.clut[l] =
			COLOR_CREATE(
			 COLOR_RED(bg) + ((int32) (l*(COLOR_RED(fg)-COLOR_RED(bg))))/(scale-1),
			 COLOR_GREEN(bg) + ((int32) (l*(COLOR_GREEN(fg)-COLOR_GREEN(bg))))/(scale-1),
			 COLOR_BLUE(bg) + ((int32) (l*(COLOR_BLUE(fg)-COLOR_BLUE(bg))))/(scale-1) );
	    }
	    base.data = bdfc->bitmap;
	    base.bytes_per_line = bdfc->bytes_per_line;
	    base.width = width;
	    base.height = height;
	    GDrawDrawImage(pixmap,&gi,NULL,x,y);
	}
#if 0
	if ( mv->perchar[i].selected )
	    for ( apl=mv->perchar[i].aps; apl!=NULL; apl=apl->next )
		MVDrawAnchorPoint(pixmap,mv,i,apl);
#endif
    }
    if ( si!=-1 && mv->bdf==NULL && mv->showgrid ) {
	x = mv->perchar[si].dx-mv->xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x;
	if ( si!=0 )
	    GDrawDrawLine(pixmap,x,mv->topend,x,mv->displayend,0x008000);
	if ( mv->right_to_left )
	    x -= mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	else
	    x += mv->perchar[si].dwidth+mv->perchar[si].kernafter;
	GDrawDrawLine(pixmap,x, mv->topend,x,mv->displayend,0x000080);
    }
    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old);
}

static void MVSetSubtables(MetricsView *mv) {
    GTextInfo **ti;
    SplineFont *sf;
    OTLookup *otl;
    struct lookup_subtable *sub;
    int cnt, doit;
    MetricsView *mvs;
    int selected = false;

    sf = mv->sf;
    if ( sf->cidmaster ) sf = sf->cidmaster;

    /* There might be more than one metricsview wandering around. Update them all */
    for ( mvs = sf->metrics; mvs!=NULL; mvs=mvs->next ) {
	for ( doit = 0; doit<2; ++doit ) {
	    cnt = 0;
	    for ( otl=sf->gpos_lookups; otl!=NULL; otl=otl->next ) {
		if ( otl->lookup_type == gpos_pair && FeatureTagInFeatureScriptList(
			    mvs->vertical?CHR('v','k','r','n') : CHR('k','e','r','n'),
			    otl->features)) {
		    for ( sub=otl->subtables; sub!=NULL; sub=sub->next ) {
			if ( doit ) {
			    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
			    ti[cnt]->text = utf82u_copy(sub->subtable_name);
			    ti[cnt]->userdata = sub;
			    if ( sub==mvs->cur_subtable )
				ti[cnt]->selected = selected = true;
			    ti[cnt]->disabled = sub->kc!=NULL;
			    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
			}
			++cnt;
		    }
		}
	    }
	    if ( !doit )
		ti = gcalloc(cnt+3,sizeof(GTextInfo *));
	    else {
		if ( cnt!=0 ) {
		    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
		    ti[cnt]->line = true;
		    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		    ++cnt;
		}
		ti[cnt] = gcalloc(1,sizeof(GTextInfo));
		ti[cnt]->text = utf82u_copy(_("New Lookup Subtable..."));
		ti[cnt]->userdata = NULL;
		ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
		ti[cnt]->selected = !selected;
		++cnt;
		ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	    }
	}

	GGadgetSetList(mvs->subtable_list,ti,false);
    }
}

static void MVSetFeatures(MetricsView *mv) {
    SplineFont *sf = mv->sf;
    int i, j, cnt;
    GTextInfo **ti=NULL;
    uint32 *tags = NULL, script, lang;
    char buf[8];
    uint32 *stds;
    const unichar_t *pt = _GGadgetGetTitle(mv->script);

    script = DEFAULT_SCRIPT;
    lang = DEFAULT_LANG;
    if ( u_strlen(pt)>=4 )
	script = (pt[0]<<24) | (pt[1]<<16) | (pt[2]<<8) | pt[3];
    if ( pt[4]=='{' && u_strlen(pt)>=9 )
	lang = (pt[5]<<24) | (pt[6]<<16) | (pt[7]<<8) | pt[8];
    stds = StdFeaturesOfScript(script);

    tags = SFFeaturesInScriptLang(sf,-1,script,lang);
    /* Never returns NULL */
    for ( cnt=0; tags[cnt]!=0; ++cnt );

    /*qsort(tags,cnt,sizeof(uint32),tag_comp);*/ /* The glist will do this for us */

    ti = galloc((cnt+2)*sizeof(GTextInfo *));
    for ( i=0; i<cnt; ++i ) {
	ti[i] = gcalloc( 1,sizeof(GTextInfo));
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	buf[0] = tags[i]>>24; buf[1] = tags[i]>>16; buf[2] = tags[i]>>8; buf[3] = tags[i]; buf[4] = 0;
	ti[i]->text = uc_copy(buf);
	for ( j=0; stds[j]!=0; ++j ) {
	    if ( stds[j] == tags[i] ) {
		ti[i]->selected = true;
	break;
	    }
	}
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(mv->features,ti,false);
}

static void MVSelectSubtable(MetricsView *mv, struct lookup_subtable *sub) {
    int32 len; int i;
    GTextInfo **old = GGadgetGetList(mv->subtable_list,&len);

    for ( i=0; i<len && (old[i]->userdata!=sub || old[i]->line); ++i );
    GGadgetSelectOneListItem(mv->subtable_list,i);
    if ( sub!=NULL )
	mv->cur_subtable = sub;
}

static void MVRedrawI(MetricsView *mv,int i,int oldxmin,int oldxmax) {
    GRect r;
    BDFChar *bdfc;
    int off = 0;

    if ( mv->right_to_left || mv->vertical ) {
	/* right to left clipping is hard to think about, it doesn't happen */
	/*  often enough (I think) for me to put the effort to make it efficient */
	GDrawRequestExpose(mv->gw,NULL,false);
return;
    }
    if ( mv->perchar[i].selected )
	off = mv->activeoff;
    r.y = mv->topend; r.height = mv->displayend-mv->topend;
    r.x = mv->perchar[i].dx-mv->xoff; r.width = mv->perchar[i].dwidth;
    if ( mv->perchar[i].kernafter>0 )
	r.width += mv->perchar[i].kernafter;
    if ( mv->perchar[i].xoff<0 ) {
	r.x += mv->perchar[i].xoff;
	r.width -= mv->perchar[i].xoff;
    } else
	r.width += mv->perchar[i].xoff;
    bdfc = mv->bdf==NULL ?  BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos) :
			    mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos];
    if ( bdfc==NULL )
return;
    if ( bdfc->xmax+off+1>r.width ) r.width = bdfc->xmax+off+1;
    if ( oldxmax+1>r.width ) r.width = oldxmax+1;
    if ( bdfc->xmin+off<0 ) {
	r.x += bdfc->xmin+off;
	r.width -= bdfc->xmin+off;
    }
    if ( oldxmin<bdfc->xmin ) {
	r.width += (bdfc->xmin+off-oldxmin);
	r.x -= (bdfc->xmin+off-oldxmin);
    }
    if ( mv->right_to_left )
	r.x = mv->dwidth - r.x - r.width;
    GDrawRequestExpose(mv->gw,&r,false);
    if ( mv->perchar[i].selected && i!=0 ) {
	struct lookup_subtable *sub = mv->glyphs[i].kp!=NULL ? mv->glyphs[i].kp->subtable : mv->glyphs[i].kc!=NULL ? mv->glyphs[i].kc->subtable : NULL;
	if ( sub!=NULL )
	    MVSelectSubtable(mv,sub);
    }
}

static void MVDeselectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = false;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,mv->bdf==NULL);
    MVRedrawI(mv,i,0,0);
}

static void MVSelectSubtableForScript(MetricsView *mv,uint32 script) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(mv->subtable_list,&len);
    struct lookup_subtable *sub;
    int i;

    sub = NULL;
    for ( i=0; i<len; ++i )
	if ( ti[i]->userdata!=NULL &&
		FeatureScriptTagInFeatureScriptList(
		    mv->vertical?CHR('v','k','r','n') : CHR('k','e','r','n'),
		    script,((struct lookup_subtable *) (ti[i]->userdata))->lookup->features)) {
	    sub = ti[i]->userdata;
    break;
	}
    if ( sub!=NULL )
	MVSelectSubtable(mv,sub);
}

static void MVSelectChar(MetricsView *mv, int i) {

    mv->perchar[i].selected = true;
    if ( mv->perchar[i].name!=NULL )
	GGadgetSetEnabled(mv->perchar[i].name,false);
    if ( mv->glyphs[i].kp!=NULL )
	MVSelectSubtable(mv,mv->glyphs[i].kp->subtable);
    else if ( mv->glyphs[i].kc!=NULL )
	MVSelectSubtable(mv,mv->glyphs[i].kc->subtable);
    else
	MVSelectSubtableForScript(mv,SCScriptFromUnicode(mv->glyphs[i].sc));
    MVRedrawI(mv,i,0,0);
}
    
static void MVDoSelect(MetricsView *mv, int i) {
    int j;

    for ( j=0; j<mv->glyphcnt; ++j )
	if ( j!=i && mv->perchar[j].selected )
	    MVDeselectChar(mv,j);
    MVSelectChar(mv,i);
}

void MVRefreshChar(MetricsView *mv, SplineChar *sc) {
    int i;
    for ( i=0; i<mv->glyphcnt; ++i ) if ( mv->glyphs[i].sc == sc )
	MVRedrawI(mv,i,0,0);
}

static void MVRefreshValues(MetricsView *mv, int i) {
    char buf[40];
    DBounds bb;
    SplineChar *sc = mv->glyphs[i].sc;
    int kern_offset;

    SplineCharFindBounds(sc,&bb);

    GGadgetSetTitle8(mv->perchar[i].name,sc->name);

    sprintf(buf,"%d",mv->vertical ? sc->vwidth : sc->width);
    GGadgetSetTitle8(mv->perchar[i].width,buf);

    sprintf(buf,"%.2f",mv->vertical ? sc->parent->ascent-(double) bb.maxy : (double) bb.minx);
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    GGadgetSetTitle8(mv->perchar[i].lbearing,buf);

    sprintf(buf,"%.2f",(double) (mv->vertical ? sc->vwidth-(sc->parent->ascent-bb.miny) : sc->width-bb.maxx));
    if ( buf[strlen(buf)-1]=='0' ) {
	buf[strlen(buf)-1] = '\0';
	if ( buf[strlen(buf)-1]=='0' ) {
	    buf[strlen(buf)-1] = '\0';
	    if ( buf[strlen(buf)-1]=='.' )
		buf[strlen(buf)-1] = '\0';
	}
    }
    GGadgetSetTitle8(mv->perchar[i].rbearing,buf);

    kern_offset = 0x7ffffff;
    if ( mv->glyphs[i].kp!=NULL )
	kern_offset = mv->glyphs[i].kp->off;
    else if ( mv->glyphs[i].kc!=NULL )
	kern_offset = mv->glyphs[i].kc->offsets[ mv->glyphs[i].kc_index ];
    if ( kern_offset!=0x7ffffff && i!=mv->glyphcnt-1 ) {
	sprintf(buf,"%d",kern_offset);
	GGadgetSetTitle8(mv->perchar[i+1].kern,buf);
    } else if ( i!=mv->glyphcnt-1 )
	GGadgetSetTitle8(mv->perchar[i+1].kern,"");
}

static void MVMakeLabels(MetricsView *mv) {
    static GBox small = { 0 };
    GGadgetData gd;
    GTextInfo label;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));

    mv->mwidth = GGadgetScale(60);
    mv->displayend = mv->height- mv->sbh - 5*(mv->fh+4);
    mv->pixelsize = mv_scales[mv->scale_index]*(mv->displayend - mv->topend - 4);

    label.text = (unichar_t *) _("Name:");
    label.text_is_1byte = true;
    label.font = mv->font;
    gd.pos.x = 2; gd.pos.width = mv->mwidth-4;
    gd.pos.y = mv->displayend+2;
    gd.pos.height = mv->fh;
    gd.label = &label;
    gd.box = &small;
    gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_dontcopybox;
    mv->namelab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) (mv->vertical ? _("Height:") : _("Width:") );
    gd.pos.y += mv->fh+4;
    mv->widthlab = GLabelCreate(mv->gw,&gd,NULL);

/* GT: Top/Left (side) bearing */
    label.text = (unichar_t *) (mv->vertical ? _("TBearing:") : _("LBearing:") );
    gd.pos.y += mv->fh+4;
    mv->lbearinglab = GLabelCreate(mv->gw,&gd,NULL);

/* GT: Bottom/Right (side) bearing */
    label.text = (unichar_t *) (mv->vertical ? _("BBearing:") : _("RBearing:") );
    gd.pos.y += mv->fh+4;
    mv->rbearinglab = GLabelCreate(mv->gw,&gd,NULL);

    label.text = (unichar_t *) (mv->vertical ? _("VKern:") : _("Kern:"));
    gd.pos.y += mv->fh+4;
    mv->kernlab = GLabelCreate(mv->gw,&gd,NULL);
}

static int MV_KernChanged(GGadget *g, GEvent *e);
static int MV_RBearingChanged(GGadget *g, GEvent *e);
static int MV_LBearingChanged(GGadget *g, GEvent *e);
static int MV_WidthChanged(GGadget *g, GEvent *e);

static void MVCreateFields(MetricsView *mv,int i) {
    static GBox small = { 0 };
    GGadgetData gd;
    GTextInfo label;
    static unichar_t nullstr[1] = { 0 };
    int j;

    small.main_background = small.main_foreground = COLOR_DEFAULT;
    small.disabled_foreground = 0x808080;
    small.disabled_background = COLOR_DEFAULT;

    memset(&gd,'\0',sizeof(gd));
    memset(&label,'\0',sizeof(label));
    label.text = nullstr;
    label.font = mv->font;
    mv->perchar[i].mx = gd.pos.x = mv->mbase+(i+1-mv->coff)*mv->mwidth+2;
    mv->perchar[i].mwidth = gd.pos.width = mv->mwidth-4;
    gd.pos.y = mv->displayend+2;
    gd.pos.height = mv->fh;
    gd.label = &label;
    gd.box = &small;
    gd.flags = gg_visible | gg_pos_in_pixels | gg_dontcopybox;
    if ( mv->bdf==NULL )
	gd.flags |= gg_enabled;
    mv->perchar[i].name = GLabelCreate(mv->gw,&gd,(void *) (intpt) i);
    if ( mv->perchar[i].selected )
	GGadgetSetEnabled(mv->perchar[i].name,false);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_WidthChanged;
    mv->perchar[i].width = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_LBearingChanged;
    mv->perchar[i].lbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    gd.pos.y += mv->fh+4;
    gd.handle_controlevent = MV_RBearingChanged;
    mv->perchar[i].rbearing = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

    if ( i!=0 ) {
	gd.pos.y += mv->fh+4;
	gd.pos.x -= mv->mwidth/2;
	gd.handle_controlevent = MV_KernChanged;
	mv->perchar[i].kern = GTextFieldCreate(mv->gw,&gd,(void *) (intpt) i);

	if ( i>=mv->glyphcnt ) {
	    for ( j=mv->glyphcnt+1; j<=i ; ++ j )
		mv->perchar[j].dx = mv->perchar[j-1].dx;
	    mv->glyphcnt = i+1;
	}
    }

    GWidgetIndicateFocusGadget(mv->text);
}

static void MVSetSb(MetricsView *mv);
static int MVSetVSb(MetricsView *mv);

static void MVRemetric(MetricsView *mv) {
    SplineChar *anysc, *goodsc;
    int i, cnt, x, y, goodpos;
    const unichar_t *_script = _GGadgetGetTitle(mv->script);
    uint32 script, lang, *feats;
    char buf[20];
    int32 len;
    GTextInfo **ti;
    double scale = mv->pixelsize/(double) (mv->sf->ascent+mv->sf->descent);
    SplineChar *sc;
    BDFChar *bdfc;

    anysc = goodsc = NULL; goodpos = -1;
    for ( i=0; mv->chars[i]; ++i ) {
	if ( anysc==NULL ) anysc = mv->chars[i];
	if ( SCScriptFromUnicode(mv->chars[i])!=DEFAULT_SCRIPT ) {
	    goodsc = mv->chars[i];
	    goodpos = i;
    break;
	}
    }
    if ( _script[0]=='D' && _script[1]=='F' && _script[2]=='L' && _script[3]=='T' ) {
	if ( goodsc!=NULL ) {
	    /* Set the script */ /* Remember if we get here the script is DFLT */
	    script = SCScriptFromUnicode(goodsc);
	    buf[0] = script>>24; buf[1] = script>>16; buf[2] = script>>8; buf[3] = script;
	    strcpy(buf+4,"{dflt}");
	    GGadgetSetTitle8(mv->script,buf);
	    MVSelectSubtableForScript(mv,script);
	    MVSetFeatures(mv);
	}
    } else {
	if ( anysc==NULL ) {
	    /* If we get here the script is not DFLT */
	    GGadgetSetTitle8(mv->script,"DFLT{dflt}");
	    MVSetFeatures(mv);
	}
    }
    _script = _GGadgetGetTitle(mv->script);
    script = DEFAULT_SCRIPT; lang = DEFAULT_LANG;
    if ( u_strlen(_script)>=4 && (u_strchr(_script,'{')==NULL || u_strchr(_script,'{')-_script>=4)) {
	unichar_t *pt;
	script = (_script[0]<<24) | (_script[1]<<16) | (_script[2]<<8) | _script[3];
	if ( (pt = u_strchr(_script,'{'))!=NULL && u_strlen(pt+1)>=4 &&
		(u_strchr(pt+1,'}')==NULL || u_strchr(pt+1,'}')-(pt+1)>=4 ))
	    lang = (pt[1]<<24) | (pt[2]<<16) | (pt[3]<<8) | pt[4];
    }

    ti = GGadgetGetList(mv->features,&len);
    for ( i=cnt=0; i<len; ++i )
	if ( ti[i]->selected ) ++cnt;
    feats = gcalloc(cnt+1,sizeof(uint32));
    for ( i=cnt=0; i<len; ++i )
	if ( ti[i]->selected )
	    feats[cnt++] = (ti[i]->text[0]<<24) | (ti[i]->text[1]<<16) | (ti[i]->text[2]<<8) | ti[i]->text[3];

    free(mv->glyphs);
    mv->glyphs = ApplyTickedFeatures(mv->sf,feats,script, lang, mv->chars);
    free(feats);
    if ( goodsc!=NULL )
	mv->right_to_left = SCRightToLeft(goodsc)?1:0;

    for ( cnt=0; mv->glyphs[cnt].sc!=NULL; ++cnt );
    if ( cnt>=mv->max ) {
	int oldmax=mv->max;
	mv->max = cnt+10;
	mv->perchar = grealloc(mv->perchar,mv->max*sizeof(struct metricchar));
	memset(mv->perchar+oldmax,'\0',(mv->max-oldmax)*sizeof(struct metricchar));
    }
    for ( i=cnt; i<mv->glyphcnt; ++i ) {
	static unichar_t nullstr[] = { 0 };
	GGadgetSetTitle(mv->perchar[i].name,nullstr);
	GGadgetSetTitle(mv->perchar[i].width,nullstr);
	GGadgetSetTitle(mv->perchar[i].lbearing,nullstr);
	GGadgetSetTitle(mv->perchar[i].rbearing,nullstr);
	if ( mv->perchar[i].kern!=NULL )
	    GGadgetSetTitle(mv->perchar[i].kern,nullstr);
    }
    mv->glyphcnt = cnt;
    for ( i=0; i<cnt; ++i ) {
	if ( mv->perchar[i].width==NULL ) {
	    MVCreateFields(mv,i);
	}
    }
    x = mv->xstart + 10; y = mv->topend + 10;
    for ( i=0; i<cnt; ++i ) {
	MVRefreshValues(mv,i);
	sc = mv->glyphs[i].sc;
	bdfc = mv->bdf!=NULL ? mv->bdf->glyphs[sc->orig_pos] : BDFPieceMeal(mv->show,sc->orig_pos);
	mv->perchar[i].dwidth = bdfc->width;
	mv->perchar[i].dx = x;
	mv->perchar[i].xoff = rint(mv->glyphs[i].vr.xoff*scale);
	mv->perchar[i].yoff = rint(mv->glyphs[i].vr.yoff*scale);
	mv->perchar[i].kernafter = rint(mv->glyphs[i].vr.h_adv_off*scale);
	x += bdfc->width + mv->perchar[i].kernafter;

	mv->perchar[i].dheight = rint(sc->vwidth*scale);
	mv->perchar[i].dy = y;
	if ( mv->vertical ) {
	    mv->perchar[i].kernafter = rint(mv->glyphs[i].vr.v_adv_off*scale);
	    y += mv->perchar[i].dheight + mv->perchar[i].kernafter;
	}
    }
    MVSetVSb(mv);
    MVSetSb(mv);
}

void MVReKern(MetricsView *mv) {
    MVRemetric(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

void MVRegenChar(MetricsView *mv, SplineChar *sc) {
    int i;

    if ( mv->bdf==NULL ) {
	BDFCharFree(mv->show->glyphs[sc->orig_pos]);
	mv->show->glyphs[sc->orig_pos] = NULL;
    }
    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->glyphs[i].sc == sc )
    break;
    }
    if ( i>=mv->glyphcnt )
return;		/* Not displayed */
    MVRemetric(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVChangeDisplayFont(MetricsView *mv, BDFFont *bdf) {
    int i;

    if ( mv->bdf==bdf )
return;
    if ( (mv->bdf==NULL) != (bdf==NULL) ) {
	for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	    GGadgetSetEnabled(mv->perchar[i].width,bdf==NULL);
	    GGadgetSetEnabled(mv->perchar[i].lbearing,bdf==NULL);
	    GGadgetSetEnabled(mv->perchar[i].rbearing,bdf==NULL);
	    if ( i!=0 )
		GGadgetSetEnabled(mv->perchar[i].kern,bdf==NULL);
	}
    }
    if ( mv->bdf==NULL ) {
	BDFFontFree(mv->show);
	mv->show = NULL;
    } else if ( bdf==NULL ) {
	BDFFontFree(mv->show);
	mv->show = SplineFontPieceMeal(mv->sf,mv->pixelsize,mv->antialias?pf_antialias:0,NULL);
    }
    MVRemetric(mv);
}

static int MV_WidthChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->glyphs[which].sc;
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=sc->width ) {
	    SCPreserveWidth(sc);
	    SCSynchronizeWidth(sc,val,sc->width,mv->fv);
	    SCCharChangedUpdate(sc);
	} else if ( mv->vertical && val!=sc->vwidth ) {
	    SCPreserveVWidth(sc);
	    sc->vwidth = val;
	    SCCharChangedUpdate(sc);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_LBearingChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->glyphs[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=bb.minx ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = val-bb.minx;
	    FVTrans(mv->fv,sc,transform,NULL,false);
	} else if ( mv->vertical && val!=sc->parent->ascent-bb.maxy ) {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = sc->parent->ascent-bb.maxy-val;
	    FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int MV_RBearingChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);
	SplineChar *sc = mv->glyphs[which].sc;
	DBounds bb;
	SplineCharFindBounds(sc,&bb);
	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else if ( !mv->vertical && val!=sc->width-bb.maxx ) {
	    SCPreserveWidth(sc);
	    sc->width = rint(bb.maxx+val);
	    /* Width is an integer. Adjust the lbearing so that the rbearing */
	    /*  remains what was just typed in */
	    if ( sc->width!=bb.maxx+val ) {
		real transform[6];
		transform[0] = transform[3] = 1.0;
		transform[1] = transform[2] = transform[5] = 0;
		transform[4] = sc->width-val-bb.maxx;
		FVTrans(mv->fv,sc,transform,NULL,false);
	    }
	    SCCharChangedUpdate(sc);
	} else if ( mv->vertical && val!=sc->vwidth-(sc->parent->ascent-bb.miny) ) {
	    double vw = val+(sc->parent->ascent-bb.miny);
	    SCPreserveWidth(sc);
	    sc->vwidth = rint(vw);
	    /* Width is an integer. Adjust the lbearing so that the rbearing */
	    /*  remains what was just typed in */
	    if ( sc->width!=vw ) {
		real transform[6];
		transform[0] = transform[3] = 1.0;
		transform[1] = transform[2] = transform[4] = 0;
		transform[5] = vw-sc->vwidth;
		FVTrans(mv->fv,sc,transform,NULL,false);
	    }
	    SCCharChangedUpdate(sc);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static int AskNewKernClassEntry(SplineChar *fsc,SplineChar *lsc) {
    char *yesno[3];
#if defined(FONTFORGE_CONFIG_GDRAW)
    yesno[0] = _("_Yes");
    yesno[1] = _("_No");
#elif defined(FONTFORGE_CONFIG_GTK)
    yesno[0] = GTK_STOCK_YES;
    yesno[1] = GTK_STOCK_NO;
#endif
    yesno[2] = NULL;
return( gwwv_ask(_("Use Kerning Class?"),(const char **) yesno,0,1,_("This kerning pair (%.20s and %.20s) is currently part of a kerning class with a 0 offset for this combination. Would you like to alter this kerning class entry (or create a kerning pair for just these two glyphs)?"),
	fsc->name,lsc->name)==0 );
}

static int MV_ChangeKerning(MetricsView *mv, int which, int offset, int is_diff) {
    SplineChar *sc = mv->glyphs[which].sc;
    SplineChar *psc = mv->glyphs[which-1].sc;
    KernPair *kp;
    KernClass *kc; int index;
    int i;
    struct lookup_subtable *sub = GGadgetGetListItemSelected(mv->subtable_list)->userdata;

    kp = mv->glyphs[which-1].kp;
    kc = mv->glyphs[which-1].kc;
    index = mv->glyphs[which-1].kc_index;
    if ( kc!=NULL ) {
	if ( index==-1 )
	    kc = NULL;
	else if ( kc->offsets[index]==0 && !AskNewKernClassEntry(psc,sc))
	    kc=NULL;
	else
	    offset = kc->offsets[index] = is_diff ? kc->offsets[index]+offset : offset;
    }
    if ( kc==NULL ) {
	if ( sub!=NULL && sub->kc!=NULL ) {
	    /* If the subtable we were given contains a kern class, and for some reason */
	    /*  we can't, or don't want to, use that kern class, then see */
	    /*  if the lookup contains another subtable with no kern classes */
	    /*  and use that */
	    struct lookup_subtable *s;
	    for ( s = sub->lookup->subtables; s!=NULL && s->kc!=NULL; s=s->next );
	    sub = s;
	}
	if ( sub==NULL ) {
	    struct subtable_data sd;
	    memset(&sd,0,sizeof(sd));
	    sd.flags = (mv->vertical ? sdf_verticalkern : sdf_horizontalkern ) |
		    sdf_kernpair;
	    sub = SFNewLookupSubtableOfType(psc->parent,gpos_pair,&sd);
	    if ( sub==NULL )
return( false );
	    mv->cur_subtable = sub;
	    MVSetSubtables(mv);
	}

	if ( kp==NULL ) {
	    kp = chunkalloc(sizeof(KernPair));
	    kp->sc = sc;
	    if ( !mv->vertical ) {
		kp->next = psc->kerns;
		psc->kerns = kp;
	    } else {
		kp->next = psc->vkerns;
		psc->vkerns = kp;
	    }
	    mv->glyphs[which-1].kp = kp;
	}
	if ( !mv->vertical )
	    MMKern(sc->parent,psc,sc,kp==NULL?offset:is_diff?offset:offset-kp->off,
		    sub,kp);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	/* If we change the kerning offset, then any pixel corrections*/
	/*  will no longer apply (they only had meaning with the old  */
	/*  offset) so free the device table, if any */
	if ( (!is_diff && kp->off!=offset) || ( is_diff && offset!=0) ) {
	    DeviceTableFree(kp->adjust);
	    kp->adjust = NULL;
	}
#endif
	offset = kp->off = is_diff ? kp->off+offset : offset;
	kp->subtable = sub;
    }
    mv->perchar[which-1].kernafter = (offset*mv->pixelsize)/
	    (mv->sf->ascent+mv->sf->descent);
    if ( mv->vertical ) {
	for ( i=which; i<mv->glyphcnt; ++i ) {
	    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
		    mv->perchar[i-1].kernafter ;
	}
    } else {
	for ( i=which; i<mv->glyphcnt; ++i ) {
	    mv->perchar[i].dx = mv->perchar[i-1].dx + mv->perchar[i-1].dwidth +
		    mv->perchar[i-1].kernafter;
	}
    }
    mv->sf->changed = true;
    GDrawRequestExpose(mv->gw,NULL,false);
return( true );
}

static int MV_KernChanged(GGadget *g, GEvent *e) {
    MetricsView *mv = GDrawGetUserData(GGadgetGetWindow(g));
    int which = (intpt) GGadgetGetUserData(g);
    int i;

    if ( e->type!=et_controlevent )
return( true );
    if ( e->u.control.subtype == et_textchanged ) {
	unichar_t *end;
	int val = u_strtol(_GGadgetGetTitle(g),&end,10);

	if ( *end && !(*end=='-' && end[1]=='\0'))
	    GDrawBeep(NULL);
	else {
	    MV_ChangeKerning(mv,which,val, false);
	}
    } else if ( e->u.control.subtype == et_textfocuschanged &&
	    e->u.control.u.tf_focus.gained_focus ) {
	for ( i=0 ; i<mv->glyphcnt; ++i )
	    if ( i!=which && mv->perchar[i].selected )
		MVDeselectChar(mv,i);
	MVSelectChar(mv,which);
    }
return( true );
}

static void MVToggleVertical(MetricsView *mv) {
    int size;

    mv->vertical = !mv->vertical;

    GGadgetSetTitle8( mv->widthlab, mv->vertical ? "Height:" : "Width:" );
    GGadgetSetTitle8( mv->lbearinglab, mv->vertical ? "TBearing:" : "LBearing:" );
    GGadgetSetTitle8( mv->rbearinglab, mv->vertical ? "BBearing:" : "RBearing:" );
    GGadgetSetTitle8( mv->kernlab, mv->vertical ? "VKern:" : "Kern:" );

    if ( mv->vertical )
	if ( mv->scale_index<4 ) mv->scale_index = 4;

    size = (mv->displayend - mv->topend - 4);
    if ( mv->dwidth-20<size )
	size = mv->dwidth-20;
    size *= mv_scales[mv->scale_index];
    if ( mv->pixelsize != size ) {
	mv->pixelsize = size;
	if ( mv->bdf==NULL ) {
	    BDFFontFree(mv->show);
	    mv->show = SplineFontPieceMeal(mv->sf,mv->pixelsize,mv->antialias?pf_antialias:0,NULL);
	}
	MVRemetric(mv);
    }
}

static SplineChar *SCFromUnicode(SplineFont *sf, EncMap *map, int ch,BDFFont *bdf) {
    int i = SFFindSlot(sf,map,ch,NULL);
    SplineChar *sc;

    if ( i==-1 )
return( NULL );
    else {
	sc = SFMakeChar(sf,map,i);
	if ( bdf!=NULL )
	    BDFMakeChar(bdf,map,i);
    }
return( sc );
}

static void MVMoveFieldsBy(MetricsView *mv,int diff) {
    int i;
    int y,x;

    for ( i=0; i<mv->max && mv->perchar[i].width!=NULL; ++i ) {
	y = mv->displayend+2;
	x = mv->perchar[i].mx-diff;
	if ( x<mv->mbase+mv->mwidth ) x = -2*mv->mwidth;
	GGadgetMove(mv->perchar[i].name,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].width,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].lbearing,x,y);
	y += mv->fh+4;
	GGadgetMove(mv->perchar[i].rbearing,x,y);
	y += mv->fh+4;
	if ( i!=0 )
	    GGadgetMove(mv->perchar[i].kern,x-mv->mwidth/2,y);
    }
}

static int MVDisplayedCnt(MetricsView *mv) {
    int i, wid = mv->mbase;

    for ( i=mv->coff; i<mv->glyphcnt; ++i ) {
	wid += mv->perchar[i].dwidth;
	if ( wid>mv->dwidth )
return( i-mv->coff );
    }
return( i-mv->coff );		/* There's extra room. don't know exactly how much but allow for some */
}

static void MVSetSb(MetricsView *mv) {
    int cnt = (mv->dwidth-mv->mbase-mv->mwidth)/mv->mwidth;
    int dcnt = MVDisplayedCnt(mv);

    if ( cnt>dcnt ) cnt = dcnt;
    if ( cnt==0 ) cnt = 1;

    GScrollBarSetBounds(mv->hsb,0,mv->glyphcnt,cnt);
    GScrollBarSetPos(mv->hsb,mv->coff);
}

static int MVSetVSb(MetricsView *mv) {
    int max, min, i, ret, ybase, yoff;

    if ( mv->displayend==0 )
return(0);		/* Setting the scroll bar is premature */

    if ( mv->vertical ) {
	min = max = 0;
	if ( mv->glyphcnt!=0 )
	    max = mv->perchar[mv->glyphcnt-1].dy + mv->perchar[mv->glyphcnt-1].dheight;
    } else {
	SplineFont *sf = mv->sf;
	ybase = 2 + (mv->pixelsize/mv_scales[mv->scale_index] * sf->ascent / (sf->ascent+sf->descent));
	min = -ybase;
	max = mv->displayend-mv->topend-ybase;
	for ( i=0; i<mv->glyphcnt; ++i ) {
	    BDFChar *bdfc = mv->bdf==NULL ? BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos) :
				mv->bdf->glyphs[mv->glyphs[i].sc->orig_pos];
	    if ( bdfc!=NULL ) {
		if ( min>-bdfc->ymax ) min = -bdfc->ymax;
		if ( max<-bdfc->ymin ) max = -bdfc->ymin;
	    }
	}
	min += ybase;
	max += ybase;
    }
    min -= 10;
    max += 10;
    GScrollBarSetBounds(mv->vsb,min,max,mv->displayend-mv->topend);
    yoff = mv->yoff;
    if ( yoff+mv->displayend-mv->topend > max )
	yoff = max - (mv->displayend-mv->topend);
    if ( yoff<min ) yoff = min;
    ret = yoff!=mv->yoff;
    mv->yoff = yoff;
    GScrollBarSetPos(mv->vsb,yoff);
return( ret );
}

static void MVHScroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->coff;
    int cnt = (mv->dwidth-mv->mbase-mv->mwidth)/mv->mwidth;
    int dcnt = MVDisplayedCnt(mv);

    if ( cnt>dcnt ) cnt = dcnt;
    if ( cnt==0 ) cnt = 1;

    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= cnt;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += cnt;
      break;
      case et_sb_bottom:
        newpos = mv->glyphcnt-cnt;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>mv->glyphcnt-cnt )
        newpos = mv->glyphcnt-cnt;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=mv->coff ) {
	int old = mv->coff;
	int diff = newpos-mv->coff;
	int charsize = mv->perchar[newpos].dx-mv->perchar[old].dx;
	GRect fieldrect, charrect;

	mv->coff = newpos;
	charrect.x = mv->xstart; charrect.width = mv->dwidth-mv->xstart;
	charrect.y = mv->topend; charrect.height = mv->displayend-mv->topend;
	fieldrect.x = mv->mbase+mv->mwidth; fieldrect.width = mv->width-mv->mbase;
	fieldrect.y = mv->displayend; fieldrect.height = mv->height-mv->sbh-mv->displayend;
	GScrollBarSetBounds(mv->hsb,0,mv->glyphcnt,cnt);
	GScrollBarSetPos(mv->hsb,mv->coff);
	MVMoveFieldsBy(mv,newpos*mv->mwidth);
	GDrawScroll(mv->gw,&fieldrect,-diff*mv->mwidth,0);
	mv->xoff = mv->perchar[newpos].dx-mv->perchar[0].dx;
	if ( mv->right_to_left ) {
	    charsize = -charsize;
	}
	GDrawScroll(mv->gw,&charrect,-charsize,0);
    }
}

static void MVVScroll(MetricsView *mv,struct sbevent *sb) {
    int newpos = mv->yoff;
    int32 min, max, page;

    GScrollBarGetBounds(mv->vsb,&min,&max,&page);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= page;
      break;
      case et_sb_up:
        newpos -= (page)/15;
      break;
      case et_sb_down:
        newpos += (page)/15;
      break;
      case et_sb_downpage:
        newpos += page;
      break;
      case et_sb_bottom:
        newpos = max-page;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>max-page )
        newpos = max-page;
    if ( newpos<min ) newpos = min;
    if ( newpos!=mv->yoff ) {
	int diff = newpos-mv->yoff;
	GRect charrect;

	mv->yoff = newpos;
	charrect.x = mv->xstart; charrect.width = mv->dwidth-mv->xstart;
	charrect.y = mv->topend+1; charrect.height = mv->displayend-mv->topend-1;
	GScrollBarSetPos(mv->vsb,mv->yoff);
	GDrawScroll(mv->gw,&charrect,0,diff);
    }
}

void MVSetSCs(MetricsView *mv, SplineChar **scs) {
    /* set the list of characters being displayed to those in scs */
    int len;
    unichar_t *ustr;

    for ( len=0; scs[len]!=NULL; ++len );
    if ( len>=mv->cmax )
	mv->chars = realloc(mv->chars,(mv->cmax=len+10)*sizeof(SplineChar *));
    memcpy(mv->chars,scs,(len+1)*sizeof(SplineChar *));
    mv->clen = len;

    ustr = galloc((len+1)*sizeof(unichar_t));
    for ( len=0; scs[len]!=NULL; ++len )
	if ( scs[len]->unicodeenc>0 && scs[len]->unicodeenc<0x10000 )
	    ustr[len] = scs[len]->unicodeenc;
	else
	    ustr[len] = 0xfffd;
    ustr[len] = 0;
    GGadgetSetTitle(mv->text,ustr);
    free(ustr);

    MVRemetric(mv);

    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVTextChanged(MetricsView *mv) {
    const unichar_t *ret, *pt, *ept, *tpt;
    int i,ei, j, start=0, end=0;
    int missing;
    int direction_change = false;
    SplineChar **hold = NULL;

    ret = _GGadgetGetTitle(mv->text);
    if (( isrighttoleft(ret[0]) && !mv->right_to_left ) ||
	    ( !isrighttoleft(ret[0]) && mv->right_to_left )) {
	direction_change = true;
	mv->right_to_left = !mv->right_to_left;
    }
    for ( pt=ret, i=0; i<mv->glyphcnt && *pt!='\0'; ++i, ++pt )
	if ( *pt!=mv->chars[i]->unicodeenc &&
		(*pt!=0xfffd || mv->chars[i]->unicodeenc!=-1 ))
    break;
    if ( i==mv->glyphcnt && *pt=='\0' )
return;					/* Nothing changed */
    for ( ept=ret+u_strlen(ret)-1, ei=mv->glyphcnt-1; ; --ei, --ept )
	if ( ei<0 || ept<ret || (*ept!=mv->chars[ei]->unicodeenc &&
		(*ept!=0xfffd || mv->chars[ei]->unicodeenc!=-1 ))) {
	    ++ei; ++ept;
    break;
	} else if ( ei<i || ept<pt ) {
	    ++ei; ++ept;
    break;
	}
    if ( ei==i && ept==pt )
	IError("No change when there should have been one in MV_TextChanged");
    if ( u_strlen(ret)>=mv->cmax ) {
	int oldmax=mv->cmax;
	mv->cmax = u_strlen(ret)+10;
	mv->chars = grealloc(mv->chars,mv->cmax*sizeof(SplineChar *));
	memset(mv->chars+oldmax,'\0',(mv->max-oldmax)*sizeof(SplineChar *));
    }

    missing = 0;
    for ( tpt=pt; tpt<ept; ++tpt )
	if ( SFFindSlot(mv->sf,mv->fv->map,*tpt,NULL)==-1 )
	    ++missing;

    if ( ept-pt-missing > ei-i ) {
	if ( i<mv->glyphcnt ) {
	    int diff = (ept-pt-missing) - (ei-i);
	    hold = galloc((mv->glyphcnt+diff+6)*sizeof(SplineChar *));
	    for ( j=mv->glyphcnt-1; j>=ei; --j )
		hold[j+diff] = mv->chars[j];
	    start = ei+diff; end = mv->glyphcnt+diff;
	}
    } else if ( ept-pt-missing != ei-i ) {
	int diff = (ept-pt-missing) - (ei-i);
	for ( j=ei; j<mv->glyphcnt; ++j )
	    if ( j+diff>=0 )
		mv->chars[j+diff] = mv->chars[j];
    }
    for ( j=i; pt<ept; ++pt ) {
	SplineChar *sc = SCFromUnicode(mv->sf,mv->fv->map,*pt,mv->bdf);
	if ( sc!=NULL )
	    mv->chars[j++] = sc;
    }
    if ( hold!=NULL ) {
	/* We had to figure out what sc's there were before we wrote over them*/
	/*  but we couldn't put them where they belonged until everything before*/
	/*  them was set properly */
	for ( j=start; j<end; ++j )
	    mv->chars[j] = hold[j];
	free(hold);
    }
    mv->clen = u_strlen(ret)-missing;
    mv->chars[mv->clen] = NULL;
    MVRemetric(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static int MV_TextChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	MVTextChanged(GGadgetGetUserData(g));
    }
return( true );
}

static int MV_ScriptLangChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	const unichar_t *sstr = _GGadgetGetTitle(g);
	MetricsView *mv = GGadgetGetUserData(g);
	if ( e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    GGadgetSetTitle8(g,mv->scriptlangs[e->u.control.u.tf_changed.from_pulldown].userdata );
	    sstr = _GGadgetGetTitle(g);
	} else {
	    if ( u_strlen(sstr)<4 || !isalpha(sstr[0]) || !isalnum(sstr[1]) /*|| !isalnum(sstr[2]) || !isalnum(sstr[3])*/ )
return( true );
	    if ( u_strlen(sstr)==4 )
		/* No language, we'll use default */;
	    else if ( u_strlen(sstr)!=10 || sstr[4]!='{' || sstr[9]!='}' ||
		    !isalpha(sstr[5]) || !isalpha(sstr[6]) || !isalpha(sstr[7])  )
return( true );
	}
	MVSetFeatures(mv);
	if ( mv->clen!=0 )/* if there are no chars, remetricking will set the script field to DFLT */
	    MVRemetric(mv);
	GDrawRequestExpose(mv->gw,NULL,false);
    }
return( true );
}

static int MV_FeaturesChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetricsView *mv = GGadgetGetUserData(g);
	MVRemetric(mv);
	GDrawRequestExpose(mv->gw,NULL,false);
    }
return( true );
}

static void MV_FriendlyFeatures(GGadget *g, int pos) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(g,&len);

    if ( pos<0 || pos>=len )
	GGadgetEndPopup();
    else {
	const unichar_t *pt = ti[pos]->text;
	uint32 tag;
	int i;
	tag = (pt[0]<<24) | (pt[1]<<16) | (pt[2]<<8) | pt[3];
	LookupUIInit();
	for ( i=0; friendlies[i].friendlyname!=NULL; ++i )
	    if ( friendlies[i].tag==tag )
	break;
	if ( friendlies[i].friendlyname!=NULL )
	    GGadgetPreparePopup8(GGadgetGetWindow(g),friendlies[i].friendlyname);
    }
}

static int MV_SubtableChanged(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	MetricsView *mv = GGadgetGetUserData(g);
	int32 len;
	GTextInfo **ti = GGadgetGetList(g,&len);
	int i;
	KernPair *kp;
	struct lookup_subtable *sub;

	if ( ti[len-1]->selected ) {/* New lookup subtable */
	    struct subtable_data sd;
	    memset(&sd,0,sizeof(sd));
	    sd.flags = (mv->vertical ? sdf_verticalkern : sdf_horizontalkern ) |
		    sdf_kernpair | sdf_dontedit;
	    sub = SFNewLookupSubtableOfType(mv->sf,gpos_pair,&sd);
	    if ( sub==NULL )
return( true );
	    mv->cur_subtable = sub;
	    MVSetSubtables(mv);
	} else if ( ti[len-2]->selected ) {	/* Idiots. They selected the line, can't have that */
	    MVSetSubtables(mv);
	    sub = mv->cur_subtable;
	} else
	    mv->cur_subtable = GGadgetGetListItemSelected(mv->subtable_list)->userdata;

	for ( i=0; i<mv->glyphcnt; ++i ) {
	    if ( mv->perchar[i].selected )
	break;
	}
	kp = mv->glyphs[i].kp;
	if ( kp!=NULL )
	    kp->subtable = mv->cur_subtable;
    }
return( true );
}

#define MID_ZoomIn	2002
#define MID_ZoomOut	2003
#define MID_Next	2005
#define MID_Prev	2006
#define MID_Outline	2007
#define MID_ShowGrid	2008
#define MID_NextDef	2012
#define MID_PrevDef	2013
#define MID_AntiAlias	2014
#define MID_FindInFontView	2015
#define MID_Ligatures	2020
#define MID_KernPairs	2021
#define MID_AnchorPairs	2022
#define MID_Vertical	2023
#define MID_ReplaceChar	2024
#define MID_InsertCharB	2025
#define MID_InsertCharA	2026
#define MID_CharInfo	2201
#define MID_FindProblems 2216
#define MID_MetaFont	2217
#define MID_Transform	2202
#define MID_Stroke	2203
#define MID_RmOverlap	2204
#define MID_Simplify	2205
#define MID_Correct	2206
#define MID_BuildAccent	2208
#define MID_AvailBitmaps	2210
#define MID_RegenBitmaps	2211
#define MID_Autotrace	2212
#define MID_Round	2213
#define MID_ShowDependents	2222
#define MID_AddExtrema	2224
#define MID_CleanupGlyph	2225
#define MID_TilePath	2226
#define MID_BuildComposite	2227
#define MID_Intersection	2229
#define MID_FindInter	2230
#define MID_Effects	2231
#define MID_SimplifyMore	2232
#define MID_Center	2600
#define MID_OpenBitmap	2700
#define MID_OpenOutline	2701
#define MID_Display	2706
#define MID_Cut		2101
#define MID_Copy	2102
#define MID_Paste	2103
#define MID_Clear	2104
#define MID_SelAll	2106
#define MID_UnlinkRef	2108
#define MID_Undo	2109
#define MID_Redo	2110
#define MID_CopyRef	2107
#define MID_CopyWidth	2111
#define MID_CopyLBearing	2125
#define MID_CopyRBearing	2126
#define MID_CopyVWidth	2127
#define MID_Join	2128
#define MID_Center	2600
#define MID_Thirds	2604
#define MID_VKernClass	2605
#define MID_VKernFromHKern	2606
#define MID_Recent	2703

#define MID_Warnings	3000

static void MVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawDestroyWindow(gw);
}

static void MVMenuOpenBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    EncMap *map;
    int i;

    if ( mv->sf->bitmaps==NULL )
return;
    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    map = mv->fv->map;
    if ( i!=mv->glyphcnt )
	BitmapViewCreatePick(map->backmap[mv->glyphs[i].sc->orig_pos],mv->fv);
}

static void MVMenuMergeKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MergeKernInfo(mv->sf,mv->fv->map);
}

static void MVMenuOpenOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )
	CharViewCreate(mv->glyphs[i].sc,mv->fv,-1);
}

static void MVMenuSave(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSave(mv->fv);
}

static void MVMenuSaveAs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuSaveAs(mv->fv);
}

static void MVMenuGenerate(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv,false);
}

static void MVMenuGenerateFamily(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _FVMenuGenerate(mv->fv,true);
}

static void MVMenuPrint(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    PrintDlg(NULL,NULL,mv);
}

static void MVMenuDisplay(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    DisplayDlg(mv->sf);
}

static void MVUndo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_undo) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->glyphs[i].sc->layers[ly_fore].undoes!=NULL )
	    SCDoUndo(mv->glyphs[i].sc,ly_fore);
    }
}

static void MVRedo(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_redo) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	if ( mv->glyphs[i].sc->layers[ly_fore].redoes!=NULL )
	    SCDoRedo(mv->glyphs[i].sc,ly_fore);
    }
}

static void MVClear(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    BDFFont *bdf;
    extern int onlycopydisplayed;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_clear) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	sc = mv->glyphs[i].sc;
	if ( sc->dependents!=NULL ) {
	    int yes;
	    char *buts[4];
	    buts[1] = _("_Unlink");
#if defined(FONTFORGE_CONFIG_GDRAW)
	    buts[0] = _("_Yes");
	    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
	    buts[0] = GTK_STOCK_YES;
	    buts[2] = GTK_STOCK_CANCEL;
#endif
	    buts[3] = NULL;
	    yes = gwwv_ask(_("Bad Reference"),(const char **) buts,1,2,_("You are attempting to clear %.30s which is referred to by\nanother character. Are you sure you want to clear it?"),sc->name);
	    if ( yes==2 )
return;
	    if ( yes==1 )
		UnlinkThisReference(NULL,sc);
	}

	if ( onlycopydisplayed && mv->bdf==NULL ) {
	    SCClearAll(sc);
	} else if ( onlycopydisplayed ) {
	    BCClearAll(mv->bdf->glyphs[sc->orig_pos]);
	} else {
	    SCClearAll(sc);
	    for ( bdf=mv->sf->bitmaps; bdf!=NULL; bdf = bdf->next )
		BCClearAll(bdf->glyphs[sc->orig_pos]);
	}
    }
}

static void MVCut(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_cut) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	MVCopyChar(mv,mv->glyphs[i].sc,ct_fullcopy);
	MVClear(gw,mi,e);
    }
}

static void MVCopy(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_copy) )
	/* MVTextChanged(mv) */;
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	MVCopyChar(mv,mv->glyphs[i].sc,ct_fullcopy);
    }
}

static void MVMenuCopyRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    MVCopyChar(mv,mv->glyphs[i].sc,ct_reference);
}

static void MVMenuCopyWidth(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCCopyWidth(mv->glyphs[i].sc,
		   mi->mid==MID_CopyWidth?ut_width:
		   mi->mid==MID_CopyVWidth?ut_vwidth:
		   mi->mid==MID_CopyLBearing?ut_lbearing:
					 ut_rbearing);
}

static void MVMenuJoin(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, changed;
    extern float joinsnap;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
return;
    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    SCPreserveState(mv->glyphs[i].sc,false);
    mv->glyphs[i].sc->layers[ly_fore].splines =
	    SplineSetJoin(mv->glyphs[i].sc->layers[ly_fore].splines,true,joinsnap,&changed);
    if ( changed )
	SCCharChangedUpdate(mv->glyphs[i].sc);
}

static void MVPaste(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GGadgetActiveGadgetEditCmd(mv->gw,ec_paste) )
	/*MVTextChanged(mv)*/;		/* Should get an event now */
    else {
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;
	if ( i==-1 )
return;
	PasteIntoMV(mv,mv->glyphs[i].sc,true);
    }
}

static void MVUnlinkRef(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    RefChar *rf, *next;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 )
return;
    sc = mv->glyphs[i].sc;
    SCPreserveState(sc,false);
    for ( rf=sc->layers[ly_fore].refs; rf!=NULL ; rf=next ) {
	next = rf->next;
	SCRefToSplines(sc,rf);
    }
    SCCharChangedUpdate(sc);
}

static void MVSelectAll(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    GGadgetActiveGadgetEditCmd(mv->gw,ec_selectall);
}

static void MVMenuFontInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    DelayEvent(FontMenuFontInfo,mv->fv);
}

static void MVMenuCharInfo(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCCharInfo(mv->glyphs[i].sc,mv->fv->map,-1);
}

static void MVMenuShowDependents(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
return;
    if ( mv->glyphs[i].sc->dependents==NULL )
return;
    SCRefBy(mv->glyphs[i].sc);
}

static void MVMenuFindProblems(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	FindProblems(mv->fv,NULL,mv->glyphs[i].sc);
}

static void MVMenuBitmaps(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )
	BitmapDlg(mv->fv,mv->glyphs[i].sc,mi->mid==MID_AvailBitmaps );
    else if ( mi->mid==MID_AvailBitmaps )
	BitmapDlg(mv->fv,NULL,true );
}

static int getorigin(void *d,BasePoint *base,int index) {
    SplineChar *sc = (SplineChar *) d;
    DBounds bb;

    base->x = base->y = 0;
    switch ( index ) {
      case 0:		/* Character origin */
	/* all done */
      break;
      case 1:		/* Center of selection */
	SplineCharFindBounds(sc,&bb);
	base->x = (bb.minx+bb.maxx)/2;
	base->y = (bb.miny+bb.maxy)/2;
      break;
      default:
return( false );
    }
return( true );
}

static void MVTransFunc(void *_sc,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags flags ) {
    SplineChar *sc = _sc;

    FVTrans(sc->parent->fv,sc,transform, NULL,flags);
}

static void MVMenuTransform(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	TransformDlgCreate( mv->glyphs[i].sc,MVTransFunc,getorigin,true,cvt_none );
}

static void MVMenuStroke(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCStroke(mv->glyphs[i].sc);
}

#ifdef FONTFORGE_CONFIG_TILEPATH
static void MVMenuTilePath(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCTile(mv->glyphs[i].sc);
}
#endif

static void _MVMenuOverlap(MetricsView *mv,enum overlap_type ot) {
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	if ( !SCRoundToCluster(sc,-2,false,.03,.12))
	    SCPreserveState(sc,false);
	MinimumDistancesFree(sc->md);
	sc->md = NULL;
	sc->layers[ly_fore].splines = SplineSetRemoveOverlap(sc,sc->layers[ly_fore].splines,ot);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuOverlap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuOverlap(mv,mi->mid==MID_RmOverlap ? over_remove :
		      mi->mid==MID_Intersection ? over_intersect :
			   over_findinter);
}

static void MVMenuInline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,NULL,mv,true);
}

static void MVMenuOutline(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    OutlineDlg(NULL,NULL,mv,false);
}

static void MVMenuShadow(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,NULL,mv,false);
}

static void MVMenuWireframe(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShadowDlg(NULL,NULL,mv,true);
}

static void MVSimplify( MetricsView *mv,int type ) {
    int i;
    static struct simplifyinfo smpls[] = {
	    { sf_normal },
	    { sf_normal,.75,.05,0,-1 },
	    { sf_normal,.75,.05,0,-1 }};
    struct simplifyinfo *smpl = &smpls[type+1];

    if ( smpl->linelenmax==-1 ) {
	smpl->err = (mv->sf->ascent+mv->sf->descent)/1000.;
	smpl->linelenmax = (mv->sf->ascent+mv->sf->descent)/100.;
    }

    if ( type==1 ) {
	if ( !SimplifyDlg(mv->sf,smpl))
return;
	if ( smpl->set_as_default )
	    smpls[1] = *smpl;
    }

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveState(sc,false);
	sc->layers[ly_fore].splines = SplineCharSimplify(sc,sc->layers[ly_fore].splines,smpl);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuSimplify(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,false);
}

static void MVMenuSimplifyMore(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,true);
}

static void MVMenuCleanup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    MVSimplify(mv,-1);
}

static void MVMenuAddExtrema(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineFont *sf = mv->sf;
    int emsize = sf->ascent+sf->descent;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	SCPreserveState(sc,false);
	SplineCharAddExtrema(sc,sc->layers[ly_fore].splines,ae_only_good,emsize);
	SCCharChangedUpdate(sc);
    }
}

static void MVMenuRound2Int(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SCPreserveState(mv->glyphs[i].sc,false);
	SCRound2Int( mv->glyphs[i].sc,1.0);
    }
}

static void MVMenuMetaFont(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	MetaFont(NULL, NULL, mv->glyphs[i].sc);
}

static void MVMenuAutotrace(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 )
	SCAutoTrace(mv->glyphs[i].sc,mv->gw,e!=NULL && (e->u.mouse.state&ksm_shift));
}

static void MVMenuCorrectDir(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	int changed = false, refchanged=false;
	RefChar *ref;
	int asked=-1;

	for ( ref=sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next ) {
	    if ( ref->transform[0]*ref->transform[3]<0 ||
		    (ref->transform[0]==0 && ref->transform[1]*ref->transform[2]>0)) {
		if ( asked==-1 ) {
		    char *buts[4];
		    buts[0] = _("_Unlink");
#if defined(FONTFORGE_CONFIG_GDRAW)
		    buts[1] = _("_No");
		    buts[2] = _("_Cancel");
#elif defined(FONTFORGE_CONFIG_GTK)
		    buts[1] = GTK_RESPONSE_NO;
		    buts[2] = GTK_RESPONSE_CANCEL;
#endif
		    buts[3] = NULL;
		    asked = gwwv_ask(_("Flipped Reference"),(const char **) buts,0,2,_("%.50s contains a flipped reference. This cannot be corrected as is. Would you like me to unlink it and then correct it?"), sc->name );
		    if ( asked==2 )
return;
		    else if ( asked==1 )
	break;
		}
		if ( asked==0 ) {
		    if ( !refchanged ) {
			refchanged = true;
			SCPreserveState(sc,false);
		    }
		    SCRefToSplines(sc,ref);
		}
	    }
	}

	if ( !refchanged )
	    SCPreserveState(sc,false);
	sc->layers[ly_fore].splines = SplineSetsCorrect(sc->layers[ly_fore].splines,&changed);
	if ( changed || refchanged )
	    SCCharChangedUpdate(sc);
    }
}

static void _MVMenuBuildAccent(MetricsView *mv,int onlyaccents) {
    int i;
    extern int onlycopydisplayed;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=-1 ) {
	SplineChar *sc = mv->glyphs[i].sc;
	if ( SFIsSomethingBuildable(mv->sf,sc,onlyaccents) )
	    SCBuildComposit(mv->sf,sc,!onlycopydisplayed,mv->fv);
    }
}

static void MVMenuBuildAccent(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv,false);
}

static void MVMenuBuildComposite(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    _MVMenuBuildAccent(mv,true);
}

static void MVResetText(MetricsView *mv) {
    unichar_t *new, *pt;
    int i;

    new = galloc((mv->clen+1)*sizeof(unichar_t));
    for ( pt=new, i=0; i<mv->clen; ++i ) {
	if ( mv->chars[i]->unicodeenc==-1 || mv->chars[i]->unicodeenc>=0x10000 )
	    *pt++ = 0xfffd;
	else
	    *pt++ = mv->chars[i]->unicodeenc;
    }
    *pt = '\0';
    GGadgetSetTitle(mv->text,new);
    free(new );
}

static void MVMenuLigatures(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowLigatures(mv->sf,NULL);
}

static void MVMenuKernPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->sf,NULL,NULL);
}

static void MVMenuAnchorPairs(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SFShowKernPairs(mv->sf,NULL,mi->ti.userdata);
}

static void MVMenuScale(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    if ( mi->mid==MID_ZoomIn ) {
	if ( --mv->scale_index<0 ) mv->scale_index = 0;
    } else {
	if ( ++mv->scale_index >= sizeof(mv_scales)/sizeof(mv_scales[0]) )
	    mv->scale_index = sizeof(mv_scales)/sizeof(mv_scales[0])-1;
    }

    mv->pixelsize = mv_scales[mv->scale_index]*(mv->displayend - mv->topend - 4);
    if ( mv->bdf==NULL ) {
	BDFFontFree(mv->show);
	mv->show = SplineFontPieceMeal(mv->sf,mv->pixelsize,mv->antialias?pf_antialias:0,NULL);
    }
    MVReKern(mv);
    MVSetVSb(mv);
}

static void MVMenuInsertChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
    int i, j, pos = GotoChar(sf,mv->fv->map);

    if ( pos==-1 || pos>=mv->fv->map->enccount )
return;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt )	/* Something selected */
	i = mv->glyphs[i].orig_index;		/* Index in the string of chars, not glyphs */
    else if ( mi->mid==MID_InsertCharA )
	i = mv->clen;
    else
	i = 0;
    if ( mi->mid==MID_InsertCharA ) {
	if ( i!=mv->clen )
	    ++i;
    } else {
	if ( i==mv->clen ) i = 0;
    }

    if ( mv->clen+1>=mv->cmax ) {
	int oldmax=mv->cmax;
	mv->cmax = mv->clen+10;
	mv->chars = grealloc(mv->chars,mv->cmax*sizeof(SplineChar *));
	memset(mv->chars+oldmax,'\0',(mv->cmax-oldmax)*sizeof(SplineChar *));
    }
    for ( j=mv->clen; j>i; --j )
	mv->chars[j] = mv->chars[j-1]; 
    mv->chars[i] = SFMakeChar(sf,mv->fv->map,pos);
    ++mv->clen;
    MVRemetric(mv);
    for ( j=0; j<mv->glyphcnt; ++j )
	if ( mv->glyphs[j].orig_index==i ) {
	    MVDoSelect(mv,j);
    break;
	}
    GDrawRequestExpose(mv->gw,NULL,false);
    MVResetText(mv);
}

static void MVMenuChangeChar(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
    SplineChar *sc;
    EncMap *map = mv->fv->map;
    int i, pos, gid;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt ) {
	pos = -1;
	i = mv->glyphs[i].orig_index;
	sc = mv->chars[ i ];
	if ( mi->mid == MID_Next ) {
	    pos = map->backmap[sc->orig_pos]+1;
	} else if ( mi->mid==MID_Prev ) {
	    pos = map->backmap[sc->orig_pos]-1;
	} else if ( mi->mid==MID_NextDef ) {
	    for ( pos = map->backmap[sc->orig_pos]+1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); ++pos );
	    if ( pos>=map->enccount )
return;
	} else if ( mi->mid==MID_PrevDef ) {
	    for ( pos = map->backmap[sc->orig_pos]-1;
		    pos<map->enccount && ((gid=map->map[pos])==-1 || sf->glyphs[gid]==NULL); --pos );
	    if ( pos<0 )
return;
	} else if ( mi->mid==MID_ReplaceChar ) {
	    pos = GotoChar(sf,mv->fv->map);
	    if ( pos<0 )
return;
	}
	if ( pos>=0 && pos<map->enccount ) {
	    mv->chars[i] = SFMakeChar(sf,mv->fv->map,pos);
	    MVRemetric(mv);
	    MVResetText(mv);
	    GDrawRequestExpose(mv->gw,NULL,false);
	}
    }
}

static void MVMenuFindInFontView(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    for ( i=0; i<mv->glyphcnt; ++i ) {
	if ( mv->perchar[i].selected ) {
	    FVChangeChar(mv->fv,mv->fv->map->backmap[mv->glyphs[i].sc->orig_pos]);
	    GDrawSetVisible(mv->fv->gw,true);
	    GDrawRaise(mv->fv->gw);
    break;
	}
    }
}

static void MVMenuShowGrid(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    mv->showgrid = !mv->showgrid;
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuAA(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    mv_antialias = mv->antialias = !mv->antialias;
    BDFFontFree(mv->show);
    mv->show = SplineFontPieceMeal(mv->sf,mv->pixelsize,mv->antialias?pf_antialias:0,NULL);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuVertical(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    if ( !mv->sf->hasvmetrics ) {
	if ( mv->vertical )
	    MVToggleVertical(mv);
    } else
	MVToggleVertical(mv);
    GDrawRequestExpose(mv->gw,NULL,false);
}

static void MVMenuShowBitmap(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    BDFFont *bdf = mi->ti.userdata;

    if ( mv->bdf!=bdf ) {
	MVChangeDisplayFont(mv,bdf);
	GDrawRequestExpose(mv->gw,NULL,false);
    }
}

static void MVMenuCenter(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    DBounds bb;
    real transform[6];
    SplineChar *sc;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected )
    break;
    if ( i!=mv->glyphcnt ) {
	sc = mv->glyphs[i].sc;
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0.0;
	SplineCharFindBounds(sc,&bb);
	if ( mi->mid==MID_Center )
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/2 - bb.minx;
	else
	    transform[4] = (sc->width-(bb.maxx-bb.minx))/3 - bb.minx;
	if ( transform[4]!=0 )
	    FVTrans(mv->fv,sc,transform,NULL,fvt_dontmovewidth);
    }
}

static void MVMenuKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShowKernClasses(mv->sf,mv,false);
}

static void MVMenuVKernByClasses(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    ShowKernClasses(mv->sf,mv,true);
}

static void MVMenuVKernFromHKern(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    FVVKernFromHKern(mv->fv);
}

static void MVMenuKPCloseup(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineChar *sc1=NULL, *sc2=NULL;
    int i;

    for ( i=0; i<mv->glyphcnt; ++i )
	if ( mv->perchar[i].selected ) {
	    sc1 = mv->glyphs[i].sc;
	    if ( i+1<mv->glyphcnt )
		sc2 = mv->glyphs[i+1].sc;
    break;
	}
    KernPairD(mv->sf,sc1,sc2,mv->vertical);
}

static GMenuItem2 wnmenu[] = {
    { { (unichar_t *) N_("New O_utline Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'u' }, H_("New Outline Window|Ctl+H"), NULL, NULL, MVMenuOpenOutline, MID_OpenOutline },
    { { (unichar_t *) N_("New _Bitmap Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("New Bitmap Window|Ctl+J"), NULL, NULL, MVMenuOpenBitmap, MID_OpenBitmap },
    { { (unichar_t *) N_("New _Metrics Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("New Metrics Window|Ctl+K"), NULL, NULL, /* No function, never avail */NULL },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Warnings"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Warnings|No Shortcut"), NULL, NULL, _MenuWarnings, MID_Warnings },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { NULL }
};

static void MVWindowMenuBuild(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;
    struct gmenuitem *wmi;

    WindowMenuBuild(gw,mi,e);

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( wmi = mi->sub; wmi->ti.text!=NULL || wmi->ti.line ; ++wmi ) {
	switch ( wmi->mid ) {
	  case MID_OpenOutline:
	    wmi->ti.disabled = sc==NULL;
	  break;
	  case MID_OpenBitmap:
	    mi->ti.disabled = mv->sf->bitmaps==NULL || sc==NULL;
	  break;
	  case MID_Warnings:
	    wmi->ti.disabled = ErrorWindowExists();
	  break;
	}
    }
}

static GMenuItem2 dummyitem[] = { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, NULL };
static GMenuItem2 fllist[] = {
    { { (unichar_t *) N_("Font|_New"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("New|Ctl+N"), NULL, NULL, MenuNew },
    { { (unichar_t *) N_("_Open"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Open|Ctl+O"), NULL, NULL, MenuOpen },
    { { (unichar_t *) N_("Recen_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, NULL, dummyitem, MenuRecentBuild, NULL, MID_Recent },
    { { (unichar_t *) N_("_Close"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Close|Ctl+Shft+Q"), NULL, NULL, MVMenuClose },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Save"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Save|Ctl+S"), NULL, NULL, MVMenuSave },
    { { (unichar_t *) N_("S_ave as..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Save as...|Ctl+Shft+S"), NULL, NULL, MVMenuSaveAs },
    { { (unichar_t *) N_("_Generate Fonts..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Generate Fonts...|Ctl+Shft+G"), NULL, NULL, MVMenuGenerate },
    { { (unichar_t *) N_("Generate Mac _Family..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Generate Mac Family...|Alt+Ctl+G"), NULL, NULL, MVMenuGenerateFamily },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Merge Feature Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Merge Kern Info...|Ctl+Shft+K"), NULL, NULL, MVMenuMergeKern },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Print..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Print...|Ctl+P"), NULL, NULL, MVMenuPrint },
    { { (unichar_t *) N_("_Display..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Display...|Alt+Ctl+P"), NULL, NULL, MVMenuDisplay, MID_Display },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Pr_eferences..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'e' }, H_("Preferences...|No Shortcut"), NULL, NULL, MenuPrefs },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Quit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'Q' }, H_("Quit|Ctl+Q"), NULL, NULL, MenuExit },
    { NULL }
};

static GMenuItem2 edlist[] = {
    { { (unichar_t *) N_("_Undo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Undo|Ctl+Z"), NULL, NULL, MVUndo, MID_Undo },
    { { (unichar_t *) N_("_Redo"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Redo|Ctl+Y"), NULL, NULL, MVRedo, MID_Redo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Cu_t"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 't' }, H_("Cut|Ctl+X"), NULL, NULL, MVCut, MID_Cut },
    { { (unichar_t *) N_("_Copy"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Copy|Ctl+C"), NULL, NULL, MVCopy, MID_Copy },
    { { (unichar_t *) N_("C_opy Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Copy Reference|Ctl+G"), NULL, NULL, MVMenuCopyRef, MID_CopyRef },
    { { (unichar_t *) N_("Copy _Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Copy Width|Ctl+W"), NULL, NULL, MVMenuCopyWidth, MID_CopyWidth },
    { { (unichar_t *) N_("Copy _VWidth"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Copy VWidth|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyVWidth },
    { { (unichar_t *) N_("Co_py LBearing"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'p' }, H_("Copy LBearing|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyLBearing },
    { { (unichar_t *) N_("Copy RBearin_g"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'g' }, H_("Copy RBearing|No Shortcut"), NULL, NULL, MVMenuCopyWidth, MID_CopyRBearing },
    { { (unichar_t *) N_("_Paste"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Paste|Ctl+V"), NULL, NULL, MVPaste, MID_Paste },
    { { (unichar_t *) N_("C_lear"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, H_("Clear|No Shortcut"), NULL, NULL, MVClear, MID_Clear },
    { { (unichar_t *) N_("_Join"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'J' }, H_("Join|Ctl+Shft+J"), NULL, NULL, MVMenuJoin, MID_Join },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Select _All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Select All|Ctl+A"), NULL, NULL, MVSelectAll, MID_SelAll },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("U_nlink Reference"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'U' }, H_("Unlink Reference|Ctl+U"), NULL, NULL, MVUnlinkRef, MID_UnlinkRef },
    { NULL }
};

static GMenuItem2 smlist[] = {
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify|Ctl+Shft+M"), NULL, NULL, MVMenuSimplify, MID_Simplify },
    { { (unichar_t *) N_("Simplify More..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Simplify More...|Alt+Ctl+Shft+M"), NULL, NULL, MVMenuSimplifyMore, MID_SimplifyMore },
    { { (unichar_t *) N_("Clea_nup Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'n' }, H_("Cleanup Glyph|No Shortcut"), NULL, NULL, MVMenuCleanup, MID_CleanupGlyph },
    { NULL }
};

static GMenuItem2 rmlist[] = {
    { { (unichar_t *) N_("_Remove Overlap"), &GIcon_rmoverlap, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Remove Overlap|Ctl+Shft+O"), NULL, NULL, MVMenuOverlap, MID_RmOverlap },
    { { (unichar_t *) N_("_Intersect"), &GIcon_intersection, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Intersect|No Shortcut"), NULL, NULL, MVMenuOverlap, MID_Intersection },
    { { (unichar_t *) N_("_Find Intersections"), &GIcon_findinter, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Find Intersections|No Shortcut"), NULL, NULL, MVMenuOverlap, MID_FindInter },
    { NULL }
};

static GMenuItem2 eflist[] = {
    { { (unichar_t *) N_("_Inline"), &GIcon_inline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'O' }, H_("Inline|No Shortcut"), NULL, NULL, MVMenuInline },
    { { (unichar_t *) N_("_Outline"), &GIcon_outline, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Outline|No Shortcut"), NULL, NULL, MVMenuOutline },
    { { (unichar_t *) N_("_Shadow"), &GIcon_shadow, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'S' }, H_("Shadow|No Shortcut"), NULL, NULL, MVMenuShadow },
    { { (unichar_t *) N_("_Wireframe"), &GIcon_wireframe, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, true, 0, 0, 0, 0, 1, 1, 0, 'W' }, H_("Wireframe|No Shortcut"), NULL, NULL, MVMenuWireframe },
    { NULL }
};

static GMenuItem2 balist[] = {
    { { (unichar_t *) N_("_Build Accented Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Accented Glyph|Ctl+Shft+A"), NULL, NULL, MVMenuBuildAccent, MID_BuildAccent },
    { { (unichar_t *) N_("Build _Composite Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Build Composite Glyph|No Shortcut"), NULL, NULL, MVMenuBuildComposite, MID_BuildComposite },
    { NULL }
};

static void balistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;
    SplineChar *sc;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_BuildAccent:
	    mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent,sc,true);
	  break;
	  case MID_BuildComposite:
	    mi->ti.disabled = sc==NULL || !SFIsSomethingBuildable(sc->parent,sc,false);
	  break;
        }
    }
}

static GMenuItem2 ellist[] = {
    { { (unichar_t *) N_("_Font Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, H_("Font Info...|Ctl+Shft+F"), NULL, NULL, MVMenuFontInfo },
    { { (unichar_t *) N_("Glyph _Info..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("Glyph Info...|Ctl+I"), NULL, NULL, MVMenuCharInfo, MID_CharInfo },
    { { (unichar_t *) N_("S_how Dependent"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Show Dependent|Alt+Ctl+I"), NULL, NULL, MVMenuShowDependents, MID_ShowDependents },
    { { (unichar_t *) N_("Find Pr_oblems..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Find Problems...|Ctl+E"), NULL, NULL, MVMenuFindProblems, MID_FindProblems },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Bitm_ap Strikes Available..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'A' }, H_("Bitmap Strikes Available...|Ctl+Shft+B"), NULL, NULL, MVMenuBitmaps, MID_AvailBitmaps },
    { { (unichar_t *) N_("Regenerate _Bitmap Glyphs..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Regenerate Bitmap Glyphs...|Ctl+B"), NULL, NULL, MVMenuBitmaps, MID_RegenBitmaps },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Transform..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Transform...|No Shortcut"), NULL, NULL, MVMenuTransform, MID_Transform },
    { { (unichar_t *) N_("_Expand Stroke..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, H_("Expand Stroke...|Ctl+Shft+E"), NULL, NULL, MVMenuStroke, MID_Stroke },
#ifdef FONTFORGE_CONFIG_TILEPATH
    { { (unichar_t *) N_("Tile _Path..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Tile Path...|No Shortcut"), NULL, NULL, MVMenuTilePath, MID_TilePath },
#endif
    { { (unichar_t *) N_("_Remove Overlap"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'O' }, NULL, rmlist, NULL, NULL, MID_RmOverlap },
    { { (unichar_t *) N_("_Simplify"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'S' }, NULL, smlist, NULL, NULL, MID_Simplify },
    { { (unichar_t *) N_("Add E_xtrema"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'x' }, H_("Add Extrema|Ctl+Shft+X"), NULL, NULL, MVMenuAddExtrema, MID_AddExtrema },
    { { (unichar_t *) N_("To _Int"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'I' }, H_("To Int|Ctl+Shft+_"), NULL, NULL, MVMenuRound2Int, MID_Round },
    { { (unichar_t *) N_("Effects"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, '\0' }, NULL, eflist, NULL, NULL, MID_Effects },
    { { (unichar_t *) N_("_Meta Font..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, H_("Meta Font...|Ctl+Shft+!"), NULL, NULL, MVMenuMetaFont, MID_MetaFont },
    { { (unichar_t *) N_("Autot_race"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'r' }, H_("Autotrace|Ctl+Shft+T"), NULL, NULL, MVMenuAutotrace, MID_Autotrace },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Correct Direction"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Correct Direction|Ctl+Shft+D"), NULL, NULL, MVMenuCorrectDir, MID_Correct },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("B_uild"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, NULL, balist, balistcheck, NULL, MID_BuildAccent },
    { NULL }
};

static GMenuItem2 dummyall[] = {
    { { (unichar_t *) N_("All"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 1, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("All|No Shortcut"), NULL, NULL, NULL },
    NULL
};

/* Builds up a menu containing all the anchor classes */
static void aplistbuild(GWindow base,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(base);
    extern void GMenuItemArrayFree(GMenuItem *mi);

    GMenuItemArrayFree(mi->sub);
    mi->sub = NULL;

    _aplistbuild(mi,mv->sf,MVMenuAnchorPairs);
}

static GMenuItem2 cblist[] = {
    { { (unichar_t *) N_("_Kern Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Kern Pairs|No Shortcut"), NULL, NULL, MVMenuKernPairs, MID_KernPairs },
    { { (unichar_t *) N_("_Anchored Pairs"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'K' }, H_("Anchored Pairs|No Shortcut"), dummyall, aplistbuild, MVMenuAnchorPairs, MID_AnchorPairs },
    { { (unichar_t *) N_("_Ligatures"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'L' }, H_("Ligatures|No Shortcut"), NULL, NULL, MVMenuLigatures, MID_Ligatures },
    NULL
};

static void cblistcheck(GWindow gw,struct gmenuitem *mi, GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    SplineFont *sf = mv->sf;
    int i, anyligs=0, anykerns=0;
    PST *pst;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type==pst_ligature ) {
		anyligs = true;
		if ( anykerns )
    break;
	    }
	}
	if ( (mv->vertical ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns)!=NULL ) {
	    anykerns = true;
	    if ( anyligs )
    break;
	}
    }

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Ligatures:
	    mi->ti.disabled = !anyligs;
	  break;
	  case MID_KernPairs:
	    mi->ti.disabled = !anykerns;
	  break;
	  case MID_AnchorPairs:
	    mi->ti.disabled = sf->anchor==NULL;
	  break;
	}
    }
}

static GMenuItem2 vwlist[] = {
    { { (unichar_t *) N_("Z_oom out"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'o' }, H_("Zoom out|Alt+Ctl+-"), NULL, NULL, MVMenuScale, MID_ZoomOut },
    { { (unichar_t *) N_("Zoom _in"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'i' }, H_("Zoom in|Alt+Ctl+Shft++"), NULL, NULL, MVMenuScale, MID_ZoomIn },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Insert Glyph _After..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Insert Glyph After...|No Shortcut"), NULL, NULL, MVMenuInsertChar, MID_InsertCharA },
    { { (unichar_t *) N_("Insert Glyph _Before..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'B' }, H_("Insert Glyph Before...|No Shortcut"), NULL, NULL, MVMenuInsertChar, MID_InsertCharB },
    { { (unichar_t *) N_("_Replace Glyph..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'R' }, H_("Replace Glyph...|Ctl+G"), NULL, NULL, MVMenuChangeChar, MID_ReplaceChar },
    { { (unichar_t *) N_("_Next Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'N' }, H_("Next Glyph|Ctl+]"), NULL, NULL, MVMenuChangeChar, MID_Next },
    { { (unichar_t *) N_("_Prev Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'P' }, H_("Prev Glyph|Ctl+["), NULL, NULL, MVMenuChangeChar, MID_Prev },
    { { (unichar_t *) N_("Next _Defined Glyph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'D' }, H_("Next Defined Glyph|Alt+Ctl+]"), NULL, NULL, MVMenuChangeChar, MID_NextDef },
    { { (unichar_t *) N_("Prev Defined Gl_yph"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'a' }, H_("Prev Defined Glyph|Alt+Ctl+["), NULL, NULL, MVMenuChangeChar, MID_PrevDef },
    { { (unichar_t *) N_("Find In Font _View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, H_("Find In Font View|Ctl+Shft+<"), NULL, NULL, MVMenuFindInFontView, MID_FindInFontView },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Com_binations"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'b' }, NULL, cblist, cblistcheck },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Hide _Grid"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'G' }, H_("Hide Grid|No Shortcut"), NULL, NULL, MVMenuShowGrid, MID_ShowGrid },
    { { (unichar_t *) N_("_Anti Alias"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'A' }, H_("Anti Alias|Ctl+5"), NULL, NULL, MVMenuAA, MID_AntiAlias },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Vertical"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, '\0' }, H_("Vertical|No Shortcut"), NULL, NULL, MVMenuVertical, MID_Vertical },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("_Outline"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 1, 1, 0, 'O' }, H_("Outline|No Shortcut"), NULL, NULL, MVMenuShowBitmap, MID_Outline },
    { NULL },			/* Some extra room to show bitmaps */
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL }, { NULL },
    { NULL }
};

# ifdef FONTFORGE_CONFIG_GDRAW
static void MVMenuContextualHelp(GWindow base,struct gmenuitem *mi,GEvent *e) {
# elif defined(FONTFORGE_CONFIG_GTK)
void MetricsViewMenu_ContextualHelp(GtkMenuItem *menuitem, gpointer user_data) {
# endif
    help("metricsview.html");
}

static GMenuItem2 mtlist[] = {
    { { (unichar_t *) N_("_Center in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'C' }, H_("Center in Width|No Shortcut"), NULL, NULL, MVMenuCenter, MID_Center },
    { { (unichar_t *) N_("_Thirds in Width"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Thirds in Width|No Shortcut"), NULL, NULL, MVMenuCenter, MID_Thirds },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) N_("Ker_n By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Kern By Classes...|No Shortcut"), NULL, NULL, MVMenuKernByClasses },
    { { (unichar_t *) N_("VKern By Classes..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern By Classes...|No Shortcut"), NULL, NULL, MVMenuVKernByClasses, MID_VKernClass },
    { { (unichar_t *) N_("VKern From HKern"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("VKern From HKern|No Shortcut"), NULL, NULL, MVMenuVKernFromHKern, MID_VKernFromHKern },
    { { (unichar_t *) N_("Kern Pair Closeup..."), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'T' }, H_("Kern Pair Closeup...|No Shortcut"), NULL, NULL, MVMenuKPCloseup },
    { NULL }
};

static void fllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Recent:
	    mi->ti.disabled = !RecentFilesAny();
	  break;
	  case MID_Display:
	    mi->ti.disabled = (mv->sf->onlybitmaps && mv->sf->bitmaps==NULL) ||
		    mv->sf->multilayer;
	  break;
	}
    }
}

static void edlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i;

    if ( GWindowGetFocusGadgetOfWindow(gw)!=NULL )
	i = -1;
    else
	for ( i=mv->glyphcnt-1; i>=0; --i )
	    if ( mv->perchar[i].selected )
	break;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_Join:
	  case MID_Copy: case MID_CopyRef: case MID_CopyWidth:
	  case MID_CopyLBearing: case MID_CopyRBearing:
	  case MID_Cut: case MID_Clear:
	    mi->ti.disabled = i==-1;
	  break;
	  case MID_CopyVWidth:
	    mi->ti.disabled = i==-1 || !mv->sf->hasvmetrics;
	  break;
	  case MID_Undo:
	    mi->ti.disabled = i==-1 || mv->glyphs[i].sc->layers[ly_fore].undoes==NULL;
	  break;
	  case MID_Redo:
	    mi->ti.disabled = i==-1 || mv->glyphs[i].sc->layers[ly_fore].redoes==NULL;
	  break;
	  case MID_UnlinkRef:
	    mi->ti.disabled = i==-1 || mv->glyphs[i].sc->layers[ly_fore].refs==NULL;
	  break;
	  case MID_Paste:
	    mi->ti.disabled = i==-1 ||
		(!CopyContainsSomething() &&
#ifndef _NO_LIBPNG
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/png") &&
#endif
#ifndef _NO_LIBXML
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/svg") &&
#endif
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/bmp") &&
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/ps") &&
		    !GDrawSelectionHasType(mv->gw,sn_clipboard,"image/eps"));
	  break;
	}
    }
}

static void ellistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, anybuildable;
    SplineChar *sc;
    int order2 = mv->sf->order2;

    for ( i=mv->glyphcnt-1; i>=0; --i )
	if ( mv->perchar[i].selected )
    break;
    if ( i==-1 ) sc = NULL; else sc = mv->glyphs[i].sc;

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_RegenBitmaps:
	    mi->ti.disabled = mv->sf->bitmaps==NULL;
	  break;
	  case MID_CharInfo:
	    mi->ti.disabled = sc==NULL /*|| mv->fv->cidmaster!=NULL*/;
	  break;
	  case MID_ShowDependents:
	    mi->ti.disabled = sc==NULL || sc->dependents == NULL;
	  break;
	  case MID_MetaFont:
	    mi->ti.disabled = sc==NULL || order2;
	  break;
	  case MID_FindProblems:
	  case MID_Transform:
	    mi->ti.disabled = sc==NULL;
	  break;
	  case MID_Effects:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps || order2;
	  break;
	  case MID_RmOverlap: case MID_Stroke:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
	  case MID_AddExtrema:
	  case MID_Round: case MID_Correct:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
#ifdef FONTFORGE_CONFIG_TILEPATH
	  case MID_TilePath:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps || ClipBoardToSplineSet()==NULL || order2;
	  break;
#endif
	  case MID_Simplify:
	    mi->ti.disabled = sc==NULL || mv->sf->onlybitmaps;
	  break;
	  case MID_BuildAccent:
	    anybuildable = false;
	    if ( sc!=NULL && SFIsSomethingBuildable(mv->sf,sc,false) )
		anybuildable = true;
	    mi->ti.disabled = !anybuildable;
	  break;
	  case MID_Autotrace:
	    mi->ti.disabled = !(FindAutoTraceName()!=NULL && sc!=NULL &&
		    sc->layers[ly_back].images!=NULL );
	  break;
	}
    }
}

static void vwlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);
    int i, j, base, aselection;
    BDFFont *bdf;
    char buffer[60];
    extern void GMenuItemArrayFree(GMenuItem *mi);
    extern GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt);

    aselection = false;
    for ( j=0; j<mv->glyphcnt; ++j )
	if ( mv->perchar[j].selected ) {
	    aselection = true;
    break;
	}

    for ( i=0; vwlist[i].mid!=MID_Outline; ++i )
	switch ( vwlist[i].mid ) {
	  case MID_ZoomIn:
	    vwlist[i].ti.disabled = mv->bdf!=NULL || mv->scale_index==0;
	  break;
	  case MID_ZoomOut:
	    vwlist[i].ti.disabled = mv->bdf!=NULL || mv->scale_index>=sizeof(mv_scales)/sizeof(mv_scales[0])-1;
	  break;
	  case MID_ShowGrid:
	    vwlist[i].ti.text = (unichar_t *) (mv->showgrid?_("Hide Grid"):_("Show _Grid"));
	  break;
	  case MID_AntiAlias:
	    vwlist[i].ti.checked = mv->antialias;
	    vwlist[i].ti.disabled = mv->bdf!=NULL;
	  break;
	  case MID_ReplaceChar:
	  case MID_FindInFontView:
	  case MID_Next:
	  case MID_Prev:
	  case MID_NextDef:
	  case MID_PrevDef:
	    vwlist[i].ti.disabled = !aselection;
	  break;
	  case MID_Vertical:
	    vwlist[i].ti.checked = mv->vertical;
	    vwlist[i].ti.disabled = !mv->sf->hasvmetrics;
	  break;
	}
    base = i+1;
    for ( i=base; vwlist[i].ti.text!=NULL; ++i ) {
	free( vwlist[i].ti.text);
	vwlist[i].ti.text = NULL;
    }

    vwlist[base-1].ti.checked = mv->bdf==NULL;
    if ( mv->sf->bitmaps!=NULL ) {
	for ( bdf = mv->sf->bitmaps, i=base;
		i<sizeof(vwlist)/sizeof(vwlist[0])-1 && bdf!=NULL;
		++i, bdf = bdf->next ) {
	    if ( BDFDepth(bdf)==1 )
		sprintf( buffer, _("%d pixel bitmap"), bdf->pixelsize );
	    else
		sprintf( buffer, _("%d@%d pixel bitmap"),
			bdf->pixelsize, BDFDepth(bdf) );
	    vwlist[i].ti.text = utf82u_copy(buffer);
	    vwlist[i].ti.checkable = true;
	    vwlist[i].ti.checked = bdf==mv->bdf;
	    vwlist[i].ti.userdata = bdf;
	    vwlist[i].invoke = MVMenuShowBitmap;
	    vwlist[i].ti.fg = vwlist[i].ti.bg = COLOR_DEFAULT;
	}
    }
    GMenuItemArrayFree(mi->sub);
    mi->sub = GMenuItem2ArrayCopy(vwlist,NULL);
}

static void mtlistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    for ( mi = mi->sub; mi->ti.text!=NULL || mi->ti.line ; ++mi ) {
	switch ( mi->mid ) {
	  case MID_VKernClass:
	  case MID_VKernFromHKern:
	    mi->ti.disabled = !mv->sf->hasvmetrics;
	  break;
	}
    }
}

static GMenuItem2 mblist[] = {
    { { (unichar_t *) N_("_File"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'F' }, NULL, fllist, fllistcheck },
    { { (unichar_t *) N_("_Edit"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'E' }, NULL, edlist, edlistcheck },
    { { (unichar_t *) N_("E_lement"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'l' }, NULL, ellist, ellistcheck },
    { { (unichar_t *) N_("_View"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'V' }, NULL, vwlist, vwlistcheck },
    { { (unichar_t *) N_("_Metrics"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'M' }, NULL, mtlist, mtlistcheck },
    { { (unichar_t *) N_("_Window"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'W' }, NULL, wnmenu, MVWindowMenuBuild, NULL },
    { { (unichar_t *) N_("_Help"), NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1, 0, 'H' }, NULL, helplist, NULL },
    { NULL }
};

static void MVResize(MetricsView *mv) {
    GRect pos, wsize;
    int i;
    int size;

    GDrawGetSize(mv->gw,&wsize);
    if ( wsize.height < mv->topend+20 + mv->height-mv->displayend ||
	    wsize.width < 30 ) {
	int width= wsize.width < 30 ? 30 : wsize.width;
	int height;

	if ( wsize.height < mv->topend+20 + mv->height-mv->displayend )
	    height = mv->topend+20 + mv->height-mv->displayend;
	else
	    height = wsize.height;
	GDrawResize(mv->gw,width,height);
return;
    }

    mv->width = wsize.width;
    mv->displayend = wsize.height - (mv->height-mv->displayend);
    mv->height = wsize.height;

    pos.width = wsize.width;
    pos.height = mv->sbh;
    pos.y = wsize.height - pos.height; pos.x = 0;
    GGadgetResize(mv->hsb,pos.width,pos.height);
    GGadgetMove(mv->hsb,pos.x,pos.y);

    mv->dwidth = mv->width - mv->sbh;
    GGadgetResize(mv->vsb,mv->sbh,mv->displayend-mv->topend);
    GGadgetMove(mv->vsb,wsize.width-mv->sbh,mv->topend);

    GGadgetResize(mv->features,mv->xstart,mv->displayend - mv->topend);

    size = (mv->displayend - mv->topend - 4);
    if ( mv->dwidth-20<size )
	size = mv->dwidth-20;
    mv->pixelsize = mv_scales[mv->scale_index]*size;
    if ( mv->bdf==NULL ) {
	BDFFontFree(mv->show);
	mv->show = SplineFontPieceMeal(mv->sf,mv->pixelsize,mv->antialias?pf_antialias:0,NULL);
    }

    for ( i=0; i<mv->max; ++i ) if ( mv->perchar[i].width!=NULL ) {
	GGadgetMove(mv->perchar[i].name,mv->perchar[i].mx,mv->displayend+2);
	GGadgetMove(mv->perchar[i].width,mv->perchar[i].mx,mv->displayend+2+mv->fh+4);
	GGadgetMove(mv->perchar[i].lbearing,mv->perchar[i].mx,mv->displayend+2+2*(mv->fh+4));
	GGadgetMove(mv->perchar[i].rbearing,mv->perchar[i].mx,mv->displayend+2+3*(mv->fh+4));
	if ( mv->perchar[i].kern!=NULL )
	    GGadgetMove(mv->perchar[i].kern,mv->perchar[i].mx-mv->perchar[i].mwidth/2,mv->displayend+2+4*(mv->fh+4));
    }
    GGadgetMove(mv->namelab,2,mv->displayend+2);
    GGadgetMove(mv->widthlab,2,mv->displayend+2+mv->fh+4);
    GGadgetMove(mv->lbearinglab,2,mv->displayend+2+2*(mv->fh+4));
    GGadgetMove(mv->rbearinglab,2,mv->displayend+2+3*(mv->fh+4));
    GGadgetMove(mv->kernlab,2,mv->displayend+2+4*(mv->fh+4));

    MVRemetric(mv);
    GDrawRequestExpose(mv->gw,NULL,true);
}

static void MVChar(MetricsView *mv,GEvent *event) {
    if ( event->u.chr.keysym=='s' &&
	    (event->u.chr.state&ksm_control) &&
	    (event->u.chr.state&ksm_meta) )
	MenuSaveAll(NULL,NULL,NULL);
    else if ( event->u.chr.keysym=='I' &&
	    (event->u.chr.state&ksm_shift) &&
	    (event->u.chr.state&ksm_meta) )
	MVMenuCharInfo(mv->gw,NULL,NULL);
    else if ( event->u.chr.keysym == GK_Help ) {
	MenuHelp(NULL,NULL,NULL);	/* Menu does F1 */
    }
}

static int hitsbit(BDFChar *bc, int x, int y) {
    if ( bc->byte_data )
return( bc->bitmap[y*bc->bytes_per_line+x] );
    else
return( bc->bitmap[y*bc->bytes_per_line+(x>>3)]&(1<<(7-(x&7))) );
}

#if 0
static struct aplist *hitsaps(MetricsView *mv,int i, int x,int y) {
    SplineFont *sf = mv->sf;
    int emsize = sf->ascent+sf->descent;
    double scale = mv->pixelsize / (double) emsize;
    struct aplist *apl;
    int ax, ay;

    for ( apl = mv->perchar[i].aps; apl!=NULL; apl=apl->next ) {
	ax = apl->ap->me.x*scale;
	ay = apl->ap->me.y*scale;
	if ( x>ax-3 && x<ax+3   &&   y>ay-3 && y<ay+3 )
return( apl );
    }
return( NULL );
}
#endif

static void _MVVMouse(MetricsView *mv,GEvent *event) {
    int i, x, y, j, within, sel, xbase;
    SplineChar *sc;
    int diff;
    int onwidth, onkern;
    SplineFont *sf = mv->sf;
    int as = rint(mv->pixelsize*sf->ascent/(double) (sf->ascent+sf->descent));
    double scale = mv->pixelsize/(double) (sf->ascent+sf->descent);

    xbase = mv->dwidth/2;
    within = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	y = mv->perchar[i].dy + mv->perchar[i].yoff;
	x = xbase - mv->pixelsize/2 - mv->perchar[i].xoff;
	if ( mv->bdf==NULL ) {
	    BDFChar *bdfc = BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos);
	    if ( event->u.mouse.x >= x+bdfc->xmin &&
		event->u.mouse.x <= x+bdfc->xmax &&
		event->u.mouse.y <= (y+as)-bdfc->ymin &&
		event->u.mouse.y >= (y+as)-bdfc->ymax &&
		hitsbit(bdfc,event->u.mouse.x-x-bdfc->xmin,
			event->u.mouse.y-(y+as-bdfc->ymax)) )
    break;
	}
	y += -mv->perchar[i].yoff;
	if ( event->u.mouse.y >= y && event->u.mouse.y < y+mv->perchar[i].dheight+ mv->perchar[i].kernafter )
	    within = i;
    }
    if ( i==mv->glyphcnt )
	sc = NULL;
    else
	sc = mv->glyphs[i].sc;

    diff = event->u.mouse.y-mv->pressed_y;
    sel = onwidth = onkern = false;
    if ( sc==NULL ) {
	if ( within>0 && mv->perchar[within-1].selected &&
		event->u.mouse.y<mv->perchar[within].dy+3 )
	    onwidth = true;		/* previous char */
	else if ( within!=-1 && within+1<mv->glyphcnt &&
		mv->perchar[within+1].selected &&
		event->u.mouse.y>mv->perchar[within+1].dy-3 )
	    onkern = true;			/* subsequent char */
	else if ( within>=0 && mv->perchar[within].selected &&
		event->u.mouse.y<mv->perchar[within].dy+3 )
	    onkern = true;
	else if ( within>=0 &&
		event->u.mouse.y>mv->perchar[within].dy+mv->perchar[within].dheight+mv->perchar[within].kernafter-3 ) {
	    onwidth = true;
	    sel = true;
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ) {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	} else if ( sc!=NULL ) {
	    if ( mv->cursor!=ct_lbearing )
		ct = ct_lbearing;
	} else if ( onwidth ) {
	    if ( mv->cursor!=ct_rbearing )
		ct = ct_rbearing;
	} else if ( onkern ) {
	    if ( mv->cursor!=ct_kerning )
		ct = ct_kerning;
	} else {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	}
	if ( ct!=-1 ) {
	    GDrawSetCursor(mv->gw,ct);
	    mv->cursor = ct;
	}
    }

    if ( event->type == et_mousemove && !mv->pressed ) {
	if ( sc==NULL && within!=-1 )
	    sc = mv->glyphs[within].sc;
	if ( sc!=NULL ) 
	    SCPreparePopup(mv->gw,sc,mv->fv->map->remap,mv->fv->map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( sc!=NULL ) {
	    for ( j=0; j<mv->glyphcnt; ++j )
		if ( j!=i && mv->perchar[j].selected )
		    MVDeselectChar(mv,j);
	    MVSelectChar(mv,i);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	    mv->pressed = true;
	} else if ( within!=-1 ) {
	    mv->pressedwidth = onwidth;
	    mv->pressedkern = onkern;
	    if ( mv->pressedwidth || mv->pressedkern ) {
		mv->pressed = true;
		if ( sel && !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	} else if ( event->u.mouse.y<mv->perchar[mv->glyphcnt-1].dy+
				    mv->perchar[mv->glyphcnt-1].dheight+
			            mv->perchar[mv->glyphcnt-1].kernafter+3 ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->glyphcnt-1].selected )
		    MVDoSelect(mv,mv->glyphcnt-1);
	}
	mv->pressed_y = event->u.mouse.y;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    mv->perchar[i].dwidth = rint(mv->glyphs[i].sc->vwidth*scale) + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    kp = mv->glyphs[i-1].kp;
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=mv->glyphs[i-1].kc)!=NULL )
		kpoff = kc->offsets[mv->glyphs[i-1].kc_index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize /
			(mv->sf->descent+mv->sf->ascent);
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dy = mv->perchar[j-1].dy+mv->perchar[j-1].dheight+
			    mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else {
	    int olda = mv->activeoff;
	    BDFChar *bdfc = BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,bdfc->xmin+olda,bdfc->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->glyphs[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->glyphs[within].sc->orig_pos],mv->bdf,mv->fv,-1);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->glyphs[i].sc;
	if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		sc->vwidth += diff;
		SCCharChangedUpdate(sc);
		for ( ; i<mv->glyphcnt; ++i )
		    mv->perchar[i].dy = mv->perchar[i-1].dy+mv->perchar[i-1].dheight +
			    mv->perchar[i-1].kernafter ;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff/scale;
	    if ( diff!=0 )
		MV_ChangeKerning(mv, i, diff, true);
	    MVRefreshValues(mv,i-1);
	} else {
	    real transform[6];
	    DBounds bb;
	    SplineCharFindBounds(sc,&bb);
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[4] = 0;
	    transform[5] = -diff/scale;
	    if ( transform[5]!=0 )
		FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	for ( j=0; j<mv->glyphcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
    }
}

static void MVMouse(MetricsView *mv,GEvent *event) {
    int i, x, y, j, within, sel, ybase;
    SplineChar *sc;
    struct aplist *apl=NULL;
    int diff;
    int onwidth, onkern;
    BDFChar *bdfc;

    if ( event->u.mouse.x>=mv->xstart )
	GGadgetEndPopup();
    if ( event->u.mouse.y< mv->topend || event->u.mouse.y >= mv->displayend ) {
	if ( event->u.mouse.y >= mv->displayend &&
		event->u.mouse.y<mv->height-mv->sbh ) {
	    event->u.mouse.x += (mv->coff*mv->mwidth);
	    for ( i=0; i<mv->glyphcnt; ++i ) {
		if ( event->u.mouse.x >= mv->perchar[i].mx &&
			event->u.mouse.x < mv->perchar[i].mx+mv->perchar[i].mwidth )
	    break;
	    }
	    if ( i<mv->glyphcnt )
		SCPreparePopup(mv->gw,mv->glyphs[i].sc,mv->fv->map->remap,
			mv->fv->map->backmap[mv->glyphs[i].sc->orig_pos],
			mv->glyphs[i].sc->unicodeenc);
	}
	if ( mv->cursor!=ct_mypointer ) {
	    GDrawSetCursor(mv->gw,ct_mypointer);
	    mv->cursor = ct_mypointer;
	}
return;
    }

    if ( event->type==et_mouseup ) {
	event->type = et_mousemove;
	MVMouse(mv,event);
	event->u.mouse.x -= mv->xoff;
	event->u.mouse.y -= mv->yoff;
	event->type = et_mouseup;
    }

    event->u.mouse.x += mv->xoff;
    event->u.mouse.y += mv->yoff;
    if ( mv->vertical ) {
	_MVVMouse(mv,event);
return;
    }

    ybase = mv->topend + 2 + (mv->pixelsize/mv_scales[mv->scale_index] * mv->sf->ascent /
	    (mv->sf->ascent+mv->sf->descent));
    within = -1;
    for ( i=0; i<mv->glyphcnt; ++i ) {
	x = mv->perchar[i].dx + mv->perchar[i].xoff;
	if ( mv->right_to_left )
	    x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter;
	y = ybase - mv->perchar[i].yoff;
	if ( mv->bdf==NULL ) {
#if 0
	    if ( mv->perchar[i].selected && mv->perchar[i].aps!=NULL &&
		(apl=hitsaps(mv,i,event->u.mouse.x-x,y-event->u.mouse.y))!=NULL )
    break;
#endif
	    bdfc = BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos);
	    if ( event->u.mouse.x >= x+bdfc->xmin &&
		event->u.mouse.x <= x+bdfc->xmax &&
		event->u.mouse.y <= y-bdfc->ymin &&
		event->u.mouse.y >= y-bdfc->ymax &&
		hitsbit(bdfc,event->u.mouse.x-x-bdfc->xmin,
			bdfc->ymax-(y-event->u.mouse.y)) )
    break;
	}
	x += mv->right_to_left ? mv->perchar[i].xoff : -mv->perchar[i].xoff;
	if ( event->u.mouse.x >= x && event->u.mouse.x < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter )
	    within = i;
    }
    if ( i==mv->glyphcnt )
	sc = NULL;
    else
	sc = mv->glyphs[i].sc;

    diff = event->u.mouse.x-mv->pressed_x;
    /*if ( mv->right_to_left ) diff = -diff;*/
    sel = onwidth = onkern = false;
    if ( sc==NULL && apl==NULL ) {
	if ( !mv->right_to_left ) {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.x<mv->perchar[within].dx+3 )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->glyphcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.x>mv->perchar[within+1].dx-3 )
		onkern = true;			/* subsequent char */
	    else if ( within>=0 && mv->perchar[within].selected &&
		    event->u.mouse.x<mv->perchar[within].dx+3 )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.x>mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3 ) {
		onwidth = true;
		sel = true;
	    }
	} else {
	    if ( within>0 && mv->perchar[within-1].selected &&
		    event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		onwidth = true;		/* previous char */
	    else if ( within!=-1 && within+1<mv->glyphcnt &&
		    mv->perchar[within+1].selected &&
		    event->u.mouse.x<mv->dwidth-(mv->perchar[within+1].dx-3) )
		onkern = true;			/* subsequent char */
	    else if ( within>=0 && mv->perchar[within].selected &&
		    event->u.mouse.x>mv->dwidth-(mv->perchar[within].dx+3) )
		onkern = true;
	    else if ( within>=0 &&
		    event->u.mouse.x<mv->dwidth-(mv->perchar[within].dx+mv->perchar[within].dwidth+mv->perchar[within].kernafter-3) ) {
		onwidth = true;
		sel = true;
	    }
	}
    }

    if ( event->type != et_mousemove || !mv->pressed ) {
	int ct = -1;
	if ( mv->bdf!=NULL ) {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	} else if ( apl!=NULL ) {
	    if ( mv->cursor!=ct_4way )
		ct = ct_4way;
	} else if ( sc!=NULL ) {
	    if ( mv->cursor!=ct_lbearing )
		ct = ct_lbearing;
	} else if ( onwidth ) {
	    if ( mv->cursor!=ct_rbearing )
		ct = ct_rbearing;
	} else if ( onkern ) {
	    if ( mv->cursor!=ct_kerning )
		ct = ct_kerning;
	} else {
	    if ( mv->cursor!=ct_mypointer )
		ct = ct_mypointer;
	}
	if ( ct!=-1 ) {
	    GDrawSetCursor(mv->gw,ct);
	    mv->cursor = ct;
	}
    }

    if ( event->type == et_mousemove && !mv->pressed ) {
	if ( sc==NULL && within!=-1 )
	    sc = mv->glyphs[within].sc;
	if ( sc!=NULL ) 
	    SCPreparePopup(mv->gw,sc,mv->fv->map->remap,mv->fv->map->backmap[sc->orig_pos],sc->unicodeenc);
/* Don't allow any editing when displaying a bitmap font */
    } else if ( event->type == et_mousedown && mv->bdf==NULL ) {
	CVPaletteDeactivate();
	if ( apl!=NULL ) {
	    mv->pressed_apl = apl;
	    apl->selected = true;
	    mv->pressed = true;
	    mv->ap_owner = i;
	    mv->xp = event->u.mouse.x; mv->yp = event->u.mouse.y;
	    mv->ap_start = apl->ap->me;
	    MVRedrawI(mv,i,0,0);
	    SCPreserveState(mv->glyphs[i].sc,false);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	} else if ( sc!=NULL ) {
	    for ( j=0; j<mv->glyphcnt; ++j )
		if ( j!=i && mv->perchar[j].selected )
		    MVDeselectChar(mv,j);
	    MVSelectChar(mv,i);
	    GWindowClearFocusGadgetOfWindow(mv->gw);
	    mv->pressed = true;
	} else if ( within!=-1 ) {
	    mv->pressedwidth = onwidth;
	    mv->pressedkern = onkern;
	    if ( mv->pressedwidth || mv->pressedkern ) {
		mv->pressed = true;
		if ( sel && !mv->perchar[within].selected ) {
		    MVDoSelect(mv,within);
		}
	    }
	} else if ( !mv->right_to_left && mv->glyphcnt>=1 &&
		event->u.mouse.x<mv->perchar[mv->glyphcnt-1].dx+mv->perchar[mv->glyphcnt-1].dwidth+mv->perchar[mv->glyphcnt-1].kernafter+3 ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->glyphcnt-1].selected )
		    MVDoSelect(mv,mv->glyphcnt-1);
	} else if ( mv->right_to_left && mv->glyphcnt>=1 &&
		event->u.mouse.x>mv->dwidth - (mv->perchar[mv->glyphcnt-1].dx+mv->perchar[mv->glyphcnt-1].dwidth+mv->perchar[mv->glyphcnt-1].kernafter+3) ) {
	    mv->pressed = mv->pressedwidth = true;
	    GDrawSetCursor(mv->gw,ct_rbearing);
	    mv->cursor = ct_rbearing;
	    if ( !mv->perchar[mv->glyphcnt-1].selected )
		    MVDoSelect(mv,mv->glyphcnt-1);
	}
	mv->pressed_x = event->u.mouse.x;
    } else if ( event->type == et_mousemove && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	if ( mv->pressed_apl ) {
	    double scale = mv->pixelsize/(double) (mv->sf->ascent+mv->sf->descent);
	    mv->pressed_apl->ap->me.x = mv->ap_start.x + (event->u.mouse.x-mv->xp)/scale;
	    mv->pressed_apl->ap->me.y = mv->ap_start.y + (mv->yp-event->u.mouse.y)/scale;
	    MVRedrawI(mv,mv->ap_owner,0,0);
	} else if ( mv->pressedwidth ) {
	    int ow = mv->perchar[i].dwidth;
	    if ( mv->right_to_left ) diff = -diff;
	    bdfc = BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->perchar[i].dwidth = bdfc->width + diff;
	    if ( ow!=mv->perchar[i].dwidth ) {
		for ( j=i+1; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else if ( mv->pressedkern ) {
	    int ow = mv->perchar[i-1].kernafter;
	    KernPair *kp;
	    int kpoff;
	    KernClass *kc;
	    int index;
	    for ( kp = mv->glyphs[i-1].sc->kerns; kp!=NULL && kp->sc!=mv->glyphs[i].sc; kp = kp->next );
	    if ( kp!=NULL )
		kpoff = kp->off;
	    else if ((kc=SFFindKernClass(mv->sf,mv->glyphs[i-1].sc,mv->glyphs[i].sc,&index,false))!=NULL )
		kpoff = kc->offsets[index];
	    else
		kpoff = 0;
	    kpoff = kpoff * mv->pixelsize /
			(mv->sf->descent+mv->sf->ascent);
	    if ( mv->right_to_left ) diff = -diff;
	    mv->perchar[i-1].kernafter = kpoff + diff;
	    if ( ow!=mv->perchar[i-1].kernafter ) {
		for ( j=i; j<mv->glyphcnt; ++j )
		    mv->perchar[j].dx = mv->perchar[j-1].dx+mv->perchar[j-1].dwidth+ mv->perchar[j-1].kernafter;
		GDrawRequestExpose(mv->gw,NULL,false);
	    }
	} else {
	    int olda = mv->activeoff;
	    bdfc = BDFPieceMeal(mv->show,mv->glyphs[i].sc->orig_pos);
	    mv->activeoff = diff;
	    MVRedrawI(mv,i,bdfc->xmin+olda,bdfc->xmax+olda);
	}
    } else if ( event->type == et_mouseup && event->u.mouse.clicks>1 &&
	    (within!=-1 || sc!=NULL)) {
	mv->pressed = false; mv->activeoff = 0;
	mv->pressedwidth = mv->pressedkern = false;
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	}
	if ( within==-1 ) within = i;
	if ( mv->bdf==NULL )
	    CharViewCreate(mv->glyphs[within].sc,mv->fv,-1);
	else
	    BitmapViewCreate(mv->bdf->glyphs[mv->glyphs[within].sc->orig_pos],mv->bdf,mv->fv,-1);
    } else if ( event->type == et_mouseup && mv->pressed ) {
	for ( i=0; i<mv->glyphcnt && !mv->perchar[i].selected; ++i );
	mv->pressed = false;
	mv->activeoff = 0;
	sc = mv->glyphs[i].sc;
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	    MVRedrawI(mv,mv->ap_owner,0,0);
	    SCCharChangedUpdate(mv->glyphs[mv->ap_owner].sc);
	} else if ( mv->pressedwidth ) {
	    mv->pressedwidth = false;
	    if ( mv->right_to_left ) diff = -diff;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		SCPreserveWidth(sc);
		SCSynchronizeWidth(sc,sc->width+diff,sc->width,mv->fv);
		SCCharChangedUpdate(sc);
	    }
	} else if ( mv->pressedkern ) {
	    mv->pressedkern = false;
	    diff = diff*(mv->sf->ascent+mv->sf->descent)/mv->pixelsize;
	    if ( diff!=0 ) {
		if ( mv->right_to_left ) diff = -diff;
		MV_ChangeKerning(mv, i, diff, true);
		MVRefreshValues(mv,i-1);
	    }
	} else {
	    real transform[6];
	    transform[0] = transform[3] = 1.0;
	    transform[1] = transform[2] = transform[5] = 0;
	    transform[4] = diff*
		    (mv->sf->ascent+mv->sf->descent)/mv->pixelsize;
	    if ( transform[4]!=0 )
		FVTrans(mv->fv,sc,transform,NULL,false);
	}
    } else if ( event->type == et_mouseup && mv->bdf!=NULL && within!=-1 ) {
	if ( mv->pressed_apl!=NULL ) {
	    mv->pressed_apl->selected = false;
	    mv->pressed_apl = NULL;
	}
	for ( j=0; j<mv->glyphcnt; ++j )
	    if ( j!=within && mv->perchar[j].selected )
		MVDeselectChar(mv,j);
	MVSelectChar(mv,within);
    }
}

static void MVDrop(MetricsView *mv,GEvent *event) {
    int x,ex = event->u.drag_drop.x + mv->xoff;
    int y,ey = event->u.drag_drop.y + mv->yoff;
    int within, i, cnt, ch;
    int32 len;
    char *cnames, *start, *pt;
    unichar_t *newtext;
    const unichar_t *oldtext;
    SplineChar **founds;
    /* We should get a list of character names. Add them before the character */
    /*  on which they are dropped */

    if ( !GDrawSelectionHasType(mv->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(mv->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    within = mv->glyphcnt;
    if ( !mv->vertical ) {
	for ( i=0; i<mv->glyphcnt; ++i ) {
	    x = mv->perchar[i].dx;
	    if ( mv->right_to_left )
		x = mv->dwidth - x - mv->perchar[i].dwidth - mv->perchar[i].kernafter ;
	    if ( ex >= x && ex < x+mv->perchar[i].dwidth+ mv->perchar[i].kernafter ) {
		within = i;
	break;
	    }
	}
    } else {
	for ( i=0; i<mv->glyphcnt; ++i ) {
	    y = mv->perchar[i].dy;
	    if ( ey >= y && ey < y+mv->perchar[i].dheight+
		    mv->perchar[i].kernafter ) {
		within = i;
	break;
	    }
	}
    }

    founds = galloc(len*sizeof(SplineChar *));	/* Will be a vast over-estimate */
    start = cnames;
    for ( i=0; *start; ) {
	while ( *start==' ' ) ++start;
	if ( *start=='\0' )
    break;
	for ( pt=start; *pt && *pt!=' '; ++pt );
	ch = *pt; *pt = '\0';
	if ( (founds[i]=SFGetChar(mv->sf,-1,start))!=NULL )
	    ++i;
	*pt = ch;
	start = pt;
    }
    cnt = i;
    free( cnames );
    if ( cnt==0 )
return;
    within = mv->glyphs[within].orig_index;

    if ( mv->clen+cnt>=mv->cmax ) {
	mv->cmax = mv->clen+cnt+10;
	mv->glyphs = grealloc(mv->glyphs,mv->cmax*sizeof(SplineChar *));
    }
    oldtext = _GGadgetGetTitle(mv->text);
    newtext = galloc((mv->glyphcnt+cnt+1)*sizeof(unichar_t));
    u_strcpy(newtext,oldtext);
    newtext[mv->clen+cnt]='\0';
    for ( i=mv->clen+cnt-1; i>=within+cnt; --i ) {
	newtext[i] = newtext[i-cnt];
	mv->chars[i] = mv->chars[i-cnt];
    }
    for ( i=within; i<within+cnt; ++i ) {
	mv->chars[i] = founds[i-within];
	newtext[i] = (founds[i-within]->unicodeenc>=0 && founds[i-within]->unicodeenc<0x10000)?
		founds[i-within]->unicodeenc : 0xfffd;
    }
    mv->glyphcnt += cnt;
    MVRemetric(mv);
    free(founds);

    GGadgetSetTitle(mv->text,newtext);
    free(newtext);

    GDrawRequestExpose(mv->gw,NULL,false);
}

static int mv_e_h(GWindow gw, GEvent *event) {
    MetricsView *mv = (MetricsView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_selclear:
	ClipboardClear();
      break;
      case et_expose:
	GDrawSetLineWidth(gw,0);
	MVExpose(mv,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    MVResize(mv);
      break;
      case et_char:
	MVChar(mv,event);
      break;
      case et_mouseup: case et_mousemove: case et_mousedown:
	if (( event->type==et_mouseup || event->type==et_mousedown ) &&
		(event->u.mouse.button==4 || event->u.mouse.button==5) ) {
return( GGadgetDispatchEvent(mv->vsb,event));
	}
	MVMouse(mv,event);
      break;
      case et_drop:
	MVDrop(mv,event);
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    if ( event->u.control.g==mv->hsb )
		MVHScroll(mv,&event->u.control.u.sb);
	    else
		MVVScroll(mv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_close:
	MVMenuClose(gw,NULL,NULL);
      break;
      case et_destroy:
	if ( mv->sf->metrics==mv )
	    mv->sf->metrics = mv->next;
	else {
	    MetricsView *n;
	    for ( n=mv->sf->metrics; n->next!=mv; n=n->next );
	    n->next = mv->next;
	}
	KCLD_MvDetach(mv->sf->kcld,mv);
	MetricsViewFree(mv);
      break;
      case et_focus:
#if 0
	if ( event->u.focus.gained_focus )
	    CVPaletteDeactivate();
#endif
      break;
    }
return( true );
}
static GTextInfo *SLOfFont(SplineFont *sf) {
    uint32 *scripttags, *langtags;
    int s, l, i, k, cnt;
    extern GTextInfo scripts[], languages[];
    GTextInfo *ret = NULL;
    char *sname, *lname, *temp;
    char sbuf[8], lbuf[8];

    LookupUIInit();
    scripttags = SFScriptsInLookups(sf,-1);
    if ( scripttags==NULL )
return( NULL );

    for ( k=0; k<2; ++k ) {
	cnt = 0;
	for ( s=0; scripttags[s]!=0; ++s ) {
	    if ( k ) {
		for ( i=0; scripts[i].text!=NULL; ++i )
		    if ( scripttags[s] == (intpt) (scripts[i].userdata))
		break;
		sname = (char *) (scripts[i].text);
		sbuf[0] = scripttags[s]>>24;
		sbuf[1] = scripttags[s]>>16;
		sbuf[2] = scripttags[s]>>8;
		sbuf[3] = scripttags[s];
		sbuf[4] = 0;
		if ( sname==NULL )
		    sname = sbuf;
	    }
	    langtags = SFLangsInScript(sf,-1,scripttags[s]);
	    /* This one can't be NULL */
	    for ( l=0; langtags[l]!=0; ++l ) {
		if ( k ) {
		    for ( i=0; languages[i].text!=NULL; ++i )
			if ( langtags[l] == (intpt) (languages[i].userdata))
		    break;
		    lname = (char *) (languages[i].text);
		    lbuf[0] = langtags[l]>>24;
		    lbuf[1] = langtags[l]>>16;
		    lbuf[2] = langtags[l]>>8;
		    lbuf[3] = langtags[l];
		    lbuf[4] = 0;
		    if ( lname==NULL )
			lname = lbuf;
		    temp = galloc(strlen(sname)+strlen(lname)+3);
		    strcpy(temp,sname); strcat(temp,"{"); strcat(temp,lname); strcat(temp,"}");
		    ret[cnt].text = (unichar_t *) temp;
		    ret[cnt].text_is_1byte = true;
		    temp = galloc(11);
		    strcpy(temp,sbuf); temp[4] = '{'; strcpy(temp+5,lbuf); temp[9]='}'; temp[10] = 0;
		    ret[cnt].userdata = temp;
		}
		++cnt;
	    }
	    free(langtags);
	}
	if ( !k )
	    ret = gcalloc((cnt+1),sizeof(GTextInfo));
    }
return( ret );
}

#define metricsicon_width 16
#define metricsicon_height 16
static unsigned char metricsicon_bits[] = {
   0x04, 0x10, 0xf0, 0x03, 0x24, 0x12, 0x20, 0x00, 0x24, 0x10, 0xe0, 0x00,
   0x24, 0x10, 0x20, 0x00, 0x24, 0x10, 0x20, 0x00, 0x74, 0x10, 0x00, 0x00,
   0x55, 0x55, 0x00, 0x00, 0x04, 0x10, 0x00, 0x00};

static void MetricsViewInit(void ) {
    mb2DoGetText(mblist);
}

MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetData gd;
    GRect gsize;
    MetricsView *mv = gcalloc(1,sizeof(MetricsView));
    FontRequest rq;
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', ',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    static GWindow icon = NULL;
    extern int _GScrollBar_Width;
    char buf[100], *pt;
    GTextInfo label;
    int i,j,cnt;
    int as,ds,ld;

    MetricsViewInit();

    if ( icon==NULL )
	icon = GDrawCreateBitmap(NULL,metricsicon_width,metricsicon_height,metricsicon_bits);

    mv->fv = fv;
    mv->sf = fv->sf;
    mv->bdf = bdf;
    mv->showgrid = true;
    mv->antialias = mv_antialias;
    mv->scale_index = 2;
    mv->next = fv->sf->metrics;
    fv->sf->metrics = mv;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_icon;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.cursor = ct_mypointer;
    snprintf(buf,sizeof(buf)/sizeof(buf[0]),
	    _("Metrics For %.50s"), fv->sf->fontname);
    wattrs.utf8_window_title = buf;
    wattrs.icon = icon;
    pos.x = pos.y = 0;
    pos.width = 800;
    pos.height = 300;
    mv->gw = gw = GDrawCreateTopWindow(NULL,&pos,mv_e_h,mv,&wattrs);
    mv->width = pos.width; mv->height = pos.height;

    memset(&gd,0,sizeof(gd));
    gd.flags = gg_visible | gg_enabled;
    helplist[0].invoke = MVMenuContextualHelp;
    gd.u.menu2 = mblist;
    mv->mb = GMenu2BarCreate( gw, &gd, NULL);
    GGadgetGetSize(mv->mb,&gsize);
    mv->mbh = gsize.height;

    gd.pos.height = GDrawPointsToPixels(gw,_GScrollBar_Width);
    gd.pos.y = pos.height-gd.pos.height;
    gd.pos.x = 0; gd.pos.width = pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    mv->hsb = GScrollBarCreate(gw,&gd,mv);
    GGadgetGetSize(mv->hsb,&gsize);
    mv->sbh = gsize.height;
    mv->dwidth = mv->width-mv->sbh;

    gd.pos.width = mv->sbh;
    gd.pos.y = 0; gd.pos.height = pos.height;	/* we'll fix these later */
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    mv->vsb = GScrollBarCreate(gw,&gd,mv);

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = -12;
    rq.weight = 400;
    mv->font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GDrawFontMetrics(mv->font,&as,&ds,&ld);
    mv->fh = as+ds; mv->as = as;

    pt = buf;
    mv->chars = gcalloc(mv->cmax=20,sizeof(SplineChar *));
    if ( sc!=NULL ) {
	mv->chars[mv->clen++] = sc;
    } else {
	EncMap *map = fv->map;
	for ( j=1; (j<=fv->sel_index || j<1) && mv->clen<15; ++j ) {
	    for ( i=0; i<map->enccount && mv->clen<15; ++i ) {
		int gid = map->map[i];
		if ( gid!=-1 && fv->selected[i]==j && fv->sf->glyphs[gid]!=NULL ) {
		    mv->chars[mv->clen++] = fv->sf->glyphs[gid];
		}
	    }
	}
    }
    mv->chars[mv->clen] = NULL;

    for ( cnt=0; cnt<mv->clen; ++cnt )
	pt = utf8_idpb(pt,
		mv->chars[cnt]->unicodeenc==-1 || mv->chars[cnt]->unicodeenc>=0x10000?
		0xfffd: mv->chars[cnt]->unicodeenc);
    *pt = '\0';

    memset(&gd,0,sizeof(gd));
    memset(&label,0,sizeof(label));
    gd.pos.y = mv->mbh+2; gd.pos.x = 10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,100);
    gd.label = &label;
    label.text = (unichar_t *) "DFLT{dflt}";
    label.text_is_1byte = true;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.u.list = mv->scriptlangs = SLOfFont(mv->sf);
    gd.handle_controlevent = MV_ScriptLangChanged;
    mv->script = GListFieldCreate(gw,&gd,mv);
    GGadgetGetSize(mv->script,&gsize);
    mv->topend = gsize.y + gsize.height + 2;

    gd.pos.x = gd.pos.x+gd.pos.width+10;
    gd.pos.width = GDrawPointsToPixels(mv->gw,200);
    label.text = (unichar_t *) buf;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_text_xim;
    gd.handle_controlevent = MV_TextChanged;
    mv->text = GTextFieldCreate(gw,&gd,mv);

    gd.pos.x = gd.pos.x+gd.pos.width+10; --gd.pos.y;
    gd.pos.width += 30;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = MV_SubtableChanged;
    gd.label = NULL;
    mv->subtable_list = GListButtonCreate(gw,&gd,mv);
    MVSetSubtables(mv);

    gd.pos.y = mv->topend; gd.pos.x = 0;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_pos_use0|gg_list_multiplesel|gg_list_alphabetic;
    gd.pos.width = GDrawPointsToPixels(mv->gw,50);
    gd.handle_controlevent = MV_FeaturesChanged;
    mv->features = GListCreate(gw,&gd,mv);
    GListSetSBAlwaysVisible(mv->features,true);
    GListSetPopupCallback(mv->features,MV_FriendlyFeatures);
    mv->xstart = gd.pos.width;
    MVSetFeatures(mv);
    MVMakeLabels(mv);
    MVResize(mv);

    GDrawSetVisible(gw,true);
    /*GWidgetHidePalettes();*/
return( mv );
}

void MetricsViewFree(MetricsView *mv) {

    if ( mv->scriptlangs!=NULL ) {
	int i;
	for ( i=0; mv->scriptlangs[i].text!=NULL ; ++i )
	    free(mv->scriptlangs[i].userdata );
	GTextInfoListFree(mv->scriptlangs);
    }
    BDFFontFree(mv->show);
    /* the fields will free themselves */
    free(mv->chars);
    free(mv->glyphs);
    free(mv->perchar);
    free(mv);
}

void MVRefreshAll(MetricsView *mv) {

    if ( mv!=NULL ) {
	MVRemetric(mv);
	GDrawRequestExpose(mv->gw,NULL,false);
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
