
/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

#define LOAD_BUF_SIZE (10 * 1024)

typedef struct {
  unsigned char* buf;
  int size;
  int done;
} bytecode;

static bytecode *lua_bytecode_registers = NULL;
static int lua_bytecode_max = -1;

void dumpluacregisters (void) {
  int k,n;
  bytecode b;
  dumpint(lua_bytecode_max);
  if (lua_bytecode_registers != NULL) {
    n = 0;
    for (k=0;k<=lua_bytecode_max;k++) {
      if (lua_bytecode_registers[k].size != 0)
	n++;
    }
    dumpint(n);
    for (k=0;k<=lua_bytecode_max;k++) {
      b = lua_bytecode_registers[k];
      if (b.size != 0) {
	dumpint(k);
	dumpint(b.size);
	fprintf(stderr,"*%d,%d* ",k,b.size);
	do_zdump ((char *)b.buf,1,(b.size), DUMP_FILE);
      }
    }
  }
}

void undumpluacregisters (void) {
  int k,i,n;
  bytecode b;
  undumpint(lua_bytecode_max);
  if (lua_bytecode_max>=0) {
    lua_bytecode_registers = xmalloc(sizeof(bytecode)*(lua_bytecode_max+1));
    for (i=0;i<=lua_bytecode_max;i++) {
      lua_bytecode_registers[i].done = 0;
      lua_bytecode_registers[i].size = 0;
      lua_bytecode_registers[i].buf = NULL;
    }
    undumpint(n);
    for (i=0;i<n;i++) {
      undumpint(k);
      undumpint(b.size);
      fprintf(stderr,"*%d,%d* ",k,b.size);
      b.buf=xmalloc(LOAD_BUF_SIZE);
      memset(b.buf, 0, LOAD_BUF_SIZE);
      do_zundump ((char *)b.buf,1, b.size, DUMP_FILE);
      lua_bytecode_registers[k].size = b.size;
      lua_bytecode_registers[k].buf = b.buf;
    }
  }
}


int writer(lua_State* L, const void* b, size_t size, void* B) {
  bytecode* buf = (bytecode*)B;
  unsigned char *r;
  if(!(LOAD_BUF_SIZE - buf->size >= size)) {
    return 1;
  }
  memcpy(buf->buf+buf->size, b, size);
  buf->size += size;
  if (0) {
    fprintf(stderr,"+size=%d\n",buf->size);
    r = buf->buf;
    while ((r - buf->buf ) <= buf->size) {
      fprintf(stderr,"%x",*r++);
    }
  }
  return 0;
}

const char* reader(lua_State* L, void* ud, size_t* size) {
  bytecode* buf = (bytecode*)ud;
  unsigned char *r;
  if (buf->done == buf->size) {
    *size = 0;
    buf->done = 0;
    return NULL;
  }
  *size = buf->size;
  buf->done = buf->size;
  if (0) {
    fprintf(stderr,"-size=%d\n",*size);
    r = buf->buf;
    while ((r - buf->buf ) <= buf->size) {
      fprintf(stderr,"%x",*r++);
    }
  }
  return (const char*)buf->buf;
}

int get_bytecode (lua_State *L) {
  int i,j,k;
  k = (int)luaL_checkinteger(L,-1);
  if (k<0) {
    lua_pushnil(L);
  } else if (k<=lua_bytecode_max && lua_bytecode_registers[k].buf != NULL) {
    if(lua_load(L,reader,(void *)(lua_bytecode_registers+k),"bytecode")) {
      lua_error(L);
      lua_pushnil(L);
    }
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
      lua_bytecode_registers[i].buf=NULL;
      lua_bytecode_registers[i].size=0;
      lua_bytecode_registers[i].done=0;
    }
    lua_bytecode_max = k;
  }
  if (lua_bytecode_registers[k].buf != NULL) {
    xfree(lua_bytecode_registers[k].buf);
    lua_bytecode_registers[k].buf == NULL;
  }
  lua_bytecode_registers[k].buf=xmalloc(LOAD_BUF_SIZE);
  memset(lua_bytecode_registers[k].buf, 0, LOAD_BUF_SIZE);
  //
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

