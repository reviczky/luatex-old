/* $Id */

#include "luatex-api.h"
#include <ptexlib.h>

integer getcurv AA((void));
integer getcurh AA((void));

int findcurv (lua_State *L) {
  int j;
  j = getcurv();
  lua_pushnumber(L, j);
  return 1;
}

int findcurh (lua_State *L) {
  int j;
  j = getcurh();
  lua_pushnumber(L, j);
  return 1;
}

int makecurv (lua_State *L) {
  lua_pop(L,1); /* table at -1 */
  return 0;
}

int makecurh (lua_State *L) {
  lua_pop(L,1); /* table at -1 */
  return 0;
}




static const struct luaL_reg pdflib [] = {
  {"getv", findcurv},
  {"geth", findcurh},
  {"setv", makecurv},
  {"seth", makecurh},
  {NULL, NULL}  /* sentinel */
};

int luaopen_pdf (lua_State *L) 
{
  luaL_openlib(L, "pdf", pdflib, 0);
  make_table(L,"v","getv","setv");
  make_table(L,"h","geth","seth");
  return 1;
}

