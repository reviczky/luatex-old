
/* 
Copyright (c) 2006 Taco Hoekwater, <taco@elvenkind.com>

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

$Id $
*/


#include "luatex-api.h"
#include <ptexlib.h>

#include "managed-sa.h"

/* the 0xFFFFFFFF is a flag value */

#define MATHCODESTACK 8
#define MATHCODEDEFAULT 0xFFFFFFFF

static sa_tree mathcode_head      = NULL;

typedef struct delcodeval {
  integer     valuea;
  integer     valueb;
} delcodeval;

/* delcodes are a bit more work */

#define DELCODESTACK 4
#define DELCODEDEFAULT 0xFFFFFFFF

static sa_tree delcode_head       = NULL;

#define DELCODEHEAP 8

static delcodeval * delcode_heap      = NULL;
static int          delcode_heapsize  = DELCODEHEAP;
static int          delcode_heapptr   = 0;

void 
unsavemathcode (quarterword gl) {
  restore_sa_stack(mathcode_head,gl);
}

void 
setmathcode (integer n, halfword v, quarterword gl) {
  set_sa_item(mathcode_head,n,v,gl);
}

halfword
getmathcode (integer n) {
  unsigned int ret;
  ret = get_sa_item(mathcode_head,n);
  if (ret==MATHCODEDEFAULT) { 
	fprintf(stderr,"\\mathcode%d=%d\n",n,n);
	return (halfword)n; 
  } else { 
	fprintf(stderr,"\\mathcode%d=%d\n",n,ret);
	return (halfword)ret;
  }
}

void 
initializemathcode (void) {
  mathcode_head = new_sa_tree(MATHCODESTACK,MATHCODEDEFAULT);
}

void 
dumpmathcode (void) {
  dump_sa_tree(mathcode_head);
}

void 
undumpmathcode (void) {
  mathcode_head = undump_sa_tree();
}

/* TODO: clean up the heap */

void 
unsavedelcode (quarterword gl) {
  restore_sa_stack(delcode_head,gl);
}

void 
setdelcode (integer n, halfword v, halfword w, quarterword gl) {
  delcodeval d;
  d.valuea = v;  d.valueb = w;
  if (delcode_heapptr==delcode_heapsize) {
	delcode_heapsize += DELCODEHEAP;
	delcode_heap = Mxrealloc_array(delcode_heap,delcodeval,delcode_heapsize);
  }
  delcode_heap[delcode_heapptr] = d;
  set_sa_item(delcode_head,n,delcode_heapptr,gl);
  delcode_heapptr++;
}

halfword
getdelcodea (integer n) {
  int ret;
  ret = get_sa_item(delcode_head,n);
  if (ret==DELCODEDEFAULT) { 
	return (halfword)-1; 
  } else { 
	return (halfword)delcode_heap[ret].valuea;
  }
}

halfword
getdelcodeb (integer n) {
  unsigned int ret;
  ret = get_sa_item(delcode_head,n);
  if (ret==DELCODEDEFAULT) { 
	return (halfword)-1; 
  } else { 
	return (halfword)delcode_heap[ret].valueb;
  }
}

void 
initializedelcode (void) {
  delcode_head = new_sa_tree(DELCODESTACK,DELCODEDEFAULT);
  delcode_heap = Mxmalloc_array(delcodeval,DELCODEHEAP);
}

void 
dumpdelcode (void) {
  int k;
  delcodeval d;
  dump_sa_tree(delcode_head);
  dumpint(delcode_heapsize);
  dumpint(delcode_heapptr);
  for (k=0;k<delcode_heapptr;k++) {
	d = delcode_heap[k];
	dumpint(d.valuea);
	dumpint(d.valueb);
  }
}

void 
undumpdelcode (void) {
  int k;
  delcodeval d;
  delcode_head = undump_sa_tree();
  undumpint(delcode_heapsize);
  undumpint(delcode_heapptr);
  delcode_heap = Mxmalloc_array(delcodeval,delcode_heapsize);
  for (k=0;k<delcode_heapptr;k++) {
	undumpint(d.valuea);
	undumpint(d.valueb);
	delcode_heap[k] = d;
  }
  d.valuea=0; d.valueb=0;
  for (k=delcode_heapptr;k<delcode_heapsize;k++) {
	delcode_heap[k] = d;
  }
}

void 
unsavemathcodes (quarterword grouplevel) {
  unsavemathcode(grouplevel);
  unsavedelcode(grouplevel);
}

void initializemathcodes (void) {
  initializemathcode();
  initializedelcode();
}

void dumpmathcodes(void) {
  dumpmathcode();
  dumpdelcode();
}

void undumpmathcodes(void) {
  undumpmathcode();
  undumpdelcode();
}
