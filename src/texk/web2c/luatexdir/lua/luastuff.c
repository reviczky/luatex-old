/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"
#include <ptexlib.h>

lua_State *Luas[65536];

extern char *startup_filename;
extern int safer_option;

int luastate_max = 0;
int luastate_bytes = 0;

void
make_table (lua_State *L, char *tab, char *getfunc, char *setfunc) {
  /* make the table */            /* [{<tex>}] */
  lua_pushstring(L,tab);          /* [{<tex>},"dimen"] */
  lua_newtable(L);                /* [{<tex>},"dimen",{}] */
  lua_settable(L, -3);            /* [{<tex>}] */
  /* fetch it back */
  lua_pushstring(L,tab);          /* [{<tex>},"dimen"] */
  lua_gettable(L, -2);            /* [{<tex>},{<dimen>}] */
  /* make the meta entries */
  luaL_newmetatable(L,tab);       /* [{<tex>},{<dimen>},{<dimen_m>}] */
  lua_pushstring(L, "__index");   /* [{<tex>},{<dimen>},{<dimen_m>},"__index"] */
  lua_pushstring(L, getfunc);     /* [{<tex>},{<dimen>},{<dimen_m>},"__index","getdimen"] */
  lua_gettable(L, -5);            /* [{<tex>},{<dimen>},{<dimen_m>},"__index",<tex.getdimen>]  */
  lua_settable(L, -3);            /* [{<tex>},{<dimen>},{<dimen_m>}]  */
  lua_pushstring(L, "__newindex");/* [{<tex>},{<dimen>},{<dimen_m>},"__newindex"] */
  lua_pushstring(L, setfunc);     /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex","setdimen"] */
  lua_gettable(L, -5);            /* [{<tex>},{<dimen>},{<dimen_m>},"__newindex",<tex.setdimen>]  */
  lua_settable(L, -3);            /* [{<tex>},{<dimen>},{<dimen_m>}]  */ 
  lua_setmetatable(L,-2);         /* [{<tex>},{<dimen>}] : assign the metatable */
  lua_pop(L,1);                   /* [{<tex>}] : clean the stack */
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

extern char **environ;

void find_env (lua_State *L){
  char *envitem;
  char *envkey;
  char **envpointer;
  envpointer = environ;
  lua_getglobal(L,"os");
  if (envpointer!=NULL && lua_istable(L,-1)) {
    luaL_checkstack(L,2,"out of stack space");
    lua_pushstring(L,"env"); 
    lua_newtable(L); 
    while (*envpointer) {
      /* TODO: perhaps a memory leak here  */
      luaL_checkstack(L,2,"out of stack space");
      envitem = strdup(*envpointer);
      envkey=envitem;
      while (*envitem != '=') {
	envitem++;
      }
      *envitem=0;
      envitem++;
      lua_pushstring(L,envkey);
      lua_pushstring(L,envitem);
      lua_rawset(L,-3);
      envpointer++;
    }
    lua_rawset(L,-3);
  }
  lua_pop(L,1);
}

void *my_luaalloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  void *ret = NULL;
  if (nsize == 0)
	free(ptr);
  else
	ret = realloc(ptr, nsize);
  luastate_bytes += (nsize-osize);
  return ret;
}

static int my_luapanic (lua_State *L) {
  (void)L;  /* to avoid warnings */
  fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
                   lua_tostring(L, -1));
  return 0;
}


void 
luainterpreter (int n) {
  lua_State *L;
  L = lua_newstate(my_luaalloc, NULL);
  if (L==NULL) {
	fprintf(stderr,"Can't create a new Lua state (%d).",n);
	return;
  }
  lua_atpanic(L, &my_luapanic);

  luastate_max++;
  luaL_openlibs(L);
  find_env(L);
  luaopen_unicode(L);
  luaopen_zip(L);
  luaopen_pdf(L);
  luaopen_tex(L);
  luaopen_texio(L);
  luaopen_kpse(L);
  luaopen_lfs(L);
  if (n==0) {
    luaopen_callback(L);
    lua_createtable(L, 0, 0);
    lua_setglobal(L, "texconfig");
  }
  luaopen_lua(L,n,startup_filename);
  luaopen_stats(L);
  luaopen_font(L);
  if (safer_option) {
	/* disable some stuff if --safer */
	(void)hide_lua_value(L, "os","execute");
	(void)hide_lua_value(L, "os","rename");
	(void)hide_lua_value(L, "os","remove");
	(void)hide_lua_value(L, "io","popen");
	/* make io.open only read files */
	luaL_checkstack(L,2,"out of stack space");
	lua_getglobal(L,"io");
	lua_getfield(L,-1,"open_ro");	
	lua_setfield(L,-2,"open");	
	(void)hide_lua_value(L, "io","tmpfile");
	(void)hide_lua_value(L, "io","output");
	(void)hide_lua_value(L, "lfs","chdir");
	(void)hide_lua_value(L, "lfs","lock");
	(void)hide_lua_value(L, "lfs","touch");
	(void)hide_lua_value(L, "lfs","rmdir");
	(void)hide_lua_value(L, "lfs","mkdir");
  }
  Luas[n] = L;
}

int hide_lua_table(lua_State *L, char *name) {
  int r=0;
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
    r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushnil(L);
    lua_setglobal(L,name);
  }
  return r;
}

void unhide_lua_table(lua_State *L, char *name, int r) {
  lua_rawgeti(L,LUA_REGISTRYINDEX,r);
  lua_setglobal(L,name);
  luaL_unref(L,LUA_REGISTRYINDEX,r);
}

int hide_lua_value(lua_State *L, char *name, char *item) {
  int r=0;
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
	lua_getfield(L,-1,item);
    r = luaL_ref(L,LUA_REGISTRYINDEX);
    lua_pushnil(L);
    lua_setfield(L,-2,item);
  }
  return r;
}

void unhide_lua_value(lua_State *L, char *name, char *item, int r) {
  lua_getglobal(L,name);
  if(lua_istable(L,-1)) {
	lua_rawgeti(L,LUA_REGISTRYINDEX,r);
	lua_setfield(L,-2,item);
	luaL_unref(L,LUA_REGISTRYINDEX,r);
  }
}


void 
luacall(int n, int s) {
  LoadS ls;
  int i, j, k ;
  char *lua_id;
  lua_id = (char *)xmalloc(12);
  if (Luas[n] == NULL) {
    luainterpreter(n);
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
  if (n!=0 && Luas[n] != NULL) {
    lua_close(Luas[n]);
	luastate_max--;
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
  strnumber s;
  const char *luaerr = lua_tostring(L, -1);
  err = (char *)xmalloc(128);
  len = snprintf(err,128,"%s",luaerr);
  if (is_fatal>0) {
    /* Normally a memory error from lua. 
       The pool may overflow during the maketexstring(), but we 
       are crashing anyway so we may as well abort on the pool size */
    s = maketexstring(err);
    luafatalerror(s);
    /* never reached */
    xfree (err);
    lua_close(L);
	luastate_max--;
    return (lua_State *)NULL;
  } else {
    /* Here, the pool could be full already, but we can possibly escape from that 
     * condition, since the lua chunk that caused the error is the current string.
     */
    s = strptr-0x200000;
    //    fprintf(stderr,"poolinfo: %d: %d,%d out of %d\n",s,poolptr,strstart[(s-1)],poolsize);
    poolptr = strstart[(s-1)];
    strstart[s] = poolptr;
    if (poolptr+len>=poolsize) {
      luanormerror(' ');
    } else {
      s = maketexstring(err);
      luanormerror(s);
      flushstr(s);
    }
    xfree (err);
    return L;
  }
}

