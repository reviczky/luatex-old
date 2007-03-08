/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

int x_token_needed = 1;

extern int callback_callbacks_id;

static int
get_cur_cmd (lua_State *L) {
  int r = 0;
  lua_getfield(L,-1,"cmd");
  if (lua_isnumber(L,-1)) {
	cur_cmd = lua_tointeger(L,-1);
	r = 1;
  }
  lua_pop(L,1);
  return r;
}

static int
get_cur_chr (lua_State *L) {
  int r = 0;
  lua_getfield(L,-1,"chr");
  if (lua_isnumber(L,-1)) {
	cur_chr = lua_tointeger(L,-1);
	r = 1;
  }
  lua_pop(L,1);
  return r;
}

static void
get_cur_cs (lua_State *L) {
  char *s;
  unsigned j,l;
  int save_nncs;
  cur_cs = 0;
  lua_getfield(L,-1,"cs");
  if (lua_isstring(L,-1)) {
	s = (char *)lua_tolstring(L,-1,&l);
	if (l>0) {
	  if (last+l>buf_size)
		overflow("buffer size",buf_size);
	  for (j=0;j<l;j++) {
		buffer[last+1+j]=*s++;
	  }
	  save_nncs = no_new_control_sequence;
	  no_new_control_sequence = true;			  
	  cur_cs = id_lookup((last+1),l);
	  no_new_control_sequence = save_nncs;
	}
  }
  lua_pop(L,1);
  /* this would set cur_tok and expands macros */ 
  x_token;
}

#define no_token_returned() { lua_pop(L,1);  x_token_needed = 1;  continue; }
#define token_returned()    { lua_pop(L,1);  x_token_needed = 0;  break; }

void
get_x_token_lua (int pmode) {
  integer callback_id;
  char *s;
  char mode[2];
  unsigned j,l;
  lua_State *L;
  callback_id=callback_defined("token_filter");
  if (callback_id>0) {
	L = Luas[0];
	lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
	while (1) {
	  lua_rawgeti(L,-1, callback_id);
	  if (lua_isfunction(L,-1)) {
		lua_newtable(L);
		if (x_token_needed) {
		  get_x_token();
		  lua_pushnumber(L,cur_cmd);  lua_setfield(L,-2,"cmd");
		  lua_pushnumber(L,cur_chr);  lua_setfield(L,-2,"chr");
		  mode[1] = 0;
		  mode[0] = zget_mode_id(pmode);
		  lua_pushstring(L,mode);  lua_setfield(L,-2,"mod");
		  if (cur_cs != 0) {
			s = makecstring(zget_cs_text(cur_cs));
			lua_pushstring(L,s);
		  } else {
			lua_pushstring(L,"");
		  }
		  lua_setfield(L,-2,"cs");
		}
		if (lua_pcall(L,1,1,0) != 0) { /* 1 arg, 1 result */
		  lua_pop(L,2);
		  error();
		  return;
		}
		if (lua_istable(L,-1) && get_cur_cmd(L) && get_cur_chr(L)) {
		  get_cur_cs(L);
		  token_returned();
		} else {
		  no_token_returned();
		}
	  } else {
		lua_pop(L,2); /* the not-a-function callback and the container */
		get_x_token();
		return;
	  }
	}
	lua_pop(L,1); /* callback container */
  } else {
	get_x_token();
  }
}
