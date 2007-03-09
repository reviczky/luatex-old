/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

extern int callback_callbacks_id;

typedef struct command_item_ {
  char *name;
  int command_offset;
  char **commands;
} command_item;
 

static command_item command_names[] = 
  { { "relax", 0, NULL },
    { "left_brace", 0 , NULL },
    { "right_brace", 0 , NULL },
    { "math_shift", 0 , NULL },
    { "tab_mark", 0 , NULL },
    { "car_ret", 0 , NULL },
    { "mac_param", 0 , NULL },
    { "sup_mark", 0 , NULL },
    { "sub_mark", 0 , NULL },
    { "endv", 0 , NULL },
    { "spacer", 0 , NULL },
    { "letter", 0 , NULL },
    { "other_char", 0 , NULL },
    { "par_end", 0 , NULL },
    { "stop", 0 , NULL },
    { "delim_num", 0 , NULL  },
    { "char_num", 0 , NULL },
    { "math_char_num", 0 , NULL },
    { "mark", 0 , NULL  },
    { "xray", 0 , NULL },
    { "make_box", 0 , NULL },
    { "hmove", 0 , NULL },
    { "vmove", 0 , NULL },
    { "un_hbox", 0 , NULL },
    { "un_vbox", 0 , NULL },
    { "remove_item", 0 , NULL },
    { "hskip", 0 , NULL },
    { "vskip", 0 , NULL },
    { "mskip", 0 , NULL },
    { "kern", 0 , NULL },
    { "mkern", 0 , NULL },
    { "leader_ship", 0 , NULL },
    { "halign", 0 , NULL },
    { "valign", 0 , NULL },
    { "no_align", 0 , NULL  },
    { "vrule", 0 , NULL },
    { "hrule", 0 , NULL },
    { "insert", 0 , NULL },
    { "vadjust", 0 , NULL },
    { "ignore_spaces", 0 , NULL },
    { "after_assignment", 0 , NULL },
    { "after_group", 0 , NULL },
    { "break_penalty", 0 , NULL  },
    { "start_part", 0 , NULL },
    { "ital_corr", 0 , NULL },
    { "accent", 0 , NULL },
    { "math_accent", 0 , NULL },
    { "discretionary", 0 , NULL },
    { "eq_no", 0 , NULL },
    { "left_right", 0 , NULL },
    { "math_comp", 0 , NULL },
    { "limit_switch", 0 , NULL },
    { "above", 0 , NULL },
    { "math_style", 0 , NULL },
    { "math_choice", 0 , NULL },
    { "non_script", 0 , NULL },
    { "vcenter", 0 , NULL },
    { "case_shift", 0 , NULL },
    { "message", 0 , NULL },
    { "extension", 0 , NULL },
    { "in_stream", 0 , NULL },
    { "begin_group", 0 , NULL },
    { "end_group", 0 , NULL },
    { "omit", 0 , NULL },
    { "ex_space", 0 , NULL },
    { "no_boundary", 0 , NULL },
    { "radical", 0 , NULL },
    { "end_cs_name", 0 , NULL },
    { "char_ghost", 0 , NULL },
    { "assign_local_box", 0 , NULL },
    { "char_given", 0 , NULL },
    { "math_given", 0 , NULL },
    { "omath_given", 0 , NULL },
    { "last_item", 0 , NULL },
    { "toks_register", 0 , NULL },
    { "assign_toks", NULL },
    { "assign_int", 0, NULL },
    { "assign_dimen", 0 , NULL },
    { "assign_glue", 0 ,NULL },
    { "assign_mu_glue", 0 , NULL },
    { "assign_font_dimen", 0 , NULL },
    { "assign_font_int", 0 , NULL },
    { "set_aux", 0 , NULL },
    { "set_prev_graf", 0 , NULL },
    { "set_page_dimen", 0 , NULL },
    { "set_page_int", 0 , NULL },
    { "set_box_dimen", 0 , NULL },
    { "set_shape", 0, NULL },
    { "def_code", 0 , NULL },
    { "extdef_code", 0 , NULL },
    { "def_family", 0 , NULL },
    { "set_font", 0 , NULL },
    { "def_font", 0 , NULL },
    { "register", 0 , NULL },
    { "assign_box_dir", 0 , NULL },
    { "assign_dir", 0 , NULL },
    { "advance", 0 , NULL },
    { "multiply", 0 , NULL },
    { "divide", 0 , NULL },
    { "prefix", 0 , NULL },
    { "let", 0 , NULL },
    { "shorthand_def", 0 , NULL },
    { "read_to_cs", 0 , NULL },
    { "def", 0 , NULL },
    { "set_box", 0 , NULL },
    { "hyph_data", 0 , NULL },
    { "set_interaction", 0 , NULL },
    { "letterspace_font", 0 , NULL },
    { "set_ocp", 0 , NULL },
    { "def_ocp", 0 , NULL },
    { "set_ocp_list", 0 , NULL },
    { "def_ocp_list", 0 , NULL },
    { "clear_ocp_lists", 0 , NULL },
    { "push_ocp_list", 0 , NULL },
    { "pop_ocp_list", 0 , NULL },
    { "ocp_list_op", 0 , NULL },
    { "ocp_trace_level", 0 , NULL},
    { "undefined_cs", 0 , NULL },
    { "expand_after", 0 , NULL },
    { "no_expand", 0 , NULL },
    { "input", 0 , NULL },
    { "if_test", 0 , NULL },
    { "fi_or_else", 0 , NULL },
    { "cs_name", 0 , NULL },
    { "convert", 0 , NULL },
    { "the", 0 , NULL  },
    { "top_bot_mark", 0 , NULL },
    { "call", 0 , NULL },
    { "long_call", 0 , NULL },
    { "outer_call", 0 , NULL },
    { "long_outer_call", 0 , NULL },
    { "end_template", 0 , NULL },
    { "dont_expand", 0, NULL },
    { "glue_ref", 0 , NULL },
    { "shape_ref", 0 , NULL },
    { "box_ref", 0 , NULL },
    { "data", 0 , NULL },
    {  NULL, 0, NULL } };



static int
get_cur_cmd (lua_State *L) {
  int i;
  int r = 0;
  char *s;
  lua_getfield(L,-1,"cmd");
  if (lua_isnumber(L,-1)) {
    cur_cmd = lua_tointeger(L,-1);
    r = 1;
  } else if (lua_isstring(L,-1)) {
    s = (char *)lua_tostring(L,-1);
    for (i=0;command_names[i].name != NULL;i++) {
      if (strcmp(s,command_names[i].name) == 0) 
	break;
    }
    if (command_names[i].name!=NULL) {
      cur_cmd = i;
      r = 1;
    }
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

static int
get_cur_cs (lua_State *L) {
  char *s;
  unsigned j,l;
  int save_nncs;
  int ret;
  ret = 0;
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
      cur_cmd = zget_eq_type(cur_cs);
      cur_chr = zget_equiv(cur_cs);
      no_new_control_sequence = save_nncs;
      ret = 1;
    }
  }
  lua_pop(L,1);
  return ret;
}

#define no_token_returned() { lua_pop(L,1);  x_token_needed = 1;  continue; }
#define token_returned()    { lua_pop(L,1);  x_token_needed = 0;  break; }

void
get_x_token_lua (int pmode) {
  integer callback_id;
  char *s;
  char mode[2];
  unsigned j,l,clen;
  int temp_loc;
  lua_State *L;
  int n;
  int x_token_needed = 1;
  callback_id=callback_defined("token_filter");
  if (callback_id>0) {
    L = Luas[0];
    lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
    while (1) {
      lua_rawgeti(L,-1, callback_id);
      if (lua_isfunction(L,-1)) {
	lua_newtable(L);
	if (x_token_needed) {
	  temp_loc = cur_input.loc_field;
	  get_next();
	  mode[1] = 0;  
	  mode[0] = zget_mode_id(pmode);
	  lua_pushstring(L,mode);     lua_setfield(L,-2,"mod");
	  lua_pushstring(L,command_names[cur_cmd].name);  lua_setfield(L,-2,"cmd");
	  lua_pushnumber(L,cur_chr);  lua_setfield(L,-2,"chr");
	  if (cur_cs != 0 && (n = zget_cs_text(cur_cs)) && n>=0) {
	    lua_pushstring(L,makecstring(n));
	  } else {
	    lua_pushstring(L,"");
	  }
	  lua_setfield(L,-2,"cs");
	}
	if (lua_pcall(L,1,1,0) != 0) { /* 1 arg, 1 result */
	  fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
	  lua_pop(L,2);
	  error();
	  return;
	}
	if (lua_istable(L,-1) && 
	    ((get_cur_cmd(L) && get_cur_chr(L)) || get_cur_cs(L))) {
	  if (cur_cmd>get_max_command()) {
	    expand();
	    no_token_returned();
	  } else {
	    token_returned();
	  }
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
