/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


static const char *const callbacknames[] = {
  "<empty>",
  "open_read_file",
  "read_ocp_file",
  "read_vf_file",
  "read_data_file",
  "read_font_file",
  "show_error_hook",
  NULL };

typedef struct {
  int is_set;
  int lua_id;
} callback_info;

#define NUM_CALLBACKS 7

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
  for (i=1; callbacknames[i]; i++)
    if (strcmp(callbacknames[i], name) == 0)
      return i;
  return 0;
}


int
callbackdefined (char *name) {  
  int i;
  for (i=0; callbacknames[i]; i++)
    if (strcmp(callbacknames[i], name) == 0)
      break;
  if (i==NUM_CALLBACKS)
    return 0;  /* undefined callbacks are never set */
  return callback_list[i].is_set;
}

#define CALLBACK_FILE           'f'
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
runsavedcallback (char *idstring, char *name, char *values, ...) {
  va_list args;
  int ret;
  int i;
  int luaid,r;
  char *luaname;
  int stacktop;
  /* split idstring in luaid and luaname */
  luaname = idstring;
  luaid=0; r=0;
  while(isdigit(*luaname)) {
    luaid = luaid*10;
    luaid += (*luaname-'0');
    luaname++;
  }
  luaname++; /* the ! */
  if (Luas[luaid] == NULL)
    return 0;
  stacktop = lua_gettop(Luas[luaid]);  
  va_start(args,values);
  ret = do_run_callback(0,luaid, luaname, name,values,args);
  va_end(args);
  lua_settop(Luas[luaid],stacktop);
  return ret;
}

void
destroysavedcallback (char *l) {
  int luaid,r;
  char *idstring;
  idstring = l;
  while(isdigit(*idstring)) {
    luaid = luaid*10;
    luaid += (*idstring-'0');
    idstring++;
  }
  if (Luas[luaid] == NULL)
    return;
  idstring++; /* the ! */
  r=0;
  while(isdigit(*idstring)) {
    r = r*10;
    r += (*idstring-'0');
    idstring++;
  }
  luaL_unref(Luas[luaid],LUA_REGISTRYINDEX,r);
}

/* 
 *  return    -> a lua id and a table name combined to a string
 *
 *  values    -> i/o specification
 *  ...       -> i/o values
 *
 * 
 */

unsigned char *
runandsavecallback (char *name, char *values, ...) {
  va_list args;
  int r,i;
  char *ret;
  int luaid;
  int stacktop;
  i = callback_name_to_id(name);
  if(i>0) {
    if (callback_list[i].is_set) {
      luaid = callback_list[i].lua_id;
      if (Luas[luaid] == NULL)
	return (unsigned char *)0;
      stacktop = lua_gettop(Luas[luaid]);
      va_start(args,values);
      r=do_run_callback(1,luaid,"callbacks", name,values,args);
      va_end(args);
      if (r==0) {
	lua_settop(Luas[luaid],stacktop);
	return (unsigned char *)0;
      }
      /* do something to save the ret table here; */
      if(lua_istable(Luas[luaid],-1)) {
	r = luaL_ref(Luas[luaid],LUA_REGISTRYINDEX);

	ret = (char *)xmalloc(30);
	snprintf(ret,30,"%d!%d",luaid,r);

	return (unsigned char *)ret;
      } else {
	fprintf(stderr,"NOT A CALLBACK\n");
      }
    }
  }
  return (unsigned char *)0;
}


int
runcallback (char *name, char *values, ...) {
  va_list args;
  int ret;
  int luaid;
  int i;
  int stacktop;
  i=callback_name_to_id(name);
  if (i>0) {
    if (callback_list[i].is_set) {
      luaid = callback_list[i].lua_id;
      if (Luas[luaid] == NULL)
	return 0;
      stacktop = lua_gettop(Luas[luaid]);
      va_start(args,values);
      ret = do_run_callback(0,luaid, "callbacks", name,values,args);
      va_end(args);
      lua_settop(Luas[luaid],stacktop);
      return ret;
    }
  }
  return 0;
}


int
do_run_callback (int special, int luaid, char *tabname, char *name, char *values, va_list vl) {
  int retval;
  int ret, len;
  int narg,nres;
  FILE **readfile;
  char *s;
  char *ss = NULL;
  int *bufloc;
  int i,r;
  retval = 0;
  /* find the callback */
  if (strcmp(tabname,"callbacks")==0)
    lua_getfield(Luas[luaid],LUA_REGISTRYINDEX,tabname);
  else {
    r=0;
    s =tabname;
    while(isdigit(*s)) {
      r=r*10;
      r+=*s-'0';
      s++;
    }
    lua_rawgeti(Luas[luaid],LUA_REGISTRYINDEX,r);
  }
  if (!lua_istable(Luas[luaid],-1)) {
    fprintf(stderr,"\nNOT A TABLE\n");
  }
  lua_getfield(Luas[luaid],-1,name);
  if (!lua_isfunction(Luas[luaid],-1)) {
    fprintf(stdout,"\nMissing callback      : %s\n", lua_tostring(Luas[luaid],-1));
    return 0;
  }
  //  va_start(vl, values);
  for (narg = 0; *values; narg++) {
    luaL_checkstack(Luas[luaid],1,"out of stack space");
    switch (*values++) {
    case CALLBACK_FILE: /* FILE * */ 
      readfile = (FILE **)lua_newuserdata(Luas[luaid], sizeof(FILE *));
      *readfile = va_arg(vl, FILE *); 
      luaL_getmetatable(Luas[luaid], LUA_TEXFILEHANDLE);
      lua_setmetatable(Luas[luaid], -2);
      break;
    case CALLBACK_CHARNUM: /* an ascii char! */ 
      s = malloc(2);
      snprintf(s,2,"%c",va_arg(vl, int));
      lua_pushstring(Luas[luaid], s);
      break;
    case CALLBACK_STRING: /* C string */ 
      s = strdup(va_arg(vl, char *));
      lua_pushstring(Luas[luaid], s);
      break;
    case CALLBACK_INTEGER: /* int */ 
      lua_pushnumber(Luas[luaid], va_arg(vl, int));
      break;
    case CALLBACK_STRNUMBER: /* TeX string */ 
      s = makecstring(va_arg(vl, int));
      lua_pushstring(Luas[luaid], s);
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
  if (special) 
    nres++;
  if(lua_pcall(Luas[luaid],narg,nres,0) != 0) {
    fprintf(stdout,"This went wrong: %s\n", lua_tostring(Luas[luaid],-1));
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
    case CALLBACK_STRING:  /* C string aka buffer */
      if (!lua_isstring(Luas[luaid],nres))
	goto EXIT;
      s = (char *)lua_tolstring(Luas[luaid],nres,(size_t *)&len);
      if (s==NULL || len == 0) 
	*va_arg(vl, int *) = 0;
      else {
	//puts(s);
	ss = xmalloc((len-1));
        (void)memcpy(ss,s,(len-1));
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
  const char *st;
  if (!lua_isstring(L,1))
    goto EXIT;
  if ((!lua_isfunction(L,2)) && !lua_isnil(L,2))
    goto EXIT;
  st = lua_tostring(L,1);
  for (i=0; callbacknames[i]; i++)
    if (strcmp(callbacknames[i], st) == 0)
      break;
  if (i==NUM_CALLBACKS)
    goto EXIT;  /* undefined callbacks are never set */
  if (lua_isfunction(L,2)) {
    lua_getglobal(L,"lua");
    lua_getfield(L,-1,"id");
    if (!lua_isnumber(L,-1)) {
      lua_pop(L,2);
      goto EXIT;
    }
    callback_list[i].lua_id = lua_tonumber(L,-1);
    callback_list[i].is_set = 1;
    lua_pop(L,2);
  } else {
    callback_list[i].is_set = 0;
  }
  lua_getfield(L,LUA_REGISTRYINDEX,"callbacks"); /* 3=table, 2=fucntion 1=name */
  lua_pushvalue(L,1);
  lua_pushvalue(L,2);
  lua_settable(L,-3);
  lua_setfield(L,LUA_REGISTRYINDEX,"callbacks");
  lua_pop(L,2);
  return 0;
 EXIT:
  lua_pushnil(L);
  lua_pushstring(L,"Invalid arguments to callback.register");
  return 2;
}


static const struct luaL_reg callbacklib [] = {
  //  {"find",    callback_find},
  {"register",callback_register},
  {NULL, NULL}  /* sentinel */
};

int luaopen_callback (lua_State *L) 
{
  //  lua_newtable(L);
  //  lua_replace(L, LUA_REGISTRYINDEX);
  luaL_register(L, "callback", callbacklib);
  lua_newtable(L);
  lua_setfield(L,LUA_REGISTRYINDEX,"callbacks");
  return 1;
}

/* this is how we know there is a pending callback */

unsigned char **input_file_callback_ids = NULL;
unsigned char **read_file_callback_ids = NULL;

void 
initfilecallbackids (int max) {
  int k;
  input_file_callback_ids = (unsigned char **)xmalloc(sizeof(unsigned char *)*(max+1));
  read_file_callback_ids  = (unsigned char **)xmalloc(sizeof(unsigned char *)*(max+1));
  for (k=0;k<=max;k++) {
    input_file_callback_ids[k]= NULL;
    read_file_callback_ids[k]= NULL;
  }
}

unsigned char*
getinputfilecallbackid (int n) {  
  return input_file_callback_ids[n]; 
}

void 
setinputfilecallbackid (int n, unsigned char *val) {
  input_file_callback_ids[n]=val; 
}

unsigned char *
getreadfilecallbackid (int n) {
  return read_file_callback_ids[n]; 
}

void 
setreadfilecallbackid (int n, unsigned char *val) {
  read_file_callback_ids[n]=val; 
}


