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
#include <stdlib.h>
#include "gdraw.h"
#include "ggadgetP.h"
#include "utype.h"
#include "ustring.h"

int GTextInfoGetWidth(GWindow base,GTextInfo *ti,FontInstance *font) {
    int width=0;
    int iwidth=0;
    int skip = 0;

    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;

	GDrawSetFont(base,font);
	width = GDrawGetTextWidth(base,ti->text, -1, NULL);
    }
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
return( width+iwidth+skip );
}

int GTextInfoGetMaxWidth(GWindow base,GTextInfo **ti,FontInstance *font) {
    int width=0, temp;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	if (( temp= GTextInfoGetWidth(base,ti[i],font))>width )
	    width = temp;
    }
return( width );
}

int GTextInfoGetHeight(GWindow base,GTextInfo *ti,FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    if ( ti->font!=NULL )
	font = ti->font;
    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
    }
    if ( (height = fh)<iheight ) height = iheight;
return( height );
}

int GTextInfoGetMaxHeight(GWindow base,GTextInfo **ti,FontInstance *font,int *allsame) {
    int height=0, temp, same=1;
    int i;

    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL; ++i ) {
	temp= GTextInfoGetHeight(base,ti[i],font);
	if ( height!=0 && height!=temp )
	    same = 0;
	if ( height<temp )
	    height = temp;
    }
    *allsame = same;
return( height );
}

int GTextInfoGetAs(GWindow base,GTextInfo *ti, FontInstance *font) {
    int fh=0, as=0, ds=0, ld;
    int iheight=0;
    int height;
    GTextBounds bounds;

    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( ti->image!=NULL ) {
	iheight = GImageGetScaledHeight(base,ti->image);
    }
    if ( (height = fh)<iheight ) height = iheight;

    if ( ti->text!=NULL )
return( as+(height>fh?(height-fh)/2:0) );

return( iheight );
}

int GTextInfoDraw(GWindow base,int x,int y,GTextInfo *ti,
	FontInstance *font,Color fg, Color sel, int ymax) {
    int fh=0, as=0, ds=0, ld;
    int iwidth=0, iheight=0;
    int height, skip = 0;
    GTextBounds bounds;
    GRect r, old;

    GDrawFontMetrics(font,&as, &ds, &ld);
    if ( ti->text!=NULL ) {
	if ( ti->font!=NULL )
	    font = ti->font;
	if ( ti->fg!=COLOR_DEFAULT && ti->fg!=COLOR_UNKNOWN )
	    fg = ti->fg;

	GDrawSetFont(base,font);
	GDrawGetTextBounds(base,ti->text, -1, NULL, &bounds);
	if ( as<bounds.as ) as = bounds.as;
	if ( ds<bounds.ds ) ds = bounds.ds;
    }
    fh = as+ds;
    if ( fg == COLOR_DEFAULT )
	fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(base));
    if ( ti->image!=NULL ) {
	iwidth = GImageGetScaledWidth(base,ti->image);
	iheight = GImageGetScaledHeight(base,ti->image);
	if ( ti->text!=NULL )
	    skip = GDrawPointsToPixels(base,6);
    }
    if ( (height = fh)<iheight ) height = iheight;

    r.y = y; r.height = height;
    r.x = 0; r.width = 10000;
    if (( ti->selected && sel!=COLOR_DEFAULT ) || ( ti->bg!=COLOR_DEFAULT && ti->bg!=COLOR_UNKNOWN )) {
	Color bg = ti->bg;
	if ( ti->selected ) {
	    if ( sel==COLOR_DEFAULT )
		sel = fg;
	    bg = sel;
	    if ( sel==fg ) {
		fg = ti->bg;
		if ( fg==COLOR_DEFAULT || fg==COLOR_UNKNOWN )
		    fg = GDrawGetDefaultBackground(GDrawGetDisplayOfWindow(base));
	    }
	}
	GDrawFillRect(base,&r,bg);
    }

    if ( ti->line ) {
	_GGroup_Init();
	GDrawGetClip(base,&r);
	r.x += GDrawPointsToPixels(base,2); r.width -= 2*GDrawPointsToPixels(base,2);
	GDrawPushClip(base,&r,&old);
	r.y = y; r.height = height;
	r.x = x; r.width = 10000;
	GBoxDrawHLine(base,&r,&_GGroup_LineBox);
	GDrawPopClip(base,&old);
    } else {
	if ( ti->image!=NULL && ti->image_precedes ) {
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
	    x += iwidth + skip;
	}
	if ( ti->text!=NULL ) {
	    int ypos = y+as+(height>fh?(height-fh)/2:0);
	    int width = GDrawDrawBiText(base,x,ypos,ti->text,-1,NULL,fg);
	    _ggadget_underlineMnemonic(base,x,ypos,ti->text,ti->mnemonic,fg,ymax);
	    x += width + skip;
	}
	if ( ti->image!=NULL && !ti->image_precedes )
	    GDrawDrawScaledImage(base,ti->image,x,y + (iheight>as?0:as-iheight));
    }

return( height );
}

unichar_t *utf82u_mncopy(const char *utf8buf,unichar_t *mn) {
    int len = strlen(utf8buf);
    unichar_t *ubuf = galloc((len+1)*sizeof(unichar_t));
    unichar_t *upt=ubuf, *uend=ubuf+len;
    const uint8 *pt = (const uint8 *) utf8buf, *end = pt+strlen(utf8buf);
    int w;
    int was_mn = false;

    *mn = '\0';
    while ( pt<end && *pt!='\0' && upt<uend ) {
	if ( *pt<=127 ) {
	    if ( *pt!='_' )
		*upt = *pt++;
	    else {
		was_mn = 2;
		++pt;
		--upt;
	    }
	} else if ( *pt<=0xdf ) {
	    *upt = ((*pt&0x1f)<<6) | (pt[1]&0x3f);
	    pt += 2;
	} else if ( *pt<=0xef ) {
	    *upt = ((*pt&0xf)<<12) | ((pt[1]&0x3f)<<6) | (pt[2]&0x3f);
	    pt += 3;
	} else if ( upt+1<uend ) {
	    /* Um... I don't support surrogates */
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    *upt++ = 0xd800 | (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    *upt   = 0xdc00 | ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    pt += 4;
	} else {
	    /* no space for surrogate */
	    pt += 4;
	}
	++upt;
	if ( was_mn==1 ) {
	    *mn = upt[-1];
	    if ( islower(*mn) ) *mn = toupper(*mn);
	}
	--was_mn;
    }
    *upt = '\0';
return( ubuf );
}

GTextInfo *GTextInfoCopy(GTextInfo *ti) {
    GTextInfo *copy;

    copy = galloc(sizeof(GTextInfo));
    *copy = *ti;
    copy->text_is_1byte = false;
    if ( copy->fg == 0 && copy->bg == 0 ) {
	copy->fg = copy->bg = COLOR_UNKNOWN;
    }
    if ( ti->text!=NULL ) {
	if ( ti->text_is_1byte && ti->text_in_resource ) {
	    copy->text = utf82u_mncopy((char *) copy->text,&copy->mnemonic);
	    copy->text_in_resource = false;
	    copy->text_is_1byte = false;
	} else if ( ti->text_in_resource ) {
	    copy->text = u_copy((unichar_t *) GStringGetResource((intpt) copy->text,&copy->mnemonic));
	    copy->text_in_resource = false;
	} else if ( ti->text_is_1byte ) {
	    copy->text = utf82u_copy((char *) copy->text);
	    copy->text_is_1byte = false;
	} else
	    copy->text = u_copy(copy->text);
    }
return( copy);
}

GTextInfo **GTextInfoArrayFromList(GTextInfo *ti, uint16 *cnt) {
    int i;
    GTextInfo **arr;

    i = 0;
    if ( ti!=NULL )
	for ( ; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i );
    if ( i==0 ) {
	arr = galloc(sizeof(GTextInfo *));
	i =0;
    } else {
	arr = galloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	    arr[i] = GTextInfoCopy(&ti[i]);
    }
    arr[i] = gcalloc(1,sizeof(GTextInfo));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

GTextInfo **GTextInfoArrayCopy(GTextInfo **ti) {
    int i;
    GTextInfo **arr;

    if ( ti==NULL || (ti[0]->image==NULL && ti[0]->text==NULL && !ti[0]->line) ) {
	arr = galloc(sizeof(GTextInfo *));
	i =0;
    } else {
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i );
	arr = galloc((i+1)*sizeof(GTextInfo *));
	for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line; ++i )
	    arr[i] = GTextInfoCopy(ti[i]);
    }
    arr[i] = gcalloc(1,sizeof(GTextInfo));
return( arr );
}

int GTextInfoArrayCount(GTextInfo **ti) {
    int i;

    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i );
return( i );
}

void GTextInfoFree(GTextInfo *ti) {
    if ( !ti->text_in_resource )
	gfree(ti->text);
    gfree(ti);
}

void GTextInfoListFree(GTextInfo *ti) {
    int i;

    if ( ti==NULL )
return;

    for ( i=0; ti[i].text!=NULL || ti[i].image!=NULL || ti[i].line; ++i )
	if ( !ti[i].text_in_resource )
	    gfree(ti[i].text);
    gfree(ti);
}

void GTextInfoArrayFree(GTextInfo **ti) {
    int i;

    if ( ti == NULL )
return;
    for ( i=0; ti[i]->text || ti[i]->image || ti[i]->line; ++i )
	GTextInfoFree(ti[i]);
    GTextInfoFree(ti[i]);	/* And free the null entry at end */
    gfree(ti);
}

int GTextInfoCompare(GTextInfo *ti1, GTextInfo *ti2) {
    if ( ti1->text == NULL && ti2->text==NULL )
return( 0 );
    else if ( ti1->text==NULL )
return( -1 );
    else if ( ti2->text==NULL )
return( 1 );
    else {
	char *t1, *t2;
	int ret;
	t1 = u2utf8_copy(ti1->text);
	t2 = u2utf8_copy(ti2->text);
	ret = strcoll(t1,t2);
	free(t1); free(t2);
return( ret );
    }
}

GTextInfo **GTextInfoFromChars(char **array, int len) {
    int i;
    GTextInfo **ti;

    if ( array==NULL || len==0 )
return( NULL );
    if ( len==-1 ) {
	for ( len=0; array[len]!=NULL; ++len );
    } else {
	for ( i=0; i<len && array[i]!=NULL; ++i );
	len = i;
    }
    ti = galloc((i+1)*sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = uc_copy(array[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

void GMenuItemArrayFree(GMenuItem *mi) {
    int i;

    if ( mi == NULL )
return;
    for ( i=0; mi[i].ti.text || mi[i].ti.image || mi[i].ti.line; ++i ) {
	GMenuItemArrayFree(mi[i].sub);
	free(mi[i].ti.text);
    }
    gfree(mi);
}

GMenuItem *GMenuItemArrayCopy(GMenuItem *mi, uint16 *cnt) {
    int i;
    GMenuItem *arr;

    if ( mi==NULL )
return( NULL );
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i );
    if ( i==0 )
return( NULL );
    arr = galloc((i+1)*sizeof(GMenuItem));
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	arr[i] = mi[i];
	if ( mi[i].ti.text!=NULL ) {
	    if ( mi[i].ti.text_in_resource && mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_mncopy((char *) mi[i].ti.text,&arr[i].ti.mnemonic);
	    else if ( mi[i].ti.text_in_resource )
		arr[i].ti.text = u_copy((unichar_t *) GStringGetResource((intpt) mi[i].ti.text,&arr[i].ti.mnemonic));
	    else if ( mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_copy((char *) mi[i].ti.text);
	    else
		arr[i].ti.text = u_copy(mi[i].ti.text);
	    arr[i].ti.text_in_resource = arr[i].ti.text_is_1byte = false;
	}
	if ( islower(arr[i].ti.mnemonic))
	    arr[i].ti.mnemonic = toupper(arr[i].ti.mnemonic);
	if ( islower(arr[i].shortcut))
	    arr[i].shortcut = toupper(arr[i].shortcut);
	if ( mi[i].sub!=NULL )
	    arr[i].sub = GMenuItemArrayCopy(mi[i].sub,NULL);
    }
    memset(&arr[i],'\0',sizeof(GMenuItem));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

void GMenuItem2ArrayFree(GMenuItem2 *mi) {
    int i;

    if ( mi == NULL )
return;
    for ( i=0; mi[i].ti.text || mi[i].ti.image || mi[i].ti.line; ++i ) {
	GMenuItem2ArrayFree(mi[i].sub);
	free(mi[i].ti.text);
    }
    gfree(mi);
}

static char *shortcut_domain = "shortcuts";

void GMenuSetShortcutDomain(char *domain) {
    shortcut_domain = domain;
}

const char *GMenuGetShortcutDomain(void) {
return(shortcut_domain);
}

void GMenuItemParseShortCut(GMenuItem *mi,char *shortcut) {
    static struct { char *modifier; int mask; } modifiers[] = {
	{ "Ctl", ksm_control },
	{ "Control", ksm_control },
	{ "Shft", ksm_shift },
	{ "Shift", ksm_shift },
	{ "CapsLk", ksm_capslock },
	{ "CapsLock", ksm_capslock },
	{ "Meta", ksm_meta },
	{ "Alt", ksm_meta },
	{ "Flag0x01", 0x01 },
	{ "Flag0x02", 0x02 },
	{ "Flag0x04", 0x04 },
	{ "Flag0x08", 0x08 },
	{ "Flag0x10", 0x10 },
	{ "Flag0x20", 0x20 },
	{ "Flag0x40", 0x40 },
	{ "Flag0x80", 0x80 },
	{ "Opt", 0x2000 },
	{ "Option", 0x2000 },
	{ NULL }};
    char *pt, *sh;
    int mask, temp, i;

    mi->short_mask = 0;
    mi->shortcut = '\0';

    sh = dgettext(shortcut_domain,shortcut);
    pt = strchr(sh,'|');
    if ( pt!=NULL )
	sh = pt+1;
    if ( *sh=='\0' || strcmp(sh,"No Shortcut")==0 )
return;

    mask = 0;
    while ( (pt=strchr(sh,'+'))!=NULL && pt!=sh ) {	/* A '+' can also occur as the short cut char itself */
	for ( i=0; modifiers[i].modifier!=NULL; ++i ) {
	    if ( strncasecmp(sh,modifiers[i].modifier,pt-sh)==0 )
	break;
	}
	if ( modifiers[i].modifier!=NULL )
	    mask |= modifiers[i].mask;
	else if ( sscanf( sh, "0x%x", &temp)==1 )
	    mask |= temp;
	else {
	    fprintf( stderr, "Could not parse short cut: %s\n", shortcut );
return;
	}
	sh = pt+1;
    }
    mi->short_mask = mask;
    for ( i=0; i<0x100; ++i ) {
	if ( GDrawKeysyms[i]!=NULL && uc_strcmp(GDrawKeysyms[i],sh)==0 ) {
	    mi->shortcut = 0xff00 + i;
    break;
	}
    }
    if ( i==0x100 ) {
	if ( mask==0 ) {
	    fprintf( stderr, "No modifiers in short cut: %s\n", shortcut );
return;
	}
	mi->shortcut = utf8_ildb((const char **) &sh);
	if ( *sh!='\0' ) {
	    fprintf( stderr, "Unexpected characters at end of short cut: %s\n", shortcut );
return;
	}
    }
}

GMenuItem *GMenuItem2ArrayCopy(GMenuItem2 *mi, uint16 *cnt) {
    int i;
    GMenuItem *arr;

    if ( mi==NULL )
return( NULL );
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i );
    if ( i==0 )
return( NULL );
    arr = gcalloc((i+1),sizeof(GMenuItem));
    for ( i=0; mi[i].ti.text!=NULL || mi[i].ti.image!=NULL || mi[i].ti.line; ++i ) {
	arr[i].ti = mi[i].ti;
	arr[i].moveto = mi[i].moveto;
	arr[i].invoke = mi[i].invoke;
	arr[i].mid = mi[i].mid;
	if ( mi[i].shortcut!=NULL )
	    GMenuItemParseShortCut(&arr[i],mi[i].shortcut);
	if ( mi[i].ti.text!=NULL ) {
	    if ( mi[i].ti.text_in_resource && mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_mncopy((char *) mi[i].ti.text,&arr[i].ti.mnemonic);
	    else if ( mi[i].ti.text_in_resource )
		arr[i].ti.text = u_copy((unichar_t *) GStringGetResource((intpt) mi[i].ti.text,&arr[i].ti.mnemonic));
	    else if ( mi[i].ti.text_is_1byte )
		arr[i].ti.text = utf82u_copy((char *) mi[i].ti.text);
	    else
		arr[i].ti.text = u_copy(mi[i].ti.text);
	    arr[i].ti.text_in_resource = arr[i].ti.text_is_1byte = false;
	}
	if ( islower(arr[i].ti.mnemonic))
	    arr[i].ti.mnemonic = toupper(arr[i].ti.mnemonic);
	if ( islower(arr[i].shortcut))
	    arr[i].shortcut = toupper(arr[i].shortcut);
	if ( mi[i].sub!=NULL )
	    arr[i].sub = GMenuItem2ArrayCopy(mi[i].sub,NULL);
    }
    memset(&arr[i],'\0',sizeof(GMenuItem));
    if ( cnt!=NULL ) *cnt = i;
return( arr );
}

/* **************************** String Resources **************************** */

/* A string resource file should begin with two shorts, the first containing
the number of string resources, and the second the number of integer resources.
(not the number of resources in the file, but the maximum resource index+1)
String resources look like
    <resource number-short> <flag,length-short> <mnemonics?> <unichar_t string>
Integer resources look like:
    <resource number-short> <resource value-int>
(numbers are stored with the high byte first)

We include a resource number because translations may not be provided for all
strings, so we will need to skip around. The flag,length field is a short where
the high-order bit is a flag indicating whether a mnemonic is present and the
remaining 15 bits containing the length of the following char string. If a
mnemonic is present it follows immediately after the flag,length short. After
that comes the string. After that a new string.
After all strings comes the integer list.

By convention string resource 0 should always be present and should be the
name of the language (or some other identifying name).
The first 10 or so resources are used by gadgets and containers and must be
 present even if the program doesn't set any resources itself.
Resource 1 should be the translation of "OK"
Resource 2 should be the translation of "Cancel"
   ...
Resource 7 should be the translation of "Replace"
   ...
*/
static unichar_t lang[] = { 'E', 'n', 'g', 'l', 'i', 's', 'h', '\0' };
static unichar_t ok[] = { 'O', 'k', '\0' };
static unichar_t cancel[] = { 'C', 'a', 'n', 'c', 'e', 'l', '\0' };
static unichar_t _open[] = { 'O', 'p', 'e', 'n', '\0' };
static unichar_t save[] = { 'S', 'a', 'v', 'e', '\0' };
static unichar_t filter[] = { 'F', 'i', 'l', 't', 'e', 'r', '\0' };
static unichar_t new[] = { 'N', 'e', 'w', '.', '.', '.', '\0' };
static unichar_t replace[] = { 'R', 'e', 'p', 'l', 'a', 'c', 'e', '\0' };
static unichar_t fileexists[] = { 'F','i','l','e',' ','E','x','i','s','t','s',  '\0' };
/* "File, %s, exists. Replace it?" */
static unichar_t fileexistspre[] = { 'F','i','l','e',',',' ',  '\0' };
static unichar_t fileexistspost[] = { ',',' ','e','x','i','s','t','s','.',' ','R','e','p','l','a','c','e',' ','i','t','?',  '\0' };
static unichar_t createdir[] = { 'C','r','e','a','t','e',' ','d','i','r','e','c','t','o','r','y','.','.','.',  '\0' };
static unichar_t dirname_[] = { 'D','i','r','e','c','t','o','r','y',' ','n','a','m','e','?',  '\0' };
static unichar_t couldntcreatedir[] = { 'C','o','u','l','d','n','\'','t',' ','c','r','e','a','t','e',' ','d','i','r','e','c','t','o','r','y',  '\0' };
static unichar_t selectall[] = { 'S','e','l','e','c','t',' ','A','l','l',  '\0' };
static unichar_t none[] = { 'N','o','n','e',  '\0' };
static const unichar_t *deffall[] = { lang, ok, cancel, _open, save, filter, new,
	replace, fileexists, fileexistspre, fileexistspost, createdir,
	dirname_, couldntcreatedir, selectall, none, NULL };
static const unichar_t deffallmn[] = { 0, 'O', 'C', 'O', 'S', 'F', 'N', 'R', 0, 0, 0, 'A', 'N' };
static const int deffallint[] = { 55, 100 };

static unichar_t **strarray=NULL; static const unichar_t **fallback=deffall;
static unichar_t *smnemonics=NULL; static const unichar_t *fmnemonics=deffallmn;
static int *intarray; static const int *fallbackint = deffallint;
static int slen=0, flen=sizeof(deffall)/sizeof(deffall[0])-1, ilen=0, filen=sizeof(deffallint)/sizeof(deffallint[0]);

const unichar_t *GStringGetResource(int index,unichar_t *mnemonic) {
    if ( index<0 || (index>=slen && index>=flen ))
return( NULL );
    if ( index<slen && strarray[index]!=NULL ) {
	if ( mnemonic!=NULL ) *mnemonic = smnemonics[index];
return( strarray[index]);
    }
    if ( mnemonic!=NULL && fmnemonics!=NULL )
	*mnemonic = fmnemonics[index];
return( fallback[index]);
}

int GIntGetResource(int index) {
    if ( _ggadget_use_gettext && index<2 ) {
	static int gt_intarray[2];
	if ( gt_intarray[0]==0 ) {
	    char *pt, *end;
/* GT: This is an unusual string. It is used to get around a limitation in */
/* GT: FontForge's widget set. You should put a number here (do NOT translate */
/* GT: "GGadget|ButtonSize|", that's only to provide context. The number should */
/* GT: be the number of points used for a standard sized button. It should be */
/* GT: big enough to contain "OK", "Cancel", "New...", "Edit...", "Delete" */
/* GT: (in their translated forms of course). */
	    pt = S_("GGadget|ButtonSize|55");
	    gt_intarray[0] = strtol(pt,&end,10);
	    if ( pt==end || gt_intarray[0]<20 || gt_intarray[0]>4000 )
		gt_intarray[0]=55;
/* GT: This is an unusual string. It is used to get around a limitation in */
/* GT: FontForge's widget set. You should put a number here (do NOT translate */
/* GT: "GGadget|ScaleFactor|", that's only to provide context. The number should */
/* GT: be a percentage and indicates the the ratio of the length of a string in */
/* GT: your language to the same string's length in English. */
/* GT: Suppose it takes 116 pixels to say "Ne pas enregistrer" in French but */
/* GT: only 67 pixels to say "Don't Save" in English. Then a value for ScaleFactor */
/* GT: might be 116*100/67 = 173 */
	    pt = S_("GGadget|ScaleFactor|100");
	    gt_intarray[1] = strtol(pt,&end,10);
	    if ( pt==end || gt_intarray[1]<20 || gt_intarray[1]>4000 )
		gt_intarray[1]=100;
	}
return( gt_intarray[index] );
    }

    if ( index<0 || (index>=ilen && index>=filen ))
return( -1 );
    if ( index<ilen && intarray[index]!=0x80000000 ) {
return( intarray[index]);
    }
return( fallbackint[index]);
}

static int getushort(FILE *file) {
    int ch;

    ch = getc(file);
    if ( ch==EOF )
return( EOF );
return( (ch<<8)|getc(file));
}

static int getint(FILE *file) {
    int ch;

    ch = getc(file);
    if ( ch==EOF )
return( EOF );
    ch = (ch<<8)|getc(file);
    ch = (ch<<8)|getc(file);
return( (ch<<8)|getc(file));
}

int GStringSetResourceFileV(char *filename,uint32 checksum) {
    FILE *res;
    int scnt, icnt;
    int strlen;
    int i,j;

    if ( filename==NULL ) {
	if ( strarray!=NULL )
	    for ( i=0; i<slen; ++i ) free( strarray[i]);
	free(strarray); free(smnemonics); free(intarray);
	strarray = NULL; smnemonics = NULL; intarray = NULL;
	slen = ilen = 0;
return( 1 );
    }

    res = fopen(filename,"r");
    if ( res==NULL )
return( 0 );

    if ( getint(res)!=checksum && checksum!=0xffffffff ) {
	fprintf( stderr, "Warning: The checksum of the resource file\n\t%s\ndoes not match the expected checksum.\nA set of fallback resources will be used instead.\n", filename );
	fclose(res);
return( 0 );
    }

    scnt = getushort(res);
    icnt = getushort(res);
    if ( strarray!=NULL )
	for ( i=0; i<slen; ++i ) free( strarray[i]);
    free(strarray); free(smnemonics); free(intarray);
    strarray = gcalloc(scnt,sizeof(unichar_t *));
    smnemonics = gcalloc(scnt,sizeof(unichar_t));
    intarray = galloc(icnt*sizeof(int));
    for ( i=0; i<icnt; ++i ) intarray[i] = 0x80000000;
    slen = ilen = 0;

    i = -1;
    while ( i+1<scnt ) {
	i = getushort(res);
	if ( i>=scnt || i==EOF ) {
	    fclose(res);
return( 0 );
	}
	strlen = getushort(res);
	if ( strlen&0x8000 ) {
	    smnemonics[i] = getushort(res);
	    strlen &= ~0x8000;
	}
	strarray[i] = galloc((strlen+1)*sizeof(unichar_t));
	for ( j=0; j<strlen; ++j )
	    strarray[i][j] = getushort(res);
	strarray[i][j] = '\0';
    }

    i = -1;
    while ( i+1<icnt ) {
	i = getushort(res);
	if ( i>=icnt || i==EOF ) {
	    fclose(res);
return( 0 );
	}
	intarray[i] = getint(res);
    }
    fclose(res);
    slen = scnt; ilen = icnt;

return( true );
}

int GStringSetResourceFile(char *filename) {
return( GStringSetResourceFileV(filename,0xffffffff));
}

/* Read a resource from a file without loading the file */
/*  I suspect this will just be used to get the language from the file */
unichar_t *GStringFileGetResource(char *filename, int index,unichar_t *mnemonic) {
    int scnt;
    FILE *res;
    int i,j, strlen;
    unichar_t *str;

    if ( filename==NULL )
return( uc_copy("Default"));

    res = fopen(filename,"r");
    if ( res==NULL )
return( 0 );

    scnt = getushort(res);
    /* icnt = */getushort(res);
    if ( index<0 || index>=scnt ) {
	fclose(res);
return( NULL );
    }

    i = -1;
    while ( i+1<=scnt ) {
	i = getushort(res);
	if ( i>=scnt ) {
	    fclose(res);
return( NULL );
	}
	strlen = getushort(res);
	if ( i==index ) {
	    if ( strlen&0x8000 ) {
		int temp = getushort(res);
		if ( mnemonic!=NULL ) *mnemonic = temp;
		strlen &= ~0x8000;
	    }
	    str = galloc((strlen+1)*sizeof(unichar_t));
	    for ( j=0; j<strlen; ++j )
		str[j] = getushort(res);
	    str[j] = '\0';
	    fclose( res );
return( str );
	} else {
	    if ( strlen&0x8000 ) {
		getushort(res);
		strlen &= ~0x8000;
	    }
	    for ( j=0; j<strlen; ++j )
		getushort(res);
	}
    }
    fclose( res );
return( NULL );
}
    
void GStringSetFallbackArray(const unichar_t **array,const unichar_t *mn,const int *ires) {
    int i=0;

    if ( array!=NULL ) while ( array[i]!=NULL ) ++i;
    flen = i;
    fallback = array;
    fmnemonics = mn;

    i=0;
    if ( ires!=NULL ) while ( ires[i]!=0x80000000 ) ++i;
    filen = i;

}

char *sgettext(const char *msgid) {
    const char *msgval = _(msgid);
    char *found;
    if (msgval == msgid)
	if ( (found = strrchr (msgid, '|'))!=NULL )
	    msgval = found+1;
return (char *) msgval;
}

#if defined( HAVE_LIBINTL_H ) && !defined( NODYNAMIC ) && !defined ( _STATIC_LIBINTL )
#  include <dynamic.h>

static DL_CONST void *libintl = NULL;

static char *(*_bind_textdomain_codeset)(const char *, const char *);
static char *(*_bindtextdomain)(const char *, const char *);
static char *(*_textdomain)(const char *);
static char *(*_gettext)(const char *);
static char *(*_ngettext)(const char *, const char *, unsigned long int);
static char *(*_dgettext)(const char *, const char *);

static int init_gettext(void) {

    if ( libintl == (void *) -1 )
return( false );
    else if ( libintl !=NULL )
return( true );

    libintl = dlopen("libintl" SO_EXT,RTLD_LAZY);
    if ( libintl==NULL ) {
	libintl = (void *) -1;
return( false );
    }

    _bind_textdomain_codeset = (char *(*)(const char *, const char *)) dlsym(libintl,"bind_textdomain_codeset");
    _bindtextdomain = (char *(*)(const char *, const char *)) dlsym(libintl,"bindtextdomain");
    _textdomain = (char *(*)(const char *)) dlsym(libintl,"textdomain");
    _gettext = (char *(*)(const char *)) dlsym(libintl,"gettext");
    _ngettext = (char *(*)(const char *, const char *, unsigned long int)) dlsym(libintl,"ngettext");
    _dgettext = (char *(*)(const char *, const char *)) dlsym(libintl,"dgettext");

    if ( _bind_textdomain_codeset==NULL || _bindtextdomain==NULL ||
	    _textdomain==NULL || _gettext==NULL || _ngettext==NULL ) {
	libintl = (void *) -1;
	fprintf( stderr, "Found a copy of libintl but could not use it.\n" );
return( false );
    }
return( true );
}

char *gwwv_bind_textdomain_codeset(const char *domain, const char *dir) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_bind_textdomain_codeset)(domain,dir));

return( NULL );
}

char *gwwv_bindtextdomain(const char *domain, const char *dir) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_bindtextdomain)(domain,dir));

return( NULL );
}

char *gwwv_textdomain(const char *domain) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_textdomain)(domain));

return( NULL );
}

char *gwwv_gettext(const char *msg) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_gettext)(msg));

return( (char *) msg );
}

char *gwwv_ngettext(const char *msg, const char *pmsg,unsigned long int n) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_ngettext)(msg,pmsg,n));

return( (char *) (n==1?msg:pmsg) );
}

char *gwwv_dgettext(const char *domain, const char *msg) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 && _dgettext!=NULL )
return( (_dgettext)(domain,msg));

return( (char *) msg );
}
#endif

int _ggadget_use_gettext = false;
void GResourceUseGetText(void) {
    _ggadget_use_gettext = true;
}
