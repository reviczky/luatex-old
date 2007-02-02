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

#if defined(OBSOLETE_STUFF)

extern integer *vf_packet_base;
extern integer vf_packet_length;

typedef struct {
    char *dataptr;
    integer len;
} packet_entry;

/* define packet_ptr, packet_array & packet_limit */
define_array (packet);

typedef struct {
    char **data;
    int *len;
    int char_count;
} vf_entry;

/* define vf_ptr, vf_array & vf_limit */
define_array (vf);

static char *packet_data_ptr;

integer new_vf_packet (internalfontnumber f)
{
  int i, n = font_ec(f) - font_bc(f) + 1;
    alloc_array (vf, 1, SMALL_ARRAY_SIZE);
    vf_ptr->len = xtalloc (n, int);
    vf_ptr->data = xtalloc (n, char *);
    vf_ptr->char_count = n;
    for (i = 0; i < n; i++) {
        vf_ptr->data[i] = NULL;
        vf_ptr->len[i] = 0;
    }
    return vf_ptr++ - vf_array;
}

void store_packet (internal_font_number f, eight_bits c, str_number s)
{
  assert(s>=2097152);
  s-=2097152;
  int l = strstart[s + 1] - strstart[s];
  /* fprintf(stdout,"Storing a packet [%d][%d], length = %d\n", f,c,l); */
  vf_array[vf_packet_base[f]].len[c - font_bc(f)] = l;
  vf_array[vf_packet_base[f]].data[c - font_bc(f)] = xtalloc (l, char);
  memcpy ((void *) vf_array[vf_packet_base[f]].data[c - font_bc(f)],
		  (void *) (strpool + strstart[s]), (unsigned) l);
}

void start_packet(internal_font_number f, eight_bits c)
{
  packet_data_ptr = vf_array[vf_packet_base[f]].data[c - font_bc(f)];
  vf_packet_length = vf_array[vf_packet_base[f]].len[c - font_bc(f)];
  /* fprintf(stdout,"Reading a packet [%d][%d], length = %d\n", f,c,vf_packet_length); */
}

real_eight_bits packet_byte()
{
    vf_packet_length--;
    return *packet_data_ptr++;
}


void push_packet_state ()
{
    alloc_array (packet, 1, SMALL_ARRAY_SIZE);
    packet_ptr->dataptr = packet_data_ptr;
    packet_ptr->len = vf_packet_length;
    packet_ptr++;
}

void pop_packet_state ()
{
    if (packet_ptr == packet_array)
        pdftex_fail ("packet stack empty, impossible to pop");
    packet_ptr--;
    packet_data_ptr = packet_ptr->dataptr;
    vf_packet_length = packet_ptr->len;
}
void vf_free (void)
{
    vf_entry *v;
    char **p;
    if (vf_array != NULL) {
        for (v = vf_array; v < vf_ptr; v++) {
            xfree (v->len);
			for (p = v->data; p - v->data < v->char_count; p++)
                xfree (*p);
            xfree (v->data);
        }
        xfree (vf_array);
    }
    xfree (packet_array);
}

#else 
void vf_free (void)
{
}
#endif

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
