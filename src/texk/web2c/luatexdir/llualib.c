
/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

typedef struct {
  int    size;
  void * data;
} bytecode;

static bytecode *lua_bytecode_registers = NULL;
static int lua_bytecode_max = -1;

void dumpluacregisters (void) {
  int k;
  dumpint(lua_bytecode_max);
  if (lua_bytecode_registers != NULL) {
	for (k=0;k<lua_bytecode_max;k++) {
	  if (lua_bytecode_registers[k].size!=0) {
		dumpint(k);
		dumpint(lua_bytecode_registers[k].size);
		dumpthings(lua_bytecode_registers[k].data,k);
 	  }
	}
  }
}

void undumpluacregisters (void) {
  int k;
  undumpint(lua_bytecode_max);
  if (lua_bytecode_max>0) {
	lua_bytecode_registers = xmalloc(sizeof(void *)*lua_bytecode_max);
	for (k=0;k<lua_bytecode_max;k++) {
	  if (lua_bytecode_registers[k].size!=0) {
		undumpint(k);
		undumpint(lua_bytecode_registers[k].size);
		undumpthings(lua_bytecode_registers[k].data,k);
	  }
	}
  }
}

static int writer(lua_State* L, const void *p, size_t size, void *u) {
  //  UNUSED(L);
  bytecode *x;
  void *buffer;
  x = (bytecode *)u;
  if (x->size != 0) {
	buffer =xrealloc(x->data,(x->size)+size);
	memcpy(buffer,x->data,x->size);
	memcpy(buffer+(x->size),p,size);
  } else {
	buffer = xmalloc(size);
	memcpy(buffer,p,size);
  }
  x->data = buffer;
  x->size += size;
  return 0;
}

static const char * reader (lua_State *L, void *u, size_t *size) {
  // UNUSED(L);
  bytecode *x;
  x = (bytecode *)u;
  if (x->size>0) {
	*size = (x->size);
	return (const char *)x->data;
  } else {
	return NULL;
  }
}

int get_bytecode (lua_State *L) {
  int i,j,k;
  k = (int)luaL_checkinteger(L,-1);
  if (k<0) {
	lua_pushnil(L);
  } else if (k<=lua_bytecode_max && lua_bytecode_registers[k].size != 0) {
	lua_load(L,reader,(void *)(lua_bytecode_registers+k),"bytecode");
  } else {
	lua_pushnil(L);
  }
  return 1;
}

int set_bytecode (lua_State *L) {
  int i,j,k;
  k = (int)luaL_checkinteger(L,-2);
  if (k<0) {
	lua_pushstring(L, "negative values not allowed");
	lua_error(L);
  }
  if (lua_type(L,-1) != LUA_TFUNCTION){
	lua_pushstring(L, "unsupported type");
	lua_error(L);
  }
  if (k>lua_bytecode_max) {
	lua_bytecode_registers = xrealloc(lua_bytecode_registers,sizeof(bytecode)*(k+1));
	for (i=(lua_bytecode_max+1);i<=k;i++) {
	  lua_bytecode_registers[i].data=NULL;
	  lua_bytecode_registers[i].size=0;
	}
	lua_bytecode_max = k;
  }
  if (lua_bytecode_registers[k].size!=0) {
	xfree(lua_bytecode_registers[k].data);
	lua_bytecode_registers[k].size=0;
	lua_bytecode_registers[k].data=NULL;
  }
  lua_dump(L,writer,(void *)(lua_bytecode_registers+k));
  lua_pop(L,1);
  return 0;
}

static const struct luaL_reg lualib [] = {
  {"getbytecode", get_bytecode},
  {"setbytecode", set_bytecode},
  {NULL, NULL}  /* sentinel */
};

int luaopen_lua (lua_State *L, int n, char *fname) 
{
  luaL_register(L, "lua", lualib);
  make_table(L,"bytecode","getbytecode","setbytecode");
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "id");
  lua_pushstring(L, "1.01");
  lua_setfield(L, -2, "version");
  if (fname == NULL) {
	lua_pushnil(L);
  } else {
	lua_pushstring(L, strdup(fname));
  }
  lua_setfield(L, -2, "startupfile");
  return 1;
}

