/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


static int 
font_read_tfm (lua_State *L) {
  internalfontnumber f;
  scaled s;
  int k;
  char *cnom;
  if(lua_isstring(L, 1)) {
    cnom = (char *)lua_tostring(L, 1);
    if(lua_isnumber(L, 2)) {
      s = (integer)lua_tonumber(L,2);
      f = new_font(fontptr+1);
      if (read_tfm_info(f, cnom, "", s)) {
	k =  font_to_lua(L,f,cnom);
	delete_font(f);
	return k;
      } else {
	lua_pushstring(L, "font loading failed");
	lua_error(L);
      }
    } else {
      lua_pushstring(L, "expected an integer size as second argument");
      lua_error(L);
    }
  } else {
    lua_pushstring(L, "expected tfm name as first argument");
    lua_error(L);
  }
}


static const struct luaL_reg fontlib [] = {
  {"read_tfm", font_read_tfm},
  {NULL, NULL}  /* sentinel */
};

int luaopen_font (lua_State *L) 
{
  luaL_register(L, "font", fontlib);
  return 1;
}
