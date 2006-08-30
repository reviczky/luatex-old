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

int
luaopen_callbacks (lua_State *L) 
{
  lua_pushstring(L,"texio");
  lua_newtable(L);
  lua_settable(L, -3);
  return 1;
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
		   path,curstr,rover,curstr,ext,rover,DIR_SEP_STRING,ext);
	} else {
	  snprintf(allocpath,alloc,"%s;%s%s?.%s", path,curstr,rover,curstr,ext);
	}
	free (path);
	path = allocpath;

      }
    }
    lua_pushstring(L, path);
    lua_setfield(L,-2,key);
  }
}


lua_State *
luainterpreter (int n) {
  lua_State *L;
  L = luaL_newstate();
  luaL_openlibs(L);
  luaopen_pdf(L);
  luaopen_tex(L);
  luaopen_unicode(L);
  luaopen_callbacks(L);
  fix_package_path(L,"path","lua",1);
#if defined(_WIN32)
  fix_package_path(L,"cpath","dll",0);
#else
  fix_package_path(L,"cpath","so",0);
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
luainitialize (int luaid, int format) {
  int error;
  char *loadaction;
  loadaction = malloc(120);
  if (loadaction==NULL)
    return;
  if (strcmp(" (INITEX)",makecstring(format))==0)
    snprintf(loadaction,120, "tex.formatname = nil; require \"startup\"");
  else
    snprintf(loadaction,120, "tex.formatname = \"%s\"; require \"startup\"",makecstring(format));
  luainterpreter (luaid);
  if(luaL_loadbuffer(Luas[luaid],loadaction,strlen(loadaction),"line")||
     lua_pcall(Luas[luaid],0,0,0)) {
    fprintf(stdout,"Error in config file loading: %s", lua_tostring(Luas[luaid],-1));
  }
  free(loadaction);
}



void 
closelua(int n) {
  if (Luas[n] != NULL) {
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

int
callbackdefined (int luaid, char *table,char *name) {  
  if (Luas[luaid] != NULL) {
	lua_getglobal(Luas[luaid],table);
	if (lua_istable(Luas[luaid],-1)) {
	  lua_getfield(Luas[luaid],-1,name);
	  if (lua_isfunction(Luas[luaid],-1)) {
		lua_pop(Luas[luaid],2);
		return 1;
	  } else {
		lua_pop(Luas[luaid],2);
	  }
	} else {
	  lua_pop(Luas[luaid],1);
	}	
  }
  return 0;
}

int
runcallback (int luaid, char *table,char *name, char *values, ...) {
  va_list vl;
  int ret;
  int narg,nres;
  int stacktop;
  FILE **readfile;
  char *s;
  //fprintf(stdout,"\nruncallback called for %s.%s\n",table,name);
  va_start(vl, values);
  if (Luas[luaid] != NULL) {
    stacktop = lua_gettop(Luas[luaid]);
    lua_getglobal(Luas[luaid],table);
    if (lua_istable(Luas[luaid],-1)) {
      lua_getfield(Luas[luaid],-1,name);
      if (lua_isfunction(Luas[luaid],-1)) {
	// <push args>
	for (narg = 0; *values; narg++) {
	  luaL_checkstack(Luas[luaid],1,"out of stack space");
	  switch (*values++) {
	  case 'f': /* FILE * */ 
	    //lua_pushboolean(Luas[luaid], va_arg(vl, int));
	    readfile = (FILE **)lua_newuserdata(Luas[luaid], sizeof(FILE *));
	    *readfile = va_arg(vl, FILE *); 
	    luaL_getmetatable(Luas[luaid], LUA_FILEHANDLE);
	    lua_setmetatable(Luas[luaid], -2);
	    break;
	  case 'c': /* C string */ 
	    s = malloc(2);
	    snprintf(s,2,"%c",va_arg(vl, int));
	    lua_pushstring(Luas[luaid], s);
	    break;
	  case 'S': /* C string */ 
	    s = va_arg(vl, char *);
	    lua_pushstring(Luas[luaid], s);
	    break;
	  case 's': /* TeX string */ 
	    s = makecstring(va_arg(vl, int));
	    lua_pushstring(Luas[luaid], s);
	    break;
	  case 'b': /* boolean */ 
	      lua_pushboolean(Luas[luaid], va_arg(vl, int));
	      break;
	  case '>': 
	    goto ENDARGS;
	  default :
	    ;
	  }
	}
      ENDARGS:
	nres = strlen(values);
	if(lua_pcall(Luas[luaid],narg,nres,0) != 0) {
	  fprintf(stdout,"This went wrong: %s", lua_tostring(Luas[luaid],-1));
	  goto EXIT;
	} else {
	  if (ret) {
	    nres = -nres;
	    while (*values) {
	      switch (*values++) {
	      case 'b': 
		if (!lua_isboolean(Luas[luaid],nres))
		  goto EXIT;
		int b = lua_toboolean(Luas[luaid],nres);
		*va_arg(vl, int *) = b;
		break;
	      case 'f': 
		readfile = (FILE **)lua_touserdata(Luas[luaid], nres);
		if (*readfile == NULL)
		  goto EXIT;
		*va_arg(vl, FILE **) = *readfile;
		break;
	      case 'S': 
		if (!lua_isstring(Luas[luaid],nres))
		  goto EXIT;
		s = (char *)lua_tostring(Luas[luaid],nres);
		if (s==NULL) 
		  *va_arg(vl, int *) = -1;
		else  
		  *va_arg(vl, int *) = maketexstring(s);
                break;
	      default: 
		fprintf(stdout,"invalid value type");
		goto EXIT;
	      }
	      nres++;
	    }
	    lua_settop(Luas[luaid],stacktop); // pop the table
	    return 1;
	  }
	}
      }
    }
  }
 EXIT:
  lua_settop(Luas[luaid],stacktop);
  return 0;
}

