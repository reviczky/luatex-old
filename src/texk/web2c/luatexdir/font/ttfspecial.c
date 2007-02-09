/* Copyright (C) 2000-2006 by George Williams */
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

#include "ptexlib.h"

#undef strstart

#include <time.h>

#include "ustring.h"
#include "ttf.h"

#define strmatch strcmp

char *utf8_idpb(char *utf8_text,uint32 ch) {
    /* Increment and deposit character */
    if ( ch<0 || ch>=17*65536 )
return( utf8_text );

    if ( ch<=127 )
        *utf8_text++ = ch;
    else if ( ch<=0x7ff ) {
        *utf8_text++ = 0xc0 | (ch>>6);
        *utf8_text++ = 0x80 | (ch&0x3f);
    } else if ( ch<=0xffff ) {
        *utf8_text++ = 0xe0 | (ch>>12);
        *utf8_text++ = 0x80 | ((ch>>6)&0x3f);
        *utf8_text++ = 0x80 | (ch&0x3f);
    } else {
        uint32 val = ch-0x10000;
        int u = ((val&0xf0000)>>16)+1, z=(val&0x0f000)>>12, y=(val&0x00fc0)>>6, x=val&0x0003f;
        *utf8_text++ = 0xf0 | (u>>2);
        *utf8_text++ = 0x80 | ((u&3)<<4) | z;
        *utf8_text++ = 0x80 | y;
        *utf8_text++ = 0x80 | x;
    }
return( utf8_text );
}


/* This file contains routines to generate non-standard true/opentype tables */
/* The first is the 'PfEd' table containing PfaEdit specific information */
/*	glyph comments & colours ... perhaps other info later */

/* ************************************************************************** */
/* *************************    The 'PfEd' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* 'PfEd' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'PfEd' 'fcmt' font comment subtable format			 */
/*  short version number 0					 */
/*  short string length						 */
/*  String in latin1 (ASCII is better)				 */

/* 'PfEd' 'cmnt' glyph comment subtable format			 */
/*  short  version number 0					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, short offset }	 */
/*  ...								 */
/*  foreach glyph >=start-glyph, <=end-glyph(+1)		 */
/*   uint32 offset		to glyph comment string (in UCS2) */
/*  ...								 */
/*  And one last offset pointing beyong the end of the last string to enable length calculations */
/*  String table in UCS2 (NUL terminated). All offsets from start*/
/*   of subtable */

/* 'PfEd' 'colr' glyph colour subtable				 */
/*  short  version number 0					 */
/*  short  count-of-ranges					 */
/*  struct { short start-glyph, end-glyph, uint32 colour (rgb) } */

#define MAX_SUBTABLE_TYPES	3

struct PfEd_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

/* *************************    The 'PfEd' table    ************************* */
/* *************************          Input         ************************* */

static void pfed_readfontcomment(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int len;
    char *pt, *end;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    len = getushort(ttf);
    pt = galloc(len+1);		/* data are stored as UCS2, but currently are ASCII */
    info->fontcomments = pt;
    end = pt+len;
    while ( pt<end )
	*pt++ = getushort(ttf);
    *pt = '\0';
}

static char *ReadUnicodeStr(FILE *ttf,uint32 offset,int len) {
    char *pt, *str;
    uint32 uch, uch2;
    int i;

    len>>=1;
    pt = str = galloc(3*len);
    fseek(ttf,offset,SEEK_SET);
    for ( i=0; i<len; ++i ) {
	uch = getushort(ttf);
	if ( uch>=0xd800 && uch<0xdc00 ) {
	    uch2 = getushort(ttf);
	    if ( uch2>=0xdc00 && uch2<0xe000 )
		uch = ((uch-0xd800)<<10) | (uch2&0x3ff);
	    else {
		pt = utf8_idpb(pt,uch);
		uch = uch2;
	    }
	}
	pt = utf8_idpb(pt,uch);
    }
    *pt++ = 0;
	return( grealloc(str,pt-str) );
}
    
static void pfed_readglyphcomments(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j;
    struct grange { int start, end; uint32 offset; } *grange;
    uint32 offset, next;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    grange = galloc(n*sizeof(struct grange));
    for ( i=0; i<n; ++i ) {
	grange[i].start = getushort(ttf);
	grange[i].end = getushort(ttf);
	grange[i].offset = getlong(ttf);
	if ( grange[i].start>grange[i].end || grange[i].end>info->glyph_cnt ) {
	    LogError( _("Bad glyph range specified in glyph comment subtable of PfEd table\n") );
	    grange[i].start = 1; grange[i].end = 0;
	}
    }
    for ( i=0; i<n; ++i ) {
	for ( j=grange[i].start; j<=grange[i].end; ++j ) {
	    fseek( ttf,base+grange[i].offset+(j-grange[i].start)*sizeof(uint32),SEEK_SET);
	    offset = getlong(ttf);
	    next = getlong(ttf);
	    info->chars[j]->comment = ReadUnicodeStr(ttf,base+offset,next-offset);
	}
    }
    free(grange);
}

static void pfed_readcolours(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int n, i, j, start, end;
    uint32 col;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )
return;			/* Bad version number */
    n = getushort(ttf);
    for ( i=0; i<n; ++i ) {
	start = getushort(ttf);
	end = getushort(ttf);
	col = getlong(ttf);
	if ( start>end || end>info->glyph_cnt )
	    LogError( _("Bad glyph range specified in colour subtable of PfEd table\n") );
	else {
	    for ( j=start; j<=end; ++j )
		info->chars[j]->color = col;
	}
    }
}

void pfed_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->pfed_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case CHR('f','c','m','t'):
	pfed_readfontcomment(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case CHR('c','m','n','t'):
	pfed_readglyphcomments(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      case CHR('c','o','l','r'):
	pfed_readcolours(ttf,info,info->pfed_start+tagoff[i].offset);
      break;
      default:
	LogError( _("Unknown subtable '%c%c%c%c' in 'PfEd' table, ignored\n"),
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* 'TeX ' table format is as follows...				 */
/* uint32  version number 0x00010000				 */
/* uint32  subtable count					 */
/* struct { uint32 tab, offset } tag/offset for first subtable	 */
/* struct { uint32 tab, offset } tag/offset for second subtable	 */
/* ...								 */

/* 'TeX ' 'ftpm' font parameter subtable format			 */
/*  short version number 0					 */
/*  parameter count						 */
/*  array of { 4chr tag, value }				 */

/* 'TeX ' 'htdp' per-glyph height/depth subtable format		 */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 height,depth }		 */

/* 'TeX ' 'sbsp' per-glyph sub/super script positioning subtable */
/*  short  version number 0					 */
/*  short  glyph-count						 */
/*  array[glyph-count] of { int16 sub,super }			 */

#undef MAX_SUBTABLE_TYPES
#define MAX_SUBTABLE_TYPES	3

struct TeX_subtabs {
    int next;
    struct {
	FILE *data;
	uint32 tag;
	uint32 offset;
    } subtabs[MAX_SUBTABLE_TYPES];
};

static uint32 tex_text_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_ExtraSp,
    0
};
static uint32 tex_math_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_Num1,
    TeX_Num2,
    TeX_Num3,
    TeX_Denom1,
    TeX_Denom2,
    TeX_Sup1,
    TeX_Sup2,
    TeX_Sup3,
    TeX_Sub1,
    TeX_Sub2,
    TeX_SupDrop,
    TeX_SubDrop,
    TeX_Delim1,
    TeX_Delim2,
    TeX_AxisHeight,
    0};
static uint32 tex_mathext_params[] = {
    TeX_Slant,
    TeX_Space,
    TeX_Stretch,
    TeX_Shrink,
    TeX_XHeight,
    TeX_Quad,
    TeX_MathSp,
    TeX_DefRuleThick,
    TeX_BigOpSpace1,
    TeX_BigOpSpace2,
    TeX_BigOpSpace3,
    TeX_BigOpSpace4,
    TeX_BigOpSpace5,
    0};

/* *************************    The 'TeX ' table    ************************* */
/* *************************          Input         ************************* */

static void TeX_readFontParams(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,pcnt;
    static uint32 *alltags[] = { tex_text_params, tex_math_params, tex_mathext_params };
    int j,k;
    uint32 tag;
    int32 val;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    pcnt = getushort(ttf);
    if ( pcnt==22 ) info->texdata.type = tex_math;
    else if ( pcnt==13 ) info->texdata.type = tex_mathext;
    else if ( pcnt>=7 ) info->texdata.type = tex_text;
    for ( i=0; i<pcnt; ++i ) {
	tag = getlong(ttf);
	val = getlong(ttf);
	for ( j=0; j<3; ++j ) {
	    for ( k=0; alltags[j][k]!=0; ++k )
		if ( alltags[j][k]==tag )
	    break;
	    if ( alltags[j][k]==tag )
	break;
	}
	if ( j<3 )
	    info->texdata.params[k] = val;
    }
}

static void TeX_readHeightDepth(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int h, d;
	h = getushort(ttf);
	d = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->tex_height = h;
	    info->chars[i]->tex_depth = d;
	}
    }
}

static void TeX_readSubSuper(FILE *ttf,struct ttfinfo *info,uint32 base) {
    int i,gcnt;

    fseek(ttf,base,SEEK_SET);
    if ( getushort(ttf)!=0 )	/* Don't know how to read this version of the subtable */
return;
    gcnt = getushort(ttf);
    for ( i=0; i<gcnt && i<info->glyph_cnt; ++i ) {
	int super, sub;
	sub = getushort(ttf);
	super = getushort(ttf);
	if ( info->chars[i]!=NULL ) {
	    info->chars[i]->tex_sub_pos = sub;
	    info->chars[i]->tex_super_pos = super;
	}
    }
}

void tex_read(FILE *ttf,struct ttfinfo *info) {
    int n,i;
    struct tagoff { uint32 tag, offset; } tagoff[MAX_SUBTABLE_TYPES+30];

    fseek(ttf,info->tex_start,SEEK_SET);

    if ( getlong(ttf)!=0x00010000 )
return;
    n = getlong(ttf);
    if ( n>=MAX_SUBTABLE_TYPES+30 )
	n = MAX_SUBTABLE_TYPES+30;
    for ( i=0; i<n; ++i ) {
	tagoff[i].tag = getlong(ttf);
	tagoff[i].offset = getlong(ttf);
    }
    for ( i=0; i<n; ++i ) switch ( tagoff[i].tag ) {
      case CHR('f','t','p','m'):
	TeX_readFontParams(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('h','t','d','p'):
	TeX_readHeightDepth(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      case CHR('s','b','s','p'):
	TeX_readSubSuper(ttf,info,info->tex_start+tagoff[i].offset);
      break;
      default:
	LogError( _("Unknown subtable '%c%c%c%c' in 'TeX ' table, ignored\n"),
		tagoff[i].tag>>24, (tagoff[i].tag>>16)&0xff, (tagoff[i].tag>>8)&0xff, tagoff[i].tag&0xff );
      break;
    }
}

/* ************************************************************************** */
/* *************************    The 'BDF ' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

/* the BDF table is used to store BDF properties so that we can do round trip */
/* conversion from BDF->otb->BDF without losing anything. */
/* Format:
	USHORT	version		: 'BDF' table version number, must be 0x0001
	USHORT	strikeCount	: number of strikes in table
	ULONG	stringTable	: offset (from start of BDF table) to string table

followed by an array of 'strikeCount' descriptors that look like: 
	USHORT	ppem		: vertical pixels-per-EM for this strike
	USHORT	num_items	: number of items (properties and atoms), max is 255

this array is followed by 'strikeCount' value sets. Each "value set" is 
an array of (num_items) items that look like:
	ULONG	item_name	: offset in string table to item name
	USHORT	item_type	: item type: 0 => non-property string (e.g. COMMENT)
					     1 => non-property atom (e.g. FONT)
					     2 => non-property int32
					     3 => non-property uint32
					  0x10 => flag for a property, ored
						  with above value types)
	ULONG	item_value	: item value. 
				strings	 => an offset into the string table
					  to the corresponding string,
					  without the surrending double-quotes

				atoms	 => an offset into the string table

				integers => the corresponding 32-bit value
Then the string table of null terminated strings. These strings should be in
ASCII.
*/

/* Internally FF stores BDF comments as one psuedo property per line. As you */
/*  might expect. But FreeType merges them into one large lump with newlines */
/*  between lines. Which means that BDF tables created by FreeType will be in*/
/*  that format. So we might as well be compatible. We will pack & unpack    */
/*  comment lines */

static int BDFPropCntMergedComments(BDFFont *bdf) {
    int i, cnt;

    cnt = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	++cnt;
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    break;
    }
    for ( ; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 )
    continue;
	++cnt;
    }
return( cnt );
}

static char *MergeComments(BDFFont *bdf) {
    int len, i;
    char *str;

    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom ))
	    len += strlen( bdf->props[i].u.atom ) + 1;
    }
    if ( len==0 )
return( copy( "" ));

    str = galloc( len+1 );
    len = 0;
    for ( i=0; i<bdf->prop_cnt; ++i ) {
	if ( strmatch(bdf->props[i].name,"COMMENT")==0 &&
		((bdf->props[i].type & ~prt_property)==prt_string ||
		 (bdf->props[i].type & ~prt_property)==prt_atom )) {
	    strcpy(str+len,bdf->props[i].u.atom );
	    len += strlen( bdf->props[i].u.atom ) + 1;
	    str[len-1] = '\n';
	}
    }
    str[len-1] = '\0';
return( str );
}
    
#define AMAX	50


/* *************************    The 'BDF ' table    ************************* */
/* *************************          Input         ************************* */

static char *getstring(FILE *ttf,long start) {
    long here = ftell(ttf);
    int len, ch;
    char *str, *pt;

    fseek(ttf,start,SEEK_SET);
    for ( len=1; (ch=getc(ttf))>0 ; ++len );
    fseek(ttf,start,SEEK_SET);
    pt = str = galloc(len);
    while ( (ch=getc(ttf))>0 )
	*pt++ = ch;
    *pt = '\0';
    fseek(ttf,here,SEEK_SET);
return( str );
}

/* COMMENTS get stored all in one lump by freetype. De-lump them */
static int CheckForNewlines(BDFFont *bdf,int k) {
    char *pt, *start;
    int cnt, i;

    for ( cnt=0, pt = bdf->props[k].u.atom; *pt; ++pt )
	if ( *pt=='\n' )
	    ++cnt;
    if ( cnt==0 )
return( k );

    bdf->prop_cnt += cnt;
    bdf->props = grealloc(bdf->props, bdf->prop_cnt*sizeof( BDFProperties ));

    pt = strchr(bdf->props[k].u.atom,'\n');
    *pt = '\0'; ++pt;
    for ( i=1; i<=cnt; ++i ) {
	start = pt;
	while ( *pt!='\n' && *pt!='\0' ) ++pt;
	bdf->props[k+i].name = copy(bdf->props[k].name);
	bdf->props[k+i].type = bdf->props[k].type;
	bdf->props[k+i].u.atom = copyn(start,pt-start);
	if ( *pt=='\n' ) ++pt;
    }
    pt = copy( bdf->props[k].u.atom );
    free( bdf->props[k].u.atom );
    bdf->props[k].u.atom = pt;
return( k+cnt );
}

void ttf_bdf_read(FILE *ttf,struct ttfinfo *info) {
#if 0
    int strike_cnt, i,j,k;
    long string_start;
    struct bdfinfo { BDFFont *bdf; int cnt; } *bdfinfo;
    BDFFont *bdf;

    if ( info->bdf_start==0 )
return;
    fseek(ttf,info->bdf_start,SEEK_SET);
    if ( getushort(ttf)!=1 )
return;
    strike_cnt = getushort(ttf);
    string_start = getlong(ttf) + info->bdf_start;

    bdfinfo = galloc(strike_cnt*sizeof(struct bdfinfo));
    for ( i=0; i<strike_cnt; ++i ) {
	int ppem, num_items;
	ppem = getushort(ttf);
	num_items = getushort(ttf);
	for ( bdf=info->bitmaps; bdf!=NULL; bdf=bdf->next )
	    if ( bdf->pixelsize==ppem )
	      break;
	bdfinfo[i].bdf = bdf;
	bdfinfo[i].cnt = num_items;
    }

    for ( i=0; i<strike_cnt; ++i ) {
	if ( (bdf = bdfinfo[i].bdf) ==NULL )
	    fseek(ttf,10*bdfinfo[i].cnt,SEEK_CUR);
	else {
	    bdf->prop_cnt = bdfinfo[i].cnt;
	    bdf->props = galloc(bdf->prop_cnt*sizeof(BDFProperties));
	    for ( j=k=0; j<bdfinfo[i].cnt; ++j, ++k ) {
		long name = getlong(ttf);
		int type = getushort(ttf);
		long value = getlong(ttf);
		bdf->props[k].type = type;
		bdf->props[k].name = getstring(ttf,string_start+name);
		switch ( type&~prt_property ) {
		  case prt_int: case prt_uint:
		    bdf->props[k].u.val = value;
		    if ( strcmp(bdf->props[k].name,"FONT_ASCENT")==0 &&
			    value<=bdf->pixelsize ) {
			bdf->ascent = value;
			bdf->descent = bdf->pixelsize-value;
		    }
		  break;
		  case prt_string: case prt_atom:
		    bdf->props[k].u.str = getstring(ttf,string_start+value);
		    k = CheckForNewlines(bdf,k);
		  break;
		}
	    }
	}
    }
#endif
}


/* ************************************************************************** */
/* *************************    The 'FFTM' table    ************************* */
/* *************************         Output         ************************* */
/* ************************************************************************** */

