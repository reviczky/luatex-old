/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


int
texio_print (lua_State *L) {
  int i, len;
  const char *st;
  strnumber s;
  if (!lua_isstring(L, -1)) {
    lua_pushstring(L, "no string to print");
    lua_error(L);
  }
  st = lua_tostring(L, -1);
  s = maketexstring(st);
  zprint (s);
  flushstr(s);
  return 0; 
}

int
texio_printnl (lua_State *L) {
  int i, len;
  const char *st;
  strnumber s;
  if (!lua_isstring(L, -1)) {
    lua_pushstring(L, "no string to print");
    lua_error(L);
  }
  st = lua_tostring(L, -1);
  s = maketexstring(st);
  zprintnl (s);
  flushstr(s);
  return 0; 
}


static const struct luaL_reg texiolib [] = {
  {"write", texio_print},
  {"write_nl", texio_printnl},
  {NULL, NULL}  /* sentinel */
};


int
luaopen_texio (lua_State *L) 
{
  luaL_register(L,"texio",texiolib);
  return 1;
}
