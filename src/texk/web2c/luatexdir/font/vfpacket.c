/*
Copyright (c) 1996-2002 Han The Thanh, <thanh@pdftex.org>

This file is part of pdfTeX.

pdfTeX is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pdfTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pdfTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id: //depot/Build/source.development/TeX/texk/web2c/pdftexdir/vfpacket.c#7 $
*/

#include "ptexlib.h"

#define packet_max_recursion 100

static integer cur_packet_byte;

#define do_packet_byte() font_packet(vf_f,cur_packet_byte++)

typedef unsigned char packet_stack_index ;  /* an index into the stack */

typedef struct packet_stack_record {
  scaled stack_h;
  scaled stack_v;
} packet_stack_record;


static packet_stack_index  packet_cur_s = 0; /* current recursion level */
static packet_stack_record packet_stack [packet_max_recursion];
static packet_stack_index  packet_stack_ptr = 0; /* pointer into |packet_stack| */


/*
@ The |do_vf_packet| procedure is called in order to interpret the
character packet for a virtual character. Such a packet may contain the
instruction to typeset a character from the same or an other virtual
font; in such cases |do_vf_packet| calls itself recursively. The
recursion level, i.e., the number of times this has happened, is kept
in the global variable |vf_cur_s| and should not exceed |packet_max_recursion|.
*/

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



/* typeset the \.{DVI} commands in the
   character packet for character |c| in current font |f| */

void 
do_vf_packet (internal_font_number vf_f, eight_bits c) {
  internal_font_number lf;
  scaled save_cur_h, save_cur_v;
  integer cmd;
  scaled i;
  int k;
  str_number s;

  packet_cur_s++;
  if (packet_cur_s >= packet_max_recursion)
    overflow_string("max level recursion of virtual fonts", packet_max_recursion);
  save_cur_v = cur_v;
  save_cur_h = cur_h;
  
  lf = 0; /* for -Wall */
  
  cur_packet_byte = packet_index(vf_f,c);
  if (cur_packet_byte == 0) {
    packet_cur_s--;
    return ;
  }
  while ((cmd = font_packet(vf_f,cur_packet_byte)) != packet_end_code) {

    cur_packet_byte++;
    switch (cmd) {
    case packet_font_code:
      lf = do_packet_byte();
      lf = lf*256 + do_packet_byte();
      lf = lf*256 + do_packet_byte();
      lf = lf*256 + do_packet_byte();
      break;
    case packet_push_code: 
      packet_stack[packet_stack_ptr].stack_h = cur_h;
      packet_stack[packet_stack_ptr].stack_v = cur_v;
      packet_stack_ptr++;
      break;
    case packet_pop_code:
      packet_stack_ptr--;
      cur_h = packet_stack[packet_stack_ptr].stack_h;
      cur_v = packet_stack[packet_stack_ptr].stack_v;
      break;
    case packet_char_code: 
      c = packet_read(vf_f,4);
      if (!char_exists(lf,c)) {
	char_warning(lf, c);
      } else {
	output_one_char(lf, c);
      }
      cur_h = cur_h + char_width(lf,c);
      break;
    case packet_rule_code: 
      rule_ht = packet_scaled(vf_f,4,font_size(vf_f));
      rule_wd = packet_scaled(vf_f,4,font_size(vf_f));
      if ((rule_wd > 0) && (rule_ht > 0)) {
	pdf_set_rule(cur_h, cur_v, rule_wd, rule_ht);
	cur_h = cur_h + rule_wd;
      }
      break;
    case packet_right_code:
      i = packet_scaled(vf_f,4,font_size(vf_f));
      cur_h = cur_h + i;
      break;
    case packet_down_code:
      cur_v = cur_v + packet_scaled(vf_f,4,font_size(vf_f));
      break;
    case packet_special_code:
      k = packet_read(vf_f,4);
      string_room(k);
      while (k > 0) {
	k--;
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
  packet_cur_s--;
}

/* this function was copied/borrowed/stolen from dvipdfm code */

scaled sqxfw (scaled sq, integer fw)
{
  int sign = 1;
  unsigned long a, b, c, d, ad, bd, bc, ac;
  unsigned long e, f, g, h, i, j, k;
  unsigned long result;
  /* Make positive. */
  if (sq < 0) {
    sign = -sign;
    sq = -sq;
  }
  if (fw < 0) {
    sign = -sign;
    fw = -fw;
  }
  a = ((unsigned long) sq) >> 16u;
  b = ((unsigned long) sq) & 0xffffu;
  c = ((unsigned long) fw) >> 16u;
  d = ((unsigned long) fw) & 0xffffu;
  ad = a*d; bd = b*d; bc = b*c; ac = a*c;
  e = bd >> 16u;
  f = ad >> 16u;
  g = ad & 0xffffu;
  h = bc >> 16u;
  i = bc & 0xffffu;
  j = ac >> 16u;
  k = ac & 0xffffu;
  result = (e+g+i + (1<<3)) >> 4u;  /* 1<<3 is for rounding */
  result += (f+h+k) << 12u;
  result += j << 28u;
  return (sign>0)?result:-result;
}
