/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>


extern int get_command_id (char *);

static int max_command = 0;
static int hash_base = 0;
static int active_base = 0;
static int null_cs = 0;

static int 
is_valid_token (lua_State *L, int i) {
  if (lua_istable(L,i)) {
    return 1;
  } else {
    lua_pushnil(L);
    return 0;
  }
}

static void 
get_token_cmd(lua_State *L, int i) {
  lua_rawgeti(L,i,1);
}

static void 
get_token_chr(lua_State *L, int i) {
  lua_rawgeti(L,i,2);
}

static void 
get_token_cs(lua_State *L, int i) {
  lua_rawgeti(L,i,3);
}


static int 
test_expandable (lua_State *L) {
  integer cmd = -1;
  if (is_valid_token(L,-1)) {
    get_token_cmd(L,-1);
    if (lua_isnumber(L,-1)) {
      cmd = lua_tointeger(L,-1);
    } else if (lua_isstring(L,-1)) {
      cmd = get_command_id((char *)lua_tostring(L,-1));
    }
    if (cmd>max_command) {
      lua_pushboolean(L,1);
    } else {
      lua_pushboolean(L,0);
    }
  }
  return 1;
}

static int 
test_activechar (lua_State *L) {
  integer cmd = -1;
  if (is_valid_token(L,-1)) {
    get_token_cs(L,-1);
    if (lua_isnumber(L,-1)) {
      cmd = lua_tointeger(L,-1);
    }
    if (cmd>0 && cmd<hash_base && cmd<null_cs) {
      lua_pushboolean(L,1);
    } else {
      lua_pushboolean(L,0);
    }
  }
  return 1;
}


static int
run_get_command_name (lua_State *L) {
  int cs;
  if (is_valid_token(L,-1)) {
    get_token_cmd(L,-1);
    if (lua_isnumber(L,-1)) {
      cs = lua_tointeger(L,-1);
      lua_pushstring(L,command_names[cs].name);
    } else {
      lua_pushstring(L,"");
    }
  }
  return 1;
}

static int
run_get_csname_name (lua_State *L) {
  int cs,n;
  if (is_valid_token(L,-1)) {
    get_token_cs(L,-1);
    if (lua_isnumber(L,-1)) {
      cs = lua_tointeger(L,-1);
      if (cs != 0 && (n = zget_cs_text(cs)) && n>=0) {
	lua_pushstring(L,makecstring(n));
      } else {
	lua_pushstring(L,"");
      }
    }
  }
  return 1;
}

static int
run_get_command_id (lua_State *L) {
  int cs = -1;
  if (lua_isstring(L,-1)) {
    cs = get_command_id((char *)lua_tostring(L,-1));
  }
  lua_pushnumber(L,cs);
  return 1;
}


static int
run_get_csname_id (lua_State *L) {
  int texstr;
  int cs = 0;
  if (lua_isstring(L,-1)) {
    texstr = maketexstring(lua_tostring(L,-1));
    cs = string_lookup(texstr);
    flush_str(texstr);
  }
  lua_pushnumber(L,cs);
  return 1;
}


static void 
make_token_table (lua_State *L, int cmd, int chr, int cs) {
  lua_newtable(L);
  lua_pushnumber(L,cmd);
  lua_rawseti(L,-2,1);
  lua_pushnumber(L,chr);
  lua_rawseti(L,-2,2);
  lua_pushnumber(L,cs);
  lua_rawseti(L,-2,3);
}

static int 
run_get_next (lua_State *L) {
  /*int saved_align_state;*/
  /*saved_align_state = align_state;*/
  get_next();
  make_token_table(L,cur_cmd,cur_chr,cur_cs);
  /*align_state = saved_align_state;*/
  return 1;
}

static int 
run_expand (lua_State *L) {
  expand();
  return 0;
}


static int 
run_lookup (lua_State *L) {
  char *s;
  unsigned l;
  str_number t;
  integer cs,cmd,chr;
  int save_nncs;
  if (lua_isstring(L,-1)) {
    s = (char *)lua_tolstring(L,-1,&l);
    if (l>0) {
      save_nncs = no_new_control_sequence;
      no_new_control_sequence = true;
      cs = id_lookup((last+1),l);
      t = maketexstring(s);
      cs = string_lookup(t);
      flush_str(t);
      cmd = zget_eq_type(cs);
      chr = zget_equiv(cs);
      make_token_table(L,cmd,chr,cs);
      no_new_control_sequence = save_nncs;
      return 1;
    }
  }
  lua_newtable(L);
  return 1;
}

static int 
run_build (lua_State *L) {
  integer cmd,chr,cs;
  if (lua_isnumber(L,1)) {
    cs =0;
    chr = lua_tointeger(L,1);
    cmd = luaL_optinteger(L,2,zget_char_cat_code(chr));
    if (cmd==0 || cmd == 9 || cmd == 14 || cmd == 15) {
      fprintf(stdout, "\n\nluatex error: not a good token.\nCatcode %i can not be returned, so I replaced it by 12 (other)",cmd);
      error();
      cmd=12;
    }
    if (cmd == 13) {
       cs=chr+active_base;
       cmd=zget_eq_type(cs); 
       chr=zget_equiv(cs);
    }
    make_token_table(L,cmd,chr,cs);
    return 1;
  } else {
    return run_lookup(L);
  }
}


static const struct luaL_reg tokenlib [] = {
  {"get_next",     run_get_next},
  {"expand",       run_expand},
  {"lookup",       run_lookup},
  {"create",       run_build},
  {"is_expandable",test_expandable},
  {"is_activechar",test_activechar},
  {"csname_id",    run_get_csname_id},
  {"csname_name",  run_get_csname_name},
  {"command_name", run_get_command_name},
  {"command_id",   run_get_command_id},
  {NULL, NULL}  /* sentinel */
};

int luaopen_token (lua_State *L) 
{
  luaL_register(L, "token", tokenlib);
  max_command  = get_max_command();
  hash_base    = get_hash_base();
  active_base  = get_active_base();
  null_cs      = get_nullcs();
  return 1;
}

