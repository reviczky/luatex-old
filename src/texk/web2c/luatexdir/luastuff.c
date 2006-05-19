/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"


static const struct luaL_reg texlib [] = {
  {"print", poolprint},
  {"setdimen", setdimen},
  {"getdimen", getdimen},
  {"setcount", setcount},
  {"getcount", getcount},
  {"settoks",  settoks},
  {"gettoks",  gettoks},
  {"setboxwd", setboxwd},
  {"getboxwd", getboxwd},
  {"setboxht", setboxht},
  {"getboxht", getboxht},
  {"setboxdp", setboxdp},
  {"getboxdp", getboxdp},
  {NULL, NULL}  /* sentinel */
};

void make_table (lua_State *L, char *tab, char *getfunc, char*setfunc)
{
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

int luaopen_tex (lua_State *L) 
{
  luaL_openlib(L, "tex", texlib, 0);
  make_table(L,"dimen","getdimen","setdimen");
  make_table(L,"count","getcount","setcount");
  make_table(L,"toks","gettoks","settoks");
  make_table(L,"wd","getboxwd","setboxwd");
  make_table(L,"ht","getboxht","setboxht");
  make_table(L,"dp","getboxdp","setboxdp");
  return 1;
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

static const char *getS(lua_State * L, void *ud, size_t * size)
{
    LoadS *ls = (LoadS *) ud;
    (void) L;
    if (ls->size == 0)
        return NULL;
    *size = ls->size;
    ls->size = 0;
    return ls->s;
}

static lua_State *L = NULL;

void luacall(int s)
{
    LoadS ls;
    int i, j, k ;
    char err[] = "LuaTeX Error";

    if (L == NULL) {
        L = luaL_newstate();
	    luaL_openlibs(L);
        luaopen_pdf(L);
        luaopen_tex(L);
		buf = malloc(BUFSIZE);
		if (buf == NULL) {
		  exit (1);
		}
    }
	luatex_init(s,&ls);
    i = lua_load(L, getS, &ls, "luacall");
    if (i != 0) {
	  luatex_error(err);
	  fprintf(stderr, "%s", lua_tostring(L, -1));
	  lua_close(L);
	  L = NULL;
	  free(buf);
	  return;
    }

    bufloc = 0;
    i = lua_pcall(L, 0, 0, 0);
    if (i != 0) {
	  luatex_error(err);
	  fprintf(stderr, "%s", lua_tostring(L, -1));
	  lua_close(L);
	  L = NULL;
	  free(buf);
    } else {
	  luatex_return();
	}	  
	return;
}
