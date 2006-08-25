
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

#define HIGHPART 68
#define MIDPART 128
#define LOWPART 128

#define textcode_value(a) (a % 0x1FFFFF)
#define textcode_level(a) (a / 0x1FFFFF)
#define level_plus_value(a,b) (a * 0x1FFFFF + b)

#define INIT_HIGHPART(a,b)   if (a == NULL) {	   \
    a = (b ***) Mxmalloc_array(void *,HIGHPART);   \
    for  (i=0; i<HIGHPART; i++) { a[i] = NULL; }  }

#define INIT_MIDPART(a,b)   if (a == NULL) {	   \
    a = (b **) Mxmalloc_array(void *,MIDPART);     \
    for  (i=0; i<MIDPART; i++) { a[i] = NULL; }  }

#define INIT_LOWPART(a,b)   if (a == NULL) {	    \
    a = Mxmalloc_array(b,LOWPART);                  \
    for  (i=0; i<LOWPART; i++) { a[i] = 0; }  }

#define Mxmalloc_array(a,b)  malloc(b*sizeof(a))
#define Mxrealloc_array(a,b,c)  realloc(a,c*sizeof(b))
#define Mxfree(a) (a)

typedef struct mathcodeval {
  integer     value;
  quarterword level;
} mathcodeval;

typedef struct mathcodestack {
  integer     value;
  integer     code;
} mathcodestack;


typedef struct delcodeval {
  integer     valuea;
  integer     valueb;
  quarterword level;
} delcodeval;

typedef struct delcodestack {
  integer     value;
  integer     code;
} delcodestack;

static mathcodeval     nullmathcodeval;
static integer     *** mathcode_head      = NULL;
static mathcodeval   * mathcode_heap      = NULL;
static mathcodestack * mathcode_stack     = NULL;
static int             mathcode_stacksize =    8;
static int             mathcode_stackptr  =    0;
static int             mathcode_heapsize  =   16;
static int             mathcode_heapptr   =    1;


static delcodeval      nulldelcodeval;
static integer     *** delcode_head      = NULL;
static delcodeval   *  delcode_heap      = NULL;
static delcodestack *  delcode_stack     = NULL;
static int             delcode_stacksize =    8;
static int             delcode_stackptr  =    0;
static int             delcode_heapsize  =   16;
static int             delcode_heapptr   =    1;

static void 
savemathcode (integer n, integer v, quarterword grouplevel) {
  mathcodestack st;
  st.value = v;
  st.code   = level_plus_value(grouplevel,n);
  if ((mathcode_stackptr+1)==mathcode_stacksize) {
    mathcode_stacksize += 16;
    mathcode_stack = Mxrealloc_array(mathcode_stack,mathcodestack,mathcode_stacksize);
  }
  mathcode_stack[mathcode_stackptr++] = st;
}

void 
unsavemathcode (quarterword grouplevel) {
  mathcodestack st;
  unsigned char h,m,l;
  integer n,ii;
  st = mathcode_stack[(mathcode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
    n = textcode_value(st.code);
    h = n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    // destroy old value
    ii = mathcode_head[h][m][l];
    if (st.value==0) {
      mathcode_head[h][m][l] = st.value;
    } else {
      if (mathcode_heap[ii].level > mathcode_heap[st.value].level) {
	mathcode_heap[ii] = nullmathcodeval;
	if (mathcode_heapptr>ii)
	  mathcode_heapptr=ii;
	mathcode_head[h][m][l] = st.value;
      }
    }
    mathcode_stackptr--;
    st = mathcode_stack[(mathcode_stackptr-1)];
  }
}

void 
setmathcode (integer n, halfword v, quarterword grouplevel) {
  unsigned char h,m,l;
  mathcodeval newmathcode;
  int i,ii;
  newmathcode.value = v;
  newmathcode.level = grouplevel;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(mathcode_head,integer);
  INIT_MIDPART(mathcode_head[h],integer);
  INIT_LOWPART(mathcode_head[h][m],integer);
  ii = mathcode_head[h][m][l];
  if (ii != 0) {
    if (mathcode_heap[ii].level <= newmathcode.level &&
        mathcode_heap[ii].value == newmathcode.value) {
      return;
    } else if (grouplevel != 1) {
      if (mathcode_heap[ii].level != newmathcode.level) {
	savemathcode(n,ii,grouplevel);
      }
    }
  } else {
    savemathcode(n,0,grouplevel);
  }
  if ((mathcode_heapptr+1)==mathcode_heapsize) {
    mathcode_heapsize += 128;
    mathcode_heap = Mxrealloc_array(mathcode_heap,mathcodeval,mathcode_heapsize);
    for (i=mathcode_heapptr+1; i<mathcode_heapsize; i++) {
      mathcode_heap[i] = nullmathcodeval;
    }
  }
  mathcode_head[h][m][l] = mathcode_heapptr++;
  while (mathcode_heap[mathcode_heapptr].level>0) {
    mathcode_heapptr++;
  }
  mathcode_heap[(mathcode_head[h][m][l])] = newmathcode;
  return;
}


halfword
getmathcode (integer n) {
  mathcodeval ret;
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (mathcode_head != NULL && mathcode_head[h] != NULL &&
      mathcode_head[h][m] != NULL && mathcode_head[h][m][l]>0) {
    ret = mathcode_heap[(mathcode_head[h][m][l])];
    return (halfword)ret.value;
  }
  /* \mathcode n=n is the default */
  return n;
}

void 
initializemathcode (void) {
  int i;
  if (mathcode_head==NULL) {
    mathcode_head = (integer ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      mathcode_head[i] = NULL;
    }
  }
  nullmathcodeval.value = 0;
  nullmathcodeval.level = 0;
  if (mathcode_heap ==NULL) {
    mathcode_heap = Mxmalloc_array(mathcodeval,mathcode_heapsize);
    for  (i=0; i<mathcode_heapsize; i++) {
      mathcode_heap[i] = nullmathcodeval;
    }
  }
  if (mathcode_stack == NULL) {
    mathcode_stack = Mxmalloc_array(mathcodestack,mathcode_stacksize);
  }
  return;
}

void 
dumpmathcode (void) {
  int h,m,l;
  unsigned int x;
  mathcodestack y;
  mathcodeval z;
  // dump the mathcode stack
  dumpint(mathcode_stacksize);
  dumpint(mathcode_stackptr);
  //  fprintf(stdout,"!mathcode_stack: %d\n",mathcode_stackptr);
  for (x= 0; x<mathcode_stackptr; x++) {
    y = mathcode_stack[x];
    dumpint(y.value);
    dumpint(y.code);
  }
  // dump the mathcode heap
  dumpint(mathcode_heapsize);
  dumpint(mathcode_heapptr);
  //  fprintf(stdout,"!mathcode_heap: %d\n",mathcode_heapptr);
  for (x= 0; x<mathcode_heapsize; x++) {
    z = mathcode_heap[x];
    dumpint(z.level);
    dumpint(z.value);
  }
  // dump the mathcode index table info
  for (h=0; h<HIGHPART;h++ ) {
    if (mathcode_head[h] != NULL) {
      x = 1;  dumpint(x);
      for (m=0; m<MIDPART; m++ ) {
	if (mathcode_head[h][m] != NULL) {
	  x = 1;  dumpint(x);
	  for (l=0;l<LOWPART;l++) {
	    x = mathcode_head[h][m][l];  dumpint(x);
	  }
	} else {
	  x = 0;  dumpint(x);
	}
      }
    } else { // head is null
      x = 0;  dumpint(x);
    }
  }
}

void 
undumpmathcode (void) {
  int h,m,l;
  unsigned int x;
  mathcodestack y;
  mathcodeval z;
  // destroy current stuff
  if (mathcode_stack != NULL)
    Mxfree(mathcode_stack);
  if (mathcode_heap != NULL)
    Mxfree(mathcode_heap);
  if (mathcode_head != NULL) {
    for (h=0; h<HIGHPART;h++ ) {
      if (mathcode_head[h] != NULL) {
	for (m=0; m<MIDPART; m++ ) {
	  if (mathcode_head[h][m] != NULL) {
	    Mxfree(mathcode_head[h][m]);
	  }
	}
      }
      Mxfree(mathcode_head[h]);
    }
    Mxfree(mathcode_head);
  }
  // undump the mathcode save stack
  undumpint(mathcode_stacksize);
  undumpint(mathcode_stackptr);
  mathcode_stack = Mxmalloc_array(mathcodestack,mathcode_stacksize);
  for (h = 0; h <mathcode_stackptr; h++) {
    undumpint(x); y.value = x;
    undumpint(x); y.code = x;
    mathcode_stack[h] = y;
  }
  // undump the mathcode heap
  undumpint(mathcode_heapsize);
  undumpint(mathcode_heapptr);
  mathcode_heap = Mxmalloc_array(mathcodeval,mathcode_heapsize);
  for (h= 0; h<mathcode_heapsize; h++) {
    undumpint(x); z.level = x;
    undumpint(x); z.value = x;
    mathcode_heap[h] = z;
  }
  // undump the mathcode index table info
  mathcode_head = (integer ***) Mxmalloc_array(void *,HIGHPART);
  for (h=0; h<HIGHPART;h++ ) {
    undumpint(x);
    if (x>0) {
      mathcode_head[h]=(integer **)Mxmalloc_array(void *,MIDPART);
      for (m=0; m<MIDPART; m++ ) {
	undumpint(x);
	if (x>0) {
	  mathcode_head[h][m]=Mxmalloc_array(integer,LOWPART);
	  for  (l=0; l<LOWPART; l++) {
	    undumpint(x);
	    mathcode_head[h][m][l] = x;
	  }
	} else {
	  mathcode_head[h][m] = NULL;
	}
      }
    } else {
      mathcode_head[h]= NULL;
    }
  }
}


static void 
savedelcode (integer n, integer v, quarterword grouplevel) {
  delcodestack st;
  st.value = v;
  st.code   = level_plus_value(grouplevel,n);
  if ((delcode_stackptr+1)==delcode_stacksize) {
    delcode_stacksize += 16;
    delcode_stack = Mxrealloc_array(delcode_stack,delcodestack,delcode_stacksize);
  }
  delcode_stack[delcode_stackptr++] = st;
}

void 
unsavedelcode (quarterword grouplevel) {
  delcodestack st;
  unsigned char h,m,l;
  integer n,ii;
  st = delcode_stack[(delcode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
    n = textcode_value(st.code);
    h = n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    // destroy old value
    ii = delcode_head[h][m][l];
    if (st.value==0) {
      delcode_head[h][m][l] = st.value;
    } else {
      if (delcode_heap[ii].level > delcode_heap[st.value].level) {
	delcode_heap[ii] = nulldelcodeval;
	if (delcode_heapptr>ii)
	  delcode_heapptr=ii;
	delcode_head[h][m][l] = st.value;
      }
    }
    delcode_stackptr--;
    st = delcode_stack[(delcode_stackptr-1)];
  }
}

void 
setdelcode (integer n, halfword v, halfword w, quarterword grouplevel) {
  unsigned char h,m,l;
  delcodeval newdelcode;
  int i,ii;
  newdelcode.valuea = v;
  newdelcode.valueb = w;
  newdelcode.level = grouplevel;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(delcode_head,integer);
  INIT_MIDPART(delcode_head[h],integer);
  INIT_LOWPART(delcode_head[h][m],integer);
  ii = delcode_head[h][m][l];
  if (ii != 0) {
    if (delcode_heap[ii].level <= newdelcode.level &&
        (delcode_heap[ii].valuea == newdelcode.valuea &&
         delcode_heap[ii].valueb == newdelcode.valueb)) {
      return;
    } else if (grouplevel != 1) {
      if (delcode_heap[ii].level != newdelcode.level) {
	savedelcode(n,ii,grouplevel);
      }
    }
  } else {
    savedelcode(n,0,grouplevel);
  }
  if ((delcode_heapptr+1)==delcode_heapsize) {
    delcode_heapsize += 128;
    delcode_heap = Mxrealloc_array(delcode_heap,delcodeval,delcode_heapsize);
    for (i=delcode_heapptr+1; i<delcode_heapsize; i++) {
      delcode_heap[i] = nulldelcodeval;
    }
  }
  delcode_head[h][m][l] = delcode_heapptr++;
  while (delcode_heap[delcode_heapptr].level>0) {
    delcode_heapptr++;
  }
  delcode_heap[(delcode_head[h][m][l])] = newdelcode;
  return;
}


halfword
getdelcodea (integer n) {
  delcodeval ret;
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (delcode_head != NULL && delcode_head[h] != NULL &&
      delcode_head[h][m] != NULL && delcode_head[h][m][l]>0) {
    ret = delcode_heap[(delcode_head[h][m][l])];
    return (halfword)ret.valuea;
  }
  /* \delcode n=-1,-1 is the default */
  return -1;
}

halfword
getdelcodeb (integer n) {
  delcodeval ret;
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (delcode_head != NULL && delcode_head[h] != NULL &&
      delcode_head[h][m] != NULL && delcode_head[h][m][l]>0) {
    ret = delcode_heap[(delcode_head[h][m][l])];
    return (halfword)ret.valueb;
  }
  /* \delcode n=n is the default */
  return -1;
}


void 
initializedelcode (void) {
  int i;
  if (delcode_head==NULL) {
    delcode_head = (integer ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      delcode_head[i] = NULL;
    }
  }
  nulldelcodeval.valuea = -1;
  nulldelcodeval.valueb = -1;
  nulldelcodeval.level = 0;
  if (delcode_heap ==NULL) {
    delcode_heap = Mxmalloc_array(delcodeval,delcode_heapsize);
    for  (i=0; i<delcode_heapsize; i++) {
      delcode_heap[i] = nulldelcodeval;
    }
  }
  if (delcode_stack == NULL) {
    delcode_stack = Mxmalloc_array(delcodestack,delcode_stacksize);
  }
  return;
}

void 
dumpdelcode (void) {
  int h,m,l;
  unsigned int x;
  delcodestack y;
  delcodeval z;
  // dump the delcode stack
  dumpint(delcode_stacksize);
  dumpint(delcode_stackptr);
  //  fprintf(stdout,"!delcode_stack: %d\n",delcode_stackptr);
  for (x= 0; x<delcode_stackptr; x++) {
    y = delcode_stack[x];
    dumpint(y.value);
    dumpint(y.code);
  }
  // dump the delcode heap
  dumpint(delcode_heapsize);
  dumpint(delcode_heapptr);
  //  fprintf(stdout,"!delcode_heap: %d\n",delcode_heapptr);
  for (x= 0; x<delcode_heapsize; x++) {
    z = delcode_heap[x];
    dumpint(z.level);
    dumpint(z.valuea);
    dumpint(z.valueb);
  }
  // dump the delcode index table info
  for (h=0; h<HIGHPART;h++ ) {
    if (delcode_head[h] != NULL) {
      x = 1;  dumpint(x);
      for (m=0; m<MIDPART; m++ ) {
	if (delcode_head[h][m] != NULL) {
	  x = 1;  dumpint(x);
	  for (l=0;l<LOWPART;l++) {
	    x = delcode_head[h][m][l];  dumpint(x);
	  }
	} else {
	  x = 0;  dumpint(x);
	}
      }
    } else { // head is null
      x = 0;  dumpint(x);
    }
  }
}

void 
undumpdelcode (void) {
  int h,m,l;
  unsigned int x;
  delcodestack y;
  delcodeval z;
  // destroy current stuff
  if (delcode_stack != NULL)
    Mxfree(delcode_stack);
  if (delcode_heap != NULL)
    Mxfree(delcode_heap);
  if (delcode_head != NULL) {
    for (h=0; h<HIGHPART;h++ ) {
      if (delcode_head[h] != NULL) {
	for (m=0; m<MIDPART; m++ ) {
	  if (delcode_head[h][m] != NULL) {
	    Mxfree(delcode_head[h][m]);
	  }
	}
      }
      Mxfree(delcode_head[h]);
    }
    Mxfree(delcode_head);
  }
  // undump the delcode save stack
  undumpint(delcode_stacksize);
  undumpint(delcode_stackptr);
  delcode_stack = Mxmalloc_array(delcodestack,delcode_stacksize);
  for (h = 0; h <delcode_stackptr; h++) {
    undumpint(x); y.value = x;
    undumpint(x); y.code = x;
    delcode_stack[h] = y;
  }
  // undump the delcode heap
  undumpint(delcode_heapsize);
  undumpint(delcode_heapptr);
  delcode_heap = Mxmalloc_array(delcodeval,delcode_heapsize);
  for (h= 0; h<delcode_heapsize; h++) {
    undumpint(x); z.level = x;
    undumpint(x); z.valuea = x;
    undumpint(x); z.valueb = x;
    delcode_heap[h] = z;
  }
  // undump the delcode index table info
  delcode_head = (integer ***) Mxmalloc_array(void *,HIGHPART);
  for (h=0; h<HIGHPART;h++ ) {
    undumpint(x);
    if (x>0) {
      delcode_head[h]=(integer **)Mxmalloc_array(void *,MIDPART);
      for (m=0; m<MIDPART; m++ ) {
	undumpint(x);
	if (x>0) {
	  delcode_head[h][m]=Mxmalloc_array(integer,LOWPART);
	  for  (l=0; l<LOWPART; l++) {
	    undumpint(x);
	    delcode_head[h][m][l] = x;
	  }
	} else {
	  delcode_head[h][m] = NULL;
	}
      }
    } else {
      delcode_head[h]= NULL;
    }
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
