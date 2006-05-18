/* $Id */

#include "luatex.h"
#include <ptexlib.h>

char *buf;

int bufloc;

integer getcurv AA((void));
integer getcurh AA((void));


int poolprint(lua_State * L)
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

int setdimen (lua_State *L) {
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

int getdimen (lua_State *L) {
  int i, j;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  check_index_range(j);
  j = gettexdimenregister(j);
  lua_pushnumber(L, j);
  return 1;
}

int setcount (lua_State *L) {
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

int getcount (lua_State *L) {
  int i, j;
  i = lua_gettop(L);
  j = (int)lua_tonumber(L,(i));
  lua_settop(L,(i-2)); /* table at -1 */
  check_index_range(j);
  j = gettexcountregister(j);
  lua_pushnumber(L, j);
  return 1;
}

int settoks (lua_State *L) {
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

int gettoks (lua_State *L) {
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

int getboxwd (lua_State *L) {
  return getboxdim (L, width_offset);
}

int getboxht (lua_State *L) {
  return getboxdim (L, height_offset);
}

int getboxdp (lua_State *L) {
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

int setboxwd (lua_State *L) {
  return setboxdim(L,width_offset);
}

int setboxht (lua_State *L) {
  return setboxdim(L,height_offset);
}

int setboxdp (lua_State *L) {
  return setboxdim(L,depth_offset);
}



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

void luatex_init (int s, LoadS *ls)
{
  ls->s = &(strpool[strstart[s]]);
  ls->size = strstart[s + 1] - strstart[s];
}

void luatex_return (void)
{
  int j,k;
  if (poolptr + bufloc >= poolsize)
	poolptr = poolsize;
  buf[bufloc] = 0;
  for (j = 0, k = poolptr; j < bufloc; j++, k++)
	strpool[k] = buf[j];
  poolptr += bufloc;
}

void luatex_error (char *err)
{
  int i,j;
  for (i = poolptr, j = 0; j < strlen(err); i++, j++)
	strpool[i] = err[j];
  poolptr += strlen(err);
}
