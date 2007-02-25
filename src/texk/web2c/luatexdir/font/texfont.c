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


#define proper_char_index(c) (c<=font_ec(f) && c>=font_bc(f))
#define dxfree(a,b) { xfree(a); a = b ; }
#define do_realloc(a,b,d)    a = xrealloc(a,(b)*sizeof(d))


texfont **font_tables = NULL;

static integer font_arr_max = 0;
static integer font_id_maxval = 0;

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
new_font_id (void) {
  int i;
  for (i=0;i<font_arr_max;i++) {
    if (font_tables[i]==NULL) {
      break;
    }
  }
  if (i>=font_arr_max)
    grow_font_table (i);
  if (i>font_id_maxval)
    font_id_maxval = i;
  return i;
}

integer
max_font_id (void) {
  return font_id_maxval;
}

void
set_max_font_id (integer i) {
  font_id_maxval = i;
}

integer
new_font (void) {
  int k;
  int id;
  charinfo *ci;
  id = new_font_id();
  font_bytes += sizeof(texfont);
  /* most stuff is zero */
  font_tables[id] = xcalloc(1,sizeof(texfont));
  
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
  /* character info zero is reserved for notdef */
  font_tables[id]->characters = new_sa_tree(1, 0); /* stack size 1, default item value 0 */
  ci = xcalloc(1,sizeof(charinfo));
  set_charinfo_name(ci,xstrdup(".notdef"));
  font_tables[id]->charinfo = ci;
  return id;
}

#define Charinfo_count(a) font_tables[a]->charinfo_count
#define Charinfo_size(a) font_tables[a]->charinfo_size
#define Characters(a) font_tables[a]->characters

charinfo *
get_charinfo (internal_font_number f, integer c) { 
  sa_tree_item glyph;
  charinfo *ci;
  if (proper_char_index(c)) {
    glyph = get_sa_item(Characters(f), c);
    if (!glyph) {
      /* this could be optimized using controlled growth */
      font_bytes += sizeof(charinfo);
      glyph = Charinfo_count(f) + 1;
      do_realloc(font_tables[f]->charinfo, (glyph+1), charinfo); 
      memset (&(font_tables[f]->charinfo[glyph]),0,sizeof(charinfo));
      Charinfo_count(f) = glyph;
      Charinfo_size(f) = glyph;
      set_sa_item(Characters(f), c, glyph, 1); /* 1= global */
    }
    return &(font_tables[f]->charinfo[glyph]);
  }
  return &(font_tables[f]->charinfo[0]);
}

charinfo *
char_info (internal_font_number f, integer c) {
  sa_tree_item glyph = 0;
  if (proper_char_index(c)) {
    glyph = get_sa_item(font_tables[f]->characters, c);
  }
  return &(font_tables[f]->charinfo[glyph]);
}

integer
char_exists (internal_font_number f, integer c) {
  sa_tree_item glyph;
  if (proper_char_index(c)) {
    glyph = get_sa_item(font_tables[f]->characters, c);
    if (glyph) 
      return 1;
  }
  return 0;
}

void set_charinfo_width       (charinfo *ci, scaled val)          { ci->width = val;     }
void set_charinfo_height      (charinfo *ci, scaled val)          { ci->height = val;    }
void set_charinfo_depth       (charinfo *ci, scaled val)          { ci->depth = val;     }
void set_charinfo_italic      (charinfo *ci, scaled val)          { ci->italic = val;    }
void set_charinfo_tag         (charinfo *ci, scaled val)          { ci->tag = val;       }
void set_charinfo_remainder   (charinfo *ci, scaled val)          { ci->remainder = val; }
void set_charinfo_used        (charinfo *ci, scaled val)          { ci->used = val;      }
void set_charinfo_index       (charinfo *ci, scaled val)          { ci->index = val;     }
void set_charinfo_name        (charinfo *ci, char *val)           { dxfree(ci->name,val);        }
void set_charinfo_ligatures   (charinfo *ci, liginfo *val)        { dxfree(ci->ligatures,val);   }
void set_charinfo_kerns       (charinfo *ci, kerninfo *val)       { dxfree(ci->kerns,val);       }
void set_charinfo_packets     (charinfo *ci, real_eight_bits *val){ dxfree(ci->packets,val);     }

void set_charinfo_extensible  (charinfo *ci, int top, int bot, int mid, int rep) { 
  if (top == 0 && bot == 0 && mid == 0 && rep == 0) {
    if (ci->extensible != NULL) {
      free(ci->extensible);
      ci->extensible = NULL;
    }
  } else {
    if (ci->extensible == NULL) {
      ci->extensible = xmalloc(4*sizeof(integer));
    }
    ci->extensible[EXT_TOP] = top;
    ci->extensible[EXT_BOT] = bot;
    ci->extensible[EXT_MID] = mid;
    ci->extensible[EXT_REP] = rep;
  }
}
 
scaled           get_charinfo_width       (charinfo *ci) { return ci->width;     }
scaled           get_charinfo_height      (charinfo *ci) { return ci->height;    }
scaled           get_charinfo_depth       (charinfo *ci) { return ci->depth;     }
scaled           get_charinfo_italic      (charinfo *ci) { return ci->italic;    }
char             get_charinfo_tag         (charinfo *ci) { return ci->tag;       }
integer          get_charinfo_remainder   (charinfo *ci) { return ci->remainder; }
char             get_charinfo_used        (charinfo *ci) { return ci->used;      }
integer          get_charinfo_index       (charinfo *ci) { return ci->index;     }
char            *get_charinfo_name        (charinfo *ci) { return ci->name;      }
liginfo         *get_charinfo_ligatures   (charinfo *ci) { return ci->ligatures; }
kerninfo        *get_charinfo_kerns       (charinfo *ci) { return ci->kerns;     }
real_eight_bits *get_charinfo_packets     (charinfo *ci) { return ci->packets;   }

integer get_charinfo_extensible  (charinfo *ci, int whch) { 
  int w = 0;
  if (ci->extensible != NULL) 
    w = ci->extensible[whch];
  return w;
}

integer ext_top  (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  integer w = get_charinfo_extensible(ci,EXT_TOP);
  /*  fprintf(stdout,"top of char 0x%4x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}

integer ext_bot  (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  integer w = get_charinfo_extensible(ci,EXT_BOT);
  /*fprintf(stdout,"bot of char 0x%4x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}
integer ext_mid  (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  integer w = get_charinfo_extensible(ci,EXT_MID);
  /*fprintf(stdout,"mid of char 0x%4x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}
integer ext_rep  (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  integer w = get_charinfo_extensible(ci,EXT_REP);
  /*fprintf(stdout,"rep of char 0x%4x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}

scaled char_width (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  scaled w = get_charinfo_width(ci);
  /*fprintf(stdout,"width of char 0x%x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}

scaled char_depth (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  scaled w = get_charinfo_depth(ci);
  /*fprintf(stdout,"depth of char 0x%x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}

scaled char_height (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  scaled w = get_charinfo_height(ci);
  /*fprintf(stdout,"height of char 0x%x in font %s: %i\n",c,font_name(f),w);*/
  return w;
}

scaled char_italic (internal_font_number f, integer c) { 
  charinfo *ci = char_info(f,c);
  return get_charinfo_italic(ci);
}

integer char_remainder (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_remainder(ci);
}

char char_tag (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_tag(ci);
}

char char_used (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_used(ci);
}
    
char *char_name (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_name(ci);
}
    
integer char_index (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_index(ci);
}

liginfo * char_ligatures (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_ligatures(ci);
}

kerninfo * char_kerns (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_kerns(ci);
}

real_eight_bits * char_packets (internal_font_number f, integer c) {
  charinfo *ci = char_info(f,c);
  return get_charinfo_packets(ci);
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
  int i, x;
  kerninfo *kern;
  liginfo *lig;
  real_eight_bits *packet;
  charinfo *co, *ci;
  integer k = new_font();
  memcpy(font_tables[k],font_tables[f],sizeof(texfont));

  set_font_cache_id(k,0);
  set_font_used(k,0);
  set_font_touched(k,0);
  
  font_tables[k]->_font_name = NULL;   
  font_tables[k]->_font_filename = NULL;   
  font_tables[k]->_font_fullname = NULL;   
  font_tables[k]->_font_encodingname = NULL;   
  font_tables[k]->_font_area = NULL;   
  font_tables[k]->_font_cidregistry = NULL;   
  font_tables[k]->_font_cidordering = NULL; 

  set_font_name(k,xstrdup(font_name(f)));
  if (font_filename(f)!= NULL)
    set_font_filename(k,xstrdup(font_filename(f)));
  if (font_fullname(f)!= NULL)
    set_font_fullname(k,xstrdup(font_fullname(f)));
  if (font_encodingname(f)!= NULL)
    set_font_encodingname(k,xstrdup(font_encodingname(f)));
  if (font_area(f)!= NULL)
    set_font_area(k,xstrdup(font_area(f)));
  if (font_cidregistry(f)!= NULL)
    set_font_cidregistry(k,xstrdup(font_cidregistry(f)));
  if (font_cidordering(f)!= NULL)
    set_font_cidordering(k,xstrdup(font_cidordering(f)));

  i = sizeof(*param_base(f))*font_params(f);  
  font_bytes += i;
  param_base(k) = xmalloc (i);
  memcpy(param_base(k),param_base(f), i);
  
  for(i=font_bc(f); i<=font_ec(f); i++) {
    if (char_exists(f,i)) {
      ci = char_info(f,i);
      co = get_charinfo(k,i);
      memcpy(co,ci,sizeof(charinfo));
      set_charinfo_used(co,false);
      co->name = NULL;
      co->packets = NULL;
      co->ligatures = NULL;
      co->kerns = NULL;
      co->extensible = NULL;
      if (ci->name!=NULL) {
	co->name = xstrdup(ci->name);
      }
      /* kerns */
      if ((kern = get_charinfo_kerns(ci)) != NULL) {
	x = 0;
	while (!kern_end(kern[x])) { x++; } x++;
	co->kerns = xmalloc (x*sizeof(kerninfo));
	memcpy(co->kerns,ci->kerns,(x*sizeof(kerninfo)));
      } 
      /* ligs */
      if ((lig = get_charinfo_ligatures(ci)) != NULL) {
	x = 0;
	while (!lig_end(lig[x])) { x++; } x++;
	co->ligatures = xmalloc (x*sizeof(liginfo));
	memcpy(co->ligatures,ci->ligatures,(x*sizeof(liginfo)));
      } 
      /* packets */
      if ((packet = get_charinfo_packets(ci)) != NULL) {
	x = vf_packet_bytes(ci);
	co->packets = xmalloc (x);
	memcpy(co->packets,ci->packets,x);
      }
      if (get_charinfo_tag(ci)==ext_tag) {
	int top, bot, rep, mid;
	top = get_charinfo_extensible(ci,EXT_TOP);
	bot = get_charinfo_extensible(ci,EXT_BOT);
	mid = get_charinfo_extensible(ci,EXT_MID);
	rep = get_charinfo_extensible(ci,EXT_REP);
	set_charinfo_extensible(co,top,bot,mid,rep);
      }
    }
  }
  return k;
}

void delete_font (integer f) {
  int i;
  charinfo *co;
  assert(f>0);
  if (font_tables[f]!=NULL) {
    set_font_name(f,NULL);
    set_font_filename(f,NULL);
    set_font_fullname(f,NULL);
    set_font_encodingname(f,NULL);
    set_font_area(f,NULL);
    set_font_cidregistry(f,NULL);
    set_font_cidordering(f,NULL);
    
    for(i=font_bc(f); i<=font_ec(f); i++) {
      if (char_exists(f,i)) {
	co = char_info(f,i);
	set_charinfo_name(co,NULL);
	set_charinfo_packets(co,NULL);
	set_charinfo_ligatures(co,NULL);
	set_charinfo_kerns(co,NULL);
	set_charinfo_extensible(co,0,0,0,0);
      }
    }
    free(font_tables[f]->charinfo);
    destroy_sa_tree(font_tables[f]->characters);

    free(param_base(f));
    free(font_tables[f]);
    font_tables[f] = NULL;

    if (font_id_maxval==f) {
      font_id_maxval--;
    }
  }
}

void 
create_null_font (void) {
  int i = new_font();
  assert(i==0);
  set_font_name(i,xstrdup("nullfont")); 
  set_font_touched(i,1);
}

boolean 
is_valid_font (integer id) {
  if (id>=0 && id<font_id_maxval && font_tables[id]!=NULL)
    return 1;
  return 0;
}

/* return 1 == identical */
boolean
cmp_font_name (integer id, strnumber t) {
  char *tid , *tt;
  if (!is_valid_font(id)) 
    return 0;
  tt = makecstring(t);
  tid = font_name(id);
  if (tt == NULL && tid == NULL)
    return 1;
  if (tt == NULL || strcmp(tid,tt)!=0)
    return 0;
  return 1;
}

boolean
cmp_font_area (integer id, strnumber t) {
  char *tt = NULL;
  char *tid = font_area(id);
  if (t == 0) {
    if (tid == NULL || strlen(tid)==0)
      return 1;
    else 
      return 0;
  }
  tt = makecstring(t);
  if ((tt == NULL || strlen(tt)==0) && (tid == NULL  || strlen(tid)==0))
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
get_tag_code (internal_font_number f, integer c) {
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
  charinfo * co;
  if (char_exists(f,c)) {
    /* abs(fix_int(i-7,0)) */
    fixedi = - (i<-7 ? -7 : (i>0 ? 0 : i )) ;
    co = char_info(f,c);
    if (fixedi >= 4) {
      if (char_tag(f,c) == ext_tag) 
	set_charinfo_tag(co,(char_tag(f,c)- ext_tag));
      fixedi = fixedi - 4;
    }
    if (fixedi >= 2) {
      if (char_tag(f,c) == list_tag) 
	set_charinfo_tag(co,(char_tag(f,c)- list_tag));
      fixedi = fixedi - 2;
    };
    if (fixedi >= 1) {
      if (has_lig(f,c)) 
	set_charinfo_ligatures(co,NULL);
      if (has_kern(f,c)) 
	set_charinfo_kerns(co,NULL);
    }
  }
}


void 
set_no_ligatures (internal_font_number f) {
  integer c;
  charinfo * co;
  for (c=font_bc(f);c<=font_ec(f);c++) {
    if (char_exists(f,c) && has_lig(f,c)) {
      co = char_info(f,c);
      set_charinfo_ligatures(co,NULL);
    }
  }
}

liginfo 
get_ligature (internal_font_number f, integer lc, integer rc) {
  liginfo t, u;
  charinfo * co;
  t.lig = 0;
  t.type_char = 0;
  if (!has_lig(f,lc))
    return t;
  k = 0;
  co = char_info(f,lc);
  while (1) {
    u = charinfo_ligature(co,k);
    if (lig_end(u))
      break;    
    if (lig_char(u) == rc) {
      if (lig_disabled(u)) {
	return t;
      } else {
	return u;
      }
    }
    k++;
  }
  return t;
}


scaled 
get_kern(internal_font_number f, integer lc, integer rc)
{
  integer k;
  kerninfo u;
  charinfo * co;
  if (!has_kern(f,lc))
    return 0;
  k = 0;
  co = char_info(f,lc);
  while (1) {
    u = charinfo_kern(co,k);
    if (kern_end(u))
      break;
    if (kern_char(u) == rc) {
      if (kern_disabled(u))
	return 0;
      else
	return kern_kern(u);
    }
    k++;
  }
  return 0;
}


/* dumping and undumping fonts */

#define dump_string(a)				\
  if (a!=NULL) {				\
    x = strlen(a)+1;				\
    dump_int(x);  dump_things(*a, x);		\
  } else {					\
    x = 0; dump_int(x);				\
  }

void
dump_charinfo (int f , int c) {
  charinfo *co;
  int x;
  liginfo *lig;
  kerninfo *kern;
  real_eight_bits *packets;

  dump_int(c);
  co = char_info(f,c);
  set_charinfo_used(co,0);
  dump_int(get_charinfo_width(co));
  dump_int(get_charinfo_height(co));
  dump_int(get_charinfo_depth(co));
  dump_int(get_charinfo_italic(co));
  dump_int(get_charinfo_tag(co));
  dump_int(get_charinfo_remainder(co));
  dump_int(get_charinfo_used(co));
  dump_int(get_charinfo_index(co));
  dump_string(get_charinfo_name(co));

  /* ligatures */
  x = 0;
  if ((lig = get_charinfo_ligatures(co)) != NULL) {
    while (!lig_end(lig[x])) { x++; }
    x++;
    dump_int(x);  dump_things(*lig, x);
  } else {
    dump_int(x);
  }
  /* kerns */
  x = 0;
  if ((kern = get_charinfo_kerns(co)) != NULL) {
    while (!kern_end(kern[x])) { x++; }
    x++;
    dump_int(x);  dump_things(*kern, x);
  } else {
    dump_int(x);	
  }
  /* packets */
  x= vf_packet_bytes(co);
  dump_int(x);  
  if (x>0) {
    dump_things(*get_charinfo_packets(co), x);
  }

  if (get_charinfo_tag(co)==ext_tag) {
    x = get_charinfo_extensible(co,EXT_TOP);       dump_int(x);
    x = get_charinfo_extensible(co,EXT_BOT);       dump_int(x);
    x = get_charinfo_extensible(co,EXT_MID);       dump_int(x);
    x = get_charinfo_extensible(co,EXT_REP);       dump_int(x);
  }
}

void
dump_font (int f) {
  int i,x;

  set_font_used(f,0);
  dump_things(*(font_tables[f]), 1);
  dump_string(font_name(f));
  dump_string(font_area(f));
  dump_string(font_filename(f));
  dump_string(font_fullname(f));
  dump_string(font_encodingname(f));
  dump_string(font_cidregistry(f));
  dump_string(font_cidordering(f));

  dump_things(*param_base(f),font_params(f));

  for(i=font_bc(f); i<=font_ec(f); i++) {
    if (char_exists(f,i)) {
      dump_charinfo(f,i);
    }
  }
}

int
undump_charinfo (int f) {
  charinfo *co;
  int x,i;
  char *s = NULL;
  liginfo *lig = NULL;
  kerninfo *kern = NULL;
  real_eight_bits *packet = NULL;
  int top = 0, bot = 0, mid = 0, rep = 0;

  undump_int(i);
  
  co = get_charinfo(f,i);

  undump_int(x); set_charinfo_width(co,x);
  undump_int(x); set_charinfo_height(co,x);
  undump_int(x); set_charinfo_depth(co,x);
  undump_int(x); set_charinfo_italic(co,x);
  undump_int(x); set_charinfo_tag(co,x);
  undump_int(x); set_charinfo_remainder(co,x);
  undump_int(x); set_charinfo_used(co,x);
  undump_int(x); set_charinfo_index(co,x);

  /* name */
  undump_int (x);
  if (x>0) {
    font_bytes += x;
    s = xmalloc(x); 
    undump_things(*s,x);
  }
  set_charinfo_name(co,s); 

  /* ligatures */
  undump_int (x);      
  if (x>0) {
    font_bytes += x*sizeof(liginfo);
    lig = xmalloc(x*sizeof(liginfo)); 
    undump_things(*lig,x);
  }
  set_charinfo_ligatures(co,lig); 

  /* kerns */
  undump_int (x);      
  if (x>0) {
    font_bytes += x*sizeof(kerninfo);
    kern = xmalloc(x*sizeof(kerninfo)); 
    undump_things(*kern,x);
  }
  set_charinfo_kerns(co,kern); 

  /* packets */
  undump_int (x);      
  if (x>0) {
    font_bytes += x;
    packet = xmalloc(x); 
    undump_things(*packet,x);
  }
  set_charinfo_packets(co,packet); 

  if (get_charinfo_tag(co)==ext_tag) {
    undump_int(top);
    undump_int(bot);
    undump_int(mid);
    undump_int(rep);
    set_charinfo_extensible(co,top,bot,mid,rep);
  }
  return i;
}

#define undump_font_string(a)   undump_int (x);	\
  if (x>0) {					\
    font_bytes += x;				\
    s = xmalloc(x); undump_things(*s,x);	\
    a(f,s); }


void
undump_font(int f)
{
  int x, i;
  texfont *tt;
  charinfo *ci;
  char *s;
  grow_font_table (f);
  font_tables[f] = NULL;
  font_bytes += sizeof(texfont);
  tt = xmalloc(sizeof(texfont));
  undump_things(*tt,1);
  font_tables[f] = tt;

  undump_font_string(set_font_name);
  undump_font_string(set_font_area);
  undump_font_string(set_font_filename);
  undump_font_string(set_font_fullname);
  undump_font_string(set_font_encodingname);
  undump_font_string(set_font_cidregistry);
  undump_font_string(set_font_cidordering);

  i = sizeof(*param_base(f))*(font_params(f)+1); 
  font_bytes += i;
  param_base(f) = xmalloc (i);
  undump_things(*param_base(f),    font_params(f));

  font_tables[f]->characters = new_sa_tree(1, 0); /* stack size 1, default item value 0 */
  ci = xcalloc(1,sizeof(charinfo));
  set_charinfo_name(ci,xstrdup(".notdef"));
  font_tables[f]->charinfo = ci;
  i=font_bc(f);
  while(i<font_ec(f)) {
    i = undump_charinfo(f);
  }
}

