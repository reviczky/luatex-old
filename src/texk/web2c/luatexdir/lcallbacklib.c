/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


static const char *const callbacknames[] = {
  "input_line",
  "open_write_file",
  "open_read_file",
  "show_error_hook",
  NULL };

typedef struct {
  int is_set;
  int lua_id;
} callback_info;

#define NUM_CALLBACKS 4

static callback_info *callback_list;

int 
callback_initialize (void) {
  callback_list = (callback_info *)calloc(NUM_CALLBACKS,sizeof(callback_info));
  if (callback_list == NULL)
    return 0;
  return 1;
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

/* this is only because it has the correct environment */
static int callback_find (lua_State *L) {
  lua_rawget(L,LUA_REGISTRYINDEX);
  return 1;
}

int
runcallback (char *name, char *values, ...) {
  va_list vl;
  int retval;
  int ret, len;
  int narg,nres;
  int stacktop;
  FILE **readfile;
  char *s;
  char *ss = NULL;
  int *bufloc;
  int i;
  int luaid ;
  retval = 0;
  for (i=0; callbacknames[i]; i++)
    if (strcmp(callbacknames[i], name) == 0)
      break;
  if (i==NUM_CALLBACKS)
    return 0;  /* undefined callbacks are never set */
  if (!callback_list[i].is_set)
    return 0;  /* somebody messed up !  */
  luaid = callback_list[i].lua_id;
  if (Luas[luaid] == NULL)
    return 0;
  /* find the callback */
  stacktop = lua_gettop(Luas[luaid]);

  lua_pushcfunction(Luas[luaid],callback_find);
  lua_pushstring(Luas[luaid],name);
  if(lua_pcall(Luas[luaid],1,1,0) != 0) {
    fprintf(stdout,"\nMissing callback_find: %s\n", lua_tostring(Luas[luaid],-1));
    lua_settop(Luas[luaid],stacktop);
    return 0;
   };
  if (!lua_isfunction(Luas[luaid],-1)) {
    fprintf(stdout,"\nMissing callback      : %s\n", lua_tostring(Luas[luaid],-1));
    return 0;
  }
  
  va_start(vl, values);

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
    case CALLBACK_CHARNUM: /* an ascii char! */ 
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
    default: 
      fprintf(stdout,"invalid return value type");
      goto EXIT;
    }
    nres++;
  }
  retval = 1;
 EXIT:
  lua_settop(Luas[luaid],stacktop);
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
    lua_getglobal(L,"luaid");
    if (!lua_isnumber(L,-1))
      goto EXIT;
    callback_list[i].lua_id = lua_tonumber(L,-1);
    callback_list[i].is_set = 1;
    lua_pop(L,1);
  } else {
    callback_list[i].is_set = 0;
  }
  lua_rawset(L,LUA_REGISTRYINDEX);
  return 0;
 EXIT:
  lua_pushnil(L);
  lua_pushstring(L,"Invalid arguments to callback.register");
  return 2;
}


static const struct luaL_reg callbacklib [] = {
  {"find",    callback_find},
  {"register",callback_register},
  {NULL, NULL}  /* sentinel */
};

int luaopen_callback (lua_State *L) 
{
  //  lua_newtable(L);
  //  lua_replace(L, LUA_REGISTRYINDEX);
  luaL_register(L, "callback", callbacklib);
  return 1;
}

