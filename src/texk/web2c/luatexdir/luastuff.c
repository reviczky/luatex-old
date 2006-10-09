/* $Id: luastuff.c,v 1.16 2005/08/10 22:21:53 hahe Exp hahe $ */

#include "luatex-api.h"
#include <ptexlib.h>

lua_State *Luas[65536];

extern char *startup_filename;

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

/* package.path should contain:
  (1) the local dir          :  ./?.lua
  (2) global lua macros      :  $LUASCRIPTS/?.lua
                                $LUASCRIPTS/?/init.lua

      $LUASCRIPTS = bit of expansion of $TEXMFSCRIPTS 
                    containing /lua/
                                
  (3) the base lua libraries :  $BINDIR/../lib/lua/5.1/?.lua;
                                $BINDIR/../lib/lua/5.1/?/init.lua
*/


void fix_package_path (lua_State *L, char *key, char *ext, int doinit) 
{
  char *path;
  char *allocpath;
  char *result;
  char *rover;
  char *curstr;
  int alloc;
  lua_getglobal(L,"package");
  if (lua_istable(L,-1)) {
    // local
    alloc = strlen(DIR_SEP_STRING) + 3 +1 + strlen(ext);
    path = malloc(alloc);
    if (path==NULL)
      return;
    snprintf(path,alloc,".%s?.%s", DIR_SEP_STRING, ext);
    // texmfscripts
    if (doinit) {
      result = (char *)kpse_path_expand(kpse_expand ("$TEXMFSCRIPTS"));
      if (result!=NULL) {
	rover = result;
	curstr = result;
	while (*rover) {
	  if (IS_ENV_SEP(*rover)) {
	    *rover = 0;
	    if (!(strstr(curstr,"lua")==NULL) &&
		(strstr(curstr,"LUA")==NULL)) {
	      alloc = strlen(path)+1+2*strlen(curstr)+2+2+6+3*strlen(DIR_SEP_STRING)+2*strlen(ext);
	      allocpath = malloc (alloc);
	      if (allocpath==NULL) {
		*rover = ENV_SEP; // enable cleanup
		break;
	      }
	      snprintf(allocpath,alloc,"%s;%s%s?.%s;%s%s?%sinit.%s",
		       path,curstr,DIR_SEP_STRING,ext,curstr,DIR_SEP_STRING,DIR_SEP_STRING,ext);
	      free (path);
	      path = allocpath;
	    }
	    if (*(rover+1)!=0) 
	      curstr=rover+1;
	    *rover = ENV_SEP; // enable cleanup
	  }
	  rover++;
	}
	free(result);
      }
    }
    // bindir
    curstr = getenv("SELFAUTODIR");
    if (curstr!=NULL) {
      rover =  DIR_SEP_STRING "lib" DIR_SEP_STRING "lua" DIR_SEP_STRING "5.1" DIR_SEP_STRING;
      alloc = strlen(path)+2+2*strlen(curstr)+2+2*strlen(rover)+6+
                strlen(DIR_SEP_STRING)+1+2*strlen(ext);
      allocpath = malloc (alloc);
      if (allocpath!=NULL) {
	if (doinit) {
	  snprintf(allocpath,alloc,"%s;%s%s?.%s;%s%s?%sinit.%s",
		   path,curstr,rover,ext,curstr,rover,DIR_SEP_STRING,ext);
	} else {
	  snprintf(allocpath,alloc,"%s;%s%s?.%s", path,curstr,rover,ext);
	}
	free (path);
	path = allocpath;

      }
    }
    rover = path;
    while (*rover) { 
      if (*rover == '\\')
	*rover = '/';
      rover++;
    }
    lua_pushstring(L, path);
    lua_setfield(L,-2,key);
  }
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

void 
luainterpreter (int n) {
  lua_State *L;
  L = luaL_newstate();
  luaL_openlibs(L);
  find_env(L);
  luaopen_unicode(L);
  luaopen_zip(L);
  luaopen_pdf(L);
  luaopen_tex(L);
  luaopen_texio(L);
  luaopen_kpse(L);
  if (n==0)
    luaopen_callback(L);
  luaopen_lua(L,n,startup_filename);
  //  fix_package_path(L,"path","lua",1);
#if defined(_WIN32)
  //  fix_package_path(L,"cpath","dll",0);
#else
  //  fix_package_path(L,"cpath","so",0);
#endif
  Luas[n] = L;
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

