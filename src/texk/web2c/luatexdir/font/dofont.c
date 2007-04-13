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

#define TIMERS 0

#if TIMERS
#include <sys/time.h>
#endif


/* a bit more interfacing is needed for proper error reporting */

#define print_err(s) { do_print_err(maketexstring(s)); flush_str(last_tex_string); }

void do_error(char *msg, char **hlp) {
  strnumber msgmsg = 0,aa = 0,bb = 0,cc = 0,dd = 0,ee = 0;
  int k = 0;
  while (hlp[k]!=NULL)
    k++;
  if (k>0)    aa =maketexstring(hlp[0]);
  if (k>1)    bb =maketexstring(hlp[1]);
  if (k>2)    cc =maketexstring(hlp[2]);
  if (k>3)    dd =maketexstring(hlp[3]);
  if (k>4)    ee =maketexstring(hlp[4]);

  print_string(msg);
  switch (k) {
  case 5:   dohelp5(aa,bb,cc,dd,ee); break;
  }
  error();

  if (ee)    flush_str(ee);
  if (dd)    flush_str(dd);
  if (cc)    flush_str(cc);
  if (bb)    flush_str(bb);
  if (aa)    flush_str(aa);
}

static void
start_font_error_message (pointer u, strnumber nom, strnumber aire, scaled s) {
  print_err("Font "); 
  sprint_cs(u);
  print_string("="); 
  print_file_name(nom,aire,get_nullstr());
  if (s>=0 ) {
    print_string(" at "); print_scaled(s); print_string("pt");
  } else if (s!=-1000) {
    print_string(" scaled "); print_int(-s);
  }
}

static int
do_define_font (integer f, char *cnom, char *caire, scaled s, integer natural_dir) {
  int success;
  boolean res; /* was the callback successful? */
  integer callback_id;
  char *cnam;
#if TIMERS
    struct timeval tva;
    struct timeval tvb;
    double tvdiff;
#endif
  int r;
  res = 0;

  callback_id=callback_defined(define_font_callback);
  if (callback_id>0) {
    if (caire == NULL || strlen(caire)==0) {
      cnam = cnom;
    } else {
      cnam = xmalloc(strlen(cnom)+strlen(caire)+2);
      sprintf(cnam,"%s/%s",caire,cnom);
      free(caire);
      free(cnom);
    }
#if TIMERS
	gettimeofday(&tva,NULL);
#endif
    callback_id = run_and_save_callback(callback_id,"Sdd->",cnam,s,f);
#if TIMERS
	gettimeofday(&tvb,NULL);
	tvdiff = tvb.tv_sec*1000000.0;
	tvdiff += (double)tvb.tv_usec;
	tvdiff -= (tva.tv_sec*1000000.0);
	tvdiff -= (double)tva.tv_usec;
	tvdiff /= 1000000;
	fprintf(stdout,"\ncallback('define_font',%s,%i): %f seconds\n", cnam,f,tvdiff);
#endif
    free(cnam);
    if (callback_id>0) { /* success */
      luaL_checkstack(Luas[0],1,"out of stack space");
      lua_rawgeti(Luas[0],LUA_REGISTRYINDEX, callback_id);
      if (lua_istable(Luas[0],-1)) {
#if TIMERS
	gettimeofday(&tva,NULL);
#endif
	res = font_from_lua(Luas[0],f);	
	destroy_saved_callback (callback_id);
#if TIMERS
	gettimeofday(&tvb,NULL);
	tvdiff = tvb.tv_sec*1000000.0;
	tvdiff += (double)tvb.tv_usec;
	tvdiff -= (tva.tv_sec*1000000.0);
	tvdiff -= (double)tva.tv_usec;
	tvdiff /= 1000000;
	fprintf(stdout,"font_from_lua(%s,%i): %f seconds\n", font_name(f),f,tvdiff);
#endif
	lua_pop(Luas[0],1);
      } else if (lua_isnumber(Luas[0],-1)) {
	r = lua_tonumber(Luas[0],-1);	
	destroy_saved_callback (callback_id);
	delete_font(f);
	lua_pop(Luas[0],1);
	return r;
      } else {
	lua_pop(Luas[0],1);
	delete_font(f);
	return 0;
      }
    }
  } else {
    res = read_tfm_info(f,cnom,caire,s);
    if (res) {
      set_hyphen_char(f,get_default_hyphen_char());
      set_skew_char(f,get_default_skew_char());
    }
  }
  if (res) {
    do_vf(f);
    set_font_natural_dir(f,natural_dir);
    return f;
  } else {
    delete_font(f);
    return 0;
  }

}

int 
read_font_info(pointer u,  strnumber nom, strnumber aire, scaled s,
               integer natural_dir) {
  integer f;
  char *cnom, *caire = NULL;
  cnom  = xstrdup(makecstring(nom));
  if (aire != 0) 
    caire = makecstring(aire);

  f = new_font();
  if ((f = do_define_font(f, cnom,caire,s,natural_dir))) {
    return f;
  } else {
    char *help[] = {"I wasn't able to read the size data for this font,",
		    "so I will ignore the font specification.",
		    "[Wizards can fix TFM files using TFtoPL/PLtoTF.]",
		    "You might try inserting a different font spec;",
		    "e.g., type `I\font<same font id>=<substitute font name>'.",
		    NULL } ;
    start_font_error_message(u, nom, aire, s);
    do_error(" not loadable: Metric (TFM/OFM) file not found or bad",help);
    return 0;
  }
}

/* TODO This function is a placeholder. There can easily appears holes in 
   the |font_tables| array, and we could attempt to reuse those
*/

int 
find_font_id (char *nom, char *aire, scaled s) {
  integer f;
  f = new_font();
  if ((f = do_define_font(f, xstrdup(nom),xstrdup(aire),s,-1))) {
    return f;
  } else {
    return 0;
  }
}

