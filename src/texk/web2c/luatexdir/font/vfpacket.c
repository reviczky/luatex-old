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

extern integer *vf_packet_base;
extern integer vf_packet_length;

typedef struct {
    internalfontnumber font;
    char *dataptr;
    int len;
} packet_entry;

/* define packet_ptr, packet_array & packet_limit */
define_array (packet);

typedef struct {
    char **data;
    int *len;
    internalfontnumber font;
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
    for (i = 0; i < n; i++) {
        vf_ptr->data[i] = NULL;
        vf_ptr->len[i] = 0;
    }
    vf_ptr->font = f;
    return vf_ptr++ - vf_array;
}

void store_packet (integer f, integer c, integer s)
{
  if(s>=2097152) {
	s-=2097152;
    int l = strstart[s + 1] - strstart[s];
    vf_array[vf_packet_base[f]].len[c - font_bc(f)] = l;
    vf_array[vf_packet_base[f]].data[c - font_bc(f)] = xtalloc (l, char);
    memcpy ((void *) vf_array[vf_packet_base[f]].data[c - font_bc(f)],
            (void *) (strpool + strstart[s]), (unsigned) l);
  } else {
	pdftex_fail("vfpacket.c: storepacket() for single characters: NI ");
  }
}

void push_packet_state ()
{
    alloc_array (packet, 1, SMALL_ARRAY_SIZE);
    packet_ptr->font = f;
    packet_ptr->dataptr = packet_data_ptr;
    packet_ptr->len = vf_packet_length;
    packet_ptr++;
}

void pop_packet_state ()
{
    if (packet_ptr == packet_array)
        pdftex_fail ("packet stack empty, impossible to pop");
    packet_ptr--;
    f = packet_ptr->font;
    packet_data_ptr = packet_ptr->dataptr;
    vf_packet_length = packet_ptr->len;
}

void start_packet (internalfontnumber f, integer c)
{
  packet_data_ptr = vf_array[vf_packet_base[f]].data[c - font_bc(f)];
  vf_packet_length = vf_array[vf_packet_base[f]].len[c - font_bc(f)];
}

real_eight_bits packet_byte ()
{
    vf_packet_length--;
    return *packet_data_ptr++;
}

void vf_free (void)
{
    vf_entry *v;
    int n;
    char **p;
    if (vf_array != NULL) {
        for (v = vf_array; v < vf_ptr; v++) {
            xfree (v->len);
            n = font_ec(v->font) - font_bc(v->font) + 1;
            for (p = v->data; p - v->data < n; p++)
                xfree (*p);
            xfree (v->data);
        }
        xfree (vf_array);
    }
    xfree (packet_array);
}
