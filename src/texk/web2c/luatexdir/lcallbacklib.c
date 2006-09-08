/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


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

#define CALLBACK_FILE           'f'
#define CALLBACK_BOOLEAN        'b'
#define CALLBACK_INTEGER        'd'
#define CALLBACK_LINE           'l'
#define CALLBACK_STRNUMBER      's'
#define CALLBACK_STRING         'S'
#define CALLBACK_CHARNUM        'c'


int
runcallback (int luaid, char *table,char *name, char *values, ...) {
  va_list vl;
  int ret, len;
  int narg,nres;
  int stacktop;
  FILE **readfile;
  char *s;
  char *ss;
  int *bufloc;
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
	  case CALLBACK_FILE: /* FILE * */ 
	    //lua_pushboolean(Luas[luaid], va_arg(vl, int));
	    readfile = (FILE **)lua_newuserdata(Luas[luaid], sizeof(FILE *));
	    *readfile = va_arg(vl, FILE *); 
	    luaL_getmetatable(Luas[luaid], LUA_TEXFILEHANDLE);
	    lua_setmetatable(Luas[luaid], -2);
	    break;
	  case CALLBACK_CHARNUM: /* C string */ 
	    s = malloc(2);
	    snprintf(s,2,"%c",va_arg(vl, int));
	    lua_pushstring(Luas[luaid], s);
	    break;
	  case CALLBACK_STRING: /* C string */ 
	    s = va_arg(vl, char *);
	    lua_pushstring(Luas[luaid], strdup(s));
	    break;
	  case CALLBACK_INTEGER: /* int */ 
	    lua_pushnumber(Luas[luaid], va_arg(vl, int));
	    break;
	  case CALLBACK_STRNUMBER: /* TeX string */ 
	    s = makecstring(va_arg(vl, int));
	    lua_pushstring(Luas[luaid], strdup(s));
	    break;
	  case CALLBACK_BOOLEAN: /* boolean */ 
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
	};
	nres = -nres;
	while (*values) {
	  switch (*values++) {
	  case CALLBACK_BOOLEAN: 
	    if (!lua_isboolean(Luas[luaid],nres))
	      goto EXIT;
	    int b = lua_toboolean(Luas[luaid],nres);
	    *va_arg(vl, int *) = b;
	    break;
	  case CALLBACK_INTEGER: 
	    if (!lua_isnumber(Luas[luaid],nres))
	      goto EXIT;
	    b = lua_tonumber(Luas[luaid],nres);
	    *va_arg(vl, int *) = b;
	    break;
	  case CALLBACK_FILE: 
	    readfile = (FILE **)lua_touserdata(Luas[luaid], nres);
	    if (readfile == NULL)
	      goto EXIT;
	    *va_arg(vl, FILE **) = *readfile;
	    break;
	  case CALLBACK_LINE:  /* TeX line */
	    if (!lua_isstring(Luas[luaid],nres))
	      goto EXIT;
	    ss = (char *)lua_tostring(Luas[luaid],nres);
	    if (ss!=NULL) {
	      s = strdup(ss);
	      bufloc = va_arg(vl, int *);
	      ret = *bufloc;
	      len = strlen(s);
	      check_buf ((*bufloc) + ret,bufsize);
	      while (len--)
		buffer[(*bufloc)++] = *s++;
	      while ((*bufloc)-1>ret && buffer[(*bufloc)-1] == ' ')
		(*bufloc)--;
	    }
	    break;
	  case CALLBACK_STRNUMBER:  /* TeX string */
	    if (!lua_isstring(Luas[luaid],nres))
	      goto EXIT;
	    s = (char *)lua_tostring(Luas[luaid],nres);
	    if (s==NULL) 
	      *va_arg(vl, int *) = -1;
	    else {
	      //puts(s);
	      *va_arg(vl, int *) = maketexstring(strdup(s));
	    }
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
  EXIT:
    lua_settop(Luas[luaid],stacktop);
  }
  return 0;
}

