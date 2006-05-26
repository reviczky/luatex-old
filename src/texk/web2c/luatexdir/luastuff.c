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
  char *lua_id;
  lua_id = (char *)xmalloc(12);
  if (Luas[n] == NULL) {
	Luas[n] = luaL_newstate();
	luaL_openlibs(Luas[n]);
	luaopen_pdf(Luas[n]);
	luaopen_tex(Luas[n]);
  }
  luatex_load_init(s,&ls);
  snprintf(lua_id,12,"luas[%d]",n);
  i = lua_load(Luas[n], getS, &ls, lua_id);
  if (i != 0) {
	Luas[n] = luatex_error(Luas[n],(i == LUA_ERRSYNTAX ? 0 : 1));
  } else {
	i = lua_pcall(Luas[n], 0, 0, 0);
	if (i != 0) {
	  Luas[n] = luatex_error(Luas[n],(i == LUA_ERRRUN ? 0 : 1));
	}	 
  }
}

void 
closelua(int n) {
  if (Luas[n] != NULL) {
    lua_close(Luas[n]);
	fprintf(stderr,"closing lua %d\n",n);
    Luas[n] = NULL;
  }
}


void 
luatex_load_init (int s, LoadS *ls) {
  ls->s = (const char *)&(strpool[strstart[s]]);
  ls->size = strstart[s + 1] - strstart[s];
}

lua_State *
luatex_error (lua_State * L, int is_fatal) {
  int i,j;
  size_t len;
  char *err;
  poolpointer b;
  const char *luaerr = lua_tostring(L, -1);
  err = (char *)xmalloc(128);
  len = snprintf(err,128,"%s",luaerr);
  if (is_fatal>0) {
	luafatalerror(maketexstring(err));
	/* never reached */
	xfree (err);
	lua_close(L);
	return (lua_State *)NULL;
  } else {
	/* running maketexstring() at this point is a bit tricky
      because we also perform our I/O though the pool: we have
      to do a rollback immediately after the error */
	b = poolptr;
	luanormerror(maketexstring(err));
	strptr--; poolptr=b;
	xfree (err);
	return L;
  }
}

