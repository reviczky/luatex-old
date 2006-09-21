
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
static int catcode_max = 0;
static unsigned char *catcode_valid = NULL;

void check_catcode_sizes (int h) {
  int k;
  if (h < 0)
    uexit(1);
  if (h>catcode_max) {
	catcode_heads = Mxrealloc_array(catcode_heads,sa_tree,(h+1));
	catcode_valid = Mxrealloc_array(catcode_valid,unsigned char,(h+1));
	for (k=(catcode_max+1);k<=h;k++) {
	  catcode_heads[k] = NULL;
	  catcode_valid[k] = 0;
	}
	catcode_max = h;
  }
}

void  setcatcode (integer h, integer n, halfword v, quarterword gl) {
  check_catcode_sizes(h);
  if (catcode_heads[h] == NULL) {
    catcode_heads[h] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
  }
  set_sa_item(catcode_heads[h],n,v,gl);
}

halfword getcatcode (integer h, integer n) {
  check_catcode_sizes(h);
  if (catcode_heads[h] == NULL) {
    catcode_heads[h] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
  }
  return (halfword)get_sa_item(catcode_heads[h],n);
}

void unsavecatcodes (integer h, quarterword gl) {
  int k;
  check_catcode_sizes(h);
  for (k=0;k<=catcode_max;k++) {
    if (catcode_heads[k] != NULL)
      restore_sa_stack(catcode_heads[k],gl);
  }
}

void clearcatcodestack(integer h) {
  clear_sa_stack(catcode_heads[h]);
}

static void initializecatcodes (void) {
  catcode_max   = 0;
  catcode_heads = Mxmalloc_array(sa_tree,(catcode_max+1));
  catcode_valid = Mxmalloc_array(unsigned char,(catcode_max+1));
  catcode_valid[0] = 1;
  catcode_heads[0] = new_sa_tree(CATCODESTACK,CATCODEDEFAULT);
}

static void dumpcatcodes (void) {
  int k,total;
  dumpint(catcode_max);
  total = 0;
  for (k=0;k<=catcode_max;k++) {
    if (catcode_valid[k]) {
      total++;
    }
  }
  dumpint(total);
  for (k=0;k<=catcode_max;k++) {
    if (catcode_valid[k]) {
      dumpint(k);
      dump_sa_tree(catcode_heads[k]);
    }
  }
}

static void undumpcatcodes (void) {
  int total,h,k;
  undumpint(catcode_max);
  catcode_heads = Mxmalloc_array(sa_tree,(catcode_max+1));
  catcode_valid = Mxmalloc_array(unsigned char,(catcode_max+1));
  for (k=0;k<=catcode_max;k++) {
    catcode_heads[k]=NULL;
    catcode_valid[k]=0;
  }
  undumpint(total);
  for (k=0;k<total;k++) {
    undumpint(h);
    catcode_heads[h] = undump_sa_tree(); 
    catcode_valid[h] = 1;
  }
}

int validcatcodetable (int h) {
  if (h<=catcode_max && h>=0 && catcode_valid[h]) {
    return 1;
  }
  return 0;
}

void copycatcodes (int from, int to) {
  if (from<0 || from>catcode_max || catcode_valid[from] == 0) {
	uexit(1);
  }
  check_catcode_sizes(to);
  destroy_sa_tree(catcode_heads[to]);
  catcode_heads[to] = copy_sa_tree(catcode_heads[from]); 
  catcode_valid[to] = 1;
}

void initexcatcodes (int h) {
  int k;
  check_catcode_sizes(h);
  destroy_sa_tree(catcode_heads[h]);
  catcode_heads[h] = NULL;
  setcatcode(h,'\r',car_ret,1);
  setcatcode(h,' ',spacer,1);
  setcatcode(h,'\\',escape,1);
  setcatcode(h,'%',comment,1);
  setcatcode(h,127,invalid_char,1);
  setcatcode(h,0,ignore,1);
  for (k='A';k<='Z';k++) {
	setcatcode(h,k,letter,1); 
	setcatcode(h,k+'a'-'A',letter,1);
  }
  catcode_valid[h] = 1;
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

