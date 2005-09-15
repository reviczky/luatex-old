/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include <stdlib.h>
#include <stdio.h>
#include <../lua51/lua.h>
#include <../lua51/lauxlib.h>
#include <../lua51/lualib.h>
#include <ptexlib.h>

#define BUFSIZE (4096*256)

char *buf;

/* this is because ptexlib.h reads pdftexcoerce.h instead of 
   pdfetexcoerce.h */

scaled zgettexdimenregister AA((integer j));
#define gettexdimenregister(j) zgettexdimenregister((integer) (j))
integer zsettexdimenregister AA((integer j,scaled v));
#define settexdimenregister(j, v) zsettexdimenregister((integer) (j), (scaled) (v))

scaled zgettexcountregister AA((integer j));
#define gettexcountregister(j) zgettexcountregister((integer) (j))
integer zsettexcountregister AA((integer j,scaled v));
#define settexcountregister(j, v) zsettexcountregister((integer) (j), (scaled) (v))

strnumber zgettextoksregister AA((integer j));
#define gettextoksregister(j) zgettextoksregister((integer) (j))
integer zsettextoksregister AA((integer j,strnumber s));
#define settextoksregister(j, s) zsettextoksregister((integer) (j), (strnumber) (s))

scaled zgettexboxwidth AA((integer j));
#define gettexboxwidth(j) zgettexboxwidth((integer) (j))
integer zsettexboxwidth AA((integer j,scaled v));
#define settexboxwidth(j, v) zsettexboxwidth((integer) (j), (scaled) (v))

scaled zgettexboxheight AA((integer j));
#define gettexboxheight(j) zgettexboxheight((integer) (j))
integer zsettexboxheight AA((integer j,scaled v));
#define settexboxheight(j, v) zsettexboxheight((integer) (j), (scaled) (v))

scaled zgettexboxdepth AA((integer j));
#define gettexboxdepth(j) zgettexboxdepth((integer) (j))
integer zsettexboxdepth AA((integer j,scaled v));
#define settexboxdepth(j, v) zsettexboxdepth((integer) (j), (scaled) (v))

integer getcurv AA((void));
integer getcurh AA((void));

int bufloc;

static int poolprint(lua_State * L)
{
    int i, j, k, n, len;
    const char *st;
    n = lua_gettop(L);
    for (i = 1; i <= n; i++) {
      if (!lua_isstring(L, i)) {
        lua_pushstring(L, "LuaTeX Error: no string to print");
        lua_error(L);
      }
      st = lua_tostring(L, i);
      len = lua_strlen(L, i);
      if (len) {
        for (j = 0, k = bufloc; j < len; j++, k++) {
          if (k < BUFSIZE)
		    buf[k] = st[j];
        }
        bufloc = k;
      }
    }
    return 0;
}

/* local (static) versions */

#define width_offset 1
#define depth_offset 2
#define height_offset 3

#define check_index_range(j)                            \
   if (j<0 || j > 255) {                                \
	lua_pushstring(L, "LuaTeX Error: incorrect index value\n");	\
	lua_error(L);  }


static int dimen_to_number (lua_State *L,char *s){
  double v;
  char *d;
  int j;
  v = strtod(s,&d);
  if      (strcmp (d,"in") == 0) { j = (int)((v*7227)/100)   *65536; }
  else if (strcmp (d,"pc") == 0) { j = (int)(v*12)           *65536; } 
  else if (strcmp (d,"cm") == 0) { j = (int)((v*7227)/254)   *65536; }
  else if (strcmp (d,"mm") == 0) { j = (int)((v*7227)/2540)  *65536; }
  else if (strcmp (d,"bp") == 0) { j = (int)((v*7227)/7200)  *65536; }
  else if (strcmp (d,"dd") == 0) { j = (int)((v*1238)/1157)  *65536; }
  else if (strcmp (d,"cc") == 0) { j = (int)((v*14856)/1157) *65536; }
  else if (strcmp (d,"nd") == 0) { j = (int)((v*21681)/20320)*65536; }
  else if (strcmp (d,"nc") == 0) { j = (int)((v*65043)/5080) *65536; }
  else if (strcmp (d,"pt") == 0) { j = (int)v                *65536; }
  else if (strcmp (d,"sp") == 0) { j = (int)v; }
  else {
	lua_pushstring(L, "LuaTeX Error: unknown dimension specifier\n");
	lua_error(L);
	j = 0;
  }
  return j;
}

static int setdimen (lua_State *L) {
  int i,j,k;
  i = lua_gettop(L);
  if (!lua_isnumber(L,i))
	j = dimen_to_number(L,(char *)lua_tostring(L,i));
  else
    j = (int)lua_tonumber(L,i);
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  check_index_range(k);
  if(settexdimenregister(k,j)) {
	lua_pushstring(L, "LuaTeX Error: incorrect value\n");
	lua_error(L);
  }
  return 0;
}

static int getdimen (lua_State *L) {
  int i, j;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  check_index_range(j);
  j = gettexdimenregister(j);
  lua_pushnumber(L, j);
  return 1;
}

static int setcount (lua_State *L) {
  int i,j,k;
  i = lua_gettop(L);
  if (!lua_isnumber(L,i)) {
    lua_pushstring(L, "LuaTeX Error: unsupported value type\n");
    lua_error(L);
  }
  j = (int)lua_tonumber(L,i);
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  check_index_range(k);
  if (settexcountregister(k,j)) {
	lua_pushstring(L, "LuaTeX Error: incorrect value\n");
	lua_error(L);
  }
  return 0;
}

static int getcount (lua_State *L) {
  int i, j;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  check_index_range(j);
  j = gettexcountregister(j);
  lua_pushnumber(L, j);
  return 1;
}

static int settoks (lua_State *L) {
  int i,j,k,l,len;
  i = lua_gettop(L);
  char *st;
  if (!lua_isstring(L,i)) {
    lua_pushstring(L, "LuaTeX Error: unsupported value type\n");
    lua_error(L);
  }
  st = (char *)lua_tostring(L,i);
  len = lua_strlen(L, i);
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  check_index_range(k);
  if(settextoksregister(k,maketexstring(st))) {
	lua_pushstring(L, "LuaTeX Error: incorrect value\n");
	lua_error(L);
  }
  return 0;
}

static int gettoks (lua_State *L) {
  int i,j;
  strnumber t;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  check_index_range(j);
  t = gettextoksregister(j);
  lua_pushstring(L, makecstring(t));
  flushstr(t);
  return 1;
}

static int getboxdim (lua_State *L, int whichdim) {
  int i, j, q;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  if (j<0 || j > 255) {
	lua_pushstring(L, "LuaTeX Error: incorrect index\n");
	lua_error(L);
  }
  switch (whichdim) {
  case width_offset: 
	lua_pushnumber(L, gettexboxwidth(j));
	break;
  case height_offset: 
	lua_pushnumber(L, gettexboxheight(j));
	break;
  case depth_offset: 
	lua_pushnumber(L, gettexboxdepth(j));
  }
  return 1;
}

static int getboxwd (lua_State *L) {
  return getboxdim (L, width_offset);
}

static int getboxht (lua_State *L) {
  return getboxdim (L, height_offset);
}

static int getboxdp (lua_State *L) {
  return getboxdim (L, depth_offset);
}

static int setboxdim (lua_State *L, int whichdim) {
  int i,j,k,err;
  i = lua_gettop(L);
  if (!lua_isnumber(L,i)) {
	j = dimen_to_number(L,(char *)lua_tostring(L,i));
  } else {
    j = (int)lua_tonumber(L,i);
  }
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  if (k<0 || k > 255) {
	lua_pushstring(L, "LuaTeX Error: incorrect index\n");
	lua_error(L);
  }
  err = 0;
  switch (whichdim) {
  case width_offset: 
	err = settexboxwidth(k,j);
	break;
  case height_offset: 
	err = settexboxheight(k,j);
	break;
  case depth_offset: 
	err = settexboxdepth(k,j);
  }
  if (err) {
	lua_pushstring(L, "LuaTeX Error: not a box\n");
	lua_error(L);
  }
  return 0;
}

static int setboxwd (lua_State *L) {
  return setboxdim(L,width_offset);
}

static int setboxht (lua_State *L) {
  return setboxdim(L,height_offset);
}

static int setboxdp (lua_State *L) {
  return setboxdim(L,depth_offset);
}

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


static int findcurv (lua_State *L) {
  int j;
  j = getcurv();
  lua_pushnumber(L, j);
  return 1;
}

static int findcurh (lua_State *L) {
  int j;
  j = getcurh();
  lua_pushnumber(L, j);
  return 1;
}

static int makecurv (lua_State *L) {
  lua_pop(L,1); /* table at -1 */
  return 0;
}

static int makecurh (lua_State *L) {
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

typedef struct LoadS {
    const char *s;
    size_t size;
} LoadS;

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

void luacall(strnumber s)
{
    LoadS ls;
    int i, j, k ;
    char err[] = "LuaTeX Error";

    if (L == NULL) {
        L = lua_open();
        luaopen_base(L);
        luaopen_string(L);
        luaopen_table(L);
        luaopen_math(L);
        /*luaopen_io(L);*/
        luaopen_debug(L);
        luaopen_pdf(L);
        luaopen_tex(L);
		buf = malloc(BUFSIZE);
		if (buf == NULL) {
		  exit (1);
		}
    }

    ls.s = &(strpool[strstart[s]]);
    ls.size = strstart[s + 1] - strstart[s];

    i = lua_load(L, getS, &ls, "luacall");

    if (i != 0) {
        for (i = poolptr, j = 0; j < strlen(err); i++, j++)
            strpool[i] = err[j];
        poolptr += strlen(err);
		fprintf(stderr, "%s", lua_tostring(L, -1));
		lua_close(L);
		L = NULL;
		free(buf);
        return;
    }

    bufloc = 0;
    i = lua_pcall(L, 0, 0, 0);
    if (i != 0) {
	  fprintf(stderr, "%s", lua_tostring(L, -1));
	  lua_close(L);
	  L = NULL;
	  free(buf);
	  for (i = poolptr, j = 0; j < strlen(err); i++, j++)
		strpool[i] = err[j];
	  poolptr += strlen(err);
    } else {
	  if (poolptr + bufloc >= poolsize)
		poolptr = poolsize;
	  buf[bufloc] = 0;
	  for (j = 0, k = poolptr; j < bufloc; j++, k++)
		strpool[k] = buf[j];
	  poolptr += bufloc;
	}	  
	return;
}
