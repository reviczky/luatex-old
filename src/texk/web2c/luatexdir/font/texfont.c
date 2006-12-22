
/*
  Copyright (c) 1996-2006 Taco Hoekwater <taco@luatex.org>
  
  This file is part of LuaTeX.
  
  LuaTeX is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  LuaTeX is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with LuaTeX; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  $Id$ */

/* Main font API implementation for the pascal parts */

/* stuff to watch out for: 
 *
 * - Knuth had a 'null_character' that was used when a character could
 * not be found by the fetch() routine, to signal an error. This has
 * been deleted, but it may mean that the output of luatex is
 * incompatible with TeX after fetch() has detected an error condition.
 *
 * - Knuth also had a font_glue() optimization. I've removed that
 * because it was a bit of dirty programming and it also was
 * problematic if 0 != null.
 */

#include "ptexlib.h"

texfont **font_tables = NULL;

static integer font_max = 0;

static void grow_font_table (integer id) {
  if (id>=font_max) {
    fontbytes += (font_max-id+8)*sizeof(texfont *);
    font_tables = xrealloc(font_tables,(id+8)*sizeof(texfont *));
    font_max = id+8;
  }
}

integer
new_font (integer id) {
  int k;
  grow_font_table(id);
  fontbytes += sizeof(texfont);
  font_tables[id] = xmalloc(sizeof(texfont));
  /* most stuff is zero */
  (void)memset (font_tables[id],0,sizeof(texfont));
  
  set_font_bc(id, 1);
  set_hyphen_char(id,'-');
  set_skew_char(id,-1);
  set_bchar_label(id,non_address);
  set_font_bchar(id,non_char);
  set_font_false_bchar(id,non_char);

  /* allocate eight values including 0 */
  set_font_params(id,7);
  for (k=0;k<=7;k++) { 
    set_font_param(id,k,0);
  }
  return id;
}

integer
copy_font (integer f) {
  int i;
  integer k = new_font((fontptr+1));
  memcpy(font_tables[k],font_tables[f],sizeof(texfont));

  set_font_used(k,0);
  
  i = sizeof(*char_base(f))    *char_infos(f);   fontbytes += i;
  char_base(k)     = xmalloc (i);
  i = sizeof(*param_base(f))   *font_params(f);  fontbytes += i;
  param_base(k)    = xmalloc (i);
  i = sizeof(*width_base(f))   *font_widths(f);  fontbytes += i;
  width_base(k)    = xmalloc (i);
  i = sizeof(*depth_base(f))   *font_depths(f);  fontbytes += i;
  depth_base(k)    = xmalloc (i);
  i = sizeof(*height_base(f))  *font_heights(f); fontbytes += i;
  height_base(k)   = xmalloc (i);
  i = sizeof(*italic_base(f))  *font_italics(f); fontbytes += i;
  italic_base(k)   = xmalloc (i);
  i = sizeof(*lig_base(f))     *font_ligs(f);    fontbytes += i;
  lig_base(k)      = xmalloc (i);
  i = sizeof(*kern_base(f))    *font_kerns(f);   fontbytes += i;
  kern_base(k)     = xmalloc (i);
  i = sizeof(*exten_base(f))   *font_extens(f);  fontbytes += i;
  exten_base(k)    = xmalloc (i);

  memcpy(char_base(k),     char_base(f),     sizeof(*char_base(f))    *char_infos(f));
  memcpy(param_base(k),    param_base(f),    sizeof(*param_base(f))   *font_params(f));
  memcpy(width_base(k),    width_base(f),    sizeof(*width_base(f))   *font_widths(f));
  memcpy(depth_base(k),    depth_base(f),    sizeof(*depth_base(f))   *font_depths(f));
  memcpy(height_base(k),   height_base(f),   sizeof(*height_base(f))  *font_heights(f));
  memcpy(italic_base(k),   italic_base(f),   sizeof(*italic_base(f))  *font_italics(f));
  memcpy(lig_base(k),      lig_base(f),      sizeof(*lig_base(f))     *font_ligs(f));
  memcpy(kern_base(k),     kern_base(f),     sizeof(*kern_base(f))    *font_kerns(f));
  memcpy(exten_base(k),    exten_base(f),    sizeof(*exten_base(f))   *font_extens(f));

  return k;
}

void delete_font (integer f) {

  if (font_tables[f]!=NULL) {
	if (char_base(f)!=NULL)   free(char_base(f));
	if (param_base(f)!=NULL)  free(param_base(f));
	if (width_base(f)!=NULL)  free(width_base(f));
	if (height_base(f)!=NULL) free(height_base(f));
	if (depth_base(f)!=NULL)  free(depth_base(f));
	if (italic_base(f)!=NULL) free(italic_base(f));
	if (kern_base(f)!=NULL)   free(kern_base(f));
	if (lig_base(f)!=NULL)    free(lig_base(f));
	if (exten_base(f)!=NULL)  free(exten_base(f));
	free(font_tables[f]);
	font_tables[f] = NULL;
  }
}

integer
scale_font (integer oldf, integer atsize) {
  integer f;
  integer i;
  double multiplier;
  multiplier = (double)atsize/(double)font_size(oldf);
  f = copy_font(oldf);
  set_font_size(f,atsize);
  fprintf(stderr,"COPY: font %s from %d to %d using %g as factor\n", font_name(oldf),oldf, f,multiplier);
  for (i=0;i<font_widths(f);i++) {
    set_font_width(f,i,(scaled)(multiplier*(integer)font_width(f,i)));
  }
  for (i=0;i<font_heights(f);i++) {
    set_font_height(f,i,(scaled)(multiplier*(integer)font_height(f,i)));
  }
  for (i=0;i<font_depths(f);i++) {
    set_font_depth(f,i,(scaled)(multiplier*(integer)font_depth(f,i)));
  }
  fontptr++;
  return f;
}



void 
create_null_font (void) {
  (void)new_font(0);
  set_font_name(0,"nullfont"); 
  set_font_area(0,"");
}

boolean 
is_valid_font (integer id) {
  if (id>=0 && id<font_max && font_tables[id]!=NULL)
    return 1;
  return 0;
}

boolean
cmp_font_name (integer id, strnumber t) 
{
  char *tt = makecstring(t);
  if (tt == NULL || strcmp(font_name(id),tt)!=0)
	return 0;
  return 1;
}

boolean
cmp_font_area (integer id, strnumber t) 
{
  char *tt = makecstring(t);
  if (tt == NULL || strcmp(font_area(id),tt)!=0)
	return 0;
  return 1;
}


integer 
test_no_ligatures (internalfontnumber f) {
 integer c;
 for (c=font_bc(f);c<=font_ec(f);c++) {
   if (char_exists(f,c) && has_lig(f,c))
     return 0;
 }
 return 1;
}

integer
get_tag_code (internalfontnumber f, eightbits c) {
  smallnumber i;
  if (char_exists(f,c)) {
    i = char_tag(f,c);
    if      (i==lig_tag)   return 1;
    else if (i==list_tag)  return 2;
    else if (i==ext_tag)   return 4;
    else                   return 0;
  }
  return  -1;
}

void
set_tag_code (internalfontnumber f, eightbits c, integer i) {
  integer fixedi;
  if (char_exists(g,c)) {
    /* abs(fix_int(i-7,0)) */
    fixedi = - (i<-7 ? -7 : (i>0 ? 0 : i )) ;

    if (fixedi >= 4) {
      if (char_tag(f,c) == ext_tag) 
	set_char_tag(f,c,(char_tag(f,c)- ext_tag));
      fixedi = fixedi - 4;
    }
    if (fixedi >= 2) {
      if (char_tag(f,c) == list_tag) 
	set_char_tag(f,c,(char_tag(f,c)- list_tag));
      fixedi = fixedi - 2;
    };
    if (fixedi >= 1) {
      if (has_lig(f,c)) 
	set_char_lig(f,c,0);
      if (has_kern(f,c)) 
	set_char_kern(f,c,0);
    }
  }
}


void 
set_no_ligatures (internalfontnumber f) {
  integer c;
  for (c=font_bc(f);c<=font_ec(f);c++) {
    if (char_exists(f,c) && has_lig(f,c))
      set_char_lig(f,c,0);
  }
}

liginfo 
get_ligature (internalfontnumber f, integer lc, integer rc) {
  liginfo t,u;
  t.lig = 0;
  t.type_char = 0;
  if (!has_lig(f,lc))
    return t;
  k = lig_index(f,lc);
  while (1) {
    u = font_lig(f,k);
    if (lig_end(u))
      break;    
    if (lig_char(u) == rc) {
      if (lig_disabled(u))
	return t;
      else
	return u;
    }
    k++;
  }
  return t;
}


scaled 
get_kern(internalfontnumber f, integer lc, integer rc)
{
  integer k;
  if (!has_kern(f,lc))
    return 0;
  k = kern_index(f,lc);
  while (!kern_end(f,k)) {
    if (kern_char(f,k) == rc) {
      if (kern_disabled(f,k))
	return 0;
      else
	return kern_kern(f,k);
    }
    k++;
  }
  return 0;
}


/* dumping and undumping fonts */

void
dump_font (int f)
{
  int x;

  set_font_used(f,0);
  dumpthings(*(fonttables[f]), 1);

  x = strlen(font_name(f))+1;
  dumpint(x);  dumpthings(*font_name(f), x);

  x = strlen(font_area(f))+1;
  dumpint(x);  dumpthings(*font_area(f), x);

  dumpthings(*char_base(f),     char_infos(f));
  dumpthings(*param_base(f),    font_params(f));
  dumpthings(*width_base(f),    font_widths(f));
  dumpthings(*depth_base(f),    font_depths(f));
  dumpthings(*height_base(f),   font_heights(f));
  dumpthings(*italic_base(f),   font_italics(f));
  dumpthings(*lig_base(f),      font_ligs(f));
  dumpthings(*kern_base(f),     font_kerns(f));
  dumpthings(*exten_base(f),    font_extens(f));
}

void
undump_font(int f)
{
  int x, i;
  texfont *tt;
  char *s;
  grow_font_table (f);
  font_tables[f] = NULL;
  fontbytes += sizeof(texfont);
  tt = xmalloc(sizeof(texfont));
  undumpthings(*tt,1);
  fonttables[f] = tt;

  undumpint (x); fontbytes += x; 
  s = xmalloc(x); undumpthings(*s,x);  
  set_font_name(f,s);

  undumpint (x); fontbytes += x; 
  s = xmalloc(x); undumpthings(*s,x);
  set_font_area(f,s);

  i = sizeof(*char_base(f)) *char_infos(f);  fontbytes += i;
  char_base(f)     = xmalloc (i);
  i = sizeof(*param_base(f))   *font_params(f);  fontbytes += i;
  param_base(f)    = xmalloc (i);
  i = sizeof(*width_base(f))   *font_widths(f);  fontbytes += i;
  width_base(f)    = xmalloc (i);
  i = sizeof(*depth_base(f))   *font_depths(f);  fontbytes += i;
  depth_base(f)    = xmalloc (i);
  i = sizeof(*height_base(f))  *font_heights(f); fontbytes += i;
  height_base(f)   = xmalloc (i);
  i = sizeof(*italic_base(f))  *font_italics(f); fontbytes += i;
  italic_base(f)   = xmalloc (i);
  i = sizeof(*lig_base(f))     *font_ligs(f);    fontbytes += i;
  lig_base(f) = xmalloc (i);
  i = sizeof(*kern_base(f))    *font_kerns(f);   fontbytes += i;
  kern_base(f)     = xmalloc (i);
  i = sizeof(*exten_base(f))   *font_extens(f);  fontbytes += i;
  exten_base(f)    = xmalloc (i);

  undumpthings(*char_base(f),     char_infos(f));
  undumpthings(*param_base(f),    font_params(f));
  undumpthings(*width_base(f),    font_widths(f));
  undumpthings(*depth_base(f),    font_depths(f));
  undumpthings(*height_base(f),   font_heights(f));
  undumpthings(*italic_base(f),   font_italics(f));
  undumpthings(*lig_base(f),      font_ligs(f));
  undumpthings(*kern_base(f),     font_kerns(f));
  undumpthings(*exten_base(f),    font_extens(f));
  
}

