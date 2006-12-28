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

/* some pascal interfacing (underscore-glue) macros */

#define internal_font_number internalfontnumber
#define font_ptr fontptr
#define get_default_hyphen_char getdefaulthyphenchar
#define get_default_skew_char getdefaultskewchar
#define sprint_cs sprintcs
#define print_file_name printfilename
#define print_scaled printscaled
#define print_int printint


/* a bit more interfacing is needed for proper error reporting */

#define print_err(s) { doprinterr(maketexstring(s)); flushstr(last_tex_string); }
#define print_string(s) { print(maketexstring(s)); flushstr(last_tex_string); }

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

  if (ee)    flushstr(ee);
  if (dd)    flushstr(dd);
  if (cc)    flushstr(cc);
  if (bb)    flushstr(bb);
  if (aa)    flushstr(aa);
}

static void
start_font_error_message (pointer u, strnumber nom, strnumber aire, scaled s) {
  print_err("Font "); 
  sprint_cs(u);
  print_string("="); 
  print_file_name(nom,aire,getnullstr());
  if (s>=0 ) {
    print_string(" at "); print_scaled(s); print_string("pt");
  } else if (s!=-1000) {
    print_string(" scaled "); print_int(-s);
  }
}

int 
read_font_info(pointer u,  strnumber nom, strnumber aire, scaled s,
               integer natural_dir) {
  internal_font_number f; /* the new font's number */
  int success;
  char *cnom, *caire;
  boolean res; /* was the callback successful? */
  integer callback_id;
  res = 0;
  cnom  = makecstring(nom);
  caire = makecstring(aire);
  callback_id=callbackdefined("define_font");
  if (callback_id>0) {
    callback_id = runandsavecallback(callback_id,"SSd->",cnom,caire,s);
    if (callback_id>0) {
      luaL_checkstack(Luas[0],1,"out of stack space");
      lua_rawgeti(Luas[0],LUA_REGISTRYINDEX, callback_id);
      if (lua_istable(Luas[0],-1)) {
	f = new_font((font_ptr+1));
	res = font_from_lua(Luas[0],f);	
      }
      lua_pop(Luas[0],1);
    }
  } else {
    f = new_font((font_ptr+1));
    res = read_tfm_info(f,cnom,caire,s);
  }
  if (res) {
    set_font_natural_dir(f,natural_dir);  
    set_hyphen_char(f,get_default_hyphen_char());
    set_skew_char(f,get_default_skew_char());
    font_ptr++;
    return f;
  } else {
    start_font_error_message(u, nom, aire, s);
    char *help[] = {"I wasn't able to read the size data for this font,",
		    "so I will ignore the font specification.",
		    "[Wizards can fix TFM files using TFtoPL/PLtoTF.]",
		    "You might try inserting a different font spec;",
		    "e.g., type `I\font<same font id>=<substitute font name>'.",
		    NULL } ;
    do_error(" not loadable: Metric (TFM/OFM) file not found or bad",help);
    delete_font(f);
    return 0;
  }
}

