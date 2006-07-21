/* $Id */

#include "luatex-api.h"
#include <ptexlib.h>

int 
poolprint(lua_State * L) {
  int i, j, k, n, len;
  const char *st;
  n = lua_gettop(L);
  for (i = 1; i <= n; i++) {
	if (!lua_isstring(L, i)) {
	  lua_pushstring(L, "no string to print");
	  lua_error(L);
	}
	st = lua_tostring(L, i);
	len = lua_strlen(L, i);
	if (len) {
	  if (poolptr + len >= poolsize) {
		lua_pushstring(L, "TeX pool full");
		lua_error(L);
	  } else {
		for (j = 0, k = poolptr; j < len; j++, k++)
		  strpool[k] = st[j];
		poolptr += len;
	  }
	}
  }
  return 0;
}

/* local (static) versions */

#define width_offset 1
#define depth_offset 2
#define height_offset 3

#define check_index_range(j)                            \
   if (j<0 || j > 65535) {                                \
	lua_pushstring(L, "incorrect index value");	\
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
	lua_pushstring(L, "unknown dimension specifier");
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
	lua_pushstring(L, "incorrect value");
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
    lua_pushstring(L, "unsupported value type");
    lua_error(L);
  }
  j = (int)lua_tonumber(L,i);
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  check_index_range(k);
  if (settexcountregister(k,j)) {
	lua_pushstring(L, "incorrect value");
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
    lua_pushstring(L, "unsupported value type");
    lua_error(L);
  }
  st = (char *)lua_tostring(L,i);
  len = lua_strlen(L, i);
  k = (int)lua_tonumber(L,(i-1));
  lua_settop(L,(i-3)); /* table at -2 */
  check_index_range(k);
  if(settextoksregister(k,maketexstring(st))) {
	lua_pushstring(L, "incorrect value");
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
  if (j<0 || j > 65535) {
	lua_pushstring(L, "incorrect index");
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
  if (k<0 || k > 65535) {
	lua_pushstring(L, "incorrect index");
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
	lua_pushstring(L, "not a box");
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

/* do this aleph stuff here, for now */

memoryword **fonttables;
static int font_entries = 0;

void
allocatefonttable P2C(int, font_number, int, font_size)
{
    int i;
    if (font_entries==0) {
      fonttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      font_entries=256;
    } else if ((font_number==256)&&(font_entries==256)) {
      fonttables = xrealloc(fonttables, 65536);
      font_entries=65536;
    }
    fonttables[font_number] =
       (memoryword *) xmalloc((font_size+1)*sizeof(memoryword));
    fonttables[font_number][0].cint = font_size;
    fonttables[font_number][0].cint1 = 0;
    for (i=1; i<=font_size; i++) {
        fonttables[font_number][i].cint  = 0;
        fonttables[font_number][i].cint1 = 0;
    }
}

void
dumpfonttable P2C(int, font_number, int, words)
{
    fonttables[font_number][0].cint=words;
    dumpthings(fonttables[font_number][0], fonttables[font_number][0].cint+1);
}

void
undumpfonttable(font_number)
int font_number;
{
    memoryword sizeword;
    if (font_entries==0) {
      fonttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      font_entries=256;
    } else if ((font_number==256)&&(font_entries==256)) {
      fonttables = xrealloc(fonttables, 65536);
      font_entries=65536;
    }

    undumpthings(sizeword,1);
    fonttables[font_number] =
        (memoryword *) xmalloc((sizeword.cint+1)*sizeof(memoryword));
    fonttables[font_number][0].cint = sizeword.cint;
    undumpthings(fonttables[font_number][1], sizeword.cint);
}

memoryword **fontsorttables;
static int fontsort_entries = 0;

void
allocatefontsorttable P2C(int, fontsort_number, int, fontsort_size)
{
    int i;
    if (fontsort_entries==0) {
      fontsorttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      fontsort_entries=256;
    } else if ((fontsort_number==256)&&(fontsort_entries==256)) {
      fontsorttables = xrealloc(fontsorttables, 65536);
      fontsort_entries=65536;
    }
    fontsorttables[fontsort_number] =
       (memoryword *) xmalloc((fontsort_size+1)*sizeof(memoryword));
    fontsorttables[fontsort_number][0].cint = fontsort_size;
    fontsorttables[fontsort_number][0].cint1 = 0;
    for (i=1; i<=fontsort_size; i++) {
        fontsorttables[fontsort_number][i].cint  = 0;
        fontsorttables[fontsort_number][i].cint1 = 0;
    }
}

void
dumpfontsorttable P2C(int, fontsort_number, int, words)
{
    fontsorttables[fontsort_number][0].cint=words;
    dumpthings(fontsorttables[fontsort_number][0],
               fontsorttables[fontsort_number][0].cint+1);
}

void
undumpfontsorttable(fontsort_number)
int fontsort_number;
{
    memoryword sizeword;
    if (fontsort_entries==0) {
      fontsorttables = (memoryword **) xmalloc(256*sizeof(memoryword**));
      fontsort_entries=256;
    } else if ((fontsort_number==256)&&(fontsort_entries==256)) {
      fontsorttables = xrealloc(fontsorttables, 65536);
      fontsort_entries=65536;
    }

    undumpthings(sizeword,1);
    fontsorttables[fontsort_number] =
        (memoryword *) xmalloc((sizeword.cint+1)*sizeof(memoryword));
    fontsorttables[fontsort_number][0].cint = sizeword.cint;
    undumpthings(fontsorttables[fontsort_number][1], sizeword.cint);
}
