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

static integer font_arr_max = 0;

static void grow_font_table (integer id) {
  int j;
  if (id>=font_arr_max) {
    font_bytes += (font_arr_max-id+8)*sizeof(texfont *);
    font_tables = xrealloc(font_tables,(id+8)*sizeof(texfont *));
    j = 8;
    while (j--) {
      font_tables[id+j] = NULL;
    }
    font_arr_max = id+8;
  }
}

integer
new_font (integer id) {
  int k;
  grow_font_table(id);
  font_bytes += sizeof(texfont);
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

void
set_char_infos(internal_font_number f, int b) { 
  int i;
  i = char_infos(f);
  if (i!=b) {
	font_bytes += (b-i)*sizeof(characterinfo);
	do_realloc(char_base(f), b, characterinfo);
	/* new characters are all zeroed */
	while (i<b) {
	  (void)memset ((characterinfo *)(char_base(f)+i),0,sizeof(characterinfo));
	  i++;
	}
	char_infos(f) = b; 
  } 
}

void
set_font_params(internal_font_number f, int b) {
  int i;
  i = font_params(f);
  if (i!=b) {								   
	font_bytes += (b-font_params(f))*sizeof(scaled); 
	do_realloc(param_base(f), (b+1), integer);	
	font_params(f) = b;
	if (b>i) {
	  while (i<b) {
		i++;
		set_font_param(f,i,0);
	  }
	}
  } 
}


integer
copy_font (integer f) {
  int i;
  integer k = new_font((font_ptr+1));
  memcpy(font_tables[k],font_tables[f],sizeof(texfont));

  set_font_cache_id(k,0);
  set_font_used(k,0);
  set_font_touched(k,0);
  set_font_name(k,font_name(f));
  set_font_fullname(k,font_fullname(f));
  set_font_encodingname(k,font_encodingname(f));
  set_font_area(k,font_area(f));
  
  i = sizeof(*char_base(f))    *char_infos(f);   font_bytes += i;
  char_base(k)     = xmalloc (i);
  i = sizeof(*param_base(f))   *font_params(f);  font_bytes += i;
  param_base(k)    = xmalloc (i);
  i = sizeof(*width_base(f))   *font_widths(f);  font_bytes += i;
  width_base(k)    = xmalloc (i);
  i = sizeof(*depth_base(f))   *font_depths(f);  font_bytes += i;
  depth_base(k)    = xmalloc (i);
  i = sizeof(*height_base(f))  *font_heights(f); font_bytes += i;
  height_base(k)   = xmalloc (i);
  i = sizeof(*italic_base(f))  *font_italics(f); font_bytes += i;
  italic_base(k)   = xmalloc (i);
  i = sizeof(*lig_base(f))     *font_ligs(f);    font_bytes += i;
  lig_base(k)      = xmalloc (i);
  i = sizeof(*kern_base(f))    *font_kerns(f);   font_bytes += i;
  kern_base(k)     = xmalloc (i);
  i = sizeof(*exten_base(f))   *font_extens(f);  font_bytes += i;
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

  for(i=font_bc(k); i<=font_ec(k); i++) {
	set_char_used(k,i,false);
	if (char_name(f,i)!=NULL)
	  set_char_name(k,i,xstrdup(char_name(f,i)));
  }
  return k;
}

void delete_font (integer f) {
  int i;
  if (font_tables[f]!=NULL) {
	for(i=font_bc(f); i<=font_ec(f); i++) {
	  set_char_name(f,i,NULL);
	}
	if (char_base(f)!=NULL)   free(char_base(f));
	if (param_base(f)!=NULL)  free(param_base(f));
	if (width_base(f)!=NULL)  free(width_base(f));
	if (height_base(f)!=NULL) free(height_base(f));
	if (depth_base(f)!=NULL)  free(depth_base(f));
	if (italic_base(f)!=NULL) free(italic_base(f));
	if (kern_base(f)!=NULL)   free(kern_base(f));
	if (lig_base(f)!=NULL)    free(lig_base(f));
	if (exten_base(f)!=NULL)  free(exten_base(f));

	set_font_name(f,NULL);
	set_font_fullname(f,NULL);
	set_font_encodingname(f,NULL);
	set_font_area(f,NULL);

	free(font_tables[f]);
	font_tables[f] = NULL;
  }
}

/* this code is an experiment waiting for completion. */

integer
scale_font (integer oldf, integer atsize) {
  integer f;
  integer i;
  double multiplier;
  multiplier = (double)atsize/(double)font_size(oldf);
  f = copy_font(oldf);
  set_font_size(f,atsize);
  for (i=0;i<font_widths(f);i++) {
    set_font_width(f,i,(scaled)(multiplier*(integer)font_width(f,i)));
  }
  for (i=0;i<font_heights(f);i++) {
    set_font_height(f,i,(scaled)(multiplier*(integer)font_height(f,i)));
  }
  for (i=0;i<font_depths(f);i++) {
    set_font_depth(f,i,(scaled)(multiplier*(integer)font_depth(f,i)));
  }
  font_ptr++;
  return f;
}



void 
create_null_font (void) {
  (void)new_font(0);
  set_font_name(0,xstrdup("nullfont")); 
  set_font_touched(k,1);
}

boolean 
is_valid_font (integer id) {
  if (id>=0 && id<font_arr_max && font_tables[id]!=NULL)
    return 1;
  return 0;
}

/* return 1 == identical */
boolean
cmp_font_name (integer id, strnumber t) {
  char *tt = makecstring(t);
  char *tid = font_name(id);
  if (tt == NULL && tid == NULL)
	return 1;
  if (tt == NULL || strcmp(tid,tt)!=0)
	return 0;
  return 1;
}

boolean
cmp_font_area (integer id, strnumber t) {
  char *tt = makecstring(t);
  char *tid = font_area(id);
  if (tt == NULL && tid == NULL)
	return 1;
  if (tt == NULL || strcmp(tid,tt)!=0)
	return 0;
  return 1;
}


integer 
test_no_ligatures (internal_font_number f) {
 integer c;
 for (c=font_bc(f);c<=font_ec(f);c++) {
   if (char_exists(f,c) && has_lig(f,c))
     return 0;
 }
 return 1;
}

integer
get_tag_code (internal_font_number f, eight_bits c) {
  small_number i;
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
set_tag_code (internal_font_number f, eight_bits c, integer i) {
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
set_no_ligatures (internal_font_number f) {
  integer c;
  for (c=font_bc(f);c<=font_ec(f);c++) {
    if (char_exists(f,c) && has_lig(f,c))
      set_char_lig(f,c,0);
  }
}

liginfo 
get_ligature (internal_font_number f, integer lc, integer rc) {
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
get_kern(internal_font_number f, integer lc, integer rc)
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
  int i,x;

  set_font_used(f,0);
  dump_things(*(font_tables[f]), 1);

  if (font_name(f)!=NULL) {
	x = strlen(font_name(f))+1;
	dump_int(x);  dump_things(*font_name(f), x);
  } else {
	x = 0; dump_int(x);
  }

  if (font_area(f)!=NULL) {
	x = strlen(font_area(f))+1;
	dump_int(x);  dump_things(*font_area(f), x);
  } else {
	x = 0; dump_int(x);
  }

  if (font_fullname(f)!=NULL) {
	x = strlen(font_fullname(f))+1;
	dump_int(x);  dump_things(*font_fullname(f), x);
  } else {
	x = 0; dump_int(x);
  }

  if (font_encodingname(f)!=NULL) {
	x = strlen(font_encodingname(f))+1;
	dump_int(x);  dump_things(*font_encodingname(f), x);
  } else {
	x = 0; dump_int(x);
  }

  dump_things(*char_base(f),     char_infos(f));
  dump_things(*param_base(f),    font_params(f));
  dump_things(*width_base(f),    font_widths(f));
  dump_things(*depth_base(f),    font_depths(f));
  dump_things(*height_base(f),   font_heights(f));
  dump_things(*italic_base(f),   font_italics(f));
  dump_things(*lig_base(f),      font_ligs(f));
  dump_things(*kern_base(f),     font_kerns(f));
  dump_things(*exten_base(f),    font_extens(f));

  for(i=font_bc(f); i<=font_ec(f); i++) {
	if (char_name(f,i)!=NULL) {
	  x = strlen(char_name(f,i))+1;
	  dump_int(x);  dump_things(*char_name(f,i), x);
	} else {
	  x = 0;
	  dump_int(x);  
	}
  }
}

void
undump_font(int f)
{
  int x, i;
  texfont *tt;
  char *s;
  grow_font_table (f);
  font_tables[f] = NULL;
  font_bytes += sizeof(texfont);
  tt = xmalloc(sizeof(texfont));
  undump_things(*tt,1);
  font_tables[f] = tt;

  undump_int (x); 
  if (x>0) {
	font_bytes += x; 
	s = xmalloc(x); undump_things(*s,x);  
	set_font_name(f,s);
  }

  undump_int (x); 
  if (x>0) {
	font_bytes += x; 
	s = xmalloc(x); undump_things(*s,x);   
	set_font_area(f,s);
  }

  undump_int (x); 
  if (x>0) {
	font_bytes += x; 
	s = xmalloc(x); undump_things(*s,x);
	set_font_fullname(f,s);
  }

  undump_int (x); 
  if (x>0) {
	font_bytes += x; 
	s = xmalloc(x); undump_things(*s,x);
	set_font_encodingname(f,s);
  }

  i = sizeof(*char_base(f)) *char_infos(f);  font_bytes += i;
  char_base(f)     = xmalloc (i);
  i = sizeof(*param_base(f))   *font_params(f);  font_bytes += i;
  param_base(f)    = xmalloc (i);
  i = sizeof(*width_base(f))   *font_widths(f);  font_bytes += i;
  width_base(f)    = xmalloc (i);
  i = sizeof(*depth_base(f))   *font_depths(f);  font_bytes += i;
  depth_base(f)    = xmalloc (i);
  i = sizeof(*height_base(f))  *font_heights(f); font_bytes += i;
  height_base(f)   = xmalloc (i);
  i = sizeof(*italic_base(f))  *font_italics(f); font_bytes += i;
  italic_base(f)   = xmalloc (i);
  i = sizeof(*lig_base(f))     *font_ligs(f);    font_bytes += i;
  lig_base(f) = xmalloc (i);
  i = sizeof(*kern_base(f))    *font_kerns(f);   font_bytes += i;
  kern_base(f)     = xmalloc (i);
  i = sizeof(*exten_base(f))   *font_extens(f);  font_bytes += i;
  exten_base(f)    = xmalloc (i);

  undump_things(*char_base(f),     char_infos(f));
  undump_things(*param_base(f),    font_params(f));
  undump_things(*width_base(f),    font_widths(f));
  undump_things(*depth_base(f),    font_depths(f));
  undump_things(*height_base(f),   font_heights(f));
  undump_things(*italic_base(f),   font_italics(f));
  undump_things(*lig_base(f),      font_ligs(f));
  undump_things(*kern_base(f),     font_kerns(f));
  undump_things(*exten_base(f),    font_extens(f));
  
  for(i=font_bc(f); i<=font_ec(f); i++) {
	undump_int (x); 
	if (x > 0) {
	  font_bytes += x; 
	  s = xmalloc(x); undump_things(*s,x);
	  set_char_name(f,i,s);
	}
  }
}

