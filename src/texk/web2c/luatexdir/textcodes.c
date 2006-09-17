
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
#include <stdarg.h>

#include "managed-sa.h"

#define LCCODESTACK  8
#define LCCODEDEFAULT 0

static sa_tree  lccode_head  = NULL;

#define UCCODESTACK  8
#define UCCODEDEFAULT 0

static sa_tree  uccode_head  = NULL;

#define SFCODESTACK 8
#define SFCODEDEFAULT 1000

static sa_tree  sfcode_head  = NULL;

#define CATCODESTACK 8
#define CATCODEDEFAULT 12

void  setlccode (integer n, halfword v, quarterword gl) {
  set_sa_item(lccode_head,n,v,gl);
}

halfword getlccode (integer n) {
  return (halfword)get_sa_item(lccode_head,n);
}

static void unsavelccodes (quarterword gl) {
  restore_sa_stack(lccode_head,gl);
}

static void initializelccodes (void) {
  lccode_head = new_sa_tree(LCCODESTACK,LCCODEDEFAULT);
}

static void dumplccodes (void) {
  dump_sa_tree(lccode_head);
}

static void undumplccodes (void) {
  lccode_head  = undump_sa_tree();
}

void  setuccode (integer n, halfword v, quarterword gl) {
  set_sa_item(uccode_head,n,v,gl);
}

halfword getuccode (integer n) {
  return (halfword)get_sa_item(uccode_head,n);
}

static void unsaveuccodes (quarterword gl) {
  restore_sa_stack(uccode_head,gl);
}

static void initializeuccodes (void) {
  uccode_head = new_sa_tree(UCCODESTACK,UCCODEDEFAULT);
}

static void dumpuccodes (void) {
  dump_sa_tree(uccode_head);
}

static void undumpuccodes (void) {
  uccode_head  = undump_sa_tree();
}

void  setsfcode (integer n, halfword v, quarterword gl) {
  set_sa_item(sfcode_head,n,v,gl);
}

halfword getsfcode (integer n) {
  return (halfword)get_sa_item(sfcode_head,n);
}

static void unsavesfcodes (quarterword gl) {
  restore_sa_stack(sfcode_head,gl);
}

static void initializesfcodes (void) {
  sfcode_head = new_sa_tree(SFCODESTACK,SFCODEDEFAULT);
}

static void dumpsfcodes (void) {
  dump_sa_tree(sfcode_head);
}

static void undumpsfcodes (void) {
  sfcode_head  = undump_sa_tree();
}


static sa_tree *catcode_heads = NULL;
static int catcode_ptr = 0;
static unsigned char catcode_valid[256];

void  setcatcode (integer h, integer n, halfword v, quarterword gl) {
  if (h>255 || h < 0) {
    uexit(1);
  }
  if (catcode_heads[h] == NULL) {
    catcode_heads[h] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
    if (h>catcode_ptr) 
      catcode_ptr = h;
  }
  set_sa_item(catcode_heads[h],n,v,gl);
}

halfword getcatcode (integer h, integer n) {
  if (h>255 || h < 0) {
    uexit(1);
  }
  if (catcode_heads[h] == NULL) {
    catcode_heads[h] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
    if (h>catcode_ptr) 
      catcode_ptr = h;
  }
  return (halfword)get_sa_item(catcode_heads[h],n);
}

void unsavecatcodes (integer h, quarterword gl) {
  if (h>255 || h < 0) {
    uexit(1);
  }
  if (catcode_heads[h] == NULL) {
    catcode_heads[h] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
    if (h>catcode_ptr) 
      catcode_ptr = h;
  }
  restore_sa_stack(catcode_heads[h],gl);
}

static void initializecatcodes (void) {
  int k;
  catcode_heads = Mxmalloc_array(sa_tree,256);
  for (k=0;k<=255;k++) {
    catcode_heads[k]=NULL;
  }
  catcode_heads[0] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
  catcode_valid[0]=1;
  catcode_ptr = 0;
}

static void dumpcatcodes (void) {
  int k,total;
  dumpint(catcode_ptr);
  total = 0;
  for (k=0;k<=255;k++) {
    if (catcode_valid[k]) {
      total++;
    }
  }
  dumpint(total);
  for (k=0;k<=255;k++) {
    if (catcode_valid[k]) {
      dumpint(k);
      dump_sa_tree(catcode_heads[k]);
    }
  }
}

static void undumpcatcodes (void) {
  int total,current;
  int k = 0;
  catcode_heads = Mxmalloc_array(sa_tree,256);
  for (k=0;k<=255;k++) {
    catcode_heads[k]=NULL;
    catcode_valid[k]=0;
  }
  undumpint(catcode_ptr);
  undumpint(total);
  k = 0;
  while (k!=total) {
    undumpint(current);
    catcode_heads[current] = undump_sa_tree(); 
    catcode_valid[current]=1;
    k++;
  }
  
}

int validcatcodetable (int h) {
  if (h<=255 && h >= 0 && catcode_valid[h]) {
    return 1;
  }
  return 0;
}

void copycatcodes (int from, int to) {
  if (from>255 || from < 0 || to>255 || to<0 || catcode_heads[from] == NULL) {
    uexit(1);
  }
  catcode_heads[to] = copy_sa_tree(catcode_heads[from]); 
  catcode_valid[to]=1;
  if (to>catcode_ptr) 
    catcode_ptr = to;
}


void 
unsavetextcodes (quarterword grouplevel) {
  unsavesfcodes(grouplevel);
  unsaveuccodes(grouplevel);
  unsavelccodes(grouplevel);
}

void 
initializetextcodes (void) {
  initializesfcodes();
  initializeuccodes();
  initializecatcodes();
  initializelccodes();
}

void 
dumptextcodes (void) {
  dumpsfcodes();
  dumpuccodes();
  dumplccodes();
  dumpcatcodes();
}

void 
undumptextcodes (void) {
  undumpsfcodes();
  undumpuccodes();
  undumplccodes();
  undumpcatcodes();
}

