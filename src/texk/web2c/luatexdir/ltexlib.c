/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

typedef struct {
  char *text;
  void *next;
  unsigned char stringtype;
} rope;

static rope luacstring_head = {NULL,NULL,0};

static rope *luacstring_rover = NULL;

static int 
int_luacprint(char *st, int detok) {
  rope *rn;
  luacstrings++; /* tex-side variable */
  rn = (rope *)malloc(sizeof(rope));
  rn->text = st;
  rn->stringtype = detok;
  rn->next = NULL;
  luacstring_rover->next = rn;
  luacstring_rover = rn;
}

int 
luacprint(lua_State * L) {
  int i, n;
  char *st;
  n = lua_gettop(L);
  if (luacstring_rover == NULL)
    luacstring_rover = &luacstring_head;
  for (i = 1; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "no string to print");
      lua_error(L);
    }
    st = strdup(lua_tostring(L, i));
    if (st)
      int_luacprint(st,0);
  }
  return 0;
}

int 
luacwrite(lua_State * L) {
  int i, n;
  char *st;
  n = lua_gettop(L);
  if (luacstring_rover == NULL)
    luacstring_rover = &luacstring_head;
  for (i = 1; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "no string to print");
      lua_error(L);
    }
    st = strdup(lua_tostring(L, i));
    if (st)
      int_luacprint(st,1);
  }
  return 0;
}

int 
luacsprint(lua_State * L) {
  int i, n;
  char *st;
  n = lua_gettop(L);
  if (luacstring_rover == NULL)
    luacstring_rover = &luacstring_head;
  for (i = 1; i <= n; i++) {
    if (!lua_isstring(L, i)) {
      lua_pushstring(L, "no string to print");
      lua_error(L);
    }
    st = strdup(lua_tostring(L, i));
    if (st)
      int_luacprint(st,2);
  }
  return 0;
}



boolean 
luacstringdetokenized (void) {
  return (luacstring_head.stringtype == 1);
}

boolean 
luacstringsimple (void) {
  return (luacstring_head.stringtype == 2);
}

boolean 
luacstringpenultimate (void) {
  return (luacstring_head.next == NULL);
}

boolean 
luacstringinput (void) {
  char *st;
  rope *t;
  int ret,len;
  if (luacstring_head.next != NULL) {
    luacstrings--; /* tex-side variable */
    t = luacstring_head.next;
    st = t->text;
    /* put that thing in the buffer */
    last = first;
    ret = last;
    len = strlen(st);
    check_buf (last + len,bufsize);
    while (len-->0)
      buffer[last++] = *st++;
    while (last-1>ret && buffer[last-1] == ' ')
      last--;
    if (luacstring_rover == t)
      luacstring_rover = &luacstring_head;
    free(t->text);
    luacstring_head.stringtype = t->stringtype;
    luacstring_head.next = t->next;
    free(t);    
    return 1;
  }
  return 0;
}

void 
luacstringclose (void) {
  if (luacstring_head.next != NULL) {
    free(luacstring_head.next);
    luacstring_head.next = NULL;
  }
  luacstring_rover = NULL;
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
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  // find the value
  if (!lua_isnumber(L,i))
    if (lua_isstring(L,i)) {
	j = dimen_to_number(L,(char *)lua_tostring(L,i));
    } else {
      lua_pushstring(L, "unsupported value type");
      lua_error(L);
    }
  else
    j = (int)lua_tonumber(L,i);
  // find the index
  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    k = zgetequiv(cur_cs)-getscaledbase();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  if(settexdimenregister(k,j)) {
    lua_pushstring(L, "incorrect value");
    lua_error(L);
  }
  return 0;
}

int getdimen (lua_State *L) {
  int i,j,  k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    if (isundefinedcs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zgetequiv(cur_cs)-getscaledbase();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }
  check_index_range(k);
  j = gettexdimenregister(k);
  lua_pushnumber(L, j);
  return 1;
}

int setcount (lua_State *L) {
  int i,j,k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  j = (int)luaL_checkinteger(L,i);
  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    k = zgetequiv(cur_cs)-getcountbase();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  if (settexcountregister(k,j)) {
	lua_pushstring(L, "incorrect value");
	lua_error(L);
  }
  return 0;
}

int getcount (lua_State *L) {
  int i, j, k;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    if (isundefinedcs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zgetequiv(cur_cs)-getcountbase();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }
  check_index_range(k);
  j = gettexcountregister(k);
  lua_pushnumber(L, j);
  return 1;
}

int settoks (lua_State *L) {
  int i,j,k,l,len;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  char *st;
  if (!lua_isstring(L,i)) {
    lua_pushstring(L, "unsupported value type");
    lua_error(L);
  }
  st = (char *)lua_tostring(L,i);
  len = lua_strlen(L, i);

  if (lua_type(L,i-1)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i-1));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    k = zgetequiv(cur_cs)-gettoksbase();
  } else {
    k = (int)luaL_checkinteger(L,i-1);
  }
  check_index_range(k);
  j = maketexstring(st);
  if(zsettextoksregister(k,j)) {
    flushstr(j);
    lua_pushstring(L, "incorrect value");
    lua_error(L);
  }
  return 0;
}

int gettoks (lua_State *L) {
  int i,k;
  strnumber t;
  int cur_cs;
  int texstr;
  i = lua_gettop(L);
  if (lua_type(L,i)==LUA_TSTRING) {
    texstr = maketexstring(lua_tostring(L,i));
    cur_cs = stringlookup(texstr);
    flushstr(texstr);
    if (isundefinedcs(cur_cs)) {
      lua_pushnil(L);
      return 1;
    }
    k = zgetequiv(cur_cs)-gettoksbase();
  } else {
    k = (int)luaL_checkinteger(L,i);
  }

  check_index_range(k);
  t = gettextoksregister(k);
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

int settex (lua_State *L) {
  char *st;
  int i,j,texstr;
  int cur_cs, cur_cmd;
  i = lua_gettop(L);
  if (lua_isstring(L,(i-1))) {
    st = (char *)lua_tostring(L,(i-1));
    texstr = maketexstring(st);
    if (zisprimitive(texstr)) {
      cur_cs = stringlookup(texstr);
      flushstr(texstr);
      cur_cmd = zgeteqtype(cur_cs);
      if (isintassign(cur_cmd)) {
	if (lua_isnumber(L,i)) {
	  assigninternalint(zgetequiv(cur_cs),lua_tonumber(L,i));
	} else {
	  lua_pushstring(L, "unsupported value type");
	  lua_error(L);
	}
      } else if (isdimassign(cur_cmd)) {
	if (!lua_isnumber(L,i))
	  if (lua_isstring(L,i)) {
	    j = dimen_to_number(L,(char *)lua_tostring(L,i));
	  } else {
	    lua_pushstring(L, "unsupported value type");
	    lua_error(L);
	  }
	else
	  j = (int)lua_tonumber(L,i);
	assigninternaldim(zgetequiv(cur_cs),j);
      } else {
	lua_pushstring(L, "unsupported tex internal assignment");
	lua_error(L);
      }
    } else {
      lua_rawset(L,(i-2));
    }
  } else {
    lua_rawset(L,(i-2));
  }
  return 0;
}

char *
get_something_internal (int cur_cmd, int cur_code) {
  int texstr;
  char *str;
  int save_cur_val,save_cur_val_level;
  save_cur_val = curval;
  save_cur_val_level = curvallevel;
  zscansomethingsimple(cur_cmd,cur_code);
  texstr = thescannedresult();
  curval = save_cur_val;
  curvallevel = save_cur_val_level;  
  str = makecstring(texstr);
  flushstr(texstr);
  return str;
}

char *
get_convert (int cur_code) {
  int texstr;
  char *str = NULL;
  texstr = theconvertstring(cur_code);
  if (texstr) {
    str = makecstring(texstr);
    flushstr(texstr);
  }
  return str;
}


int
gettex (lua_State *L) {
  char *st;
  int i,texstr;
  char *str;
  int cur_cs, cur_cmd, cur_code;
  int save_cur_val,save_cur_val_level;
  i = lua_gettop(L);
  if (lua_isstring(L,i)) {
    st = (char *)lua_tostring(L,i);
    texstr = maketexstring(st);
    cur_cs = zprimlookup(texstr);
    flushstr(texstr);
    if (cur_cs) {
      cur_cmd = zgetprimeqtype(cur_cs);
      cur_code = zgetprimequiv(cur_cs);
      if (isconvert(cur_cmd))
	str = get_convert(cur_code);
      else 
	str = get_something_internal(cur_cmd,cur_code);
      if (str)
	lua_pushstring(L,str);
      else 
	lua_pushnil(L);
      return 1;
    } else {
      lua_rawget(L,(i-1));
      return 1;
    }    
  } else {
    lua_rawget(L,(i-1));
    return 1;
  }
  return 0; // not reached
}


static const struct luaL_reg texlib [] = {
  {"write",    luacwrite},
  {"print",    luacprint},
  {"sprint",   luacsprint},
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
  luaL_register(L, "tex", texlib);
  make_table(L,"dimen","getdimen","setdimen");
  make_table(L,"count","getcount","setcount");
  make_table(L,"toks","gettoks","settoks");
  make_table(L,"wd","getboxwd","setboxwd");
  make_table(L,"ht","getboxht","setboxht");
  make_table(L,"dp","getboxdp","setboxdp");
  /* make the meta entries */
  /* fetch it back */
  luaL_newmetatable(L,"tex_meta"); 
  lua_pushstring(L, "__index");
  lua_pushcfunction(L, gettex); 
  lua_settable(L, -3);
  lua_pushstring(L, "__newindex");
  lua_pushcfunction(L, settex); 
  lua_settable(L, -3);
  lua_setmetatable(L,-2); /* meta to itself */
  return 1;
}

