/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

typedef void (*texio_printer) (strnumber s);

typedef enum {
  no_print=16,
  term_only=17,
  log_only=18,
  term_and_log=19,
  pseudo=20,
  new_string=21 } selector_settings;

static int
do_texio_print (lua_State *L, texio_printer printfunction) {
  strnumber s,u;
  char *outputstr;
  char save_selector;
  int n;
  u = 0;
  n = lua_gettop(L);
  if (!lua_isstring(L, -1)) {
    lua_pushstring(L, "no string to print");
    lua_error(L);
  }
  if (n==2) {
    if (!lua_isstring(L, -2)) {
      lua_pushstring(L, "invalid argument for print selector");
      lua_error(L);
    } else {
      save_selector = selector;  
      outputstr=lua_tostring(L, -2);
      if      (strcmp(outputstr,"log") == 0)          { selector = log_only;     }
      else if (strcmp(outputstr,"term") == 0)         { selector = term_only;    }
      else if (strcmp(outputstr,"term and log") == 0) {	selector = term_and_log; }
      else {
	lua_pushstring(L, "invalid argument for print selector");
	lua_error(L);
      }
    }
  } else {
    if (n!=1) {
      lua_pushstring(L, "invalid number of arguments");
      lua_error(L);
    }
    save_selector = selector;
    if (selector!=log_only &&
	selector!=term_only &&
	selector != term_and_log) {
      normalizeselector(); /* sets selector */
    }
  }
  if (strstart[strptr-0x200000]<poolptr) 
    u=makestring();
  s = maketexstring(lua_tostring(L, -1));
  printfunction(s);
  flushstr(s);
  selector = save_selector;
  if (u!=0) strptr--;
  return 0; 
}

static int
texio_print (lua_State *L) {
  return do_texio_print(L,zprint);
}

static int
texio_printnl (lua_State *L) {
  return do_texio_print(L,zprintnl);
}

static const struct luaL_reg texiolib [] = {
  {"write", texio_print},
  {"write_nl", texio_printnl},
  {NULL, NULL}  /* sentinel */
};

int
luaopen_texio (lua_State *L) {
  luaL_register(L,"texio",texiolib);
  return 1;
}
