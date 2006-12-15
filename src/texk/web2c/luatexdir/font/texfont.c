
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

#include "ptexlib.h"

texfont **font_tables = NULL;

static integer font_max = 0;

static void grow_font_table (integer id) {
  if (id==font_max) {
    font_tables = xrealloc(font_tables,(id+16)*sizeof(texfont *));
    font_max = id+16;
  }
}

void 
do_set_font_lig_kerns (int f, int b ) {
  if (font_tables[f]->_font_lig_kerns != b) {					 
    font_tables[f]->_lig_kern_base = 
      xrealloc(font_tables[f]->_lig_kern_base,(b)*sizeof(fourquarters));
    font_tables[f]->_font_lig_kerns = b;  
  }
} 

integer
new_font (integer id) {
  int k;
  grow_font_table(id);
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
  integer k = new_font((fontptr+1));
  memcpy(font_tables[k],font_tables[f],sizeof(texfont));
  /* TODO: deep copy arrays */
  return k;
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

/*
  The global variable |null_character| is set up to be the
  |char_info| for a character that doesn't exist. Such a variable
  provides a convenient way to deal with erroneous situations.
*/
							  
characterinfo null_character = {0,0,0,0,0,0};

/*
Here are some macros that help process ligatures and kerns.
We write |char_kern(f)(j)| to find the amount of kerning specified by
kerning command~|j| in font~|f|. If |j| is the |char_info| for a character
with a ligature/kern program, the first instruction of that program is either
|i=font_info(f)(lig_kern_start(f)(j))| or
|font_info(f)(lig_kern_restart(f)(i))|,
depending on whether or not |skip_byte(i)<=stop_flag|.

The constant |kern_base_offset| should be simplified, for \PASCAL\ compilers
that do not do local optimization.
@^system dependencies@>
 */


integer 
test_no_ligatures (internalfontnumber f) {
 integer c;
 for (c=font_bc(f);c<=font_ec(f);c++) {
   if (char_exists(f,c) && odd(char_tag(f,c)))
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

/* todo: next is probably wrong */
scaled 
get_kern(internalfontnumber f, eightbits lc, eightbits rc)
{
  fontindex k;
  if (char_tag(f,lc) != lig_tag)
    return 0;
  k = lig_kern_start(f,lc);
  if (skip_byte(f,k) > stop_flag)
    k = lig_kern_restart(f,k);
 continue1:
  if ((next_char(f,k) == rc) && 
      (skip_byte(f,k) <= stop_flag) &&
      (op_byte(f,k) >= kern_flag)) {
    return char_kern(f,k);
  }
  if (skip_byte(f,k) == 0) {
    k++;
  } else {
    if (skip_byte(f,k) >= stop_flag) 
      return 0;
    k += skip_byte(f,k) + 1;
  }
  goto continue1;
}


/* dumping and undumping fonts */

void
dump_font (int font_number)
{
  int x;
  dumpthings(*(fonttables[font_number]), 1);

  x = strlen(font_name(font_number))+1;
  dumpint(x);  dumpthings(*font_name(font_number), x);

  x = strlen(font_area(font_number))+1;
  dumpint(x);  dumpthings(*font_area(font_number), x);

}

void
undump_font(int font_number)
{
  memoryword size_word;
  texfont *tt;
  char *s;
  int x;
  grow_font_table (font_number);
  font_tables[font_number] = NULL;
  tt = xmalloc(sizeof(texfont));
  undumpthings(*tt,1);
  fonttables[font_number] = tt;
  undumpint (x); s = xmalloc(x); 
  undumpthings(*s,x);  
  set_font_name(font_number,s);
  undumpint (x); s = xmalloc(x); 
  undumpthings(*s,x);
  set_font_area(font_number,s);
}

