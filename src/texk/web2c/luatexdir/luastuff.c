/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"
#include <ptexlib.h>

static lua_State *Luas[65356];

void
make_table (lua_State *L, char *tab, char *getfunc, char*setfunc) {
  /* make the table */
  lua_pushstring(L,tab);
  lua_newtable(L);
  lua_settable(L, -3);
  /* fetch it back */
  lua_pushstring(L,tab);
  lua_gettable(L, -2); 
  /* make the meta entries */
  lua_pushstring(L, "__index");
  lua_pushstring(L, getfunc); 
  lua_gettable(L, -4); /* get tex.getdimen */
  lua_settable(L, -3); /* tex.dimen.__index = tex.getdimen */
  lua_pushstring(L, "__newindex");
  lua_pushstring(L, setfunc); 
  lua_gettable(L, -4); /* get tex.setdimen */
  lua_settable(L, -3); /* tex.dimen.__newindex = tex.setdimen */
  
  lua_pushvalue(L, -1);   /* copy the table */
  lua_setmetatable(L,-2); /* meta to itself */
  lua_pop(L,1);  /* clean the stack */
}

static 
const char *getS(lua_State * L, void *ud, size_t * size) {
    LoadS *ls = (LoadS *) ud;
    (void) L;
    if (ls->size == 0)
        return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}

void 
luacall(int n, int s) {
    LoadS ls;
    int i, j, k ;
    char err[] = "LuaTeX Error";
    if (Luas[n] == NULL) {
        Luas[n] = luaL_newstate();
	    luaL_openlibs(Luas[n]);
        luaopen_pdf(Luas[n]);
        luaopen_tex(Luas[n]);
    }
	luatex_init(s,&ls);
    i = lua_load(Luas[n], getS, &ls, "luacall");
    if (i != 0) {
	  luatex_error(err);
	  fprintf(stderr, "%s", lua_tostring(Luas[n], -1));
	  lua_close(Luas[n]);
	  Luas[n] = NULL;
	  return;
    }

    i = lua_pcall(Luas[n], 0, 0, 0);
    if (i != 0) {
	  luatex_error(err);
	  fprintf(stderr, "%s", lua_tostring(Luas[n], -1));
	  lua_close(Luas[n]);
	  Luas[n] = NULL;
    } else {
      luatex_return () ; /* does nothing */
    }	  
	return;
}

void 
luatex_init (int s, LoadS *ls) {
  ls->s = &(strpool[strstart[s]]);
  ls->size = strstart[s + 1] - strstart[s];
}

void 
luatex_return (void) {
  return;
}

void 
luatex_error (char *err) {
  int i,j;
  if (poolptr+strlen(err) < poolsize) {
	for (i = poolptr, j = 0; j < strlen(err); i++, j++)
	  strpool[i] = err[j];
	poolptr += strlen(err);
  }
}

