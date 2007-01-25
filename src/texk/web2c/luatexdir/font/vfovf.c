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

$Id$
*/

#include "ptexlib.h"

#include "luatex-api.h"

#define new_font_type 0 /* new font (has not been used yet) */
#define virtual_font_type 1 /* virtual font */
#define real_font_type 2  /* real font */
#define subst_font_type 3 /* substituted font */

/* this is a hack! */
#define font_max 5000
/* this too! */
#define scan_special 3 /* look into special text */


integer *vf_packet_base = NULL; /*base addresses of character packets from virtual fonts*/
internal_font_number *vf_default_font; /*default font in a \.{VF} file*/
internal_font_number *vf_local_font_num; /*number of local fonts in a \.{VF} file*/
integer vf_packet_length; /*length of the current packet*/

internal_font_number vf_nf = 0; /* the local fonts counter */
internal_font_number *vf_e_fnts; /* external font numbers */
internal_font_number *vf_i_fnts; /* corresponding internal font numbers */
memory_word tmp_w; /* accumulator */
integer vf_z; /* multiplier */
integer vf_alpha; /* correction for negative values */
char vf_beta; /* divisor */
real_eight_bits *vf_buffer = NULL; /* byte buffer for vf files */
integer vf_size = 0; /* total size of the vf file */
integer vf_cur = 0; /* index into |vf_buffer| */

#define set_char_0 0 /* typeset character 0 and move right */
#define set1 128 /* typeset a character and move right */
#define set2 129 /* typeset a character and move right */
#define set3 130 /* typeset a character and move right */
#define set4 131 /* typeset a character and move right */
#define set_rule 132 /* typeset a rule and move right */
#define put1  133 /* typeset a character without moving */
#define put2  134 /* typeset a character without moving */
#define put3  135 /* typeset a character without moving */
#define put4  136 /* typeset a character without moving */
#define put_rule 137 /* typeset a rule */
#define nop 138 /* no operation */
#define bop 139 /* beginning of page */
#define eop 140 /* ending of page */
#define push 141 /* save the current positions */
#define pop 142 /* restore previous positions */
#define right1  143 /* move right */
#define right2  144 /* move right */
#define right3  145 /* move right */
#define right4  146 /* move right, 4 bytes */
#define w0 147 /* move right by |w| */
#define w1 148 /* move right and set |w| */
#define w2 149 /* move right and set |w| */
#define w3 150 /* move right and set |w| */
#define w4 151 /* move right and set |w| */
#define x0 152 /* move right by |x| */
#define x1 153 /* move right and set |x| */
#define x2 154 /* move right and set |x| */
#define x3 155 /* move right and set |x| */
#define x4 156 /* move right and set |x| */
#define down1 157 /* move down */
#define down2 158 /* move down */
#define down3 159 /* move down */
#define down4 160 /* move down, 4 bytes */
#define y0 161 /* move down by |y| */
#define y1 162 /* move down and set |y| */
#define y2 163 /* move down and set |y| */
#define y3 164 /* move down and set |y| */
#define y4 165 /* move down and set |y| */
#define z0 166 /* move down by |z| */
#define z1 167 /* move down and set |z| */
#define z2 168 /* move down and set |z| */
#define z3 169 /* move down and set |z| */
#define z4 170 /* move down and set |z| */
#define fnt_num_0 171 /* set current font to 0 */
#define fnt1 235 /* set current font */
#define xxx1 239 /* extension to DVI  primitives */
#define xxx2 240 /* extension to DVI  primitives */
#define xxx3 241 /* extension to DVI  primitives */
#define xxx4 242 /* potentially long extension to DVI primitives */
#define fnt_def1 243 /* define the meaning of a font number */
#define pre 247 /* preamble */
#define post 248 /* postamble beginning */
#define post_post 249 /* postamble ending */


#define null_font 0

/* the max length of character packet in \.{VF} file */
#define vf_max_packet_length 10000 


#define long_char 242 /* \.{VF} command for general character packet */
#define vf_id 202 /* identifies \.{VF} files */

#define vf_byte vf_buffer[vf_cur++] /* get a byte from\.{VF} file */

/* go out \.{VF} processing with an error message */
#define bad_vf(a) { print_nlp();			\
    print_string("Error in processing VF font (");	\
    print_string(font_name(f));				\
    print_string(".vf): ");				\
    print_string(a);					\
    print_string(", virtual font will be ignored");	\
    print_ln();	 return; }

#define tmp_b0  tmp_w.qqqq.b0
#define tmp_b1  tmp_w.qqqq.b1
#define tmp_b2  tmp_w.qqqq.b2
#define tmp_b3  tmp_w.qqqq.b3
#define tmp_int tmp_w.cint

/* convert |tmp_b1..tmp_b3| to an unsigned scaled dimension */
#define scaled3u (((((tmp_b3*vf_z)/256)+(tmp_b2*vf_z))/256)+(tmp_b1*vf_z))/ vf_beta

/* convert |tmp_b0..tmp_b3| to a scaled dimension */
#define scaled4(a) { a = scaled3u; if (tmp_b0==255) a -= vf_alpha; }

/* convert |tmp_b1..tmp_b3| to a scaled dimension */
#define scaled3(a)   { a = scaled3u; if (tmp_b1>127) a -= vf_alpha; }

/* convert |tmp_b2..tmp_b3| to a scaled dimension */
#define scaled2(a) {  if (tmp_b2>127) tmp_b1=255; else tmp_b1=0;	scaled3(a); }

/* convert |tmp_b3| to a scaled dimension */
#define scaled1(a) { if (tmp_b3>127) tmp_b1=255; else tmp_b1 = 0;	\
                     tmp_b2 = tmp_b1; scaled3(a); }


#define vf_max_recursion 10 /* \.{VF} files shouldn't recurse beyond this level */
#define vf_stack_size 100  /* \.{DVI} files shouldn't |push| beyond this depth */

typedef unsigned char vf_stack_index ;  /* an index into the stack */

typedef struct vf_stack_record {
    scaled stack_h, stack_v, stack_w, stack_x, stack_y, stack_z;
} vf_stack_record;

vf_stack_index  vf_cur_s = 0; /* current recursion level */
vf_stack_record vf_stack[256];
vf_stack_index  vf_stack_ptr = 0; /* pointer into |vf_stack| */


void print_string (char *j) {
  while (*j) {
    print_char(*j);
	j++;
  }
}

#define overflow_string(a,b) { overflow(maketexstring(a),b); flush_str(last_tex_string); }

boolean auto_expand_vf(internal_font_number f);

static void vf_replace_z (void) {
  vf_alpha=16;
  while (vf_z>=040000000) {
	vf_z= vf_z / 2;
	vf_alpha += vf_alpha;
  }
  vf_beta=256 / vf_alpha;
  vf_alpha=vf_alpha*vf_z;
}

/* read |k| bytes as an integer from \.{VF} file */
static integer  vf_read(integer k) {
  integer i = 0;
  while (k > 0) {
	i  = i*256 + vf_byte;
	decr(k);
  }
  return i;
}

/*print a warning message if an error ocurrs during processing local fonts in
  \.{VF} file*/

static void
vf_local_font_warning(internal_font_number f, internal_font_number k, char *s) {
  print_nlp();
  print_string(s);
  print_string(" in local font ");
  print_string(font_name(k));
  print_string(" in virtual font ");
  print_string(font_name(f));
  print_string(".vf ignored.");
}


/* process a local font in \.{VF} file */

internal_font_number 
vf_def_font(internal_font_number f) {
    internal_font_number k;
    str_number s;
    scaled ds,fs;
    four_quarters cs;

    cs.b0 = vf_byte; cs.b1 = vf_byte; cs.b2 = vf_byte; cs.b3 = vf_byte;
    tmp_b0 = vf_byte; tmp_b1 = vf_byte; tmp_b2 = vf_byte; tmp_b3 = vf_byte;
    scaled4(fs);
    ds = vf_read(4) / 16;
    tmp_b0 = vf_byte;
    tmp_b1 = vf_byte;
    while (tmp_b0 > 0) {
      tmp_b0--;
      if (vf_byte > 0) 
	; /* skip the font path */
    }
    string_room(tmp_b1);
    while (tmp_b1 > 0) {
      tmp_b1--;
      append_pool_char(vf_byte);
    }
    s = make_string();
    k = tfm_lookup(s, fs);
    if (k == null_font) 
      k = read_font_info(get_nullcs(), s, get_nullstr(), fs, -1);
    if (k != null_font) {
      if (((cs.b0 != 0) || (cs.b1 != 0) || (cs.b2 != 0) || (cs.b3 != 0)) &&
           ((font_check_0(k) != 0) || (font_check_1(k) != 0) ||
            (font_check_2(k) != 0) || (font_check_3(k) != 0)) &&
           ((cs.b0 != font_check_0(k)) || (cs.b1 != font_check_1(k)) ||
            (cs.b2 != font_check_2(k)) || (cs.b3 != font_check_3(k))))
	vf_local_font_warning(f, k, "checksum mismatch");
      if (ds != font_dsize(k))
	vf_local_font_warning(f, k, "design size mismatch");
    }
    if (pdf_font_expand_ratio[f] != 0)
      set_expand_params(k, pdf_font_auto_expand[f],
			pdf_font_expand_ratio[pdf_font_stretch[f]],
			pdf_font_expand_ratio[pdf_font_shrink[f]],
			pdf_font_step[f], pdf_font_expand_ratio[f]);
    return k;
}


int
open_vf_file (unsigned char **vf_buffer, integer *vf_size) {
  boolean res; /* was the callback successful? */
  integer callback_id;
  boolean file_read; /* was |vf_file| successfully read? */
  FILE *vf_file;
  char *fnam = NULL;
  namelength = strlen(font_name(f));
  nameoffile = xmalloc(namelength+2);
  strcpy((char *)(nameoffile+1),font_name(f));

  callback_id=callback_defined("find_vf_file");
  if (callback_id>0) {
    res = run_callback(callback_id,"S->S",stringcast(nameoffile+1), &fnam);
    if (res && (fnam!=0) && (strlen(fnam)>0)) {
      /* @<Fixup |nameoffile| after callback@>; */
      free(nameoffile);
      namelength = strlen(fnam);
      nameoffile = xmalloc(namelength+2);
      strcpy((char *)(nameoffile+1),fnam);
    }
  }
  callback_id=callback_defined("read_vf_file");
  if (callback_id>0) {
    file_read = false;
    res = run_callback(callback_id,"S->bSd",stringcast(nameoffile+1),
		       &file_read, vf_buffer,vf_size);
    if (res && file_read && (*vf_size>0)) {
      return 1; 
    }
    if (!file_read)
      return 0;    /* -1 */
  } else {
    if (ovf_b_open_in(vf_file) || vf_b_open_in(vf_file)) {
      res = read_vf_file(vf_file,vf_buffer,vf_size);
      b_close(vf_file);
      if (res) {
	return 1;
      }
    } else {
      return 0; /* -1 */
    }
  }
  return 0;
}




/*
  @ The |do_vf| procedure attempts to read the \.{VF} file for a font, and sets
  |pdf_font_type| to |real_font_type| if the \.{VF} file could not be found
  or loaded, otherwise sets |pdf_font_type| to |virtual_font_type|.  At this
  time, |tmp_f| is the internal font number of the current \.{TFM} font.  To
  process font definitions in virtual font we call |vf_def_font|.
*/



void 
do_vf(internal_font_number f) {
  integer cmd, k, n;
  integer cc, cmd_length;
  scaled tfm_width;
  str_number s;
  vf_stack_index stack_level;
  internal_font_number save_vf_nf;

  pdf_font_type[f] = real_font_type;
  if (auto_expand_vf(f))
    return; /* auto-expanded virtual font */
  stack_level = 0;

  /* @<Open |vf_file|, return if not found@>; */
  if (vf_buffer!=NULL)
    free(vf_buffer);
  vf_cur=0; vf_buffer=NULL; vf_size=0;
  if (!open_vf_file(&vf_buffer, &vf_size))
    return;
  /* @<Process the preamble@>;@/ */
  if (vf_byte != pre) 
    bad_vf("PRE command expected");
  if (vf_byte != vf_id)
    bad_vf("wrong id byte");
  cmd_length = vf_byte;
  for (k = 1;k<= cmd_length;k++)
    tmp_int = vf_byte;
  tmp_b0 = vf_byte; tmp_b1 = vf_byte; tmp_b2 = vf_byte; tmp_b3 = vf_byte;
  if (((tmp_b0 != 0) || (tmp_b1 != 0) || (tmp_b2 != 0) || (tmp_b3 != 0)) &&
      ((font_check_0(f) != 0) || (font_check_1(f) != 0) ||
       (font_check_2(f) != 0) || (font_check_3(f) != 0)) &&
      ((tmp_b0 != font_check_0(f)) || (tmp_b1 != font_check_1(f)) ||
       (tmp_b2 != font_check_2(f)) || (tmp_b3 != font_check_3(f)))) {
    print_nlp();
    print_string("checksum mismatch in font ");
    print_string(font_name(f));
    print_string(".vf ignored ");
  }
  if ((vf_read(4) / 16) != font_dsize(f)) {
    print_nlp();
    print_string("design size mismatch in font ");
    print_string(font_name(f));
    print_string(".vf ignored");
  }
  /* update_terminal;*/
  vf_z = font_size(f);
  vf_replace_z();
  /* @<Process the font definitions@>;@/ */
  cmd = vf_byte;
  save_vf_nf = vf_nf;
  while ((cmd >= fnt_def1) && (cmd <= fnt_def1 + 3)) {
    allocvffnts(font_max);
    vf_e_fnts[vf_nf] = vf_read(cmd - fnt_def1 + 1);
    vf_i_fnts[vf_nf] = vf_def_font(f);
    vf_nf++;
    cmd = vf_byte;
  }
  vf_default_font[f] = save_vf_nf;
  vf_local_font_num[f] = vf_nf - save_vf_nf;

  /* @<Allocate memory for the new virtual font@>;@/ */
  vf_packet_base[f] = new_vf_packet(f);

  while (cmd <= long_char) {
    /* @<Build a character packet@>;@/ */
    if (cmd == long_char) {
      vf_packet_length = vf_read(4);
      cc = vf_read(4);
      if (!char_exists(f,cc))
        bad_vf("invalid character code");
      tmp_b0 = vf_byte; tmp_b1 = vf_byte; tmp_b2 = vf_byte; tmp_b3 = vf_byte;
      scaled4(tfm_width);
    } else {
      vf_packet_length = cmd;
      cc = vf_byte;
      if (!char_exists(f,cc)) 
        bad_vf("invalid character code");
      tmp_b1 = vf_byte; tmp_b2 = vf_byte; tmp_b3 = vf_byte;
      scaled3(tfm_width);
    }
    if (vf_packet_length < 0)
      bad_vf("negative packet length");
    if (vf_packet_length > vf_max_packet_length)
      bad_vf("packet length too long");
    if (tfm_width != char_width(f,cc)) {
      print_nlp();
      print_string("character width mismatch in font ");
      print_string(font_name(f));
      print_string(".vf ignored");
    }
    string_room(vf_packet_length);
    while (vf_packet_length > 0) {
      cmd = vf_byte;
      decr(vf_packet_length);
      /* @<Cases of \.{DVI} commands that can appear in character packet@>; */

      if ((cmd >= set_char_0) && (cmd < set1)) {
	cmd_length = 0;
      } else if (((fnt_num_0 <= cmd) && (cmd <= fnt_num_0 + 63)) ||
		 ((fnt1 <= cmd) && (cmd <= fnt1 + 3))) {
	if (cmd >= fnt1) {
	  k = vf_read(cmd - fnt1 + 1);
	  vf_packet_length = vf_packet_length - (cmd - fnt1 + 1);
	} else {
	  k = cmd - fnt_num_0;
	}
	if (k >= 256)
	  bad_vf("too many local fonts");
	n = 0;
	while ((n < vf_local_font_num[f]) &&
	       (vf_e_fnts[vf_default_font[f] + n] != k)) {
	  incr(n);
	}
	if (n == vf_local_font_num[f])
	  bad_vf("undefined local font");
	if (k <= 63) {
	  append_pool_char(fnt_num_0 + k);
	} else {
	  append_pool_char(fnt1);
	  append_pool_char(k);
	}
	cmd_length = 0;
	cmd = nop;
      } else { 
	switch (cmd) {
	case set_rule: 
	case put_rule: 
	  cmd_length = 8;
	  break;
	case set1: 
	case set2: 
	case set3: 
	case set4:
	  cmd_length = cmd - set1 + 1;
	  break;
	case put1: 
	case put2: 
	case put3: 
	case put4:
	  cmd_length = cmd - put1 + 1;
	  break;
	case right1: 
	case right2: 
	case right3: 
	case right4: 
	  cmd_length = cmd - right1 + 1;
	  break;
	case w1: 
	case w2: 
	case w3: 
	case w4:
	  cmd_length = cmd - w1 + 1;
	  break;
	case x1:
	case x2: 
	case x3: 
	case x4:
	  cmd_length = cmd - x1 + 1;
	  break;
	case down1:  
	case down2:  
	case down3:  
	case down4:  
	  cmd_length = cmd - down1 + 1;
	  break;
	case y1: 
	case y2: 
	case y3: 
	case y4:
	  cmd_length = cmd - y1 + 1;
	  break;
	case z1: 
	case z2: 
	case z3: 
	case z4:
	  cmd_length = cmd - z1 + 1;
	  break;
	case xxx1: 
	case xxx2: 
	case xxx3: 
	case xxx4:
	  cmd_length = vf_read(cmd - xxx1 + 1);
	  vf_packet_length = vf_packet_length - (cmd - xxx1 + 1);
	  if (cmd_length > vf_max_packet_length)
	    bad_vf("packet length too long");
	  if (cmd_length < 0)
	    bad_vf("string of negative length");
	  append_pool_char(xxx1);
	  append_pool_char(cmd_length);
	  cmd = nop; /* |cmd| has been already stored above as |xxx1| */
	  break;
	case w0: 
	case x0: 
	case y0: 
	case z0: 
	case nop:
	  cmd_length = 0;
	  break;
	case push: 
	case pop: 
	  cmd_length = 0;
	  if (cmd == push) {
	    if (stack_level == vf_stack_size)
	      overflow_string("virtual font stack size", vf_stack_size)
	    else
	      incr(stack_level);
	  } else {
	    if (stack_level == 0)
	      bad_vf("more POPs than PUSHs in character")
	    else
	      decr(stack_level);
	  }
	  break;
	default:
	  bad_vf("improver DVI command");
	}
      }
      if (cmd != nop)
	append_pool_char(cmd);
      vf_packet_length = vf_packet_length - cmd_length;
      while (cmd_length > 0) {
        decr(cmd_length);
        append_pool_char(vf_byte);
      }
    }
    if (stack_level != 0)
      bad_vf("more PUSHs than POPs in character packet");
    if (vf_packet_length != 0) 
      bad_vf("invalid packet length or DVI command in packet");
    /* @<Store the packet being built@>; */
    s = make_string();
    store_packet(f, cc, s);
    flush_str(s);
    cmd = vf_byte;
  }
  if (cmd != post)
    bad_vf("POST command expected");
  pdf_font_type[f] = virtual_font_type;
}


/* Some functions for processing character packets. */

static integer
packet_read (integer k) { /* read |k| bytes as an integer from character packet */
  integer i = 0;
  while (k > 0) {
    i = i*256 + packet_byte();
    k--;
  }
  return i;
}


static integer 
packet_scaled(integer k) { /* get |k| bytes from packet as a scaled */
  scaled s = 0;
  switch (k) {
  case 1:
    tmp_b3 = packet_byte();
    scaled1(s);
    break;
  case 2:
    tmp_b2 = packet_byte();
    tmp_b3 = packet_byte();
    scaled2(s);
    break;
  case 3:
    tmp_b1 = packet_byte();
    tmp_b2 = packet_byte();
    tmp_b3 = packet_byte();
    scaled3(s);
    break;
  case 4:
    tmp_b0 = packet_byte();
    tmp_b1 = packet_byte();
    tmp_b2 = packet_byte();
    tmp_b3 = packet_byte();
    scaled4(s);
    break;
  default:
    pdf_error("vf", "invalid number size");
  };
  return s;
}

/*
@ The |do_vf_packet| procedure is called in order to interpret the
character packet for a virtual character. Such a packet may contain the
instruction to typeset a character from the same or an other virtual
font; in such cases |do_vf_packet| calls itself recursively. The
recursion level, i.e., the number of times this has happened, is kept
in the global variable |vf_cur_s| and should not exceed |vf_max_recursion|.
*/


/* typeset the \.{DVI} commands in the
   character packet for character |c| in current font |f| */

void 
do_vf_packet (internal_font_number vf, eight_bits c) {
  internal_font_number save_vf, k, n;
  scaled save_h, save_v;
  integer cmd;
  scaled w, x, y, z;
  str_number s;
  
  f = vf; /* ugly ! */
  vf_cur_s++;
  if (vf_cur_s > vf_max_recursion)
    overflow_string("max level recursion of virtual fonts", vf_max_recursion);
  push_packet_state();
  start_packet(f, c);
  vf_z = font_size(f);
  vf_replace_z();
  save_vf = f;
  f = vf_i_fnts[vf_default_font[save_vf]];
  save_v = cur_v;
  save_h = cur_h;
  w = 0; x = 0; y = 0; z = 0;
  while (vf_packet_length > 0) {
    cmd = packet_byte();

    /* short commands first */
    /* ascii characters 0 .. 127: */
    if ((cmd >= set_char_0) && (cmd < set1)) {
      if (!char_exists(f,cmd)) {
	char_warning(f, cmd);
      } else {
	output_one_char(cmd);
	cur_h = cur_h + char_width(f,cmd);
      }
      continue;
    } 
    /* font commands: */
    if (((cmd >= fnt_num_0) && (cmd <= (fnt_num_0 + 63))) || (cmd == fnt1)) {
      if (cmd == fnt1)
	k = packet_byte();
      else
	k = cmd - fnt_num_0;
      n = 0;
      while ((n < vf_local_font_num[save_vf]) &&
	     (vf_e_fnts[vf_default_font[save_vf] + n] != k))
	n++;
      if (n == vf_local_font_num[save_vf]) 
	pdf_error("vf", "local font not found");
      else
	f = vf_i_fnts[vf_default_font[save_vf] + n];
      continue;
    } 

    /* the rest: */
    switch (cmd) {
    case push: 
      vf_stack[vf_stack_ptr].stack_h = cur_h;
      vf_stack[vf_stack_ptr].stack_v = cur_v;
      vf_stack[vf_stack_ptr].stack_w = w;
      vf_stack[vf_stack_ptr].stack_x = x;
      vf_stack[vf_stack_ptr].stack_y = y;
      vf_stack[vf_stack_ptr].stack_z = z;
      vf_stack_ptr++;
      break;
    case pop:
      vf_stack_ptr--;
      cur_h = vf_stack[vf_stack_ptr].stack_h;
      cur_v = vf_stack[vf_stack_ptr].stack_v;
      w = vf_stack[vf_stack_ptr].stack_w;
      x = vf_stack[vf_stack_ptr].stack_x;
      y = vf_stack[vf_stack_ptr].stack_y;
      z = vf_stack[vf_stack_ptr].stack_z;
      break;
    case set1: 
    case set2: 
    case set3: 
    case set4: 
      c = packet_read(cmd - set1 + 1);
      if (!char_exists(f,c)) {
	char_warning(f, c);
      } else {
	output_one_char(c);
      }
      cur_h = cur_h + char_width(f,c);
      break;
    case put1: 
    case put2: 
    case put3: 
    case put4: 
      c = packet_read(cmd - put1 + 1);
      if (!char_exists(f,c)) {
	char_warning(f, c);
      } else {
	output_one_char(c);
      }
      break;
    case set_rule: 
    case put_rule: 
      rule_ht = packet_scaled(4);
      rule_wd = packet_scaled(4);
      if ((rule_wd > 0) && (rule_ht > 0)) {
	pdf_set_rule(cur_h, cur_v, rule_wd, rule_ht);
	if (cmd == set_rule) 
	  cur_h = cur_h + rule_wd;
      }
      break;
    case right1: 
    case right2: 
    case right3: 
    case right4:
      cur_h = cur_h + packet_scaled(cmd - right1 + 1);
      break;
    case w0: 
    case w1: 
    case w2: 
    case w3: 
    case w4:
      if (cmd > w0) 
	w = packet_scaled(cmd - w0);
      cur_h = cur_h + w;
      break;
    case x0: 
    case x1: 
    case x2: 
    case x3: 
    case x4:
      if (cmd > x0)
	x = packet_scaled(cmd - x0);
      cur_h = cur_h + x;
      break;
    case down1: 
    case down2: 
    case down3: 
    case down4:
      cur_v = cur_v + packet_scaled(cmd - down1 + 1);
      break;
    case y0: 
    case y1: 
    case y2: 
    case y3: 
    case y4:
      if (cmd > y0) 
	y = packet_scaled(cmd - y0);
      cur_v = cur_v + y;
      break;
    case z0: 
    case z1: 
    case z2: 
    case z3: 
    case z4:
      if (cmd > z0) 
	z = packet_scaled(cmd - z0);
      cur_v = cur_v + z;
      break;
    case xxx1: 
    case xxx2: 
    case xxx3: 
    case xxx4: 
      tmp_int = packet_read(cmd - xxx1 + 1);
      string_room(tmp_int);
      while (tmp_int > 0) {
	tmp_int--;
	append_pool_char(packet_byte());
      }
      s = make_string();
      literal(s, scan_special, false);
      flush_str(s);
      break;
    default: 
      pdf_error("vf", "invalid DVI command");     
    }
  };
  cur_h = save_h;
  cur_v = save_v;
  pop_packet_state();
  vf_z = font_size(f);
  vf_replace_z();
  vf_cur_s--;
}

/* check for a virtual auto-expanded font */
boolean 
auto_expand_vf(internal_font_number f) {

  internal_font_number bf, lf;
  integer e, k;

  if ((! pdf_font_auto_expand[f]) || (pdf_font_blink[f] == null_font))
    return false ; /* not an auto-expanded font */
  bf = pdf_font_blink[f];
  if (pdf_font_type[bf] == new_font_type) /* we must process the base font first */
    do_vf(bf);
  
  if (pdf_font_type[bf] != virtual_font_type)
    return false; /* not a virtual font */

  e = pdf_font_expand_ratio[f];
  for (k = 0;k<vf_local_font_num[bf];k++) {
    lf = vf_default_font[bf] + k;
    allocvffnts(font_max);
    /* copy vf local font numbers: */
    vf_e_fnts[vf_nf] = vf_e_fnts[lf];
    /* definition of local vf fonts are expanded from base fonts: */
    vf_i_fnts[vf_nf] = auto_expand_font(vf_i_fnts[lf], e);
    copy_expand_params(vf_i_fnts[vf_nf], vf_i_fnts[lf], e);
    vf_nf++;
  }
  vf_packet_base[f] = vf_packet_base[bf];
  vf_local_font_num[f] = vf_local_font_num[bf];
  vf_default_font[f] = vf_nf - vf_local_font_num[f];

  pdf_font_type[f] = virtual_font_type;
  return true;
}

void 
vf_expand_local_fonts(internal_font_number f) {
  internal_font_number lf;
  integer k;
  pdfassert(pdf_font_type[f] == virtual_font_type);
  for (k = 0;k<vf_local_font_num[f];k++) {
    lf = vf_i_fnts[vf_default_font[f] + k];
    set_expand_params(lf, pdf_font_auto_expand[f],
		      pdf_font_expand_ratio[pdf_font_stretch[f]],
		      pdf_font_expand_ratio[pdf_font_shrink[f]],
		      pdf_font_step[f], pdf_font_expand_ratio[f]);
    if (pdf_font_type[lf] == virtual_font_type)
      vf_expand_local_fonts(lf);
  }
}

internal_font_number
letter_space_font(halfword u, internal_font_number f, integer e) {
  internal_font_number k;
  scaled  w, r, fs;
  str_number s;
  integer i;
  char *new_font_name;
  /* read a new font and expand the character widths */
  k = read_font_info(u, tex_font_name(f), get_nullstr(), font_size(f), font_natural_dir(f));
  set_no_ligatures(k); /* disable ligatures for letter-spaced fonts */
  for (i = 0;i <= font_widths(k);i++) {
    set_font_width(k,i,font_width(f,i)+round_xn_over_d(quad(k), e, 1000));
  }
  /* append eg '+100ls' to font name */
  new_font_name = xmalloc(strlen(font_name(k)) + 8); /* |abs(e) <= 1000| */
  if (e > 0) {
    sprintf(new_font_name,"%s+%ils",font_name(k),(int)e);
  } else {
    sprintf(new_font_name,"%s%ils",font_name(k),(int)e);
  }
  set_font_name(k, new_font_name);

  /* create the corresponding virtual font */
  allocvffnts(font_max);
  vf_e_fnts[vf_nf] = 0;
  vf_i_fnts[vf_nf] = f;
  incr(vf_nf);
  vf_local_font_num[k] = 1;
  vf_default_font[k] = vf_nf - 1;
  pdf_font_type[k] = virtual_font_type;

  fs = font_size(f);
  vf_z = fs;
  vf_replace_z();
  w = round_xn_over_d(quad(f), e, 2000);
  if (w > 0) {
    tmp_b0 = 0;
  } else {
    tmp_b0 = 255;
    w = vf_alpha + w;
  }
  r = w*vf_beta;
  tmp_b1 = r / vf_z;
  r = r % vf_z;
  if (r == 0) {
    tmp_b2 = 0;
  } else {
    r = r * 256;
    tmp_b2 = r / vf_z;
    r = r % vf_z;
  }
  if (r == 0) {
    tmp_b3 = 0;
  } else {
    r = r * 256;
    tmp_b3 = r / vf_z;
  }
  vf_packet_base[k] = new_vf_packet(k);

  for (c=font_bc(k);c<=font_ec(k);c++) {
    string_room(12);
    append_pool_char(right1 + 3);
    append_pool_char(tmp_b0);
    append_pool_char(tmp_b1);
    append_pool_char(tmp_b2);
    append_pool_char(tmp_b3);
    if (c < set1) {
      append_pool_char(c);
    } else {
      append_pool_char(set1);
      append_pool_char(c);
    }
    append_pool_char(right1 + 3);
    append_pool_char(tmp_b0);
    append_pool_char(tmp_b1);
    append_pool_char(tmp_b2);
    append_pool_char(tmp_b3);
    s = make_string();
    store_packet(k, c, s);
    flush_str(s);
  }
  return k;
}

void alloc_vf_arrays (integer max) {

  vf_packet_base=xmallocarray(integer, max);
  vf_default_font=xmallocarray(internal_font_number, max);
  vf_local_font_num=xmallocarray(internal_font_number, max);
  vf_e_fnts=xmallocarray(integer, max);
  vf_i_fnts=xmallocarray(internal_font_number, max);

}

/* define vf_e_fnts_ptr, vf_e_fnts_array & vf_e_fnts_limit */
typedef integer vf_e_fnts_entry;
define_array(vf_e_fnts);

/* define vf_i_fnts_ptr, vf_i_fnts_array & vf_i_fnts_limit */
typedef internalfontnumber vf_i_fnts_entry;
define_array(vf_i_fnts);

void allocvffnts(int max)
{
    if (vf_e_fnts_array == NULL) {
        vf_e_fnts_array = vf_e_fnts;
        vf_e_fnts_limit = max;
        vf_e_fnts_ptr = vf_e_fnts_array;
        vf_i_fnts_array = vf_i_fnts;
        vf_i_fnts_limit = max;
        vf_i_fnts_ptr = vf_i_fnts_array;
    }
    alloc_array(vf_e_fnts, 1, max);
    vf_e_fnts_ptr++;
    alloc_array(vf_i_fnts, 1, max);
    vf_i_fnts_ptr++;
    if (vf_e_fnts_array != vf_e_fnts) {
        vf_e_fnts = vf_e_fnts_array;
        vf_i_fnts = vf_i_fnts_array;
    }
}
