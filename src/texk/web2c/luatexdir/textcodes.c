
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

#define HIGHPART 68
#define MIDPART 128
#define LOWPART 128

#define Mxmalloc_array(a,b)  malloc(b*sizeof(a))
#define Mxrealloc_array(a,b,c)  realloc(a,c*sizeof(b))
#define Mxfree(a) (a)

#define INIT_HIGHPART(a,b)   if (a == NULL) {	   \
    a = (b ***) Mxmalloc_array(void *,HIGHPART);   \
    for  (i=0; i<HIGHPART; i++) { a[i] = NULL; }  }

#define INIT_MIDPART(a,b)   if (a == NULL) {	   \
    a = (b **) Mxmalloc_array(void *,MIDPART);     \
    for  (i=0; i<MIDPART; i++) { a[i] = NULL; }  }

#define INIT_LOWPART(a,b)   if (a == NULL) {	    \
    a = Mxmalloc_array(b,LOWPART);                  \
    for  (i=0; i<LOWPART; i++) { a[i] = 0; }  }

#define CLEARTREE(a)   if (a != NULL) {              \
  for (h=0; h<HIGHPART;h++ ) {                       \
    if (a[h] != NULL) {                              \
      for (m=0; m<MIDPART; m++ ) {                   \
	if (a[h][m] != NULL) {  Mxfree(a[h][m]); }}} \
      Mxfree(a[h]);    }                             \
    Mxfree(a);  }

#define UNDUMPTREE(a)   a = (textcode_val ***) Mxmalloc_array(void *,HIGHPART); \
  for (h=0; h<HIGHPART;h++ ) {  undumpqqqq(f);  if (f>0)                        \
   { a[h]=(textcode_val **)Mxmalloc_array(void *,MIDPART);                      \
     for (m=0; m<MIDPART; m++ )  { undumpqqqq(f);                               \
        if (f>0) { a[h][m]=Mxmalloc_array(textcode_val,LOWPART);                \
	  for (l=0; l<LOWPART; l++)  { undumpint(x);  a[h][m][l] = x; } }       \
        else { a[h][m] = NULL; }   }  } else { a[h]= NULL; } }


#define DUMPTREE(a)  for (h=0; h<HIGHPART;h++ ) { \
    if (a[h] != NULL) {                           \
     f = 1;  dumpqqqq(f);                         \
      for (m=0; m<MIDPART; m++ ) {                \
	if (a[h][m] != NULL) {                    \
	  f = 1;  dumpqqqq(f);                    \
	  for (l=0;l<LOWPART;l++) {               \
	    x = a[h][m][l];  dumpint(x);  }       \
	} else { f = 0;  dumpqqqq(f);} }          \
    } else { f = 0;  dumpqqqq(f);  } }


#define textcode_value(a) (a % 0x1FFFFF)
#define textcode_level(a) (a / 0x1FFFFF)
#define level_plus_value(a,b) (a * 0x1FFFFF + b)

typedef unsigned int  textcode_val;

typedef struct {
  textcode_val value;
  integer      code;
} textcodestack;

typedef textcodestack lccodestack;

#define LCCODESTACK  64

static lccodestack    * lccode_stack     = NULL;
static int              lccode_stacksize = LCCODESTACK;
static int              lccode_stackptr  = 0;
static textcode_val *** lccode_head      = NULL;

#define UCCODESTACK  64

typedef textcodestack uccodestack;

static uccodestack    * uccode_stack     = NULL;
static int              uccode_stacksize = UCCODESTACK;
static int              uccode_stackptr  = 0;
static textcode_val *** uccode_head      = NULL;


#define SFCODESTACK 32

typedef textcodestack sfcodestack;

static sfcodestack    * sfcode_stack     = NULL;
static int              sfcode_stacksize = SFCODESTACK;
static int              sfcode_stackptr  = 0;
static textcode_val *** sfcode_head      = NULL;

#define CATCODESTACK 64

typedef textcodestack catcodestack;

static catcodestack   * catcode_stack     = NULL;
static int              catcode_stacksize = CATCODESTACK;
static int              catcode_stackptr  = 0;
static textcode_val *** catcode_head      = NULL;


static void
texprintf (const char *format, ... ) {
  char message[256];
  strnumber mystr;
  va_list ap;
  va_start(ap,format);
  vsnprintf(message,255,format,ap);
  va_end(ap);
  if (0)
	puts(message);
}

static void 
savelccode (integer n, textcode_val v, quarterword grouplevel) {
  lccodestack st;
  st.value = v;
  st.code  = level_plus_value(grouplevel,n);
  if ((lccode_stackptr+1)==lccode_stacksize) {
    lccode_stacksize += LCCODESTACK;
    lccode_stack = Mxrealloc_array(lccode_stack,lccodestack,lccode_stacksize);
  }
  lccode_stack[lccode_stackptr++] = st;
}

void 
setlccode (integer n, halfword v, quarterword grouplevel) {
  unsigned char h,m,l;
  int i;
  textcode_val ii;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(lccode_head,textcode_val);
  INIT_MIDPART(lccode_head[h],textcode_val);
  INIT_LOWPART(lccode_head[h][m],textcode_val);
  ii = lccode_head[h][m][l];
  if (ii == 0) {
    savelccode(n,level_plus_value(1,0), grouplevel);
  } else if (textcode_level(ii) <= grouplevel && textcode_value(ii) == v) {
    return;
  } else if (grouplevel != 1 && textcode_level(ii) != grouplevel) {
    savelccode(n,ii,grouplevel);
  }
  lccode_head[h][m][l] = level_plus_value(grouplevel,v);
  return;
}

halfword
getlccode (integer n) {
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (lccode_head != NULL && lccode_head[h] != NULL &&
      lccode_head[h][m] != NULL && lccode_head[h][m][l]>0) {
    return (halfword)textcode_value(lccode_head[h][m][l]);
  }
  /* \lccode n=0 is the default */
  return 0;
}

static void
unsavelccodes (quarterword grouplevel) {
  lccodestack st;
  unsigned char h,m,l;
  integer n;
  st = lccode_stack[(lccode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
    n = textcode_value(st.code);
    h = n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    lccode_head[h][m][l] = st.value;
	lccode_stackptr--;
	st = lccode_stack[(lccode_stackptr-1)];
  }
}

static void
initializelccodes (void) {
  int i;
  if (lccode_head==NULL) {
    lccode_head = (textcode_val ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      lccode_head[i] = NULL;
    }
  }
  if (lccode_stack == NULL) {
    lccode_stack = Mxmalloc_array(lccodestack,lccode_stacksize);
  }
  return;
}

static void
dumplccodes (void) {
  int h,m,l;
  unsigned int x;
  lccodestack y;
  boolean f;
  // dump the lccode stack
  dumpint(lccode_stacksize);
  dumpint(lccode_stackptr);
  for (x= 0; x<lccode_stackptr; x++) {
    y = lccode_stack[x];
    dumpint(y.code);
    dumpint(y.value);
  }
  // dump the lccode index table info
  DUMPTREE(lccode_head);
}

static void 
undumplccodes (void) {
  int h,m,l;
  unsigned int x;
  lccodestack y;
  boolean f;
  // destroy current stuff
  if (lccode_stack != NULL)
    Mxfree(lccode_stack);
  CLEARTREE(lccode_head);
  // undump the lccode save stack
  undumpint(lccode_stacksize);
  undumpint(lccode_stackptr);
  lccode_stack = Mxmalloc_array(lccodestack,lccode_stacksize);
  for (h = 0; h <lccode_stackptr; h++) {
    undumpint(x); y.code = x;
    undumpint(x); y.value = x;
    lccode_stack[h] = y;
  }
  // undump the lccode index table info
  UNDUMPTREE(lccode_head);
}


static void 
saveuccode (integer n, textcode_val v, quarterword grouplevel) {
  uccodestack st;
  st.value  = v;
  st.code   =level_plus_value(grouplevel,n);
  if ((uccode_stackptr+1)==uccode_stacksize) {
    uccode_stacksize += UCCODESTACK;
    uccode_stack = Mxrealloc_array(uccode_stack,uccodestack,uccode_stacksize);
  }
  uccode_stack[uccode_stackptr++] = st;
}

void 
setuccode (integer n, halfword v, quarterword grouplevel) {
  unsigned char h,m,l;
  int i;
  textcode_val ii;
  h =  n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(uccode_head,textcode_val);
  INIT_MIDPART(uccode_head[h],textcode_val);
  INIT_LOWPART(uccode_head[h][m],textcode_val);
  ii = uccode_head[h][m][l];
  if (ii == 0) {
    saveuccode(n,level_plus_value(1,0),grouplevel);
  } else if (textcode_level(ii) <= grouplevel &&  textcode_value(ii) == v) {
    return;
  } else if (grouplevel != 1 && textcode_level(ii) != grouplevel) {
    saveuccode(n,ii,grouplevel);
  }
  uccode_head[h][m][l] = level_plus_value(grouplevel,v);
  return;
}

halfword
getuccode (integer n) {
  unsigned char h,m,l;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (uccode_head != NULL &&  uccode_head[h] != NULL &&
      uccode_head[h][m] != NULL && uccode_head[h][m][l]>0) {
    return (halfword)textcode_value(uccode_head[h][m][l]);
  }
  /* \uccode n=0 is the default */
  return 0;
}

static void
unsaveuccodes   (quarterword grouplevel) {
  uccodestack st;
  unsigned char h,m,l;
  integer n;
  st = uccode_stack[(uccode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
	n = textcode_value(st.code);
    h =  n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    uccode_head[h][m][l] = st.value;
	uccode_stackptr--;
    st = uccode_stack[(uccode_stackptr-1)];
  }
}

static void
initializeuccodes (void) {
  int i;
  if (uccode_head==NULL) {
    uccode_head = (textcode_val ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      uccode_head[i] = NULL;
    }
  }
  if (uccode_stack == NULL) {
    uccode_stack = Mxmalloc_array(uccodestack,uccode_stacksize);
  }
  return;
}

static void
dumpuccodes (void) {
  int h,m,l;
  unsigned int x;
  uccodestack y;
  boolean f;
  // dump the uccode stack
  dumpint(uccode_stacksize);
  dumpint(uccode_stackptr);
  for (x= 0; x<uccode_stackptr; x++) {
    y = uccode_stack[x];
    dumpint(y.code);
    dumpint(y.value);
  }
  // dump the uccode index table info
  DUMPTREE(uccode_head);
}

static void 
undumpuccodes (void) {
  int h,m,l;
  unsigned int x;
  uccodestack y;
  boolean f;
  // destroy current stuff
  if (uccode_stack != NULL)
    Mxfree(uccode_stack);
  CLEARTREE(uccode_head);
  // undump the uccode save stack
  undumpint(uccode_stacksize);
  undumpint(uccode_stackptr);
  uccode_stack = Mxmalloc_array(uccodestack,uccode_stacksize);
  for (h = 0; h <uccode_stackptr; h++) {
    undumpint(x); y.code = x;
    undumpint(x); y.value = x;
    uccode_stack[h] = y;
  }
  // undump the uccode index table info
  UNDUMPTREE(uccode_head);
}

static void 
savesfcode (integer n, textcode_val v, quarterword grouplevel) {
  sfcodestack st;
  st.value = v;
  st.code  = level_plus_value(grouplevel,n);
  if ((sfcode_stackptr+1)==sfcode_stacksize) {
    sfcode_stacksize += SFCODESTACK;
    sfcode_stack = Mxrealloc_array(sfcode_stack,sfcodestack,sfcode_stacksize);
  }
  sfcode_stack[sfcode_stackptr++] = st;
}

void 
setsfcode (integer n, halfword v, quarterword grouplevel) {
  unsigned char h,m,l;
  int i;
  textcode_val ii;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(sfcode_head,textcode_val);
  INIT_MIDPART(sfcode_head[h],textcode_val);
  INIT_LOWPART(sfcode_head[h][m],textcode_val);
  ii = sfcode_head[h][m][l];
  if (ii == 0) {
    /* \sfcode n=1000 is the default */
    savesfcode(n,level_plus_value(1,1000),grouplevel);
  } else if (textcode_level(ii) <= grouplevel && textcode_value(ii) == v) {
    return;
  } else if (grouplevel != 1 && textcode_level(ii) != grouplevel) {
    savesfcode(n,ii,grouplevel);
  }
  sfcode_head[h][m][l] = level_plus_value(grouplevel,v);
  return;
}

halfword
getsfcode (integer n) {
  unsigned char h,m,l;
  h =  n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (sfcode_head != NULL && sfcode_head[h] != NULL &&
      sfcode_head[h][m] != NULL && sfcode_head[h][m][l]>0) {
    return (halfword)textcode_value(sfcode_head[h][m][l]);
  }
  /* \sfcode n=1000 is the default */
  return 1000;
}

static void
unsavesfcodes   (quarterword grouplevel) {
  sfcodestack st;
  unsigned char h,m,l;
  integer n;
  st = sfcode_stack[(sfcode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
	n = textcode_value(st.code);
    h =  n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    sfcode_head[h][m][l] = st.value;
	sfcode_stackptr--;
    st = sfcode_stack[(sfcode_stackptr-1)];
  }
}

static void
initializesfcodes (void) {
  int i;
  if (sfcode_head==NULL) {
    sfcode_head = (textcode_val ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      sfcode_head[i] = NULL;
    }
  }
  if (sfcode_stack == NULL) {
    sfcode_stack = Mxmalloc_array(sfcodestack,sfcode_stacksize);
  }
  return;
}

static void
dumpsfcodes (void) {
  int h,m,l;
  unsigned int x;
  sfcodestack y;
  boolean f;
  // dump the sfcode stack
  dumpint(sfcode_stacksize);
  dumpint(sfcode_stackptr);
  for (x= 0; x<sfcode_stackptr; x++) {
    y = sfcode_stack[x];
    dumpint(y.code);
    dumpint(y.value);
  }
  // dump the sfcode index table info
  DUMPTREE(sfcode_head);
}

static void 
undumpsfcodes (void) {
  int h,m,l;
  unsigned int x;
  sfcodestack y;
  boolean f;
  // destroy current stuff
  if (sfcode_stack != NULL)
    Mxfree(sfcode_stack);
  CLEARTREE(sfcode_head);
  // undump the sfcode save stack
  undumpint(sfcode_stacksize);
  undumpint(sfcode_stackptr);
  sfcode_stack = Mxmalloc_array(sfcodestack,sfcode_stacksize);
  for (h = 0; h <sfcode_stackptr; h++) {
    undumpint(x); y.code = x;
    undumpint(x); y.value = x;
    sfcode_stack[h] = y;
  }
  // undump the sfcode index table info
  UNDUMPTREE(sfcode_head);
}

static void 
savecatcode (textcode_val n, textcode_val v, quarterword grouplevel) {
  catcodestack st;
  st.value  = v;
  st.code   = level_plus_value(grouplevel,n);
  if ((catcode_stackptr+1)==catcode_stacksize) {
    catcode_stacksize += CATCODESTACK;
    catcode_stack = Mxrealloc_array(catcode_stack,catcodestack,catcode_stacksize);
  }
  catcode_stack[catcode_stackptr++] = st;
}

void 
setcatcode (integer n, halfword v, quarterword grouplevel) {
  unsigned char h,m,l;
  int i;
  textcode_val ii;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  INIT_HIGHPART(catcode_head,textcode_val);
  INIT_MIDPART(catcode_head[h],textcode_val);
  INIT_LOWPART(catcode_head[h][m],textcode_val);
  ii = catcode_head[h][m][l];
  if (ii == 0) {
    /* \catcode n=12 is the default */
	texprintf("{changing \\catcode %d=12}",n);
    savecatcode(n,level_plus_value(1,12),grouplevel);
  } else {
    if (textcode_level(ii) <= grouplevel &&  textcode_value(ii) == v) {
      texprintf("{reassigning \\catcode%d=%d}",n,textcode_value(ii));
      return;
    } else if (grouplevel != 1 && textcode_level(ii) != grouplevel) {
      texprintf("{changing \\catcode%d=%d}",n,textcode_value(ii));
      savecatcode(n,ii,grouplevel);
    }
  }
  texprintf("{into \\catcode%d=%d}",n,v);
  catcode_head[h][m][l] = level_plus_value(grouplevel,v);
  return;
}

halfword
getcatcode (integer n) {
  unsigned char h,m,l;
  halfword ret;
  h = n / (MIDPART*LOWPART);
  m = (n % (MIDPART*LOWPART)) / MIDPART;
  l = (n % (MIDPART*LOWPART)) % MIDPART;
  if (catcode_head != NULL &&  catcode_head[h] != NULL &&
      catcode_head[h][m] != NULL && catcode_head[h][m][l]>0) {
    ret = textcode_value(catcode_head[h][m][l]);
    texprintf("{\\catcode%d=%d}",n,ret);
    return ret;
  }
  /* \catcode n=12 is the default */
  texprintf("{\\catcode %d=12}",n);
  return 12;
}

static void
unsavecatcodes   (quarterword grouplevel) {
  catcodestack st;
  unsigned char h,m,l;
  int n;
  st = catcode_stack[(catcode_stackptr-1)];
  while (textcode_level(st.code) == grouplevel) {
	n = textcode_value(st.code);
    h = n / (MIDPART*LOWPART);
    m = (n % (MIDPART*LOWPART)) / MIDPART;
    l = (n % (MIDPART*LOWPART)) % MIDPART;
    catcode_head[h][m][l] = st.value;
    texprintf ("{restoring \\catcode%d=%d}\n",n,textcode_value(st.value));
	catcode_stackptr-- ;
	st = catcode_stack[(catcode_stackptr-1)];
  }
}

static void
initializecatcodes (void) {
  int i;
  if (catcode_head==NULL) {
    catcode_head = (textcode_val ***) Mxmalloc_array(void *,HIGHPART);
    for (i=0; i<HIGHPART; i++) {
      catcode_head[i] = NULL;
    }
  }
  if (catcode_stack == NULL) {
    catcode_stack = Mxmalloc_array(catcodestack,catcode_stacksize);
  }
  return;
}

static void
dumpcatcodes (void) {
  int h,m,l;
  unsigned int x;
  catcodestack y;
  boolean f;
  // dump the catcode stack
  dumpint(catcode_stacksize);
  dumpint(catcode_stackptr);
  for (x= 0; x<catcode_stackptr; x++) {
    y = catcode_stack[x];
    dumpint(y.code);
    dumpint(y.value);
  }
  // dump the catcode index table info
  DUMPTREE(catcode_head);
}

static void 
undumpcatcodes (void) {
  int h,m,l;
  unsigned int x;
  catcodestack y;
  boolean f;
  // undump the catcode save stack
  if (catcode_stack != NULL)
    Mxfree(catcode_stack);
  undumpint(catcode_stacksize);
  undumpint(catcode_stackptr);
  catcode_stack = Mxmalloc_array(catcodestack,catcode_stacksize);
  for (h = 0; h <catcode_stackptr; h++) {
    undumpint(x); y.code = x;
    undumpint(x); y.value = x;
    catcode_stack[h] = y;
  }
  // undump the catcode index table info
  CLEARTREE(catcode_head);
  UNDUMPTREE(catcode_head);
}

void 
unsavetextcodes (quarterword grouplevel) {
  unsavesfcodes(grouplevel);
  unsaveuccodes(grouplevel);
  unsavelccodes(grouplevel);
  unsavecatcodes(grouplevel);
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
  dumpcatcodes();
  dumplccodes();
}

void 
undumptextcodes (void) {
  undumpsfcodes();
  undumpuccodes();
  undumpcatcodes();
  undumplccodes();
}

