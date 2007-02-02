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

/* this is a hack! */
#define font_max 5000
/* this too! */
#define scan_special 3 /* look into special text */

memory_word tmp_w; /* accumulator */

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
#define fnt2 236 /* set current font */
#define fnt3 237 /* set current font */
#define fnt4 238 /* set current font */
#define xxx1 239 /* extension to DVI  primitives */
#define xxx2 240 /* extension to DVI  primitives */
#define xxx3 241 /* extension to DVI  primitives */
#define xxx4 242 /* potentially long extension to DVI primitives */
#define fnt_def1 243 /* define the meaning of a font number */
#define pre 247 /* preamble */
#define post 248 /* postamble beginning */
#define post_post 249 /* postamble ending */
#define yyy1 250 /* PDF literal text */
#define yyy2 251 /* PDF literal text */
#define yyy3 252 /* PDF literal text */
#define yyy4 253 /* PDF literal text */


#define null_font 0

/* the max length of character packet in \.{VF} file */
#define vf_max_packet_length 10000 


#define long_char 242 /* \.{VF} command for general character packet */
#define vf_id 202 /* identifies \.{VF} files */

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

#define vf_max_recursion 10 /* \.{VF} files shouldn't recurse beyond this level */
#define vf_stack_size 100  /* \.{DVI} files shouldn't |push| beyond this depth */

static integer cur_packet_byte;

#define do_packet_byte() font_packet(vf_f,cur_packet_byte++)

typedef unsigned char vf_stack_index ;  /* an index into the stack */

typedef struct vf_stack_record {
    scaled stack_h, stack_v, stack_w, stack_x, stack_y, stack_z;
} vf_stack_record;

vf_stack_index  vf_cur_s = 0; /* current recursion level */
vf_stack_record vf_stack[256];
vf_stack_index  vf_stack_ptr = 0; /* pointer into |vf_stack| */


#define overflow_string(a,b) { overflow(maketexstring(a),b); flush_str(last_tex_string); }

boolean auto_expand_vf(internal_font_number f); /* forward */

/* get a byte from\.{VF} file */

real_eight_bits
vf_byte (void) {
  if (vf_cur>=vf_size) {
    pdf_error("vf", "unexpected EOF or error");
  }
  return vf_buffer[vf_cur++] ;
}


#define vf_replace_z() {			\
    vf_alpha=16;				\
  while (vf_z>=040000000) {			\
    vf_z= vf_z / 2;				\
    vf_alpha += vf_alpha;			\
  }						\
  vf_beta=256 / vf_alpha;			\
  vf_alpha=vf_alpha*vf_z; }

/* read |k| bytes as an integer from \.{VF} file */
static integer  vf_read(integer k) {
  integer i,j;
  pdfassert((k > 0) && (k <= 4));
  i = vf_byte();
  if ((k == 4) && (i > 127))
    i = i - 256;
  decr(k);
  while (k > 0) {
    j = vf_byte();
    i  = i*256 + j;
    decr(k);
  }
  return i;
}

void
pdf_check_vf_cur_val (void) {
 internal_font_number f;
 f = cur_val;
 do_vf(f);
 if (font_type(f) == virtual_font_type)
   pdf_error("font", "command cannot be used with virtual font");
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

    cs.b0 = vf_byte(); cs.b1 = vf_byte(); cs.b2 = vf_byte(); cs.b3 = vf_byte();
    fs = sqxfw(vf_read(4), font_size(f));

    ds = vf_read(4) / 16;
    tmp_b0 = vf_byte();
    tmp_b1 = vf_byte();
    while (tmp_b0 > 0) {
      tmp_b0--;
      (void)vf_byte(); /* skip the font path */
    }
    string_room(tmp_b1);
    while (tmp_b1 > 0) {
      tmp_b1--;
      append_pool_char(vf_byte());
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
open_vf_file (internal_font_number f, unsigned char **vf_buffer, integer *vf_size) {
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
    } else {
      return 0;
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
  |font_type()| to |real_font_type| if the \.{VF} file could not be found
  or loaded, otherwise sets |font_type()| to |virtual_font_type|.  At this
  time, |tmp_f| is the internal font number of the current \.{TFM} font.  To
  process font definitions in virtual font we call |vf_def_font|.
*/

#define append_packet(k) { set_font_packet(f,vf_np,k); vf_np++; }

/* life is easier if all internal font commands are fnt4 and
   all character commands are set4 or put4 */

#define append_fnt_set(k) {			\
    assert(k>0)	;				\
    append_packet(fnt4);			\
    append_four(k); } 

#define append_four(k) {			\
    append_packet((k&0xFF000000)>>24);		\
    append_packet((k&0x00FF0000)>>16);		\
    append_packet((k&0x0000FF00)>>8);		\
    append_packet((k&0x000000FF));  } 


void 
do_vf(internal_font_number f) {
  integer cmd, k, n, i;
  integer cc, cmd_length, packet_length;
  scaled tfm_width;
  vf_stack_index stack_level;
  integer vf_z; /* multiplier */
  integer vf_alpha; /* correction for negative values */
  char vf_beta; /* divisor */
  integer vf_np,save_vf_np;
  /* local font counter */
  unsigned char vf_nf = 0;
  /* external font ids */
  integer vf_e_fnts[256] = {0};
  /* internal font ids */
  integer vf_i_fnts[256] = {0};

  set_font_type(f,real_font_type);
  if (auto_expand_vf(f))
    return; /* auto-expanded virtual font */
  stack_level = 0;
  /* @<Open |vf_file|, return if not found@>; */
  if (vf_buffer!=NULL)
    free(vf_buffer);
  vf_cur=0; vf_buffer=NULL; vf_size=0;
  
  if (!open_vf_file(f, &vf_buffer, &vf_size))
    return;
  /* @<Process the preamble@>;@/ */
  if (vf_byte() != pre) 
    bad_vf("PRE command expected");
  if (vf_byte() != vf_id)
    bad_vf("wrong id byte");
  cmd_length = vf_byte();
  for (k = 1;k<= cmd_length;k++)
    (void)vf_byte();
  tmp_b0 = vf_byte(); tmp_b1 = vf_byte(); tmp_b2 = vf_byte(); tmp_b3 = vf_byte();
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
  vf_nf = 0;
  cmd = vf_byte();
  while ((cmd >= fnt_def1) && (cmd <= fnt_def1 + 3)) {

    vf_e_fnts[vf_nf] = vf_read(cmd - fnt_def1 + 1);
    vf_i_fnts[vf_nf] = vf_def_font(f);
    incr(vf_nf);
    if (vf_nf>=255)
      bad_vf("too many local fonts");
    
    cmd = vf_byte();
  }

  /* this should often be too much, but trimming it down 
     afterwards is easier then calculating it properly. */
  set_font_packets(f,10*vf_size);

  vf_np = 1;
  save_vf_np = vf_np;

  while (cmd <= long_char) {
    /* @<Build a character packet@>;@/ */
    if (cmd == long_char) {
      packet_length = vf_read(4);
      cc = vf_read(4);
      if (!char_exists(f,cc)) {
        bad_vf("invalid character code");
      }
      tfm_width = sqxfw(vf_read(4), font_size(f));
    } else {
      packet_length = cmd;
      cc = vf_byte();
      if (!char_exists(f,cc)) {
        bad_vf("invalid character code");
      }
      tfm_width = sqxfw(vf_read(3), font_size(f));
    }
    if (packet_length < 0)
      bad_vf("negative packet length");
    if (packet_length > vf_max_packet_length)
      bad_vf("packet length too long");
    if (tfm_width != char_width(f,cc)) {
      /* precisely 'one off' errors are rampant */
      if (abs(tfm_width - char_width(f,cc))>1) {
	print_nlp();
	print_string("character width mismatch in font ");
	print_string(font_name(f));
	print_string(".vf ignored");
      }
    }
    k = 0;
    while (packet_length > 0) {
      cmd = vf_byte();
      decr(packet_length);

      if ((cmd >= set_char_0) && (cmd < set1)) {
	if (k == 0) {
	  k = vf_i_fnts[0];
	  append_fnt_set(k);
	}
	append_packet(set4);
	append_four(cmd);
	cmd_length = 0;
	cmd = nop;

      } else if (((fnt_num_0 <= cmd) && (cmd <= fnt_num_0 + 63)) ||
		 ((fnt1 <= cmd) && (cmd <= fnt1 + 3))) {
	if (cmd >= fnt1) {
	  k = vf_read(cmd - fnt1 + 1);
	  packet_length = packet_length - (cmd - fnt1 + 1);
	} else {
	  k = cmd - fnt_num_0;
	}

	/*  change from local to external font id */
	n = 0;
	while ((n < vf_nf) && (vf_e_fnts[n] != k)) 
	  n++;
	if (n==vf_nf)
	  bad_vf("undefined local font");

	k = vf_i_fnts[n];
	append_fnt_set(k);
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
	  if (k == 0) {
	    k = vf_i_fnts[0];
	    append_fnt_set(k);
	  }
	  append_packet(set4);
	  i = vf_read(cmd - set1 + 1);
	  append_four(i);
	  packet_length -= (cmd - set1 + 1);
	  cmd_length = 0;
	  cmd = nop;
	  break;
	case put1: 
	case put2: 
	case put3: 
	case put4:
	  if (k == 0) {
	    k = vf_i_fnts[0];
	    append_fnt_set(k);
	  }
	  append_packet(put4);
	  i = vf_read(cmd - put1 + 1);
	  append_four(i);
	  packet_length -= (cmd - put1 + 1);
	  cmd_length = 0;
	  cmd = nop;
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
	  packet_length = packet_length - (cmd - xxx1 + 1);
	  if (cmd_length > vf_max_packet_length)
	    bad_vf("packet length too long");
	  if (cmd_length < 0)
	    bad_vf("string of negative length");
	  append_packet(xxx1);
	  append_packet(cmd_length);
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
	append_packet(cmd);
      packet_length = packet_length - cmd_length;
      while (cmd_length > 0) {
        decr(cmd_length);
        append_packet(vf_byte());
      }
    }
    /* signal end of packet */
    append_packet(eop); 
    if (stack_level != 0)
      bad_vf("more PUSHs than POPs in character packet");
    if (packet_length != 0) 
      bad_vf("invalid packet length or DVI command in packet");
    /* @<Store the packet being built@>; */
    set_char_packet(f,cc,save_vf_np);
    save_vf_np = vf_np; 
    cmd = vf_byte();
  }
  if (cmd != post)
    bad_vf("POST command expected");
  incr(vf_np);
  set_font_packets(f,vf_np);
  set_font_type(f,virtual_font_type);
}


/* Some functions for processing character packets. */

/* read |k| bytes as an integer from character packet */

static integer packet_read (internal_font_number vf_f, integer k) {
  integer i;
  pdfassert((k > 0) && (k <= 4));
  i = do_packet_byte();
  if ((k == 4) && (i > 127)) {
    i = i - 256;
  }
  decr(k);			
  while (k > 0) {		
    i = (i*256) + do_packet_byte();
    k--;				
  }	 	     
  return i;
}


static integer 
packet_scaled(internal_font_number vf_f, integer k, scaled fs) { /* get |k| bytes from packet as a scaled */
  integer fw;
  fw = packet_read(vf_f, k);
  /* fprintf(stdout,"packet_scaled(%d,%d,%d)=>%d\n",vf_f,k,fs,fw);*/
  switch (k) {
  case 1:  
    if (fw > 127) 
      fw = fw - 256; 
    break;
  case 2:  
    if (fw > 0x8000)
      fw = fw - 0x10000; 
    break;
  case 3:  
    if (fw > 0x800000)
      fw = fw - 0x1000000; 
    break;
  };
  return sqxfw(fw, fs);
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
do_vf_packet (internal_font_number vf_f, eight_bits c) {
  internal_font_number lf;
  scaled save_cur_h, save_cur_v;
  integer cmd;
  scaled w, x, y, z, i;
  str_number s;
  
  vf_cur_s++;
  if (vf_cur_s > vf_max_recursion)
    overflow_string("max level recursion of virtual fonts", vf_max_recursion);
  save_cur_v = cur_v;
  save_cur_h = cur_h;
  /* push_packet_state(); */
  /* start_packet(vf_f, c); */
  
  lf = 0; /* for -Wall */
  
  w = 0; x = 0; y = 0; z = 0;

  cur_packet_byte = packet_index(vf_f,c);
  if (cur_packet_byte == 0) {
    vf_cur_s--;
    return ;
  }
  while ((cmd = font_packet(vf_f,cur_packet_byte))!=eop) {
    cur_packet_byte++;
    assert(!(((cmd >= set_char_0) && (cmd < set1)) ||
	     (cmd == set1) || (cmd == set2) ||(cmd == set3) ||
             (cmd == put1) || (cmd == put2) ||(cmd == put3) ||
	     (cmd == fnt1) || (cmd == fnt2) ||(cmd == fnt3) ||
             ((cmd >= fnt_num_0) && (cmd <= (fnt_num_0 + 63)))
             ));

    /* the rest: */
    switch (cmd) {
    case fnt4:
      lf = do_packet_byte();
      lf = lf*256 + do_packet_byte();
      lf = lf*256 + do_packet_byte();
      lf = lf*256 + do_packet_byte();
      break;
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
    case set4: 
    case put4: 
      c = packet_read(vf_f,4);
      if (!char_exists(lf,c)) {
		char_warning(lf, c);
      } else {
		output_one_char(lf, c);
      }
      if (cmd == set4)
	cur_h = cur_h + char_width(lf,c);
      break;
    case set_rule: 
    case put_rule: 
      rule_ht = packet_scaled(vf_f,4,font_size(vf_f));
      rule_wd = packet_scaled(vf_f,4,font_size(vf_f));
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
	  i = packet_scaled(vf_f,cmd - right1 + 1,font_size(vf_f));
      cur_h = cur_h + i;
      break;
    case w0: 
    case w1: 
    case w2: 
    case w3: 
    case w4:
      if (cmd > w0) 
		w = packet_scaled(vf_f,cmd - w0,font_size(vf_f));
      cur_h = cur_h + w;
      break;
    case x0: 
    case x1: 
    case x2: 
    case x3: 
    case x4:
      if (cmd > x0)
	x = packet_scaled(vf_f,cmd - x0,font_size(vf_f));
      cur_h = cur_h + x;
      break;
    case down1: 
    case down2: 
    case down3: 
    case down4:
      cur_v = cur_v + packet_scaled(vf_f,cmd - down1 + 1,font_size(vf_f));
      break;
    case y0: 
    case y1: 
    case y2: 
    case y3: 
    case y4:
      if (cmd > y0) 
	y = packet_scaled(vf_f,cmd - y0,font_size(vf_f));
      cur_v = cur_v + y;
      break;
    case z0: 
    case z1: 
    case z2: 
    case z3: 
    case z4:
      if (cmd > z0) 
	z = packet_scaled(vf_f,cmd - z0,font_size(vf_f));
      cur_v = cur_v + z;
      break;
    case xxx1: 
    case xxx2: 
    case xxx3: 
    case xxx4: 
      tmp_int = packet_read(vf_f,cmd - xxx1 + 1);
      string_room(tmp_int);
      while (tmp_int > 0) {
		tmp_int--;
		append_pool_char(do_packet_byte());
      }
      s = make_string();
      literal(s, scan_special, false);
      flush_str(s);
      break;
    default: 
      pdf_error("vf", "invalid DVI command");     
    }
  };
  /* pop_packet_state();*/
  cur_h = save_cur_h;
  cur_v = save_cur_v;
  vf_cur_s--;
}

/* check for a virtual auto-expanded font */

/*
  TODO 
  the fact that the external font numbers are now in the packet
  means this routine is broken and needs fixing 
*/
boolean 
auto_expand_vf(internal_font_number f) {

  internal_font_number bf, lf;
  integer e, k;

  if ((! pdf_font_auto_expand[f]) || (pdf_font_blink[f] == null_font))
    return false ; /* not an auto-expanded font */
  bf = pdf_font_blink[f];
  if (font_type(bf) == new_font_type) /* we must process the base font first */
    do_vf(bf);
  
  if (font_type(bf) != virtual_font_type)
    return false; /* not a virtual font */

  /*
  e = pdf_font_expand_ratio[f];
  for (k = 0;k<vf_local_font_num[bf];k++) {
    vf_e_fnts[vf_nf] = vf_e_fnts[lf];
    vf_i_fnts[vf_nf] = auto_expand_font(vf_i_fnts[lf], e);
    copy_expand_params(vf_i_fnts[vf_nf], vf_i_fnts[lf], e);
    vf_nf++;
  }
  vf_packet_base[f] = vf_packet_base[bf];
  vf_local_font_num[f] = vf_local_font_num[bf];
  vf_default_font[f] = vf_nf - vf_local_font_num[f];
  */
  set_font_type(f,virtual_font_type);
  return true;
}

/*
  TODO 
  the fact that the external font numbers are now in the packet
  most likely means this routine is broken and needs fixing 
*/
void 
vf_expand_local_fonts(internal_font_number f) {
  internal_font_number lf;
  integer k;
  pdfassert(font_type(f) == virtual_font_type);
  /*
  for (k = 0;k<vf_local_font_num[f];k++) {
    lf = vf_i_fnts[vf_default_font[f] + k];
    set_expand_params(lf, pdf_font_auto_expand[f],
		      pdf_font_expand_ratio[pdf_font_stretch[f]],
		      pdf_font_expand_ratio[pdf_font_shrink[f]],
		      pdf_font_step[f], pdf_font_expand_ratio[f]);
    if (font_type(lf) == virtual_font_type)
      vf_expand_local_fonts(lf);
  }
  */
}

internal_font_number 
letter_space_font (halfword u, internal_font_number f, integer e) {
  internal_font_number k;
  scaled  w, r;
  integer i;
  char *new_font_name;
  integer vf_z;
  integer vf_alpha;
  integer vf_beta;

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
    /* minus from %i */
    sprintf(new_font_name,"%s%ils",font_name(k),(int)e);
  }
  set_font_name(k, new_font_name);
  
  /* create the corresponding virtual font */
  set_font_type(k,virtual_font_type);

  vf_z = font_size(f);
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
  /*
  vf_packet_base[k] = new_vf_packet(k);

  for (c=font_bc(k);c<=font_ec(k);c++) {
    string_room(17); 
    append_fnt_set(f);
    append_packet(right1 + 3);
    append_packet(tmp_b0);
    append_packet(tmp_b1);
    append_packet(tmp_b2);
    append_packet(tmp_b3);

    append_packet(set1);
    append_packet(c);
      
    append_packet(right1 + 3);
    append_packet(tmp_b0);
    append_packet(tmp_b1);
    append_packet(tmp_b2);
    append_packet(tmp_b3);
    s = make_string();
    store_packet(k, c, s);
    flush_str(s);
  }
*/
  return k;
}

void alloc_vf_arrays (integer max) {
}

