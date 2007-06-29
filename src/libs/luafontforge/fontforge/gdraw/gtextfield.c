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
#include "gdraw.h"
#include "gkeysym.h"
#include "gresource.h"
#include "gwidget.h"
#include "ggadgetP.h"
#include "ustring.h"
#include "utype.h"
#include <math.h>

extern void (*_GDraw_InsCharHook)(GDisplay *,unichar_t);

static GBox gtextfield_box = { /* Don't initialize here */ 0 };
static FontInstance *gtextfield_font = NULL;
static int gtextfield_inited = false;

static unichar_t nullstr[] = { 0 }, nstr[] = { 'n', 0 },
	newlinestr[] = { '\n', 0 }, tabstr[] = { '\t', 0 };

static void GListFieldSelected(GGadget *g, int i);
static int GTextField_Show(GTextField *gt, int pos);
static void GTPositionGIC(GTextField *gt);

static void GTextFieldChanged(GTextField *gt,int src) {
    GEvent e;

    e.type = et_controlevent;
    e.w = gt->g.base;
    e.u.control.subtype = et_textchanged;
    e.u.control.g = &gt->g;
    e.u.control.u.tf_changed.from_pulldown = src;
    if ( gt->g.handle_controlevent != NULL )
	(gt->g.handle_controlevent)(&gt->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GTextFieldFocusChanged(GTextField *gt,int gained) {
    GEvent e;

    e.type = et_controlevent;
    e.w = gt->g.base;
    e.u.control.subtype = et_textfocuschanged;
    e.u.control.g = &gt->g;
    e.u.control.u.tf_focus.gained_focus = gained;
    if ( gt->g.handle_controlevent != NULL )
	(gt->g.handle_controlevent)(&gt->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GTextFieldMakePassword(GTextField *gt, int start_of_change) {
    int cnt = u_strlen(gt->text);
    unichar_t *pt, *end;

    if ( cnt>= gt->bilen ) {
	gt->bilen = cnt + 50;
	free( gt->bidata.text );
	gt->bidata.text = galloc(gt->bilen*sizeof(unichar_t));
	start_of_change = 0;
    }
    end = gt->bidata.text+cnt;
    for ( pt = gt->bidata.text+start_of_change ; pt<end ;  )
	*pt++ = '*';
    *pt = '\0';
}

static void GTextFieldProcessBi(GTextField *gt, int start_of_change) {
    int i, pos;
    unichar_t *pt, *end;
    GBiText bi;

    if ( !gt->dobitext )
	i = GDrawIsAllLeftToRight(gt->text+start_of_change,-1);
    else
	i = GDrawIsAllLeftToRight(gt->text,-1);
    gt->dobitext = (i!=1);
    if ( gt->dobitext ) {
	int cnt = u_strlen(gt->text);
	if ( cnt+1>= gt->bilen ) {
	    gt->bilen = cnt + 50;
	    free(gt->bidata.text); free(gt->bidata.level);
	    free(gt->bidata.override); free(gt->bidata.type);
	    free(gt->bidata.original);
	    ++gt->bilen;
	    gt->bidata.text = galloc(gt->bilen*sizeof(unichar_t));
	    gt->bidata.level = galloc(gt->bilen*sizeof(uint8));
	    gt->bidata.override = galloc(gt->bilen*sizeof(uint8));
	    gt->bidata.type = galloc(gt->bilen*sizeof(uint16));
	    gt->bidata.original = galloc(gt->bilen*sizeof(unichar_t *));
	    --gt->bilen;
	}
	bi = gt->bidata;
	pt = gt->text;
	pos = 0;
	gt->bidata.interpret_arabic = false;
	do {
	    end = u_strchr(pt,'\n');
	    if ( end==NULL || !gt->multi_line ) end = pt+u_strlen(pt);
	    else ++end;
	    bi.text = gt->bidata.text+pos;
	    bi.level = gt->bidata.level+pos;
	    bi.override = gt->bidata.override+pos;
	    bi.type = gt->bidata.type+pos;
	    bi.original = gt->bidata.original+pos;
	    bi.base_right_to_left = GDrawIsAllLeftToRight(pt,end-pt)==-1;
	    GDrawBiText1(&bi,pt,end-pt);
	    if ( bi.interpret_arabic ) gt->bidata.interpret_arabic = true;
	    pos += end-pt;
	    pt = end;
	} while ( *pt!='\0' );
	gt->bidata.len = cnt;
	if ( !gt->multi_line ) {
	    gt->bidata.base_right_to_left = bi.base_right_to_left;
	    GDrawBiText2(&gt->bidata,0,-1);
	}
    }
}

static void GTextFieldRefigureLines(GTextField *gt, int start_of_change) {
    int i;
    unichar_t *pt, *ept, *end, *temp;

    GDrawSetFont(gt->g.base,gt->font);
    if ( gt->lines==NULL ) {
	gt->lines = galloc(10*sizeof(int32));
	gt->lines[0] = 0;
	gt->lines[1] = -1;
	gt->lmax = 10;
	gt->lcnt = 1;
	if ( gt->vsb!=NULL )
	    GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt-1,gt->g.inner.height/gt->fh);
    }

    if ( gt->password )
	GTextFieldMakePassword(gt,start_of_change);	/* Sorry no support for R2L passwords */
    else
	GTextFieldProcessBi(gt,start_of_change);

    if ( !gt->multi_line ) {
	gt->xmax = GDrawGetTextWidth(gt->g.base,gt->text,-1,NULL);
return;
    }

    for ( i=0; i<gt->lcnt && gt->lines[i]<start_of_change; ++i );
    if ( !gt->wrap ) {
	if ( --i<0 ) i = 0;
	pt = gt->text+gt->lines[i];
	while ( ( ept = u_strchr(pt,'\n'))!=NULL ) {
	    if ( i>=gt->lmax )
		gt->lines = grealloc(gt->lines,(gt->lmax+=10)*sizeof(int32));
	    gt->lines[i++] = pt-gt->text;
	    pt = ept+1;
	}
	if ( i>=gt->lmax )
	    gt->lines = grealloc(gt->lines,(gt->lmax+=10)*sizeof(int32));
	gt->lines[i++] = pt-gt->text;
    } else {
	if (( i -= 2 )<0 ) i = 0;
	pt = gt->text+gt->lines[i];
	do {
	    if ( ( ept = u_strchr(pt,'\n'))==NULL )
		ept = pt+u_strlen(pt);
	    while ( pt<=ept ) {
		GDrawGetTextPtAfterPos(gt->g.base,pt, ept-pt, NULL,
			gt->g.inner.width, &end);
		if ( end!=ept && !isbreakbetweenok(*end,end[1]) ) {
		    for ( temp=end; temp>pt && !isbreakbetweenok(*temp,temp[1]); --temp );
		    if ( temp==pt )
			for ( temp=end; temp<ept && !isbreakbetweenok(*temp,temp[1]); ++temp );
		    end = temp;
		}
		if ( i>=gt->lmax )
		    gt->lines = grealloc(gt->lines,(gt->lmax+=10)*sizeof(int32));
		gt->lines[i++] = pt-gt->text;
		if ( *end=='\0' )
       goto break_2_loops;
		pt = end+1;
	    }
	} while ( *ept!='\0' );
       break_2_loops:;
    }
    if ( gt->lcnt!=i ) {
	gt->lcnt = i;
	if ( gt->vsb!=NULL )
	    GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt-1,gt->g.inner.height/gt->fh);
	if ( gt->loff_top+gt->g.inner.height/gt->fh>gt->lcnt ) {
	    gt->loff_top = gt->lcnt-gt->g.inner.height/gt->fh;
	    if ( gt->loff_top<0 ) gt->loff_top = 0;
	    if ( gt->vsb!=NULL )
		GScrollBarSetPos(&gt->vsb->g,gt->loff_top);
	}
    }
    if ( i>=gt->lmax )
	gt->lines = grealloc(gt->lines,(gt->lmax+=10)*sizeof(int32));
    gt->lines[i++] = -1;

    gt->xmax = 0;
    for ( i=0; i<gt->lcnt; ++i ) {
	int eol = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i]-1;
	int wid = GDrawGetTextWidth(gt->g.base,gt->text+gt->lines[i],eol,NULL);
	if ( wid>gt->xmax )
	    gt->xmax = wid;
    }
    if ( gt->hsb!=NULL ) {
	GScrollBarSetBounds(&gt->hsb->g,0,gt->xmax,gt->g.inner.width);
    }

    if ( gt->dobitext ) {
	int end = -1, off;
	for ( i=0; i<gt->lcnt; ++i ) {
	    if ( gt->lines[i]>end ) {
		unichar_t *ept = u_strchr(gt->text+end+1,'\n');
		if ( ept==NULL ) ept = gt->text+u_strlen(gt->text);
		gt->bidata.base_right_to_left = GDrawIsAllLeftToRight(gt->text+end+1,ept-gt->text)==-1;
		end = ept - gt->text;
	    }
	    off = 0;
	    if ( gt->text[gt->lines[i+1]-1]=='\n' ) off=1;
	    GDrawBiText2(&gt->bidata,gt->lines[i],gt->lines[i+1]-off);
	}
    }
}

static void _GTextFieldReplace(GTextField *gt, const unichar_t *str) {
    unichar_t *old = gt->oldtext;
    unichar_t *new = galloc((u_strlen(gt->text)-(gt->sel_end-gt->sel_start) + u_strlen(str)+1)*sizeof(unichar_t));

    gt->oldtext = gt->text;
    gt->sel_oldstart = gt->sel_start;
    gt->sel_oldend = gt->sel_end;
    gt->sel_oldbase = gt->sel_base;

    u_strncpy(new,gt->text,gt->sel_start);
    u_strcpy(new+gt->sel_start,str);
    gt->sel_start = u_strlen(new);
    u_strcpy(new+gt->sel_start,gt->text+gt->sel_end);
    gt->text = new;
    gt->sel_end = gt->sel_base = gt->sel_start;
    free(old);

    GTextFieldRefigureLines(gt,gt->sel_oldstart);
}

static void GTextField_Replace(GTextField *gt, const unichar_t *str) {
    _GTextFieldReplace(gt,str);
    GTextField_Show(gt,gt->sel_start);
}

static int GTextFieldFindLine(GTextField *gt, int pos) {
    int i;
    for ( i=0; gt->lines[i+1]!=-1; ++i )
	if ( pos<gt->lines[i+1])
    break;
return( i );
}

static unichar_t *GTextFieldGetPtFromPos(GTextField *gt,int i,int xpos) {
    int ll;
    unichar_t *end;

    ll = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i]-1;
    if ( gt->password ) {
	GDrawGetTextPtFromPos(gt->g.base,gt->bidata.text, -1, NULL,
		xpos-gt->g.inner.x+gt->xoff_left, &end);
	end = gt->text + (end-gt->bidata.text);
    } else if ( !gt->dobitext )
	GDrawGetTextPtFromPos(gt->g.base,gt->text+gt->lines[i], ll, NULL,
		xpos-gt->g.inner.x+gt->xoff_left, &end);
    else {
	GDrawGetTextPtFromPos(gt->g.base,gt->bidata.text+gt->lines[i], ll, NULL,
		xpos-gt->g.inner.x+gt->xoff_left, &end);
	end = gt->bidata.original[end-gt->bidata.text];
    }
return( end );
}

static int GTextFieldBiPosFromPos(GTextField *gt,int i,int pos) {
    int ll,j;
    unichar_t *pt = gt->text+pos;

    if ( !gt->dobitext )
return( pos );
    ll = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i]-1;
    for ( j=gt->lines[i]; j<ll; ++j )
	if ( gt->bidata.original[j] == pt )
return( j );

return( pos );
}

static int GTextFieldGetOffsetFromOffset(GTextField *gt,int l, int sel) {
    int i;
    unichar_t *spt = gt->text+sel;
    int llen = gt->lines[l+1]!=-1 ? gt->lines[l+1]: u_strlen(gt->text+gt->lines[l])+gt->lines[l];

    if ( !gt->dobitext )
return( sel );
    for ( i=gt->lines[l]; i<llen && gt->bidata.original[i]!=spt; ++i );
return( i );
}

static int GTextField_Show(GTextField *gt, int pos) {
    int i, ll, m, xoff, loff;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;
    int refresh=false;

    if ( pos < 0 ) pos = 0;
    if ( pos > u_strlen(gt->text)) pos = u_strlen(gt->text);
    i = GTextFieldFindLine(gt,pos);

    loff = gt->loff_top;
    if ( gt->lcnt<gt->g.inner.height/gt->fh || loff==0 )
	loff = 0;
    if ( i<loff )
	loff = i;
    if ( i>=loff+gt->g.inner.height/gt->fh ) {
	loff = i-(gt->g.inner.height/gt->fh);
	if ( gt->g.inner.height/gt->fh>2 )
	    ++loff;
    }

    xoff = gt->xoff_left;
    if ( gt->lines[i+1]==-1 ) ll = -1; else ll = gt->lines[i+1]-gt->lines[i]-1;
    if ( GDrawGetTextWidth(gt->g.base,bitext+gt->lines[i],ll,NULL)< gt->g.inner.width )
	xoff = 0;
    else {
	if ( gt->dobitext ) {
	    bitext = gt->bidata.text;
	    pos = GTextFieldBiPosFromPos(gt,i,pos);
	} else
	    bitext = gt->text;
	m = GDrawGetTextWidth(gt->g.base,bitext+gt->lines[i],pos-gt->lines[i],NULL);
	if ( m < xoff )
	    xoff = gt->nw*(m/gt->nw);
	if ( m - xoff >= gt->g.inner.width )
	    xoff = gt->nw * ((m-2*gt->g.inner.width/3)/gt->nw);
    }

    if ( xoff!=gt->xoff_left ) {
	gt->xoff_left = xoff;
	if ( gt->hsb!=NULL )
	    GScrollBarSetPos(&gt->hsb->g,xoff);
	refresh = true;
    }
    if ( loff!=gt->loff_top ) {
	gt->loff_top = loff;
	if ( gt->vsb!=NULL )
	    GScrollBarSetPos(&gt->vsb->g,loff);
	refresh = true;
    }
    GTPositionGIC(gt);
return( refresh );
}

static void *genunicodedata(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp;
    *len = gt->sel_end-gt->sel_start + 1;
    temp = galloc((*len+1)*sizeof(unichar_t));
    temp[0] = 0xfeff;		/* KDE expects a byte order flag */
    u_strncpy(temp+1,gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
return( temp );
}

static void *genutf8data(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp =u_copyn(gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    char *ret = u2utf8_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenunicodedata(void *_gt,int32 *len) {
    void *temp = genunicodedata(_gt,len);
    GTextField *gt = _gt;
    _GTextFieldReplace(gt,nullstr);
    _ggadget_redraw(&gt->g);
return( temp );
}

static void *genlocaldata(void *_gt,int32 *len) {
    GTextField *gt = _gt;
    unichar_t *temp =u_copyn(gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
    char *ret = u2def_copy(temp);
    free(temp);
    *len = strlen(ret);
return( ret );
}

static void *ddgenlocaldata(void *_gt,int32 *len) {
    void *temp = genlocaldata(_gt,len);
    GTextField *gt = _gt;
    _GTextFieldReplace(gt,nullstr);
    _ggadget_redraw(&gt->g);
return( temp );
}

static void noop(void *_gt) {
}

static void GTextFieldGrabPrimarySelection(GTextField *gt) {
    int ss = gt->sel_start, se = gt->sel_end;

    GDrawGrabSelection(gt->g.base,sn_primary);
    gt->sel_start = ss; gt->sel_end = se;
    GDrawAddSelectionType(gt->g.base,sn_primary,"text/plain;charset=ISO-10646-UCS-2",gt,gt->sel_end-gt->sel_start,
	    sizeof(unichar_t),
	    genunicodedata,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"UTF8_STRING",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genutf8data,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"text/plain;charset=UTF-8",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genutf8data,noop);
    GDrawAddSelectionType(gt->g.base,sn_primary,"STRING",gt,gt->sel_end-gt->sel_start,
	    sizeof(char),
	    genlocaldata,noop);
}

static void GTextFieldGrabDDSelection(GTextField *gt) {

    GDrawGrabSelection(gt->g.base,sn_drag_and_drop);
    GDrawAddSelectionType(gt->g.base,sn_drag_and_drop,"text/plain;charset=ISO-10646-UCS-2",gt,gt->sel_end-gt->sel_start,
	    sizeof(unichar_t),
	    ddgenunicodedata,noop);
    GDrawAddSelectionType(gt->g.base,sn_drag_and_drop,"STRING",gt,gt->sel_end-gt->sel_start,sizeof(char),
	    ddgenlocaldata,noop);
}

static void GTextFieldGrabSelection(GTextField *gt, enum selnames sel ) {

    if ( gt->sel_start!=gt->sel_end ) {
	unichar_t *temp;
	char *ctemp, *ctemp2;

	GDrawGrabSelection(gt->g.base,sel);
	temp = galloc((gt->sel_end-gt->sel_start + 2)*sizeof(unichar_t));
	temp[0] = 0xfeff;		/* KDE expects a byte order flag */
	u_strncpy(temp+1,gt->text+gt->sel_start,gt->sel_end-gt->sel_start);
	ctemp = u2utf8_copy(temp+1);
	ctemp2 = u2def_copy(temp+1);
	GDrawAddSelectionType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",temp,u_strlen(temp),
		sizeof(unichar_t),
		NULL,NULL);
	GDrawAddSelectionType(gt->g.base,sel,"UTF8_STRING",copy(ctemp),strlen(ctemp),
		sizeof(char),
		NULL,NULL);
	GDrawAddSelectionType(gt->g.base,sel,"text/plain;charset=UTF-8",ctemp,strlen(ctemp),
		sizeof(char),
		NULL,NULL);

	if ( ctemp2!=NULL && *ctemp2!='\0' /*strlen(ctemp2)==gt->sel_end-gt->sel_start*/ )
	    GDrawAddSelectionType(gt->g.base,sel,"STRING",ctemp2,strlen(ctemp2),
		    sizeof(char),
		    NULL,NULL);
	else
	    free(ctemp2);
    }
}

static int GTextFieldSelBackword(unichar_t *text,int start) {
    unichar_t ch = text[start-1];

    if ( start==0 )
	/* Can't go back */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=start-1; i>=0 && (isalnum(text[i]) || text[i]=='_') ; --i );
	start = i+1;
    } else {
	int i;
	for ( i=start-1; i>=0 && !isalnum(text[i]) && text[i]!='_' ; --i );
	start = i+1;
    }
return( start );
}

static int GTextFieldSelForeword(unichar_t *text,int end) {
    unichar_t ch = text[end];

    if ( ch=='\0' )
	/* Nothing */;
    else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=end; isalnum(text[i]) || text[i]=='_' ; ++i );
	end = i;
    } else {
	int i;
	for ( i=end; !isalnum(text[i]) && text[i]!='_' && text[i]!='\0' ; ++i );
	end = i;
    }
return( end );
}

static void GTextFieldSelectWord(GTextField *gt,int mid, int16 *start, int16 *end) {
    unichar_t *text;
    unichar_t ch = gt->text[mid];

    if ( gt->dobitext ) {
	text = gt->bidata.text;
	mid = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,mid),mid);
    } else
	text = gt->text;
    ch = text[mid];

    if ( ch=='\0' )
	*start = *end = mid;
    else if ( isspace(ch) ) {
	int i;
	for ( i=mid; isspace(text[i]); ++i );
	*end = i;
	for ( i=mid-1; i>=0 && isspace(text[i]) ; --i );
	*start = i+1;
    } else if ( isalnum(ch) || ch=='_' ) {
	int i;
	for ( i=mid; isalnum(text[i]) || text[i]=='_' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && (isalnum(text[i]) || text[i]=='_') ; --i );
	*start = i+1;
    } else {
	int i;
	for ( i=mid; !isalnum(text[i]) && text[i]!='_' && text[i]!='\0' ; ++i );
	*end = i;
	for ( i=mid-1; i>=0 && !isalnum(text[i]) && text[i]!='_' ; --i );
	*start = i+1;
    }

    if ( gt->dobitext ) {
	*start = gt->bidata.original[*start]-gt->text;
	*end = gt->bidata.original[*end]-gt->text;
    }
}

static void GTextFieldSelectWords(GTextField *gt,int last) {
    int16 ss, se;
    GTextFieldSelectWord(gt,gt->sel_base,&gt->sel_start,&gt->sel_end);
    if ( last!=gt->sel_base ) {
	GTextFieldSelectWord(gt,last,&ss,&se);
	if ( ss<gt->sel_start ) gt->sel_start = ss;
	if ( se>gt->sel_end ) gt->sel_end = se;
    }
}

static void GTextFieldPaste(GTextField *gt,enum selnames sel) {
    if ( GDrawSelectionHasType(gt->g.base,sel,"Unicode") ||
	    GDrawSelectionHasType(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2")) {
	unichar_t *temp;
	int32 len;
	temp = GDrawRequestSelection(gt->g.base,sel,"Unicode",&len);
	if ( temp==NULL || len==0 )
	    temp = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=ISO-10646-UCS-2",&len);
	/* Bug! I don't handle byte reversed selections. But I don't think there should be any anyway... */
	if ( temp!=NULL )
	    GTextField_Replace(gt,temp[0]==0xfeff?temp+1:temp);
	free(temp);
    } else if ( GDrawSelectionHasType(gt->g.base,sel,"UTF8_STRING") ||
	    GDrawSelectionHasType(gt->g.base,sel,"text/plain;charset=UTF-8")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	ctemp = GDrawRequestSelection(gt->g.base,sel,"UTF8_STRING",&len);
	if ( ctemp!=NULL ) {
	    temp = utf82u_copyn(ctemp,strlen(ctemp));
	    GTextField_Replace(gt,temp);
	    free(ctemp); free(temp);
	}
    } else if ( GDrawSelectionHasType(gt->g.base,sel,"STRING")) {
	unichar_t *temp; char *ctemp;
	int32 len;
	ctemp = GDrawRequestSelection(gt->g.base,sel,"STRING",&len);
	if ( ctemp==NULL || len==0 )
	    ctemp = GDrawRequestSelection(gt->g.base,sel,"text/plain;charset=UTF-8",&len);
	if ( ctemp!=NULL ) {
	    temp = def2u_copy(ctemp);
	    GTextField_Replace(gt,temp);
	    free(ctemp); free(temp);
	}
    }
}

static int gtextfield_editcmd(GGadget *g,enum editor_commands cmd) {
    GTextField *gt = (GTextField *) g;

    switch ( cmd ) {
      case ec_selectall:
	gt->sel_start = 0;
	gt->sel_end = u_strlen(gt->text);
return( true );
      case ec_clear:
	GTextField_Replace(gt,nullstr);
return( true );
      case ec_cut:
	GTextFieldGrabSelection(gt,sn_clipboard);
	GTextField_Replace(gt,nullstr);
      break;
      case ec_copy:
	GTextFieldGrabSelection(gt,sn_clipboard);
return( true );
      case ec_paste:
	GTextFieldPaste(gt,sn_clipboard);
	GTextField_Show(gt,gt->sel_start);
      break;
      case ec_undo:
	if ( gt->oldtext!=NULL ) {
	    unichar_t *temp = gt->text;
	    int16 s;
	    gt->text = gt->oldtext; gt->oldtext = temp;
	    s = gt->sel_start; gt->sel_start = gt->sel_oldstart; gt->sel_oldstart = s;
	    s = gt->sel_end; gt->sel_end = gt->sel_oldend; gt->sel_oldend = s;
	    s = gt->sel_base; gt->sel_base = gt->sel_oldbase; gt->sel_oldbase = s;
	    GTextFieldRefigureLines(gt, 0);
	    GTextField_Show(gt,gt->sel_end);
	}
      break;
      case ec_redo:		/* Hmm. not sure */ /* we don't do anything */
return( true );			/* but probably best to return success */
      case ec_backword:
        if ( gt->sel_start==gt->sel_end && gt->sel_start!=0 ) {
	    if ( gt->dobitext ) {
		int sel = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,gt->sel_start),gt->sel_start);
		sel = GTextFieldSelBackword(gt->bidata.text,sel);
		gt->sel_start = gt->bidata.original[sel]-gt->text;
	    } else
		gt->sel_start = GTextFieldSelBackword(gt->text,gt->sel_start);
	}
	GTextField_Replace(gt,nullstr);
      break;
      case ec_deleteword:
        if ( gt->sel_start==gt->sel_end && gt->sel_start!=0 )
	    GTextFieldSelectWord(gt,gt->sel_start,&gt->sel_start,&gt->sel_end);
	GTextField_Replace(gt,nullstr);
      break;
      default:
return( false );
    }
    GTextFieldChanged(gt,-1);
return( true );
}

static int _gtextfield_editcmd(GGadget *g,enum editor_commands cmd) {
    if ( gtextfield_editcmd(g,cmd)) {
	_ggadget_redraw(g);
	GTPositionGIC((GTextField *) g);
return( true );
    }
return( false );
}

static int GTBackPos(GTextField *gt,int pos, int ismeta) {
    int newpos,sel;

    if ( ismeta && !gt->dobitext )
	newpos = GTextFieldSelBackword(gt->text,pos);
    else if ( ismeta ) {
	sel = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,pos),pos);
	newpos = GTextFieldSelBackword(gt->bidata.text,sel);
	newpos = gt->bidata.original[newpos]-gt->text;
    } else if ( !gt->dobitext )
	newpos = pos-1;
    else {
	sel = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,pos),pos);
	if ( sel!=0 ) --sel;
	newpos = gt->bidata.original[sel]-gt->text;
    }
    if ( newpos==-1 ) newpos = pos;
return( newpos );
}

static int GTForePos(GTextField *gt,int pos, int ismeta) {
    int newpos=pos,sel;

    if ( ismeta && !gt->dobitext )
	newpos = GTextFieldSelForeword(gt->text,pos);
    else if ( ismeta ) {
	sel = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,pos),pos);
	newpos = GTextFieldSelForeword(gt->bidata.text,sel);
	newpos = gt->bidata.original[newpos]-gt->text;
    } else if ( !gt->dobitext ) {
	if ( gt->text[pos]!=0 )
	    newpos = pos+1;
    } else {
	sel = GTextFieldGetOffsetFromOffset(gt,GTextFieldFindLine(gt,pos),pos);
	if ( gt->text[sel]!=0 )
	    ++sel;
	newpos = gt->bidata.original[sel]-gt->text;
    }
return( newpos );
}

unichar_t *_GGadgetFileToUString(char *filename,int max) {
    FILE *file;
    int ch, ch2, ch3;
    int format=0;
    unichar_t *space, *upt, *end;

    file = fopen( filename,"r" );
    if ( file==NULL )
return( NULL );
    ch = getc(file); ch2 = getc(file); ch3 = getc(file);
    ungetc(ch3,file);
    if ( ch==0xfe && ch2==0xff )
	format = 1;		/* normal ucs2 */
    else if ( ch==0xff && ch2==0xfe )
	format = 2;		/* byte-swapped ucs2 */
    else if ( ch==0xef && ch2==0xbb && ch3==0xbf ) {
	format = 3;		/* utf8 */
	getc(file);
    } else {
	getc(file);		/* rewind probably undoes the ungetc, but let's not depend on it */
	rewind(file);
    }
    space = upt = galloc((max+1)*sizeof(unichar_t));
    end = space+max;
    if ( format==3 ) {		/* utf8 */
	while ( upt<end ) {
	    ch=getc(file);
	    if ( ch==EOF )
	break;
	    if ( ch<0x80 )
		*upt++ = ch;
	    else if ( ch<0xe0 ) {
		ch2 = getc(file);
		*upt++ = ((ch&0x1f)<<6)|(ch2&0x3f);
	    } else if ( ch<0xf0 ) {
		ch2 = getc(file); ch3 = getc(file);
		*upt++ = ((ch&0xf)<<12)|((ch2&0x3f)<<6)|(ch3&0x3f);
	    } else {
		int ch4, w;
		ch2 = getc(file); ch3 = getc(file); ch4=getc(file);
		w = ( ((ch&7)<<2) | ((ch2&0x30)>>4) ) -1;
		*upt++ = 0xd800 | (w<<6) | ((ch2&0xf)<<2) | ((ch3&0x30)>>4);
		if ( upt<end )
		    *upt++ = 0xdc00 | ((ch3&0xf)<<6) | (ch4&0x3f);
	    }
	}
    } else if ( format!=0 ) {
	while ( upt<end ) {
	    ch = getc(file); ch2 = getc(file);
	    if ( ch2==EOF )
	break;
	    if ( format==1 )
		*upt ++ = (ch<<8)|ch2;
	    else
		*upt ++ = (ch2<<8)|ch;
	}
    } else {
	char buffer[400];
	while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	    def2u_strncpy(upt,buffer,end-upt);
	    upt += u_strlen(upt);
	}
    }
    *upt = '\0';
    fclose(file);
return( space );
}

static unichar_t txt[] = { '*','.','t','x','t',  '\0' };
static unichar_t errort[] = { 'C','o','u','l','d',' ','n','o','t',' ','o','p','e','n',  '\0' };
static unichar_t error[] = { 'C','o','u','l','d',' ','n','o','t',' ','o','p','e','n',' ','%','.','1','0','0','h','s',  '\0' };

static void GTextFieldImport(GTextField *gt) {
    unichar_t *ret;
    char *cret;
    unichar_t *str;

    if ( _ggadget_use_gettext ) {
	char *temp = GWidgetOpenFile8(_("_Open"),NULL,"*.txt",NULL,NULL);
	ret = utf82u_copy(temp);
	free(temp);
    } else {
	ret = GWidgetOpenFile(GStringGetResource(_STR_Open,NULL),NULL,
		txt,NULL,NULL);
    }

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    str = _GGadgetFileToUString(cret,65536);
    if ( str==NULL ) {
	if ( _ggadget_use_gettext )
	    GWidgetError8(_("Could not open file"), _("Could not open %.100s"),cret);
	else
	    GWidgetError(errort,error,cret);
	free(cret);
return;
    }
    free(cret);
    GTextField_Replace(gt,str);
    free(str);
}

static void GTextFieldSave(GTextField *gt,int utf8) {
    unichar_t *ret;
    char *cret;
    FILE *file;
    unichar_t *pt;

    if ( _ggadget_use_gettext ) {
	char *temp = GWidgetOpenFile8(_("_Save"),NULL,"*.txt",NULL,NULL);
	ret = utf82u_copy(temp);
	free(temp);
    } else
	ret = GWidgetSaveAsFile(GStringGetResource(_STR_Save,NULL),NULL,
		txt,NULL,NULL);

    if ( ret==NULL )
return;
    cret = u2def_copy(ret);
    free(ret);
    file = fopen(cret,"w");
    if ( file==NULL ) {
	if ( _ggadget_use_gettext )
	    GWidgetError8(_("Could not open file"), _("Could not open %.100s"),cret);
	else
	    GWidgetError(errort,error,cret);
	free(cret);
return;
    }
    free(cret);

    if ( utf8 ) {
	putc(0xef,file);		/* Zero width something or other. Marks this as unicode, utf8 */
	putc(0xbb,file);
	putc(0xbf,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    if ( *pt<0x80 )
		putc(*pt,file);
	    else if ( *pt<0x800 ) {
		putc(0xc0 | (*pt>>6), file);
		putc(0x80 | (*pt&0x3f), file);
	    } else if ( *pt>=0xd800 && *pt<0xdc00 && pt[1]>=0xdc00 && pt[1]<0xe000 ) {
		int u = ((*pt>>6)&0xf)+1, y = ((*pt&3)<<4) | ((pt[1]>>6)&0xf);
		putc( 0xf0 | (u>>2),file );
		putc( 0x80 | ((u&3)<<4) | ((*pt>>2)&0xf),file );
		putc( 0x80 | y,file );
		putc( 0x80 | (pt[1]&0x3f),file );
	    } else {
		putc( 0xe0 | (*pt>>12),file );
		putc( 0x80 | ((*pt>>6)&0x3f),file );
		putc( 0x80 | (*pt&0x3f),file );
	    }
	}
    } else {
	putc(0xfeff>>8,file);		/* Zero width something or other. Marks this as unicode */
	putc(0xfeff&0xff,file);
	for ( pt = gt->text ; *pt; ++pt ) {
	    putc(*pt>>8,file);
	    putc(*pt&0xff,file);
	}
    }
    fclose(file);
}

#define MID_Cut		1
#define MID_Copy	2
#define MID_Paste	3

#define MID_SelectAll	4

#define MID_Save	5
#define MID_SaveUCS2	6
#define MID_Import	7

#define MID_Undo	8

static GTextField *popup_kludge;

static void GTFPopupInvoked(GWindow v, GMenuItem *mi,GEvent *e) {
    GTextField *gt;
    if ( popup_kludge==NULL )
return;
    gt = popup_kludge;
    popup_kludge = NULL;
    switch ( mi->mid ) {
      case MID_Undo:
	gtextfield_editcmd(&gt->g,ec_undo);
      break;
      case MID_Cut:
	gtextfield_editcmd(&gt->g,ec_cut);
      break;
      case MID_Copy:
	gtextfield_editcmd(&gt->g,ec_copy);
      break;
      case MID_Paste:
	gtextfield_editcmd(&gt->g,ec_paste);
      break;
      case MID_SelectAll:
	gtextfield_editcmd(&gt->g,ec_selectall);
      break;
      case MID_Save:
	GTextFieldSave(gt,true);
      break;
      case MID_SaveUCS2:
	GTextFieldSave(gt,false);
      break;
      case MID_Import:
	GTextFieldImport(gt);
      break;
    }
    _ggadget_redraw(&gt->g);
}

static GMenuItem gtf_popuplist[] = {
    { { (unichar_t *) "_Undo", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'Z', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Undo },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) "Cu_t", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'X', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Cut },
    { { (unichar_t *) "_Copy", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'C', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Copy },
    { { (unichar_t *) "_Paste", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'V', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Paste },
    { { NULL, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 1, 0, 0, }},
    { { (unichar_t *) "_Save in UTF8", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'S', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Save },
    { { (unichar_t *) "Save in _UCS2", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, '\0', ksm_control, NULL, NULL, GTFPopupInvoked, MID_SaveUCS2 },
    { { (unichar_t *) "_Import", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 1 }, 'I', ksm_control, NULL, NULL, GTFPopupInvoked, MID_Import },
    { NULL }
};
static int first = true;

static void GTFPopupMenu(GTextField *gt, GEvent *event) {
    int no_sel = gt->sel_start==gt->sel_end;

    if ( first ) {
	gtf_popuplist[0].ti.text = (unichar_t *) _("_Undo");
	gtf_popuplist[2].ti.text = (unichar_t *) _("Cu_t");
	gtf_popuplist[3].ti.text = (unichar_t *) _("_Copy");
	gtf_popuplist[4].ti.text = (unichar_t *) _("_Paste");
	gtf_popuplist[6].ti.text = (unichar_t *) _("_Save in UTF8");
	gtf_popuplist[7].ti.text = (unichar_t *) _("Save in _UCS2");
	gtf_popuplist[8].ti.text = (unichar_t *) _("_Import");
	first = false;
    }

    gtf_popuplist[0].ti.disabled = gt->oldtext==NULL;	/* Undo */
    gtf_popuplist[2].ti.disabled = no_sel;		/* Cut */
    gtf_popuplist[3].ti.disabled = no_sel;		/* Copy */
    gtf_popuplist[4].ti.disabled = !GDrawSelectionHasType(gt->g.base,sn_clipboard,"text/plain;charset=ISO-10646-UCS-2") &&
	    !GDrawSelectionHasType(gt->g.base,sn_clipboard,"UTF8_STRING") &&
	    !GDrawSelectionHasType(gt->g.base,sn_clipboard,"STRING");
    popup_kludge = gt;
    GMenuCreatePopupMenu(gt->g.base,event, gtf_popuplist);
}

static void GTextFieldIncrement(GTextField *gt,int amount) {
    unichar_t *end;
    double d = u_strtod(gt->text,&end);
    char buf[40];

    while ( *end==' ' ) ++end;
    if ( *end!='\0' ) {
	GDrawBeep(NULL);
return;
    }
    d = floor(d)+amount;
    sprintf(buf,"%g", d);
    free(gt->oldtext);
    gt->oldtext = gt->text;
    gt->text = uc_copy(buf);
    _ggadget_redraw(&gt->g);
    GTextFieldChanged(gt,-1);
}

static int GTextFieldDoChange(GTextField *gt, GEvent *event) {
    int ss = gt->sel_start, se = gt->sel_end;
    int pos, l, xpos, sel;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;
    unichar_t *upt;

    if ( ( event->u.chr.state&(ksm_control|ksm_meta)) ||
	    event->u.chr.chars[0]<' ' || event->u.chr.chars[0]==0x7f ) {
	switch ( event->u.chr.keysym ) {
	  case GK_BackSpace:
	    if ( gt->sel_start==gt->sel_end ) {
		if ( gt->sel_start==0 )
return( 2 );
		--gt->sel_start;
	    }
	    GTextField_Replace(gt,nullstr);
return( true );
	  break;
	  case GK_Delete:
	    if ( gt->sel_start==gt->sel_end ) {
		if ( gt->text[gt->sel_start]==0 )
return( 2 );
		++gt->sel_end;
	    }
	    GTextField_Replace(gt,nullstr);
return( true );
	  break;
	  case GK_Left: case GK_KP_Left:
	    if ( gt->sel_start==gt->sel_end ) {
		gt->sel_start = GTBackPos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    gt->sel_end = gt->sel_start;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( gt->sel_end==gt->sel_base ) {
		    gt->sel_start = GTBackPos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    gt->sel_end = GTBackPos(gt,gt->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		gt->sel_end = gt->sel_base = gt->sel_start;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Right: case GK_KP_Right:
	    if ( gt->sel_start==gt->sel_end ) {
		gt->sel_end = GTForePos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		if ( !(event->u.chr.state&ksm_shift ))
		    gt->sel_start = gt->sel_end;
	    } else if ( event->u.chr.state&ksm_shift ) {
		if ( gt->sel_end==gt->sel_base ) {
		    gt->sel_start = GTForePos(gt,gt->sel_start,event->u.chr.state&ksm_meta);
		} else {
		    gt->sel_end = GTForePos(gt,gt->sel_end,event->u.chr.state&ksm_meta);
		}
	    } else {
		gt->sel_start = gt->sel_base = gt->sel_end;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Up: case GK_KP_Up:
	    if ( gt->numericfield ) {
		GTextFieldIncrement(gt,(event->u.chr.state&(ksm_shift|ksm_control))?10:1);
return( 2 );
	    }
	    if ( !gt->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && gt->sel_start!=gt->sel_end )
		gt->sel_end = gt->sel_base = gt->sel_start;
	    else {
		pos = gt->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && gt->sel_start==gt->sel_base )
		    pos = gt->sel_end;
		l = GTextFieldFindLine(gt,gt->sel_start);
		sel = GTextFieldGetOffsetFromOffset(gt,l,gt->sel_start);
		xpos = GDrawGetTextWidth(gt->g.base,bitext+gt->lines[l],sel-gt->lines[l],NULL);
		if ( l!=0 )
		    pos = GTextFieldGetPtFromPos(gt,l-1,xpos) - gt->text;
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<gt->sel_base ) {
			gt->sel_start = pos;
			gt->sel_end = gt->sel_base;
		    } else {
			gt->sel_start = gt->sel_base;
			gt->sel_end = pos;
		    }
		} else {
		    gt->sel_start = gt->sel_end = gt->sel_base = pos;
		}
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Down: case GK_KP_Down:
	    if ( gt->numericfield ) {
		GTextFieldIncrement(gt,(event->u.chr.state&(ksm_shift|ksm_control))?-10:-1);
return( 2 );
	    }
	    if ( !gt->multi_line )
	  break;
	    if ( !( event->u.chr.state&ksm_shift ) && gt->sel_start!=gt->sel_end )
		gt->sel_end = gt->sel_base = gt->sel_end;
	    else {
		pos = gt->sel_start;
		if ( ( event->u.chr.state&ksm_shift ) && gt->sel_start==gt->sel_base )
		    pos = gt->sel_end;
		l = GTextFieldFindLine(gt,gt->sel_start);
		sel = GTextFieldGetOffsetFromOffset(gt,l,gt->sel_start);
		xpos = GDrawGetTextWidth(gt->g.base,bitext+gt->lines[l],sel-gt->lines[l],NULL);
		if ( l<gt->lcnt-1 )
		    pos = GTextFieldGetPtFromPos(gt,l+1,xpos) - gt->text;
		if ( event->u.chr.state&ksm_shift ) {
		    if ( pos<gt->sel_base ) {
			gt->sel_start = pos;
			gt->sel_end = gt->sel_base;
		    } else {
			gt->sel_start = gt->sel_base;
			gt->sel_end = pos;
		    }
		} else {
		    gt->sel_start = gt->sel_end = gt->sel_base = pos;
		}
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_Home: case GK_Begin: case GK_KP_Home: case GK_KP_Begin:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end = 0;
	    } else {
		gt->sel_start = 0; gt->sel_end = gt->sel_base;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  /* Move to eol. (if already at eol, move to next eol) */
	  case 'E': case 'e':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    upt = gt->text+gt->sel_base;
	    if ( *upt=='\n' )
		++upt;
	    upt = u_strchr(upt,'\n');
	    if ( upt==NULL ) upt=gt->text+u_strlen(gt->text);
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end =upt-gt->text;
	    } else {
		gt->sel_start = gt->sel_base; gt->sel_end = upt-gt->text;
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case GK_End: case GK_KP_End:
	    if ( !(event->u.chr.state&ksm_shift) ) {
		gt->sel_start = gt->sel_base = gt->sel_end = u_strlen(gt->text);
	    } else {
		gt->sel_start = gt->sel_base; gt->sel_end = u_strlen(gt->text);
	    }
	    GTextField_Show(gt,gt->sel_start);
return( 2 );
	  break;
	  case 'A': case 'a':
	    if ( event->u.chr.state&ksm_control ) {	/* Select All */
		gtextfield_editcmd(&gt->g,ec_selectall);
return( 2 );
	    }
	  break;
	  case 'C': case 'c':
	    if ( event->u.chr.state&ksm_control ) {	/* Copy */
		gtextfield_editcmd(&gt->g,ec_copy);
	    }
	  break;
	  case 'V': case 'v':
	    if ( event->u.chr.state&ksm_control ) {	/* Paste */
		gtextfield_editcmd(&gt->g,ec_paste);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'X': case 'x':
	    if ( event->u.chr.state&ksm_control ) {	/* Cut */
		gtextfield_editcmd(&gt->g,ec_cut);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'Z': case 'z':				/* Undo */
	    if ( event->u.chr.state&ksm_control ) {
		gtextfield_editcmd(&gt->g,ec_undo);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'D': case 'd':
	    if ( event->u.chr.state&ksm_control ) {	/* delete word */
		gtextfield_editcmd(&gt->g,ec_deleteword);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'W': case 'w':
	    if ( event->u.chr.state&ksm_control ) {	/* backword */
		gtextfield_editcmd(&gt->g,ec_backword);
		GTextField_Show(gt,gt->sel_start);
return( true );
	    }
	  break;
	  case 'M': case 'm': case 'J': case 'j':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    /* fall through into return case */
	  case GK_Return: case GK_Linefeed:
	    if ( gt->accepts_returns ) {
		GTextField_Replace(gt,newlinestr);
return( true );
	    }
	  break;
	  case GK_Tab:
	    if ( gt->accepts_tabs ) {
		GTextField_Replace(gt,tabstr);
return( true );
	    }
	  break;
	  case 's': case 'S':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    GTextFieldSave(gt,true);
return( 2 );
	  break;
	  case 'I': case 'i':
	    if ( !( event->u.chr.state&ksm_control ) )
return( false );
	    GTextFieldImport(gt);
return( true );
	}
    } else {
	GTextField_Replace(gt,event->u.chr.chars);
return( true );
    }

    if ( gt->sel_start == gt->sel_end )
	gt->sel_base = gt->sel_start;
    if ( ss!=gt->sel_start || se!=gt->sel_end )
	GTextFieldGrabPrimarySelection(gt);
return( false );
}

static void gt_cursor_pos(GTextField *gt, int *x, int *y) {
    int l, sel;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;

    *x = -1; *y= -1;
    GDrawSetFont(gt->g.base,gt->font);
    l = GTextFieldFindLine(gt,gt->sel_start);
    if ( l<gt->loff_top || l>=gt->loff_top + (gt->g.inner.height/gt->fh))
return;
    *y = (l-gt->loff_top)*gt->fh;
    sel = GTextFieldGetOffsetFromOffset(gt,l,gt->sel_start);
    *x = GDrawGetTextWidth(gt->g.base,bitext+gt->lines[l],sel-gt->lines[l],NULL)-
	    gt->xoff_left;
}

static void GTPositionGIC(GTextField *gt) {
    int x,y;

    if ( !gt->g.has_focus || gt->gic==NULL )
return;
    gt_cursor_pos(gt,&x,&y);
    if ( x<0 )
return;
    GDrawSetGIC(gt->g.base,gt->gic,gt->g.inner.x+x,gt->g.inner.y+y+gt->as);
}

static void gt_draw_cursor(GWindow pixmap, GTextField *gt) {
    GRect old;
    int x, y;

    if ( !gt->cursor_on || gt->sel_start != gt->sel_end )
return;
    gt_cursor_pos(gt,&x,&y);

    if ( x<0 || x>=gt->g.inner.width )
return;
    GDrawPushClip(pixmap,&gt->g.inner,&old);
    GDrawSetXORMode(pixmap);
    GDrawSetXORBase(pixmap,gt->g.box->main_background!=COLOR_DEFAULT?gt->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetFont(pixmap,gt->font);
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,gt->g.inner.x+x,gt->g.inner.y+y,
	    gt->g.inner.x+x,gt->g.inner.y+y+gt->fh,
	    gt->g.box->main_foreground!=COLOR_DEFAULT?gt->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)) );
    GDrawSetCopyMode(pixmap);
    GDrawPopClip(pixmap,&old);
}

static void GTextFieldDrawDDCursor(GTextField *gt, int pos) {
    GRect old;
    int x, y, l;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;

    l = GTextFieldFindLine(gt,pos);
    if ( l<gt->loff_top || l>=gt->loff_top + (gt->g.inner.height/gt->fh))
return;
    y = (l-gt->loff_top)*gt->fh;
    pos = GTextFieldGetOffsetFromOffset(gt,l,pos);
    x = GDrawGetTextWidth(gt->g.base,bitext+gt->lines[l],pos-gt->lines[l],NULL)-
	    gt->xoff_left;
    if ( x<0 || x>=gt->g.inner.width )
return;

    GDrawPushClip(gt->g.base,&gt->g.inner,&old);
    GDrawSetXORMode(gt->g.base);
    GDrawSetXORBase(gt->g.base,gt->g.box->main_background!=COLOR_DEFAULT?gt->g.box->main_background:
	    GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(gt->g.base)) );
    GDrawSetFont(gt->g.base,gt->font);
    GDrawSetLineWidth(gt->g.base,0);
    GDrawSetDashedLine(gt->g.base,2,2,0);
    GDrawDrawLine(gt->g.base,gt->g.inner.x+x,gt->g.inner.y+y,
	    gt->g.inner.x+x,gt->g.inner.y+y+gt->fh,
	    gt->g.box->main_foreground!=COLOR_DEFAULT?gt->g.box->main_foreground:
	    GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(gt->g.base)) );
    GDrawSetCopyMode(gt->g.base);
    GDrawPopClip(gt->g.base,&old);
    GDrawSetDashedLine(gt->g.base,0,0,0);
    gt->has_dd_cursor = !gt->has_dd_cursor;
    gt->dd_cursor_pos = pos;
}

static void GTextFieldDrawLineSel(GWindow pixmap, GTextField *gt, int line, Color fg, Color sel ) {
    GRect selr;
    int s,e, y,llen,i,j;

    y = gt->g.inner.y+(line-gt->loff_top)*gt->fh;
    selr = gt->g.inner; selr.y = y; selr.height = gt->fh;
    if ( !gt->g.has_focus ) --selr.height;
    llen = gt->lines[line+1]==-1?
	    u_strlen(gt->text+gt->lines[line])+gt->lines[line]:
	    gt->lines[line+1];
    s = gt->sel_start<gt->lines[line]?gt->lines[line]:gt->sel_start;
    e = gt->sel_end>gt->lines[line+1] && gt->lines[line+1]!=-1?gt->lines[line+1]-1:
	    gt->sel_end;

    if ( !gt->dobitext ) {
	unichar_t *text = gt->password ? gt->bidata.text : gt->text;
	if ( gt->sel_start>gt->lines[line] )
	    selr.x += GDrawGetTextWidth(pixmap,text+gt->lines[line],gt->sel_start-gt->lines[line],NULL)-
		    gt->xoff_left;
	if ( gt->sel_end <= gt->lines[line+1] || gt->lines[line+1]==-1 )
	    selr.width = GDrawGetTextWidth(pixmap,text+gt->lines[line],gt->sel_end-gt->lines[line],NULL)-
		    gt->xoff_left - (selr.x-gt->g.inner.x);
	GDrawDrawRect(pixmap,&selr,gt->g.box->active_border);
	if ( sel!=fg ) {
	    GDrawDrawText(pixmap,selr.x,y+gt->as,
		    text+s,e-s,NULL, sel );
	}
    } else {
	/* in bidirectional text the selection can be all over the */
	/*  place, so look for contiguous regions of text within the*/
	/*  selection and draw them */
	for ( i=gt->lines[line]; i<llen; ++i ) {
	    if ( gt->bidata.original[i]-gt->text >= s &&
		    gt->bidata.original[i]-gt->text < e ) {
		for ( j=i+1 ; j<llen &&
			gt->bidata.original[j]-gt->text >= s &&
			gt->bidata.original[j]-gt->text < e; ++j );
		selr.x = GDrawGetTextWidth(pixmap,gt->bidata.text+gt->lines[line],i-gt->lines[line],NULL)+
			gt->g.inner.x - gt->xoff_left;
		selr.width = GDrawGetTextWidth(pixmap,gt->bidata.text+i,j-i,NULL);
		if ( gt->g.has_focus )
		    GDrawFillRect(pixmap,&selr,gt->g.box->active_border);
		else
		    GDrawDrawRect(pixmap,&selr,gt->g.box->active_border);
		if ( sel!=fg )
		    GDrawDrawText(pixmap,selr.x,y+gt->as,
			    gt->bidata.text+i,j-i,NULL, sel );
		i = j-1;
	    }
	}
    }
}

static int gtextfield_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    GListField *ge = (GListField *) g;
    GRect old1, old2, *r = &g->r;
    Color fg,sel;
    int y,ll,i, last;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;

    if ( g->state == gs_invisible || gt->dontdraw )
return( false );

    if ( gt->listfield || gt->numericfield ) r = &ge->fieldrect;

    GDrawPushClip(pixmap,r,&old1);

    GBoxDrawBackground(pixmap,r,g->box,
	    g->state==gs_enabled? gs_pressedactive: g->state,false);
    GBoxDrawBorder(pixmap,r,g->box,g->state,false);

    GDrawPushClip(pixmap,&g->inner,&old2);
    GDrawSetFont(pixmap,gt->font);

    sel = fg = g->state==gs_disabled?g->box->disabled_foreground:
		    g->box->main_foreground==COLOR_DEFAULT?GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap)):
		    g->box->main_foreground;
    ll = 0;
    if ( (last = gt->g.inner.height/gt->fh)==0 ) last = 1;
    for ( i=gt->loff_top; i<gt->loff_top+last && gt->lines[i]!=-1; ++i ) {
	/* there is an odd complication in drawing each line. */
	/* normally we draw the selection rectangle(s) and then draw the text */
	/*  on top of that all in one go. But that doesn't work if the select */
	/*  color is the same as the foreground color (bw displays). In that */
	/*  case we draw the text first, draw the rectangles, and draw the text*/
	/*  within the rectangles */
	y = g->inner.y+(i-gt->loff_top)*gt->fh;
	if ( !gt->multi_line )
	    y = g->inner.y + (g->inner.height-gt->fh)/2;
	sel = fg;
	ll = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i];
	if ( gt->sel_start != gt->sel_end && gt->sel_end>gt->lines[i] &&
		(gt->lines[i+1]==-1 || gt->sel_start<gt->lines[i+1])) {
	    if ( g->box->active_border==fg ) {
		sel = g->state==gs_disabled?g->box->disabled_background:
				g->box->main_background==COLOR_DEFAULT?GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(pixmap)):
				g->box->main_background;
		GDrawDrawText(pixmap,g->inner.x-gt->xoff_left,y+gt->as,
			bitext+gt->lines[i],ll,NULL, fg );
	    }
	    GTextFieldDrawLineSel(pixmap,gt,i,fg,sel);
	}
	if ( sel==fg ) {
	    if ( ll>0 )
		if ( (*(bitext+gt->lines[i]+ll-1)=='\n' || *(bitext+gt->lines[i]+ll-1)=='\r' ))
		    --ll;
	    GDrawDrawText(pixmap,g->inner.x-gt->xoff_left,y+gt->as,
		    bitext+gt->lines[i],ll,NULL, fg );
	}
    }

    GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    gt_draw_cursor(pixmap, gt);

    if ( gt->listfield ) {
	int marklen = GDrawPointsToPixels(pixmap,_GListMarkSize);
	GRect r;

	GDrawPushClip(pixmap,&ge->buttonrect,&old1);

	GBoxDrawBackground(pixmap,&ge->buttonrect,g->box,
		g->state==gs_enabled? gs_pressedactive: g->state,false);
	GBoxDrawBorder(pixmap,&ge->buttonrect,g->box,g->state,false);

	r.width = marklen;
	r.x = ge->buttonrect.x + (ge->buttonrect.width - marklen)/2;
	r.height = 2*GDrawPointsToPixels(pixmap,_GListMark_Box.border_width) +
		    GDrawPointsToPixels(pixmap,3);
	r.y = g->inner.y + (g->inner.height-r.height)/2;
	GDrawPushClip(pixmap,&r,&old2);

	GBoxDrawBackground(pixmap,&r,&_GListMark_Box, g->state,false);
	GBoxDrawBorder(pixmap,&r,&_GListMark_Box,g->state,false);
	GDrawPopClip(pixmap,&old2);
	GDrawPopClip(pixmap,&old1);
    } else if ( gt->numericfield ) {
	int y;
	int half;
	GPoint pts[5];

	GBoxDrawBackground(pixmap,&ge->buttonrect,g->box,
		g->state==gs_enabled? gs_pressedactive: g->state,false);
	/* GBoxDrawBorder(pixmap,&ge->buttonrect,g->box,g->state,false); */
	/* GDrawDrawRect(pixmap,&ge->buttonrect,fg); */

	y = ge->buttonrect.y + ge->buttonrect.height/2;
	pts[0].x = ge->buttonrect.x+3;
	pts[1].x = ge->buttonrect.x+ge->buttonrect.width-3;
	pts[2].x = ge->buttonrect.x + ge->buttonrect.width/2;
	half = pts[2].x-pts[0].x;
	GDrawDrawLine(pixmap, ge->buttonrect.x,y, ge->buttonrect.x+ge->buttonrect.width,y, fg );
	pts[0].y = pts[1].y = y-2;
	pts[2].y = pts[1].y-half;
	pts[3] = pts[0];
	GDrawFillPoly(pixmap,pts,3,fg);
	pts[0].y = pts[1].y = y+2;
	pts[2].y = pts[1].y+half;
	pts[3] = pts[0];
	GDrawFillPoly(pixmap,pts,3,fg);
    }
return( true );
}

static int glistfield_mouse(GListField *ge, GEvent *event) {
    if ( event->type!=et_mousedown )
return( true );
    if ( ge->popup != NULL ) {
	GDrawDestroyWindow(ge->popup);
	ge->popup = NULL;
return( true );
    }
    ge->popup = GListPopupCreate(&ge->gt.g,GListFieldSelected,ge->ti);
return( true );
}

static int gnumericfield_mouse(GTextField *gt, GEvent *event) {
    GListField *ge = (GListField *) gt;
    if ( event->type==et_mousedown ) {
	gt->incr_down = event->u.mouse.y > (ge->buttonrect.y + ge->buttonrect.height/2);
	GTextFieldIncrement(gt,gt->incr_down?-1:1);
	if ( gt->numeric_scroll==NULL )
	    gt->numeric_scroll = GDrawRequestTimer(gt->g.base,200,100,NULL);
    } else if ( gt->numeric_scroll!=NULL ) {
	GDrawCancelTimer(gt->numeric_scroll);
	gt->numeric_scroll = NULL;
    }
return( true );
}

static int GTextFieldDoDrop(GTextField *gt,GEvent *event,int endpos) {

    if ( gt->has_dd_cursor )
	GTextFieldDrawDDCursor(gt,gt->dd_cursor_pos);

    if ( event->type == et_mousemove ) {
	if ( GGadgetInnerWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos<gt->sel_start || endpos>=gt->sel_end )
		GTextFieldDrawDDCursor(gt,endpos);
	} else if ( !GGadgetWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    GDrawPostDragEvent(gt->g.base,event,et_drag);
	}
    } else {
	if ( GGadgetInnerWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( endpos>=gt->sel_start && endpos<gt->sel_end ) {
		gt->sel_start = gt->sel_end = endpos;
	    } else {
		unichar_t *old=gt->oldtext, *temp;
		int pos=0;
		if ( event->u.mouse.state&ksm_control ) {
		    temp = galloc((u_strlen(gt->text)+gt->sel_end-gt->sel_start+1)*sizeof(unichar_t));
		    memcpy(temp,gt->text,endpos*sizeof(unichar_t));
		    memcpy(temp+endpos,gt->text+gt->sel_start,
			    (gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    u_strcpy(temp+endpos+gt->sel_end-gt->sel_start,gt->text+endpos);
		} else if ( endpos>=gt->sel_end ) {
		    temp = u_copy(gt->text);
		    memcpy(temp+gt->sel_start,temp+gt->sel_end,
			    (endpos-gt->sel_end)*sizeof(unichar_t));
		    memcpy(temp+endpos-(gt->sel_end-gt->sel_start),
			    gt->text+gt->sel_start,(gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    pos = endpos;
		} else /*if ( endpos<gt->sel_start )*/ {
		    temp = u_copy(gt->text);
		    memcpy(temp+endpos,gt->text+gt->sel_start,
			    (gt->sel_end-gt->sel_start)*sizeof(unichar_t));
		    memcpy(temp+endpos+gt->sel_end-gt->sel_start,gt->text+endpos,
			    (gt->sel_start-endpos)*sizeof(unichar_t));
		    pos = endpos+gt->sel_end-gt->sel_start;
		}
		gt->oldtext = gt->text;
		gt->sel_oldstart = gt->sel_start;
		gt->sel_oldend = gt->sel_end;
		gt->sel_oldbase = gt->sel_base;
		gt->sel_start = gt->sel_end = gt->sel_end = pos;
		gt->text = temp;
		free(old);
		GTextFieldRefigureLines(gt, endpos<gt->sel_oldstart?endpos:gt->sel_oldstart);
	    }
	} else if ( !GGadgetWithin(&gt->g,event->u.mouse.x,event->u.mouse.y) ) {
	    /* Don't delete the selection until someone actually accepts the drop */
	    /* Don't delete at all (copy not move) if control key is down */
	    if ( ( event->u.mouse.state&ksm_control ) )
		GTextFieldGrabSelection(gt,sn_drag_and_drop);
	    else
		GTextFieldGrabDDSelection(gt);
	    GDrawPostDragEvent(gt->g.base,event,et_drop);
	}
	gt->drag_and_drop = false;
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	_ggadget_redraw(&gt->g);
    }
return( false );
}
    
static int gtextfield_mouse(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    GListField *ge = (GListField *) g;
    unichar_t *end=NULL, *end1, *end2;
    int i=0,ll;
    unichar_t *bitext = gt->dobitext || gt->password?gt->bidata.text:gt->text;

    if ( gt->hidden_cursor ) {
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	gt->hidden_cursor = false;
	_GWidget_ClearGrabGadget(g);
    }
    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( event->type == et_crossing )
return( false );
    if (( gt->listfield && event->u.mouse.x>=ge->buttonrect.x &&
	    event->u.mouse.x<ge->buttonrect.x+ge->buttonrect.width &&
	    event->u.mouse.y>=ge->buttonrect.y &&
	    event->u.mouse.y<ge->buttonrect.y+ge->buttonrect.height ) ||
	( gt->listfield && ge->popup!=NULL ))
return( glistfield_mouse(ge,event));
    if ( gt->numericfield && event->u.mouse.x>=ge->buttonrect.x &&
	    event->u.mouse.x<ge->buttonrect.x+ge->buttonrect.width &&
	    event->u.mouse.y>=ge->buttonrect.y &&
	    event->u.mouse.y<ge->buttonrect.y+ge->buttonrect.height )
return( gnumericfield_mouse(gt,event));
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button==4 || event->u.mouse.button==5) &&
	    gt->vsb!=NULL )
return( GGadgetDispatchEvent(&gt->vsb->g,event));

    if ( gt->pressed==NULL && event->type == et_mousemove && g->popup_msg!=NULL &&
	    GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y))
	GGadgetPreparePopup(g->base,g->popup_msg);

    if ( event->type == et_mousedown || gt->pressed ) {
	i = (event->u.mouse.y-g->inner.y)/gt->fh + gt->loff_top;
	if ( i<0 ) i = 0;
	if ( !gt->multi_line ) i = 0;
	if ( i>=gt->lcnt )
	    end = gt->text+u_strlen(gt->text);
	else
	    end = GTextFieldGetPtFromPos(gt,i,event->u.mouse.x);
    }

    if ( event->type == et_mousedown ) {
	if ( i>=gt->lcnt )
	    end1 = end2 = end;
	else {
	    ll = gt->lines[i+1]==-1?-1:gt->lines[i+1]-gt->lines[i]-1;
	    GDrawGetTextPtBeforePos(g->base,bitext+gt->lines[i], ll, NULL,
		    event->u.mouse.x-g->inner.x+gt->xoff_left, &end1);
	    GDrawGetTextPtAfterPos(g->base,bitext+gt->lines[i], ll, NULL,
		    event->u.mouse.x-g->inner.x+gt->xoff_left, &end2);
	    if ( gt->dobitext ) {
		end1 = gt->bidata.original[end1-gt->bidata.text];
		end2 = gt->bidata.original[end2-gt->bidata.text];
	    } else if ( gt->password ) {
		end1 = gt->text + (end1-gt->bidata.text);
		end2 = gt->text + (end2-gt->bidata.text);
	    }
	}
	gt->wordsel = gt->linesel = false;
	if ( event->u.mouse.button==1 && event->u.mouse.clicks>=3 ) {
	    gt->sel_start = gt->lines[i]; gt->sel_end = gt->lines[i+1];
	    if ( gt->sel_end==-1 ) gt->sel_end = u_strlen(gt->text);
	    gt->wordsel = false; gt->linesel = true;
	} else if ( event->u.mouse.button==1 && event->u.mouse.clicks==2 ) {
	    gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	    gt->wordsel = true;
	    GTextFieldSelectWords(gt,gt->sel_base);
	} else if ( end1-gt->text>=gt->sel_start && end2-gt->text<gt->sel_end &&
		gt->sel_start!=gt->sel_end &&
		event->u.mouse.button==1 ) {
	    gt->drag_and_drop = true;
	    if ( !gt->hidden_cursor )
		gt->old_cursor = GDrawGetCursor(gt->g.base);
	    GDrawSetCursor(gt->g.base,ct_draganddrop);
	} else if ( /*event->u.mouse.button!=3 &&*/ !(event->u.mouse.state&ksm_shift) ) {
	    if ( event->u.mouse.button==1 )
		GTextFieldGrabPrimarySelection(gt);
	    gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	} else if ( end-gt->text>gt->sel_base ) {
	    gt->sel_start = gt->sel_base;
	    gt->sel_end = end-gt->text;
	} else {
	    gt->sel_start = end-gt->text;
	    gt->sel_end = gt->sel_base;
	}

	if ( event->u.mouse.button==3 &&
		GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	    GTFPopupMenu(gt,event);
return( true );
	}

	if ( gt->pressed==NULL )
	    gt->pressed = GDrawRequestTimer(gt->g.base,200,100,NULL);
	if ( gt->sel_start > u_strlen( gt->text ))	/* Ok to have selection at end, but beyond is an error */
	    fprintf( stderr, "About to crash\n" );
	_ggadget_redraw(g);
return( true );
    } else if ( gt->pressed && (event->type == et_mousemove || event->type == et_mouseup )) {
	int refresh = true;

	if ( gt->drag_and_drop ) {
	    refresh = GTextFieldDoDrop(gt,event,end-gt->text);
	} else if ( gt->linesel ) {
	    int j, e;
	    gt->sel_start = gt->lines[i]; gt->sel_end = gt->lines[i+1];
	    if ( gt->sel_end==-1 ) gt->sel_end = u_strlen(gt->text);
	    for ( j=0; gt->lines[i+1]!=-1 && gt->sel_base>=gt->lines[i+1]; ++j );
	    if ( gt->sel_start<gt->lines[i] ) gt->sel_start = gt->lines[i];
	    e = gt->lines[j+1]==-1 ? u_strlen(gt->text): gt->lines[j+1];
	    if ( e>gt->sel_end ) gt->sel_end = e;
	} else if ( gt->wordsel )
	    GTextFieldSelectWords(gt,end-gt->text);
	else if ( event->u.mouse.button!=2 ) {
	    int e = end-gt->text;
	    if ( e>gt->sel_base ) {
		gt->sel_start = gt->sel_base; gt->sel_end = e;
	    } else {
		gt->sel_start = e; gt->sel_end = gt->sel_base;
	    }
	}
	if ( event->type==et_mouseup ) {
	    GDrawCancelTimer(gt->pressed); gt->pressed = NULL;
	    if ( event->u.mouse.button==2 )
		GTextFieldPaste(gt,sn_primary);
	    if ( gt->sel_start==gt->sel_end )
		GTextField_Show(gt,gt->sel_start);
	    GTextFieldChanged(gt,-1);
	    if ( gt->sel_start<gt->sel_end && _GDraw_InsCharHook!=NULL && !gt->donthook )
		(_GDraw_InsCharHook)(GDrawGetDisplayOfWindow(gt->g.base),
			gt->text[gt->sel_start]);
	}
	if ( gt->sel_end > u_strlen( gt->text ))
	    fprintf( stderr, "About to crash\n" );
	if ( refresh )
	    _ggadget_redraw(g);
return( true );
    }
return( false );
}

static int gtextfield_key(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( gt->listfield && ((GListField *) gt)->popup!=NULL ) {
	GWindow popup = ((GListField *) gt)->popup;
	(GDrawGetEH(popup))(popup,event);
return( true );
    }

    if ( event->type == et_charup )
return( false );
    if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ||
	    (event->u.chr.keysym == GK_Return && !gt->accepts_returns ) ||
	    ( event->u.chr.keysym == GK_Tab && !gt->accepts_tabs ) ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    if ( !gt->hidden_cursor ) {	/* hide the mouse pointer */
	if ( !gt->drag_and_drop )
	    gt->old_cursor = GDrawGetCursor(gt->g.base);
	GDrawSetCursor(g->base,ct_invisible);
	gt->hidden_cursor = true;
	_GWidget_SetGrabGadget(g);	/* so that we get the next mouse movement to turn the cursor on */
    }
    if( gt->cursor_on ) {	/* undraw the blinky text cursor if it is drawn */
	gt_draw_cursor(g->base, gt);
	gt->cursor_on = false;
    }

    switch ( GTextFieldDoChange(gt,event)) {
      case 2:
      break;
      case true:
	GTextFieldChanged(gt,-1);
      break;
      case false:
return( false );
    }
    _ggadget_redraw(g);
return( true );
}

static int gtextfield_focus(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( g->state == gs_invisible || g->state == gs_disabled )
return( false );

    if ( gt->cursor!=NULL ) {
	GDrawCancelTimer(gt->cursor);
	gt->cursor = NULL;
	gt->cursor_on = false;
    }
    if ( gt->hidden_cursor && !event->u.focus.gained_focus ) {
	GDrawSetCursor(gt->g.base,gt->old_cursor);
	gt->hidden_cursor = false;
    }
    gt->g.has_focus = event->u.focus.gained_focus;
    if ( event->u.focus.gained_focus ) {
	gt->cursor = GDrawRequestTimer(gt->g.base,400,400,NULL);
	gt->cursor_on = true;
	if ( event->u.focus.mnemonic_focus != mf_normal )
	    GTextFieldSelect(&gt->g,0,-1);
	if ( gt->gic!=NULL )
	    GTPositionGIC(gt);
	else if ( GWidgetGetInputContext(gt->g.base)!=NULL )
	    GDrawSetGIC(gt->g.base,GWidgetGetInputContext(gt->g.base),10000,10000);
    }
    _ggadget_redraw(g);
    GTextFieldFocusChanged(gt,event->u.focus.gained_focus);
return( true );
}

static int gtextfield_timer(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false);
    if ( gt->cursor == event->u.timer.timer ) {
	if ( gt->cursor_on ) {
	    gt_draw_cursor(g->base, gt);
	    gt->cursor_on = false;
	} else {
	    gt->cursor_on = true;
	    gt_draw_cursor(g->base, gt);
	}
return( true );
    }
    if ( gt->numeric_scroll == event->u.timer.timer ) {
	GTextFieldIncrement(gt,gt->incr_down?-1:1);
return( true );
    }
    if ( gt->pressed == event->u.timer.timer ) {
	GEvent e;
	GDrawSetFont(g->base,gt->font);
	GDrawGetPointerPosition(g->base,&e);
	if ( (e.u.mouse.x<g->r.x && gt->xoff_left>0 ) ||
		(gt->multi_line && e.u.mouse.y<g->r.y && gt->loff_top>0 ) ||
		( e.u.mouse.x >= g->r.x + g->r.width &&
			gt->xmax-gt->xoff_left>g->inner.width ) ||
		( e.u.mouse.y >= g->r.y + g->r.height &&
			gt->lcnt-gt->loff_top > g->inner.height/gt->fh )) {
	    int l = gt->loff_top + (e.u.mouse.y-g->inner.y)/gt->fh;
	    int xpos; unichar_t *end;

	    if ( e.u.mouse.y<g->r.y && gt->loff_top>0 )
		l = --gt->loff_top;
	    else if ( e.u.mouse.y >= g->r.y + g->r.height &&
			    gt->lcnt-gt->loff_top > g->inner.height/gt->fh ) {
		++gt->loff_top;
		l = gt->loff_top + g->inner.width/gt->fh;
	    } else if ( l<gt->loff_top )
		l = gt->loff_top; 
	    else if ( l>=gt->loff_top + g->inner.height/gt->fh ) 
		l = gt->loff_top + g->inner.height/gt->fh-1;
	    if ( l>=gt->lcnt ) l = gt->lcnt-1;

	    xpos = e.u.mouse.x+gt->xoff_left;
	    if ( e.u.mouse.x<g->r.x && gt->xoff_left>0 ) {
		gt->xoff_left -= gt->nw;
		xpos = g->inner.x + gt->xoff_left;
	    } else if ( e.u.mouse.x >= g->r.x + g->r.width &&
			    gt->xmax-gt->xoff_left>g->inner.width ) {
		gt->xoff_left += gt->nw;
		xpos = g->inner.x + gt->xoff_left + g->inner.width;
	    }

	    end = GTextFieldGetPtFromPos(gt,l,xpos);
	    if ( end-gt->text > gt->sel_base ) {
		gt->sel_start = gt->sel_base;
		gt->sel_end = end-gt->text;
	    } else {
		gt->sel_start = end-gt->text;
		gt->sel_end = gt->sel_base;
	    }
	    _ggadget_redraw(g);
	    if ( gt->vsb!=NULL )
		GScrollBarSetPos(&gt->vsb->g,gt->loff_top);
	    if ( gt->hsb!=NULL )
		GScrollBarSetPos(&gt->hsb->g,gt->xoff_left);
	}
return( true );
    }
return( false );
}

static int gtextfield_sel(GGadget *g, GEvent *event) {
    GTextField *gt = (GTextField *) g;
    unichar_t *end;
    int i;

    if ( event->type == et_selclear ) {
	if ( event->u.selclear.sel==sn_primary && gt->sel_start!=gt->sel_end ) {
	    gt->sel_start = gt->sel_end = gt->sel_base;
	    _ggadget_redraw(g);
return( true );
	}
return( false );
    }

    if ( gt->has_dd_cursor )
	GTextFieldDrawDDCursor(gt,gt->dd_cursor_pos);
    GDrawSetFont(g->base,gt->font);
    i = (event->u.drag_drop.y-g->inner.y)/gt->fh + gt->loff_top;
    if ( !gt->multi_line ) i = 0;
    if ( i>=gt->lcnt )
	end = gt->text+u_strlen(gt->text);
    else
	end = GTextFieldGetPtFromPos(gt,i,event->u.drag_drop.x);
    if ( event->type == et_drag ) {
	GTextFieldDrawDDCursor(gt,end-gt->text);
    } else if ( event->type == et_dragout ) {
	/* this event exists simply to clear the dd cursor line. We've done */
	/*  that already */ 
    } else if ( event->type == et_drop ) {
	gt->sel_start = gt->sel_end = gt->sel_base = end-gt->text;
	GTextFieldPaste(gt,sn_drag_and_drop);
	GTextField_Show(gt,gt->sel_start);
	_ggadget_redraw(&gt->g);
    } else
return( false );

return( true );
}

static void gtextfield_destroy(GGadget *g) {
    GTextField *gt = (GTextField *) g;

    if ( gt==NULL )
return;
    if ( gt->listfield ) {
	GListField *glf = (GListField *) g;
	if ( glf->popup ) {
	    GDrawDestroyWindow(glf->popup);
	    GDrawSync(NULL);
	    GDrawProcessWindowEvents(glf->popup);	/* popup's destroy routine must execute before we die */
	}
	GTextInfoArrayFree(glf->ti);
    }

    if ( gt->vsb!=NULL )
	(gt->vsb->g.funcs->destroy)(&gt->vsb->g);
    if ( gt->hsb!=NULL )
	(gt->hsb->g.funcs->destroy)(&gt->hsb->g);
    GDrawCancelTimer(gt->numeric_scroll);
    GDrawCancelTimer(gt->pressed);
    GDrawCancelTimer(gt->cursor);
    free(gt->lines);
    free(gt->oldtext);
    free(gt->text);
    free(gt->bidata.text);
    free(gt->bidata.level);
    free(gt->bidata.override);
    free(gt->bidata.type);
    free(gt->bidata.original);
    _ggadget_destroy(g);
}

static void GTextFieldSetTitle(GGadget *g,const unichar_t *tit) {
    GTextField *gt = (GTextField *) g;
    unichar_t *old = gt->oldtext;
    if ( u_strcmp(tit,gt->text)==0 )	/* If it doesn't change anything, then don't trash undoes or selection */
return;
    gt->oldtext = gt->text;
    gt->sel_oldstart = gt->sel_start; gt->sel_oldend = gt->sel_end; gt->sel_oldbase = gt->sel_base;
    gt->text = u_copy(tit);		/* tit might be oldtext, so must copy before freeing */
    free(old);
    gt->sel_start = gt->sel_end = gt->sel_base = u_strlen(tit);
    GTextFieldRefigureLines(gt,0);
    GTextField_Show(gt,gt->sel_start);
    _ggadget_redraw(g);
}

static const unichar_t *_GTextFieldGetTitle(GGadget *g) {
    GTextField *gt = (GTextField *) g;
return( gt->text );
}

static void GTextFieldSetFont(GGadget *g,FontInstance *new) {
    GTextField *gt = (GTextField *) g;
    gt->font = new;
    GTextFieldRefigureLines(gt,0);
}

static FontInstance *GTextFieldGetFont(GGadget *g) {
    GTextField *gt = (GTextField *) g;
return( gt->font );
}

void GTextFieldShow(GGadget *g,int pos) {
    GTextField *gt = (GTextField *) g;

    GTextField_Show(gt,pos);
    _ggadget_redraw(g);
}

void GTextFieldSelect(GGadget *g,int start, int end) {
    GTextField *gt = (GTextField *) g;

    GTextFieldGrabPrimarySelection(gt);
    if ( end<0 ) {
	end = u_strlen(gt->text);
	if ( start<0 ) start = end;
    }
    if ( start>end ) { int temp = start; start = end; end = temp; }
    if ( end>u_strlen(gt->text)) end = u_strlen(gt->text);
    if ( start>u_strlen(gt->text)) start = end;
    else if ( start<0 ) start=0;
    gt->sel_start = gt->sel_base = start;
    gt->sel_end = end;
    _ggadget_redraw(g);			/* Should be safe just to draw the textfield gadget, sbs won't have changed */
}

void GTextFieldReplace(GGadget *g,const unichar_t *txt) {
    GTextField *gt = (GTextField *) g;

    GTextField_Replace(gt,txt);
    _ggadget_redraw(g);
}

static void GListFSelectOne(GGadget *g, int32 pos) {
    GListField *gl = (GListField *) g;
    int i;

    for ( i=0; i<gl->ltot; ++i )
	gl->ti[pos]->selected = false;
    if ( pos>=gl->ltot ) pos = gl->ltot-1;
    if ( pos<0 ) pos = 0;
    if ( gl->ltot>0 ) {
	gl->ti[pos]->selected = true;
	GTextFieldSetTitle(g,gl->ti[pos]->text);
    }
}

static int32 GListFIsSelected(GGadget *g, int32 pos) {
    GListField *gl = (GListField *) g;

    if ( pos>=gl->ltot )
return( false );
    if ( pos<0 )
return( false );
    if ( gl->ltot>0 )
return( gl->ti[pos]->selected );

return( false );
}

static int32 GListFGetFirst(GGadget *g) {
    int i;
    GListField *gl = (GListField *) g;

    for ( i=0; i<gl->ltot; ++i )
	if ( gl->ti[i]->selected )
return( i );

return( -1 );
}

static GTextInfo **GListFGet(GGadget *g,int32 *len) {
    GListField *gl = (GListField *) g;
    if ( len!=NULL ) *len = gl->ltot;
return( gl->ti );
}

static GTextInfo *GListFGetItem(GGadget *g,int32 pos) {
    GListField *gl = (GListField *) g;
    if ( pos<0 || pos>=gl->ltot )
return( NULL );

return(gl->ti[pos]);
}

static void GListFSet(GGadget *g,GTextInfo **ti,int32 docopy) {
    GListField *gl = (GListField *) g;

    GTextInfoArrayFree(gl->ti);
    if ( docopy || ti==NULL )
	ti = GTextInfoArrayCopy(ti);
    gl->ti = ti;
    gl->ltot = GTextInfoArrayCount(ti);
}

static void GListFClear(GGadget *g) {
    GListFSet(g,NULL,true);
}

static void gtextfield_redraw(GGadget *g) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL )
	_ggadget_redraw((GGadget *) (gt->vsb));
    if ( gt->hsb!=NULL )
	_ggadget_redraw((GGadget *) (gt->hsb));
    _ggadget_redraw(g);
}

static void gtextfield_move(GGadget *g, int32 x, int32 y ) {
    GTextField *gt = (GTextField *) g;
    int fxo=0, fyo=0, bxo, byo;

    if ( gt->listfield || gt->numericfield ) {
	fxo = ((GListField *) gt)->fieldrect.x - g->r.x;
	fyo = ((GListField *) gt)->fieldrect.y - g->r.y;
	bxo = ((GListField *) gt)->buttonrect.x - g->r.x;
	byo = ((GListField *) gt)->buttonrect.y - g->r.y;
    }
    if ( gt->vsb!=NULL )
	_ggadget_move((GGadget *) (gt->vsb),x+(gt->vsb->g.r.x-g->r.x),y);
    if ( gt->hsb!=NULL )
	_ggadget_move((GGadget *) (gt->hsb),x,y+(gt->hsb->g.r.y-g->r.y));
    _ggadget_move(g,x,y);
    if ( gt->listfield || gt->numericfield ) {
	((GListField *) gt)->fieldrect.x = g->r.x + fxo;
	((GListField *) gt)->fieldrect.y = g->r.y + fyo;
	((GListField *) gt)->buttonrect.x = g->r.x + bxo;
	((GListField *) gt)->buttonrect.y = g->r.y + byo;
    }
}

static void gtextfield_resize(GGadget *g, int32 width, int32 height ) {
    GTextField *gt = (GTextField *) g;
    int gtwidth=width, gtheight=height, oldheight=0;
    int fxo=0, fwo=0, fyo=0, bxo, byo;
    int l;

    if ( gt->listfield || gt->numericfield ) {
	fxo = ((GListField *) gt)->fieldrect.x - g->r.x;
	fwo = g->r.width - ((GListField *) gt)->fieldrect.width;
	fyo = ((GListField *) gt)->fieldrect.y - g->r.y;
	bxo = g->r.x+g->r.width - ((GListField *) gt)->buttonrect.x;
	byo = ((GListField *) gt)->buttonrect.y - g->r.y;
    }
    if ( gt->hsb!=NULL ) {
	oldheight = gt->hsb->g.r.y+gt->hsb->g.r.height-g->r.y;
	gtheight = height - (oldheight-g->r.height);
    }
    if ( gt->vsb!=NULL ) {
	int oldwidth = gt->vsb->g.r.x+gt->vsb->g.r.width-g->r.x;
	gtwidth = width - (oldwidth-g->r.width);
	_ggadget_move((GGadget *) (gt->vsb),gt->vsb->g.r.x+width-oldwidth,gt->vsb->g.r.y);
	_ggadget_resize((GGadget *) (gt->vsb),gt->vsb->g.r.width,gtheight);
    }
    if ( gt->hsb!=NULL ) {
	_ggadget_move((GGadget *) (gt->hsb),gt->hsb->g.r.x,gt->hsb->g.r.y+height-oldheight);
	_ggadget_resize((GGadget *) (gt->hsb),gtwidth,gt->hsb->g.r.height);
    }
    _ggadget_resize(g,gtwidth, gtheight);

    GTextFieldRefigureLines(gt,0);
    if ( gt->vsb!=NULL ) {
	GScrollBarSetBounds(&gt->vsb->g,0,gt->lcnt-1,gt->g.inner.height/gt->fh);
	l = gt->loff_top;
	if ( gt->loff_top>gt->lcnt-gt->g.inner.height/gt->fh )
	    l = gt->lcnt-gt->g.inner.height/gt->fh;
	if ( l<0 ) l = 0;
	if ( l!=gt->loff_top ) {
	    gt->loff_top = l;
	    GScrollBarSetPos(&gt->vsb->g,l);
	    _ggadget_redraw(&gt->g);
	}
    }
    if ( gt->listfield || gt->numericfield) {
	((GListField *) gt)->fieldrect.x = g->r.x + fxo;
	((GListField *) gt)->fieldrect.width = g->r.width -fwo;
	((GListField *) gt)->fieldrect.y = g->r.y + fyo;
	((GListField *) gt)->buttonrect.x = g->r.x+g->r.width - bxo;
	((GListField *) gt)->buttonrect.y = g->r.y + byo;
    }
}

static GRect *gtextfield_getsize(GGadget *g, GRect *r ) {
    GTextField *gt = (GTextField *) g;
    _ggadget_getsize(g,r);
    if ( gt->vsb!=NULL )
	r->width =  gt->vsb->g.r.x+gt->vsb->g.r.width-g->r.x;
    if ( gt->hsb!=NULL )
	r->height =  gt->hsb->g.r.y+gt->hsb->g.r.height-g->r.y;
return( r );
}

static void gtextfield_setvisible(GGadget *g, int visible ) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL ) _ggadget_setvisible(&gt->vsb->g,visible);
    if ( gt->hsb!=NULL ) _ggadget_setvisible(&gt->hsb->g,visible);
    _ggadget_setvisible(g,visible);
}

static void gtextfield_setenabled(GGadget *g, int enabled ) {
    GTextField *gt = (GTextField *) g;
    if ( gt->vsb!=NULL ) _ggadget_setenabled(&gt->vsb->g,enabled);
    if ( gt->hsb!=NULL ) _ggadget_setenabled(&gt->hsb->g,enabled);
    _ggadget_setenabled(g,enabled);
}

static int gtextfield_vscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GTextField *gt = (GTextField *) (g->data);
    int loff = gt->loff_top;

    g = (GGadget *) gt;

    if ( sbt==et_sb_top )
	loff = 0;
    else if ( sbt==et_sb_bottom ) {
	loff = gt->lcnt - gt->g.inner.height/gt->fh;
    } else if ( sbt==et_sb_up ) {
	if ( gt->loff_top!=0 ) loff = gt->loff_top-1; else loff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( gt->loff_top + gt->g.inner.height/gt->fh >= gt->lcnt )
	    loff = gt->lcnt - gt->g.inner.height/gt->fh;
	else
	    ++loff;
    } else if ( sbt==et_sb_uppage ) {
	int page = g->inner.height/gt->fh- (g->inner.height/gt->fh>2?1:0);
	loff = gt->loff_top - page;
	if ( loff<0 ) loff=0;
    } else if ( sbt==et_sb_downpage ) {
	int page = g->inner.height/gt->fh- (g->inner.height/gt->fh>2?1:0);
	loff = gt->loff_top + page;
	if ( loff + gt->g.inner.height/gt->fh >= gt->lcnt )
	    loff = gt->lcnt - gt->g.inner.height/gt->fh;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	loff = event->u.control.u.sb.pos;
    }
    if ( loff + gt->g.inner.height/gt->fh >= gt->lcnt )
	loff = gt->lcnt - gt->g.inner.height/gt->fh;
    if ( loff<0 ) loff = 0;
    if ( loff!=gt->loff_top ) {
	gt->loff_top = loff;
	GScrollBarSetPos(&gt->vsb->g,loff);
	_ggadget_redraw(&gt->g);
    }
return( true );
}

static int gtextfield_hscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GTextField *gt = (GTextField *) (g->data);
    int xoff = gt->xoff_left;

    g = (GGadget *) gt;

    if ( sbt==et_sb_top )
	xoff = 0;
    else if ( sbt==et_sb_bottom ) {
	xoff = gt->xmax - gt->g.inner.width;
	if ( xoff<0 ) xoff = 0;
    } else if ( sbt==et_sb_up ) {
	if ( gt->xoff_left>gt->nw ) xoff = gt->xoff_left-gt->nw; else xoff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( gt->xoff_left + gt->nw + gt->g.inner.width >= gt->xmax )
	    xoff = gt->xmax - gt->g.inner.width;
	else
	    xoff += gt->nw;
    } else if ( sbt==et_sb_uppage ) {
	int page = (3*g->inner.width)/4;
	xoff = gt->xoff_left - page;
	if ( xoff<0 ) xoff=0;
    } else if ( sbt==et_sb_downpage ) {
	int page = (3*g->inner.width)/4;
	xoff = gt->xoff_left + page;
	if ( xoff + gt->g.inner.width >= gt->xmax )
	    xoff = gt->xmax - gt->g.inner.width;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	xoff = event->u.control.u.sb.pos;
    }
    if ( xoff + gt->g.inner.width >= gt->xmax )
	xoff = gt->xmax - gt->g.inner.width;
    if ( xoff<0 ) xoff = 0;
    if ( gt->xoff_left!=xoff ) {
	gt->xoff_left = xoff;
	GScrollBarSetPos(&gt->hsb->g,xoff);
	_ggadget_redraw(&gt->g);
    }
return( true );
}

static void GTextFieldSetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GTextField *gt = (GTextField *) g;

    if ( outer!=NULL ) {
	g->desired_width = outer->width;
	g->desired_height = outer->height;
    } else if ( inner!=NULL ) {
	int bp = GBoxBorderWidth(g->base,g->box);
	int extra=0;

	if ( gt->listfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    2*GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    GBoxBorderWidth(gt->g.base,&_GListMark_Box);
	}
	g->desired_width = inner->width + 2*bp + extra;
	g->desired_height = inner->height + 2*bp;
	if ( gt->multi_line ) {
	    int sbadd = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width) +
		    GDrawPointsToPixels(gt->g.base,1);
	    g->desired_width += sbadd;
	    if ( !gt->wrap )
		g->desired_height += sbadd;
	}
    }
}

static void GTextFieldGetDesiredSize(GGadget *g,GRect *outer,GRect *inner) {
    GTextField *gt = (GTextField *) g;
    int width=0, height;
    int extra=0;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( gt->listfield ) {
	extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		2*GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		GBoxBorderWidth(gt->g.base,&_GListMark_Box);
    }

    width = GGadgetScale(GDrawPointsToPixels(gt->g.base,80));
    height = gt->multi_line? 4*gt->fh:gt->fh;

    if ( g->desired_width>extra+2*bp ) width = g->desired_width - extra - 2*bp;
    if ( g->desired_height>2*bp ) height = g->desired_height - 2*bp;

    if ( gt->multi_line ) {
	int sbadd = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width) +
		GDrawPointsToPixels(gt->g.base,1);
	width += sbadd;
	if ( !gt->wrap )
	    height += sbadd;
    }

    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width + extra + 2*bp;
	outer->height = height + 2*bp;
    }
}

static int gtextfield_FillsWindow(GGadget *g) {
return( ((GTextField *) g)->multi_line && g->prev==NULL &&
	(_GWidgetGetGadgets(g->base)==g ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GTextField *) g)->vsb ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GTextField *) g)->hsb ));
}

struct gfuncs gtextfield_funcs = {
    0,
    sizeof(struct gfuncs),

    gtextfield_expose,
    gtextfield_mouse,
    gtextfield_key,
    _gtextfield_editcmd,
    gtextfield_focus,
    gtextfield_timer,
    gtextfield_sel,

    gtextfield_redraw,
    gtextfield_move,
    gtextfield_resize,
    gtextfield_setvisible,
    gtextfield_setenabled,
    gtextfield_getsize,
    _ggadget_getinnersize,

    gtextfield_destroy,

    GTextFieldSetTitle,
    _GTextFieldGetTitle,
    NULL,
    NULL,
    NULL,
    GTextFieldSetFont,
    GTextFieldGetFont,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GTextFieldGetDesiredSize,
    GTextFieldSetDesiredSize,
    gtextfield_FillsWindow
};

struct gfuncs glistfield_funcs = {
    0,
    sizeof(struct gfuncs),

    gtextfield_expose,
    gtextfield_mouse,
    gtextfield_key,
    gtextfield_editcmd,
    gtextfield_focus,
    gtextfield_timer,
    gtextfield_sel,

    gtextfield_redraw,
    gtextfield_move,
    gtextfield_resize,
    gtextfield_setvisible,
    gtextfield_setenabled,
    gtextfield_getsize,
    _ggadget_getinnersize,

    gtextfield_destroy,

    GTextFieldSetTitle,
    _GTextFieldGetTitle,
    NULL,
    NULL,
    NULL,
    GTextFieldSetFont,
    GTextFieldGetFont,

    GListFClear,
    GListFSet,
    GListFGet,
    GListFGetItem,
    NULL,
    GListFSelectOne,
    GListFIsSelected,
    GListFGetFirst,
    NULL,
    NULL,
    NULL,

    GTextFieldGetDesiredSize,
    GTextFieldSetDesiredSize
};

static void GTextFieldInit() {
    static unichar_t courier[] = { 'c', 'o', 'u', 'r', 'i', 'e', 'r', ',', 'm','o','n','o','s','p','a','c','e',',','c','l','e','a','r','l','y','u',',', 'u','n','i','f','o','n','t', '\0' };
    FontRequest rq;

    GGadgetInit();
    GDrawDecomposeFont(_ggadget_default_font,&rq);
    rq.family_name = courier;
    gtextfield_font = GDrawInstanciateFont(screen_display,&rq);
    _GGadgetCopyDefaultBox(&gtextfield_box);
    gtextfield_box.padding = 3;
    gtextfield_box.flags = box_active_border_inner;
    gtextfield_font = _GGadgetInitDefaultBox("GTextField.",&gtextfield_box,gtextfield_font);
    gtextfield_inited = true;
}

static void GTextFieldAddVSb(GTextField *gt) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.y = gt->g.r.y; gd.pos.height = gt->g.r.height;
    gd.pos.width = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width);
    gd.pos.x = gt->g.r.x+gt->g.r.width - gd.pos.width;
    gd.flags = (gt->g.state==gs_invisible?0:gg_visible)|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    gd.handle_controlevent = gtextfield_vscroll;
    gt->vsb = (GScrollBar *) GScrollBarCreate(gt->g.base,&gd,gt);
    gt->vsb->g.contained = true;

    gd.pos.width += GDrawPointsToPixels(gt->g.base,1);
    gt->g.r.width -= gd.pos.width;
    gt->g.inner.width -= gd.pos.width;
}

static void GTextFieldAddHSb(GTextField *gt) {
    GGadgetData gd;

    memset(&gd,'\0',sizeof(gd));
    gd.pos.x = gt->g.r.x; gd.pos.width = gt->g.r.width;
    gd.pos.height = GDrawPointsToPixels(gt->g.base,_GScrollBar_Width);
    gd.pos.y = gt->g.r.y+gt->g.r.height - gd.pos.height;
    gd.flags = (gt->g.state==gs_invisible?0:gg_visible)|gg_enabled|gg_pos_in_pixels;
    gd.handle_controlevent = gtextfield_hscroll;
    gt->hsb = (GScrollBar *) GScrollBarCreate(gt->g.base,&gd,gt);
    gt->hsb->g.contained = true;

    gd.pos.height += GDrawPointsToPixels(gt->g.base,1);
    gt->g.r.height -= gd.pos.height;
    gt->g.inner.height -= gd.pos.height;
    if ( gt->vsb!=NULL ) {
	gt->vsb->g.r.height -= gd.pos.height;
	gt->vsb->g.inner.height -= gd.pos.height;
    }
}

static void GTextFieldFit(GTextField *gt) {
    GTextBounds bounds;
    int as=0, ds, ld, width=0;
    GRect inner, outer;
    int bp = GBoxBorderWidth(gt->g.base,gt->g.box);

    {
	FontInstance *old = GDrawSetFont(gt->g.base,gt->font);
	width = GDrawGetTextBounds(gt->g.base,gt->text, -1, NULL, &bounds);
	GDrawFontMetrics(gt->font,&as, &ds, &ld);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
	gt->fh = as+ds;
	gt->as = as;
	gt->nw = GDrawGetTextWidth(gt->g.base,nstr, 1, NULL );
	GDrawSetFont(gt->g.base,old);
    }

    GTextFieldGetDesiredSize(&gt->g,&outer,&inner);
    if ( gt->g.r.width==0 ) {
	int extra=0;
	if ( gt->listfield ) {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    2*GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    GBoxBorderWidth(gt->g.base,&_GListMark_Box);
	}
	gt->g.r.width = outer.width;
	gt->g.inner.width = inner.width;
	gt->g.inner.x = gt->g.r.x + (outer.width-inner.width-extra)/2;
    } else {
	gt->g.inner.x = gt->g.r.x + bp;
	gt->g.inner.width = gt->g.r.width - 2*bp;
    }
    if ( gt->g.r.height==0 ) {
	gt->g.r.height = outer.height;
	gt->g.inner.height = inner.height;
	gt->g.inner.y = gt->g.r.y + (outer.height-gt->g.inner.height)/2;
    } else {
	gt->g.inner.y = gt->g.r.y + bp;
	gt->g.inner.height = gt->g.r.height - 2*bp;
    }

    if ( gt->multi_line ) {
	GTextFieldAddVSb(gt);
	if ( !gt->wrap )
	    GTextFieldAddHSb(gt);
    }
    if ( gt->listfield || gt->numericfield ) {
	GListField *ge = (GListField *) gt;
	int extra;
	if ( gt->listfield )
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize) +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip) +
		    2*GBoxBorderWidth(gt->g.base,&_GListMark_Box);
	else {
	    extra = GDrawPointsToPixels(gt->g.base,_GListMarkSize)/2 +
		    GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip);
	    extra = (extra-1)|1;
	}
	ge->fieldrect = ge->buttonrect = gt->g.r;
	ge->fieldrect.width -= extra;
	extra -= GDrawPointsToPixels(gt->g.base,_GGadget_TextImageSkip)/2;
	ge->buttonrect.x = ge->buttonrect.x+ge->buttonrect.width-extra;
	ge->buttonrect.width = extra;
	if ( gt->numericfield )
	    ++ge->fieldrect.width;
    }
}

static GTextField *_GTextFieldCreate(GTextField *gt, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {

    if ( !gtextfield_inited )
	GTextFieldInit();
    gt->g.funcs = &gtextfield_funcs;
    _GGadget_Create(&gt->g,base,gd,data,def);

    gt->g.takes_input = true; gt->g.takes_keyboard = true; gt->g.focusable = true;
    if ( gd->label!=NULL ) {
	if ( gd->label->text_is_1byte )
	    gt->text = /* def2u_*/ utf82u_copy((char *) gd->label->text);
	else if ( gd->label->text_in_resource )
	    gt->text = u_copy((unichar_t *) GStringGetResource((intpt) gd->label->text,&gt->g.mnemonic));
	else
	    gt->text = u_copy(gd->label->text);
	gt->sel_start = gt->sel_end = gt->sel_base = u_strlen(gt->text);
    }
    if ( gt->text==NULL )
	gt->text = gcalloc(1,sizeof(unichar_t));
    gt->font = gtextfield_font;
    if ( gd->label!=NULL && gd->label->font!=NULL )
	gt->font = gd->label->font;
    if ( (gd->flags & gg_textarea_wrap) && gt->multi_line )
	gt->wrap = true;
    else if ( (gd->flags & gg_textarea_wrap) )	/* only used by gchardlg.c no need to make it look nice */
	gt->donthook = true;
    GTextFieldFit(gt);
    _GGadget_FinalPosition(&gt->g,base,gd);
    GTextFieldRefigureLines(gt,0);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gt->g);
    GWidgetIndicateFocusGadget(&gt->g);
    if ( gd->flags & gg_text_xim )
	gt->gic = GWidgetCreateInputContext(base,gic_overspot|gic_orlesser);
return( gt );
}

GGadget *GTextFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = _GTextFieldCreate(gcalloc(1,sizeof(GTextField)),base,gd,data,&gtextfield_box);

return( &gt->g );
}

GGadget *GPasswordCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = _GTextFieldCreate(gcalloc(1,sizeof(GTextField)),base,gd,data,&gtextfield_box);
    gt->password = true;
    GTextFieldRefigureLines(gt, 0);

return( &gt->g );
}

GGadget *GNumericFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = gcalloc(1,sizeof(GNumericField));
    gt->numericfield = true;
    _GTextFieldCreate(gt,base,gd,data,&gtextfield_box);

return( &gt->g );
}


GGadget *GTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTextField *gt = gcalloc(1,sizeof(GTextField));
    gt->multi_line = true;
    gt->accepts_returns = true;
    _GTextFieldCreate(gt,base,gd,data,&gtextfield_box);

return( &gt->g );
}

static void GListFieldSelected(GGadget *g, int i) {
    GListField *ge = (GListField *) g;

    ge->popup = NULL;
    _GWidget_ClearGrabGadget(&ge->gt.g);
    if ( i<0 || i>=ge->ltot )
return;
    GTextFieldSetTitle(g,ge->ti[i]->text);
    _ggadget_redraw(g);

    GTextFieldChanged(&ge->gt,i);
}

GGadget *GListFieldCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GListField *ge = gcalloc(1,sizeof(GListField));

    ge->gt.listfield = true;
    if ( gd->u.list!=NULL )
	ge->ti = GTextInfoArrayFromList(gd->u.list,&ge->ltot);
    _GTextFieldCreate(&ge->gt,base,gd,data,&gtextfield_box);
    ge->gt.g.funcs = &glistfield_funcs;
return( &ge->gt.g );
}
