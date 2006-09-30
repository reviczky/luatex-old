/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

static strnumber 
prep_string (lua_State *L) {
  if (!lua_isstring(L, -1)) {
    lua_pushstring(L, "no string to print");
    lua_error(L);
    return 0;
  }
  return maketexstring(lua_tostring(L, -1));
}

static int
texio_print (lua_State *L) {
  strnumber s = prep_string(L);
  zprint (s);
  flushstr(s);
  return 0; 
}

static int
texio_printnl (lua_State *L) {
  strnumber s = prep_string(L);
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
luaopen_texio (lua_State *L) {
  luaL_register(L,"texio",texiolib);
  return 1;
}
