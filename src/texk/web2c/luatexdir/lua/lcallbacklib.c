/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


static const char *const callbacknames[] = {
  "<empty>",
  "find_write_file",
  "find_output_file",
  "find_image_file",
  "find_format_file",
  "find_read_file",      "open_read_file",
  "find_ocp_file",       "read_ocp_file",
  "find_vf_file",        "read_vf_file",
  "find_data_file",      "read_data_file",
  "find_font_file",      "read_font_file",
  "find_map_file",       "read_map_file",
  "find_enc_file",       "read_enc_file",
  "find_type1_file",     "read_type1_file",
  "find_truetype_file",  "read_truetype_file",
  "find_opentype_file",  "read_opentype_file",
  "find_sfd_file",       "read_sfd_file",
  "find_pk_file",        "read_pk_file",
  "show_error_hook",
  "process_input_buffer",
  "start_page_number",  "stop_page_number",
  "start_run",          "stop_run",
  NULL };

typedef struct {
  char *name;
  int number;
  int is_set;
} callback_info;

static int callback_callbacks_id = 0;

#define NUM_CALLBACKS 35

static callback_info *callback_list;

int 
callback_initialize (void) {
  callback_list = (callback_info *)calloc(NUM_CALLBACKS,sizeof(callback_info));
  if (callback_list == NULL)
    return 0;
  return 1;
}

static int 
callback_name_to_id (char *name) {
  int i;
  for (i=1; callbacknames[i]; i++) {
    if (strcmp(callbacknames[i], name) == 0)
      return i;
  }
  return 0;
}


int
callbackdefined (char *name) {  
  int i;
  i = callback_name_to_id(name);
  if (callback_list[i].is_set)
    return i;
  return 0;
}

#define CALLBACK_BOOLEAN        'b'
#define CALLBACK_INTEGER        'd'
#define CALLBACK_LINE           'l'
#define CALLBACK_STRNUMBER      's'
#define CALLBACK_STRING         'S'
#define CALLBACK_CHARNUM        'c'

/*
 *  return    -> callback success?
 *
 *  idstring  -> to be split in a lua id and a table name 
 *  name      -> name of a field in that table
 *  values    -> i/o specification
 *  ...       -> i/o values
 */

int 
runsavedcallback (int r, char *name, char *values, ...) {
  va_list args;
  int ret;
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  va_start(args,values);
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_rawgeti(Luas[0],LUA_REGISTRYINDEX,r);
  lua_getfield(Luas[0],-1,name);
  if (lua_isnil(Luas[0],-1)) {
    ret = 1;
  } else {
    ret = do_run_callback(2,values,args);
  }
  va_end(args);
  lua_settop(Luas[0],stacktop);
  return ret;
}

void 
getluaboolean  (char *table, char *name, int *target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_getglobal(Luas[0],table);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isboolean(Luas[0],-1)) {
      *target = lua_toboolean(Luas[0],-1);
    } else if (lua_isnumber(Luas[0],-1)) {
      *target = (lua_tonumber(Luas[0],-1)==0 ? 0 : 1);
    }
  }
  lua_settop(Luas[0],stacktop);
  return;
}

void 
getsavedluaboolean (int r, char *name, int *target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_rawgeti(Luas[0],LUA_REGISTRYINDEX,r);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isboolean(Luas[0],-1)) {
      *target = lua_toboolean(Luas[0],-1);
    } else if (lua_isnumber(Luas[0],-1)) {
      *target = (lua_tonumber(Luas[0],-1)==0 ? 0 : 1);
    } 
  }
  lua_settop(Luas[0],stacktop);
  return;
}


void 
getluanumber (char *table, char *name, int *target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_getglobal(Luas[0],table);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isnumber(Luas[0],-1)) {
      *target = lua_tonumber(Luas[0],-1);
    }
  }
  lua_settop(Luas[0],stacktop);
  return;
}

void 
getsavedluanumber (int r, char *name, int *target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_rawgeti(Luas[0],LUA_REGISTRYINDEX,r);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isnumber(Luas[0],-1)) {
      *target = lua_tonumber(Luas[0],-1);
    } 
  }
  lua_settop(Luas[0],stacktop);
  return;
}


void 
getluastring (char *table, char *name, char **target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_getglobal(Luas[0],table);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isstring(Luas[0],-1)) {
      *target = (char *)lua_tostring(Luas[0],-1);
    }
  }
  lua_settop(Luas[0],stacktop);
  return;
}

void 
getsavedluastring (int r, char *name, char **target) {
  int stacktop;
  stacktop = lua_gettop(Luas[0]);  
  luaL_checkstack(Luas[0],2,"out of stack space");
  lua_rawgeti(Luas[0],LUA_REGISTRYINDEX,r);
  if (lua_istable(Luas[0],-1)) {
    lua_getfield(Luas[0],-1,name);
    if (lua_isstring(Luas[0],-1)) {
      *target = (char *)lua_tostring(Luas[0],-1);
    } 
  }
  lua_settop(Luas[0],stacktop);
  return;
}

void
destroysavedcallback (int i) {
  luaL_unref(Luas[0],LUA_REGISTRYINDEX,i);
}

/* 
 *  return    -> a lua id and a table name combined to a string
 *
 *  values    -> i/o specification
 *  ...       -> i/o values
 *
 * 
 */

int
runandsavecallback (int i, char *values, ...) {
  va_list args;
  int r;
  char *ret;
  int stacktop;
  lua_State *L;
  L = Luas[0];
  if(i>0 && callback_list[i].is_set) {
    stacktop = lua_gettop(L);
    va_start(args,values);
    luaL_checkstack(L,2,"out of stack space");
    lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
    lua_getfield(L,-1,callbacknames[i]);
    r=do_run_callback(1,values,args);
    va_end(args);
    if (r>0) {
      /* do something to save the ret table here; */
      if(lua_istable(L,-1)) {
	r = luaL_ref(L,LUA_REGISTRYINDEX);
	lua_settop(L,stacktop);
	return r;
      } else if (lua_isnil(L,-1)) {
	lua_settop(L,stacktop);
	return 0;
      } else {
	fprintf(stderr,"Expected a table, not: %s\n", lua_typename(L,lua_type(L,-1)));
      }
    }
    lua_settop(L,stacktop);
  }
  return 0;
}


int
runcallback (int i, char *values, ...) {
  va_list args;
  int ret;
  int stacktop;
  if (i>0 && callback_list[i].is_set) {
    stacktop = lua_gettop(Luas[0]);
    va_start(args,values);
    luaL_checkstack(Luas[0],2,"out of stack space");
    lua_rawgeti(Luas[0],LUA_REGISTRYINDEX,callback_callbacks_id);
    lua_getfield(Luas[0],-1,callbacknames[i]);
    ret = do_run_callback(0,values,args);
    va_end(args);
    lua_settop(Luas[0],stacktop);
    return ret;
  }
  return 0;
}


int
do_run_callback (int special, char *values, va_list vl) {
  int ret, len;
  int narg,nres;
  FILE **readfile;
  char *s;
  char *ss = NULL;
  int *bufloc;
  int i,r;
  int retval = 0;
  lua_State *L;
  L = Luas[0];
  if (!lua_istable(L,-2)) {
    fprintf(stderr,"Expected a table, not: %s\n", lua_typename(L,lua_type(L,-2)));
    return 0;
  }
  if (!lua_isfunction(L,-1)) {
    fprintf(stderr,"Expected a function, not: %s\n", lua_typename(L,lua_type(L,-1)));
    return 0;
  }
  //  va_start(vl, values);
  if (special==2) {
    /* copy the enclosing table */
    luaL_checkstack(L,1,"out of stack space");
    lua_pushvalue(L,-2);
  }
  for (narg = 0; *values; narg++) {
    luaL_checkstack(L,1,"out of stack space");
    switch (*values++) {
    case CALLBACK_CHARNUM: /* an ascii char! */ 
      s = malloc(2);
      snprintf(s,2,"%c",va_arg(vl, int));
      lua_pushstring(L, s);
      break;
    case CALLBACK_STRING: /* C string */ 
      s = va_arg(vl, char *);
      lua_pushstring(L, s);
      break;
    case CALLBACK_INTEGER: /* int */ 
      lua_pushnumber(L, va_arg(vl, int));
      break;
    case CALLBACK_STRNUMBER: /* TeX string */ 
      s = makecstring(va_arg(vl, int));
      lua_pushstring(L, s);
      break;
    case CALLBACK_BOOLEAN: /* boolean */ 
      lua_pushboolean(L, va_arg(vl, int));
      break;
    case CALLBACK_LINE: /* a buffer section, with length 'integer' */ 
	  i = va_arg(vl, int);
	  r = i;
	  ss = xmalloc(i+1);
	  ss[i]=0;
	  while (i-->0) {
		ss[i] = buffer[first+i];
	  }
	  /*fprintf(stderr, "a buffer section at %d of length %d: (%s)\n", first,r,ss);*/
      lua_pushlstring(L, ss, r);
	  free (ss);
      break;
    case '-': 
      narg--;
      break;
    case '>': 
      goto ENDARGS;
    default :
      ;
    }
  }
 ENDARGS:
  nres = strlen(values);
  if (special==1) {
    nres++;
  }
  if (special==2) {
    narg++;
  }
  if(lua_pcall(L,narg,nres,0) != 0) {
    fprintf(stdout,"%s:%s: This went wrong: %s\n", 
	    makecstring(getcurrentfilenamestring()), line, lua_tostring(L,-1));
    goto EXIT;
  };
  if (nres==0) {
    retval=1;
    goto EXIT;
  }
  nres = -nres;
  while (*values) {
    switch (*values++) {
    case CALLBACK_BOOLEAN: 
      if (!lua_isboolean(L,nres)) {
	fprintf(stderr,"Expected a boolean, not: %s\n", lua_typename(L,lua_type(L,nres)));
	goto EXIT;
      }
      int b = lua_toboolean(L,nres);
      *va_arg(vl, int *) = b;
      break;
    case CALLBACK_INTEGER: 
      if (!lua_isnumber(L,nres)) {
	fprintf(stderr,"Expected a number, not: %s\n", lua_typename(L,lua_type(L,nres)));
	goto EXIT;
      }
      b = lua_tonumber(L,nres);
      *va_arg(vl, int *) = b;
      break;
    case CALLBACK_LINE:  /* TeX line */
      if (!lua_isstring(L,nres)) {
		if (!lua_isnil(L,nres))
		  fprintf(stderr,"Expected a string for (l), not: %s\n", lua_typename(L,lua_type(L,nres))); 
		goto EXIT;
      }
      ss = (char *)lua_tolstring(L,nres, &len);
	  if (ss!=NULL) {
		bufloc = va_arg(vl, int *);
		ret = *bufloc;
		s = xmalloc(len+1);
		memcpy(s,ss,len);
		s[len] = 0;
		// len = strlen(s);
		check_buf ((*bufloc) + ret,bufsize);
		while (len--)
		  buffer[(*bufloc)++] = *s++;
		while ((*bufloc)-1>ret && buffer[(*bufloc)-1] == ' ')
		  (*bufloc)--;
      } else {
		bufloc = 0;
	  }
      break;
    case CALLBACK_STRNUMBER:  /* TeX string */
      if (!lua_isstring(L,nres)) {
		if (!lua_isnil(L,nres)) {
		  fprintf(stderr,"Expected a string for (s), not: %s\n", lua_typename(L,lua_type(L,nres)));
		  goto EXIT;
		}
      }
      if (lua_isstring(L,nres))
		s = (char *)lua_tolstring(L,nres,(size_t *)&len);
      else
	s = NULL;
      if (s==NULL || len == 0) 
	*va_arg(vl, int *) = 0;
      else {
	*va_arg(vl, int *) = maketexstring(strdup(s));
      }
      break;
    case CALLBACK_STRING:  /* C string aka buffer */
      if (!lua_isstring(L,nres)) {
	if (!lua_isnil(L,nres)) {
	  fprintf(stderr,"Expected a string for (S), not: %s\n", lua_typename(L,lua_type(L,nres)));
	  goto EXIT;
	}
      }
      if (lua_isstring(L,nres))
	s = (char *)lua_tolstring(L,nres,(size_t *)&len);
      else
	s = NULL;
      if (s==NULL || len == 0) 
	*va_arg(vl, int *) = 0;
      else {
	ss = xmalloc(len+1);
	*(ss+len)=0;
        (void)memcpy(ss,s,len);
	*va_arg(vl, char **) = ss;
      }
      break;
    default: 
      fprintf(stdout,"invalid return value type");
      goto EXIT;
    }
    nres++;
  }
  retval = 1;
 EXIT:
  return retval;
}

static int callback_register (lua_State *L) {
  int i;
  if (!lua_isstring(L,1))
    goto EXIT;
  if ((!lua_isfunction(L,2)) && !lua_isnil(L,2))
    goto EXIT;
  i = callback_name_to_id((char *)lua_tostring(L,1));
  if (i==0)
    goto EXIT;  /* undefined callbacks are never set */
  if (lua_isfunction(L,2)) {
    callback_list[i].is_set = 1;
  } else {
    callback_list[i].is_set = 0;
  }
  luaL_checkstack(L,3,"out of stack space");
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  //lua_getfield(L,LUA_REGISTRYINDEX,"callbacks"); /* 3=table, 2=fucntion 1=name */
  lua_pushvalue(L,1);
  lua_pushvalue(L,2);
  lua_settable(L,-3);
  lua_rawseti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  //  lua_setfield(L,LUA_REGISTRYINDEX,"callbacks");
  lua_pop(L,2);
  lua_pushnumber(L,i);
  return 1;
 EXIT:
  lua_pushnil(L);
  lua_pushstring(L,"Invalid arguments to callback.register");
  return 2;
}


static int callback_listf (lua_State *L) {
  int i;
  luaL_checkstack(L,1,"out of stack space");
  lua_newtable(L);
  for (i=1; callbacknames[i]; i++) {
	luaL_checkstack(L,2,"out of stack space");
	lua_pushstring(L,callbacknames[i]);
	if (callback_list[i].is_set) {
	  lua_pushboolean(L,1);
	} else {
	  lua_pushboolean(L,0);
	}
	lua_rawset(L,-3);
  }
  return 1;
}


static const struct luaL_reg callbacklib [] = {
  //  {"find",    callback_find},
  {"register",callback_register},
  {"list",callback_listf},
  {NULL, NULL}  /* sentinel */
};

int luaopen_callback (lua_State *L) 
{
  luaL_register(L, "callback", callbacklib);
  luaL_checkstack(L,1,"out of stack space");
  lua_newtable(L);
  callback_callbacks_id = luaL_ref(L,LUA_REGISTRYINDEX);
  return 1;
}



