/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>

#include "nodes.h"

#define NODE_METATABLE "luatex.node"

#define check_isnode(L,b) (halfword *)luaL_checkudata(L,b,NODE_METATABLE)

/* This routine finds the numerical value of a string (or number) at
  lua stack index |n|. If it is not a valid node type, raises a lua
  error and returns -1 */

static 
int get_node_type_id (lua_State *L, int n) {
  char *s;
  int i = -1;
  if (lua_type(L,n)==LUA_TSTRING) {
    s = (char *)lua_tostring(L,n);
    for (i=0;node_names[i]!=NULL;i++) {
      if (strcmp(s,node_names[i])==0)
      break;
    }
    if (node_names[i]==NULL)
      i=-1;
  } else if (lua_isnumber(L,n)) {
    i = lua_tonumber(L,n);
    /* do some test here as well !*/
  }
  if (i==-1) {
    lua_pushstring(L, "Invalid node type");
    lua_error(L);
  }
  return i;
}

/* Same, but for whatsits. */

static 
int get_node_subtype_id (lua_State *L, int n) {
  char *s;
  int i = -1;
  if (lua_type(L,n)==LUA_TSTRING) {
    s = (char *)lua_tostring(L,n);
    for (i=0;whatsit_node_names[i]!=NULL;i++) {
      if (strcmp(s,whatsit_node_names[i])==0)
	break;
    }
    if (whatsit_node_names[i]==NULL)
      i=-1;
  } else if (lua_isnumber(L,n)) {
    i = lua_tonumber(L,n);
    /* do some test here as well !*/
  }
  if (i==-1) {
    lua_pushstring(L, "Invalid whatsit type");
    lua_error(L);
  }
  return i;
}

/* Creates a userdata object for a number found at the stack top, 
  if it is representing a node (i.e. an pointer into |varmem|). 
  It replaces the stack entry with the new userdata, or pushes
  |nil| if the number is |null|, or if the index is definately out of
  range. This test could be improved.
*/

static void
lua_nodelib_push(lua_State *L) {
  halfword n;
  halfword *a;
  n = -1;
  if (lua_isnumber(L,-1)) {
    n = lua_tointeger(L,-1);
  }
  lua_pop(L,1);
  if ((n==null) || (n<0) || (n>var_mem_max)) {
    lua_pushnil(L);
  } else {
    a = lua_newuserdata(L, sizeof(halfword));
    *a = n;
    luaL_getmetatable(L,NODE_METATABLE);
    lua_setmetatable(L,-2);
  }
  return;
}

/* converts type strings to type ids */

static int
lua_nodelib_id(lua_State *L) {
  integer i;
  i = get_node_type_id(L,1);
  if (i>=0) {
    lua_pushnumber(L,i);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

/* allocate a new whatsit */

static int
lua_nodelib_new_whatsit(int j) {
  halfword p  = null;
  switch (j) {
  case open_node:
    p = get_node(open_node_size);  
    write_stream(p)=0;
    open_name(p) = get_nullstr();
    open_area(p) = open_name(p);
    open_ext(p) = open_name(p);
    break;
  case write_node:
    p = get_node(write_node_size);
    write_stream(p) = null;
    write_tokens(p) = null;
    break;
  case close_node:
    p = get_node(close_node_size);
    write_stream(p) = null;
    break;
  case special_node:
    p = get_node(write_node_size);
    write_stream(p) = null;
    write_tokens(p) = null;
    break;
  case language_node:
    p = get_node(language_node_size);
    what_lang(p) = 0;
    what_lhm(p) = 0;
    what_rhm(p) = 0;
    break;
  case local_par_node:
    p =get_node(local_par_size);
    local_pen_inter(p) = 0;
    local_pen_broken(p) = 0;
    local_par_dir(p) = 0;
    local_box_left(p) = null;
    local_box_left_width(p) = 0;
    local_box_right(p) = null;
    local_box_right_width(p) = 0;
    break;
  case dir_node:
    p = get_node(dir_node_size);
    dir_dir(p) = 0;
    dir_level(p) = 0;
    dir_dvi_ptr(p) = 0;
    dir_dvi_h(p) = 0;
    break;
  case pdf_literal_node: 
    p = get_node(write_node_size);
    pdf_literal_mode(p) = 0;
    pdf_literal_data(p) = null;
    break;
  case pdf_refobj_node:
    p =get_node(pdf_refobj_node_size);
    pdf_obj_objnum(p) = 0;
    break;
  case pdf_refxform_node:
    p =get_node(pdf_refxform_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_xform_objnum(p) = 0;
    break;
  case pdf_refximage_node:
    p =get_node(pdf_refximage_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_ximage_objnum(p) = 0;
    break;
  case pdf_annot_node:
    p =get_node(pdf_annot_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_annot_objnum(p) = 0;
    pdf_annot_data(p) = null;
    break;
  case pdf_start_link_node:
    p =get_node(pdf_annot_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_link_objnum(p) = 0;
    pdf_link_attr(p) = null;
    pdf_link_action(p) = null;
    break;
  case pdf_end_link_node:
    p =get_node(pdf_end_link_node_size);
    break;
  case pdf_dest_node:
    p =get_node(pdf_dest_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_dest_named_id(p) = 0;
    pdf_dest_id(p) = 0;
    pdf_dest_type(p) = 0;
    pdf_dest_xyz_zoom(p) = 0;
    pdf_dest_objnum(p) = 0;
    break;
  case pdf_thread_node:
  case pdf_start_thread_node:
    p =get_node(pdf_thread_node_size);
    pdf_width(p) = 0;
    pdf_height(p) = 0;
    pdf_depth(p) = 0;
    pdf_thread_named_id(p) = 0;
    pdf_thread_id(p) = 0;
    pdf_thread_attr(p) = null;
    break;
  case pdf_end_thread_node:
    p =get_node(pdf_end_thread_node_size);
    break;
  case pdf_save_pos_node:
    p =get_node(pdf_save_pos_node_size);
    break;
  case pdf_snap_ref_point_node:
    p =get_node(pdf_snap_ref_point_node_size);
    break;
  case pdf_snapy_node:
    p =get_node(snap_node_size);
    final_skip(p) = 0;
    snap_glue_ptr(p) = null;
    break;
  case pdf_snapy_comp_node:
    p =get_node(snap_node_size);
    snapy_comp_ratio(p) = 0;
    break;
  case late_lua_node:
    p =get_node(write_node_size);
    late_lua_reg(p) = 0;
    late_lua_data(p) = null;
    break;
  case close_lua_node:
    p =get_node(write_node_size);
    late_lua_reg(p) = 0;
    break;
  case pdf_colorstack_node:
    p =get_node(pdf_colorstack_node_size);
    pdf_colorstack_stack(p) = 0;
    pdf_colorstack_cmd(p) = 0;
    pdf_colorstack_data(p) = null;
    break;
  case pdf_setmatrix_node:
    p =get_node(pdf_setmatrix_node_size);
    pdf_setmatrix_data(p) = null;
    break;
  case pdf_save_node:
    p =get_node(pdf_save_node_size);
    break;
  case pdf_restore_node:
    p =get_node(pdf_restore_node_size);
    break;
  case user_defined_node:
    p = get_node(user_defined_node_size);
    user_node_id(p) = 0;
    user_node_type(p) = 0;
    user_node_value(p)= null;
    break;
  default:
    fprintf(stdout,"<unknown whatsit type %d>\n",j);
  }
  subtype(p) = j;
  return p;
}

static int
lua_nodelib_new(lua_State *L) {
  integer i,j;
  char *s;
  halfword *a;
  halfword n  = null;
  i = get_node_type_id(L,1);
  j = -1;
  /* the lua type test has to be really careful because if the
     main node type is whatsit, the subtype could be a string */
  if ((lua_gettop(L)>1) && (lua_type(L,2) == LUA_TNUMBER)) {
    j = lua_tointeger(L,2);
  }
  if (i<0)
    return 0;
  switch (i) {
  case hlist_node:
  case vlist_node:
	n = bare_null_box(); 
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case rule_node:
	n = bare_rule();
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case ins_node:
	n = get_node(ins_node_size); 
	float_cost(n)=0; height(n)=0; depth(n)=0;
	ins_ptr(n)=null; split_top_ptr(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case mark_node: 
	n = get_node(mark_node_size);  
	mark_ptr(n)=null;
	mark_class(n)=0;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case adjust_node: 
	n = get_node(adjust_node_size); 
	adjust_ptr(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case disc_node: 
	n = get_node(disc_node_size);  
	replace_count(n)=0;
	pre_break(n)=null;
	post_break(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case math_node: 
	n = get_node(math_node_size);  
	node_attr(n) = null;
	surround(n) = 0;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case glue_node: 
	n = get_node(glue_node_size);  
	glue_ptr(n)=null;
	leader_ptr(n)=null;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case glue_spec_node: 
	n = get_node(glue_spec_size);  
	glue_ref_count(n)=null;
	width(n)=0; stretch(n)=0; shrink(n)=0;
	stretch_order(n)=normal; shrink_order(n)=normal;
	type(n)=i;	subtype(n)=0;
	break;
  case kern_node: 
	n = get_node(kern_node_size);  
	width(n)=null;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case penalty_node: 
	n = get_node(penalty_node_size); 
	penalty(n)=null;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
	break;
  case glyph_node:          
	n = get_node(glyph_node_size); 
	node_attr(n) = null; 
	lig_ptr(n) = null; 
	character(n) = 0;
	font(n) = 0;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case margin_kern_node:          
	n = get_node(margin_kern_node_size); 
	margin_char(n) = null; 
	width(n) = 0;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
	if (j>=0) subtype(n)=j;
	break;
  case unset_node:        
	n = get_node(box_node_size); 
	node_attr(n)=null;
	type(n)=i;  span_count(n)=0;
	width(n) = 0;
	height(n) = 0;
	depth(n) = 0 ;	
	list_ptr(n) = null;
	glue_shrink(n) = 0;
	glue_stretch(n) = 0;
	glue_order(n)=normal;
	glue_sign(n)=normal;
	box_dir(n) = 0;
	if (j>=0) subtype(n)=j;
	break; 
  case whatsit_node:        
    if (lua_gettop(L)>1) {
      j = get_node_subtype_id(L,2);
    }
    if (j>=0) {
      n = lua_nodelib_new_whatsit(j);
      type(n)=i;
    } else {
      lua_pushstring(L, "Creating a whatsit requires the subtype number as a second argument");
      lua_error(L);
    }
    break;
  default: 
    fprintf(stdout,"<node type %s not supported yet>\n",node_names[i]);
    break;
  }  
  lua_pushnumber(L,n);
  lua_nodelib_push(L);
  return 1;
}

/* Free a node.
   This function returns the 'next' node, because that may be helpful */

static int
lua_nodelib_free(lua_State *L) {
  halfword *n;
  halfword p;
  n = check_isnode(L,-1);
  /* this easier than figuring out the correct size */ 
  p = vlink(*n); vlink(*n) = null;
  flush_node_list(*n); 
  lua_pushnumber(L,p);
  lua_nodelib_push(L);
  return 1;
}

/* Free a node list */

static int
lua_nodelib_flush_list(lua_State *L) {
  halfword *n_ptr = check_isnode(L,-1);
  flush_node_list(*n_ptr); 
  return 0;
}

/* Copy a node list */

static int
lua_nodelib_copy_list (lua_State *L) {
  halfword *n = check_isnode(L,-1);
  halfword m = copy_node_list(*n);
  lua_pushnumber(L,m);
  lua_nodelib_push(L);
  return 1;
}

/* (Deep) copy a node */

static int
lua_nodelib_copy(lua_State *L) {
  halfword *n;
  halfword m,p;
  n = check_isnode(L,-1);
  p = vlink(*n); vlink(*n) = null;
  m = copy_node_list(*n);
  vlink(*n) = p;
  lua_pushnumber(L,m);
  lua_nodelib_push(L);
  return 1;
}

/* A whole set of static data for field information */

static char * node_fields_list    [] =  { "next", "type", "subtype", "attr", "width", "depth", "height", "shift", "list", 
                                          "glue_order", "glue_sign", "glue_set" , "dir" ,  NULL };
static char * node_fields_rule     [] = { "next", "type", "subtupe", "attr", "width", "depth", "height", "dir", NULL };
static char * node_fields_insert   [] = { "next", "type", "subtype", "attr", "cost",  "depth", "height", "top_skip", "insert", NULL };
static char * node_fields_mark     [] = { "next", "type", "subtype", "attr", "class", "mark", NULL }; 
static char * node_fields_adjust   [] = { "next", "type", "subtype", "attr", "list", NULL }; 
static char * node_fields_disc     [] = { "next", "type", "replace", "attr", "pre", "post", "subtype", NULL };
/*static char * node_fields_whatsit  [] = { "next", "type", NULL };*/
static char * node_fields_math     [] = { "next", "type", "subtype", "attr", "surround", NULL }; 
static char * node_fields_glue     [] = { "next", "type", "subtype", "attr", "spec", "leader", NULL }; 
static char * node_fields_kern     [] = { "next", "type", "subtype", "attr", "width", NULL };
static char * node_fields_penalty  [] = { "next", "type", "subtype", "attr", "penalty", NULL };
static char * node_fields_unset    [] = { "next", "type", "subtype", "attr", "width", "depth", "height", "shrink", "list", 
                                          "glue_order", "glue_sign", "stretch" , "dir" , "span",  NULL };
static char * node_fields_style    [] = { "next", "type", "subtype", NULL };
static char * node_fields_choice   [] = { "next", "type", "subtype", NULL };
static char * node_fields_ord      [] = { "next", "type", "subtype", NULL };
static char * node_fields_op       [] = { "next", "type", "subtype", NULL };
static char * node_fields_bin      [] = { "next", "type", "subtype", NULL };
static char * node_fields_rel      [] = { "next", "type", "subtype", NULL };
static char * node_fields_open     [] = { "next", "type", "subtype", NULL };
static char * node_fields_close    [] = { "next", "type", "subtype", NULL };
static char * node_fields_punct    [] = { "next", "type", "subtype", NULL };
static char * node_fields_inner    [] = { "next", "type", "subtype", NULL };
static char * node_fields_radical  [] = { "next", "type", "subtype", NULL };
static char * node_fields_fraction [] = { "next", "type", "subtype", NULL };
static char * node_fields_under    [] = { "next", "type", "subtype", NULL };
static char * node_fields_over     [] = { "next", "type", "subtype", NULL };
static char * node_fields_accent   [] = { "next", "type", "subtype", NULL };
static char * node_fields_vcenter  [] = { "next", "type", "subtype", NULL };
static char * node_fields_left     [] = { "next", "type", "subtype", NULL };
static char * node_fields_right    [] = { "next", "type", "subtype", NULL };
static char * node_fields_margin_kern    []  = { "next", "type", "subtype", "attr",  "width", "glyph", NULL };
static char * node_fields_glyph          []  = { "next", "type", "subtype", "attr", "char", "font", "components", NULL };
static char * node_fields_attribute      []  = { "next", "type", "subtype", "id", "value", NULL };
static char * node_fields_glue_spec      []  = { "next", "type", "subtype", "width", "stretch", "shrink", 
						 "stretch_order", "shrink_order", "ref_count", NULL };
static char * node_fields_attribute_list []  = { "next", "type", "subtype", "ref_count", NULL };

/* there are holes in this list because not all node types are actually in use */

static char ** node_fields[] = { 
  node_fields_list, 
  node_fields_list,
  node_fields_rule,
  node_fields_insert,
  node_fields_mark,
  node_fields_adjust,
  node_fields_glyph,
  node_fields_disc,
  NULL, /*node_fields_whatsit,*/
  node_fields_math,
  node_fields_glue,
  node_fields_kern,
  node_fields_penalty,
  node_fields_unset,
  node_fields_style,
  node_fields_choice,
  node_fields_ord,
  node_fields_op,
  node_fields_bin,
  node_fields_rel,
  node_fields_open,
  node_fields_close, 
  node_fields_punct, 
  node_fields_inner, 
  node_fields_radical,
  node_fields_fraction,
  node_fields_under,
  node_fields_over, 
  node_fields_accent,
  node_fields_vcenter,
  node_fields_left,  /* 30 */
  node_fields_right, 
  NULL,  NULL,  NULL,  NULL,  
  NULL,  NULL,  NULL,  NULL,
  node_fields_margin_kern, /* 40 */
  node_fields_glyph,
  node_fields_attribute,
  node_fields_glue_spec,
  node_fields_attribute_list,
  NULL };


static char * node_fields_whatsit_open               [] = { "next", "type", "subtype", "attr", "stream", "name", "area", "ext", NULL };
static char * node_fields_whatsit_write              [] = { "next", "type", "subtype", "attr", "stream", "data", NULL };
static char * node_fields_whatsit_close              [] = { "next", "type", "subtype", "attr", "stream", NULL };
static char * node_fields_whatsit_special            [] = { "next", "type", "subtype", "attr", "data", NULL };
static char * node_fields_whatsit_language           [] = { "next", "type", "subtype", "attr", "lang", "left", "right", NULL };
static char * node_fields_whatsit_local_par          [] = { "next", "type", "subtype", "attr", "pen_inter", "pen_broken", "dir", 
							    "box_left", "box_left_width", "box_right", "box_right_width", NULL };
static char * node_fields_whatsit_dir                [] = { "next", "type", "subtype", "attr", "dir", "level", "dvi_ptr", "dvi_h", NULL };

static char * node_fields_whatsit_pdf_literal        [] = { "next", "type", "subtype", "attr", "mode", "data", NULL };
static char * node_fields_whatsit_pdf_refobj         [] = { "next", "type", "subtype", "attr", "objnum", NULL };
static char * node_fields_whatsit_pdf_refxform       [] = { "next", "type", "subtype", "attr", "width", "height", "depth", "objnum", NULL };
static char * node_fields_whatsit_pdf_refximage      [] = { "next", "type", "subtype", "attr", "width", "height", "depth", "objnum", NULL };
static char * node_fields_whatsit_pdf_annot          [] = { "next", "type", "subtype", "attr", "width", "height", "depth", "objnum", "data", NULL };
static char * node_fields_whatsit_pdf_start_link     [] = { "next", "type", "subtype", "attr", "width", "height", "depth", 
							    "objnum", "link_attr", "action", NULL };
static char * node_fields_whatsit_pdf_end_link       [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_pdf_dest           [] = { "next", "type", "subtype", "attr", "width", "height", "depth", 
							    "named_id", "id", "dest_type", "xyz_zoom", "objnum",  NULL };
static char * node_fields_whatsit_pdf_thread         [] = { "next", "type", "subtype", "attr", "width", "height", "depth", 
							    "named_id", "id", "thread_attr", NULL };
static char * node_fields_whatsit_pdf_start_thread   [] = { "next", "type", "subtype", "attr", "width", "height", "depth", 
							    "named_id", "id", "thread_attr", NULL };
static char * node_fields_whatsit_pdf_end_thread     [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_pdf_save_pos       [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_pdf_snap_ref_point [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_pdf_snapy          [] = { "next", "type", "subtype", "attr", "final_skip", "spec", NULL };
static char * node_fields_whatsit_pdf_snapy_comp     [] = { "next", "type", "subtype", "attr", "comp_ratio", NULL };
static char * node_fields_whatsit_late_lua           [] = { "next", "type", "subtype", "attr", "reg", "data", NULL };
static char * node_fields_whatsit_close_lua          [] = { "next", "type", "subtype", "attr", "reg", NULL };
static char * node_fields_whatsit_pdf_colorstack     [] = { "next", "type", "subtype", "attr", "stack", "cmd", "data", NULL };
static char * node_fields_whatsit_pdf_setmatrix      [] = { "next", "type", "subtype", "attr", "data", NULL };
static char * node_fields_whatsit_pdf_save           [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_pdf_restore        [] = { "next", "type", "subtype", "attr", NULL };
static char * node_fields_whatsit_user_defined       [] = { "next", "type", "subtype", "attr", "id", "value_type", "value", NULL };

/* there are holes in this list because not all extension
   codes generate nodes */

static char ** node_fields_whatsits [] = { 
  node_fields_whatsit_open,
  node_fields_whatsit_write,
  node_fields_whatsit_close,
  node_fields_whatsit_special,
  node_fields_whatsit_language,
  NULL,
  node_fields_whatsit_local_par,
  node_fields_whatsit_dir,
  node_fields_whatsit_pdf_literal,
  NULL,
  node_fields_whatsit_pdf_refobj,
  NULL,
  node_fields_whatsit_pdf_refxform,
  NULL,
  node_fields_whatsit_pdf_refximage,
  node_fields_whatsit_pdf_annot,
  node_fields_whatsit_pdf_start_link,
  node_fields_whatsit_pdf_end_link,
  NULL,
  node_fields_whatsit_pdf_dest,
  node_fields_whatsit_pdf_thread,
  node_fields_whatsit_pdf_start_thread,
  node_fields_whatsit_pdf_end_thread,
  node_fields_whatsit_pdf_save_pos,
  NULL,  NULL,  NULL,  NULL,  NULL, 
  NULL,  NULL,  NULL,  NULL,  NULL,
  node_fields_whatsit_pdf_snap_ref_point,
  node_fields_whatsit_pdf_snapy,
  node_fields_whatsit_pdf_snapy_comp,
  NULL,
  node_fields_whatsit_late_lua,
  node_fields_whatsit_close_lua,
  NULL,  NULL,
  node_fields_whatsit_pdf_colorstack,
  node_fields_whatsit_pdf_setmatrix,
  node_fields_whatsit_pdf_save,
  node_fields_whatsit_pdf_restore,
  node_fields_whatsit_user_defined,
  NULL
};


/* This function is similar to |get_node_type_id|, for field
   identifiers.  It has to do some more work, because not all
   identifiers are valid for all types of nodes.
*/

static int
get_node_field_id (lua_State *L, int n, int node ) {
  char *s = NULL;
  int i = -1;
  int t = type(node);
  char *** fields = node_fields;
  if (lua_type(L,n)==LUA_TSTRING) {
    s = (char *)lua_tostring(L,n);
    if (t==whatsit_node) {
      t = subtype(node);
      fields = node_fields_whatsits;
    }
    for (i=0;fields[t][i]!=NULL;i++) {
      if (strcmp(s,fields[t][i])==0)
	break;
    }
    if (fields[t][i]==NULL)
      i=-1;
  } else if (lua_isnumber(L,n)) {
    i = lua_tonumber(L,n);
    /* do some test here as well !*/
  }
  if (i==-1) {
    lua_pushfstring(L, "Invalid field type %s for node type %s (%d)" , s , node_names[type(node)],subtype(node));
    lua_error(L);
  }
  return i;
}

/* fetch the list of valid node types */

static int
lua_nodelib_types (lua_State *L) {
  int i;
  lua_newtable(L);
  for (i=0;node_names[i]!=NULL;i++) {
    if (strcmp(node_names[i],"!") != 0) {
      lua_pushstring(L,node_names[i]);
      lua_rawseti(L,-2,i);
    }
  }
  return 1;
}

static int
lua_nodelib_whatsits (lua_State *L) {
  int i;
  lua_newtable(L);
  for (i=0;whatsit_node_names[i]!=NULL;i++) {
    if (strcmp(whatsit_node_names[i],"!") != 0) {
      lua_pushstring(L,whatsit_node_names[i]);
      lua_rawseti(L,-2,i);
    }
  }
  return 1;
}


/* fetch the list of valid fields */

static int
lua_nodelib_fields (lua_State *L) {
  int i = -1;
  int t = get_node_type_id(L,1);
  char *** fields = node_fields;
  if (t==whatsit_node ) {
    t = get_node_subtype_id(L,2);
    fields = node_fields_whatsits;
  }
  lua_checkstack(L,2);
  lua_newtable(L);
  for (i=0;fields[t][i]!=NULL;i++) {
    lua_pushstring(L,fields[t][i]);
    lua_rawseti(L,-2,i);
  }
  return 1;
}

/* a utility function for attribute stuff */

static int
lua_nodelib_has_attribute (lua_State *L) {
  int i;
  halfword *n = check_isnode(L,1);
  halfword t=*n;
  if (type(t)==attribute_list_node)  {
    t = vlink(t);
    i = lua_tonumber(L,2);
    while (t!=null) {
      if (attribute_id(t)==i) {
	lua_pushnumber(L,attribute_value(t));
	return 1;
      } else if (attribute_id(t)>i) {
	break;
      }
      t = vlink(t);
    }
  }
  lua_pushnil(L);
  return 1;
}



/* fetching a field from a node */

#define nodelib_pushlist(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushattr(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushspec(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushstring(L,n) { lua_pushstring(L,makecstring(n)); }

static int
lua_nodelib_getfield_whatsit  (lua_State *L, int n, int field) {
  switch (subtype(n)) {
  case open_node:               
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,write_stream(n));              break;
    case  5: nodelib_pushstring(L,open_name(n));             break;
    case  6: nodelib_pushstring(L,open_area(n));             break;
    case  7: nodelib_pushstring(L,open_ext(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case write_node:              
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,write_stream(n));              break;
    case  5: tokenlist_to_lua(L,write_tokens(n));            break;
    default: lua_pushnil(L); 
    }
    break;
  case close_node:              
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,write_stream(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case special_node:            
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: tokenlist_to_luastring(L,write_tokens(n));      break;
    default: lua_pushnil(L); 
    }
    break;
  case language_node:           
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,what_lang(n));                 break;
    case  5: lua_pushnumber(L,what_lhm(n));                  break;
    case  6: lua_pushnumber(L,what_rhm(n));                  break;
    default: lua_pushnil(L); 
    }
    break;
  case local_par_node:          
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,local_pen_inter(n));           break;
    case  5: lua_pushnumber(L,local_pen_broken(n));          break;
    case  6: lua_pushnumber(L,local_par_dir(n));             break;
    case  7: nodelib_pushlist(L,local_box_left(n));          break;
    case  8: lua_pushnumber(L,local_box_left_width(n));      break;
    case  9: nodelib_pushlist(L,local_box_right(n));         break;
    case 10: lua_pushnumber(L,local_box_right_width(n));     break;
    default: lua_pushnil(L); 
    }
    break;
  case dir_node:                
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,dir_dir(n));                   break;
    case  5: lua_pushnumber(L,dir_level(n));                 break;
    case  6: lua_pushnumber(L,dir_dvi_ptr(n));               break;
    case  7: lua_pushnumber(L,dir_dvi_h(n));                 break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_literal_node:
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_literal_mode(n));          break;
    case  5: tokenlist_to_luastring(L,pdf_literal_data(n));  break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refobj_node:         
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_obj_objnum(n));            break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refxform_node:       
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_xform_objnum(n));          break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refximage_node:      
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_ximage_objnum(n));         break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_annot_node:          
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_annot_objnum(n));          break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_start_link_node:     
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_link_objnum(n));           break;
    case  8: tokenlist_to_luastring(L,pdf_link_attr(n));     break;
    case  9: nodelib_pushlist(L,pdf_link_action(n));         break; /* action ! */
    default: lua_pushnil(L); 
    }
    break;
  case pdf_end_link_node:       
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_dest_node:           
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_dest_named_id(n));         break;
    case  8: if (pdf_dest_named_id(n) == 1)
	tokenlist_to_luastring(L,pdf_dest_id(n));
      else 
	lua_pushnumber(L,pdf_dest_id(n));                    break;
    case  9: lua_pushnumber(L,pdf_dest_type(n));             break;
    case 10: lua_pushnumber(L,pdf_dest_xyz_zoom(n));         break;
    case 11: lua_pushnumber(L,pdf_dest_objnum(n));           break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_thread_node:         
  case pdf_start_thread_node:   
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_thread_named_id(n));       break;
    case  8: if (pdf_thread_named_id(n) == 1)
	tokenlist_to_luastring(L,pdf_thread_id(n));
      else 
	lua_pushnumber(L,pdf_thread_id(n));                  break;
    case  9: tokenlist_to_luastring(L,pdf_thread_attr(n));   break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_end_thread_node:     
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_save_pos_node:       
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_snap_ref_point_node: 
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_snapy_node:          
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,final_skip(n));	             break;
    case  5: nodelib_pushspec(L,snap_glue_ptr(n));	     break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_snapy_comp_node:     
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,snapy_comp_ratio(n));	     break;
    default: lua_pushnil(L); 
    }
    break;
  case late_lua_node:           
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,late_lua_reg(n));              break;
    case  5: tokenlist_to_luastring(L,late_lua_data(n));     break;
    default: lua_pushnil(L); 
    }
    break;
  case close_lua_node:          
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,late_lua_reg(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_colorstack_node:     
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,pdf_colorstack_stack(n));      break;
    case  5: lua_pushnumber(L,pdf_colorstack_cmd(n));        break;
    case  6: tokenlist_to_luastring(L,pdf_colorstack_data(n)); break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_setmatrix_node:      
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: tokenlist_to_luastring(L,pdf_setmatrix_data(n)); break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_save_node:           
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_restore_node:        
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    default: lua_pushnil(L); 
    }
    break;
  case user_defined_node:       
    switch (field) {
    case  2: lua_pushnumber(L,subtype(n));	             break;
    case  4: lua_pushnumber(L,user_node_id(n));	             break;
    case  5: lua_pushnumber(L,user_node_type(n));            break;
    case  6: 
      switch (user_node_type(n)) {
      case 'd': lua_pushnumber(L,user_node_value(n));     break;
      case 'n': nodelib_pushlist(L,user_node_value(n));   break;
      case 's': nodelib_pushstring(L,user_node_value(n)); break;
      case 't': tokenlist_to_lua(L,user_node_value(n));   break;
      default: lua_pushnumber(L,user_node_value(n));      break;
      }                                                      break;
    default: lua_pushnil(L); 
    }
    break;
  default:
    lua_pushnil(L); 
    break;
  }
}


static int
lua_nodelib_getfield  (lua_State *L) {
  halfword *n_ptr, n;
  int field;
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  field = get_node_field_id(L,2, n);
  if (field<0)
    return;
  if (field==0) {
    lua_pushnumber(L,vlink(n));
    lua_nodelib_push(L);
  } else if (field==1) {
    if (lua_type(L,2)==LUA_TSTRING) {
      lua_pushstring(L,node_names[type(n)]);
    } else {
      lua_pushnumber(L,type(n));
    }
  } else  {
    switch (type(n)) {
    case hlist_node:
    case vlist_node:
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,width(n));	      break;
      case  5: lua_pushnumber(L,depth(n));	      break;
      case  6: lua_pushnumber(L,height(n));	      break;
      case  7: lua_pushnumber(L,shift_amount(n));     break;
      case  8: nodelib_pushlist(L,list_ptr(n));       break;
      case  9: lua_pushnumber(L,glue_order(n));       break;
      case 10: lua_pushnumber(L,glue_sign(n));        break;
      case 11: lua_pushnumber(L,(double)glue_set(n)); break;
      case 12: lua_pushnumber(L,box_dir(n));          break;
      default: lua_pushnil(L);
      }
      break;
    case unset_node:
      switch (field) {
      case  2: lua_pushnumber(L,0);	              break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,width(n));	      break;
      case  5: lua_pushnumber(L,depth(n));	      break;
      case  6: lua_pushnumber(L,height(n));	      break;
      case  7: lua_pushnumber(L,glue_shrink(n));      break;
      case  8: nodelib_pushlist(L,list_ptr(n));       break;
      case  9: lua_pushnumber(L,glue_order(n));       break;
      case 10: lua_pushnumber(L,glue_sign(n));        break;
      case 11: lua_pushnumber(L,glue_stretch(n));     break;
      case 12: lua_pushnumber(L,box_dir(n));          break;
      case 13: lua_pushnumber(L,span_count(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case rule_node:
      switch (field) {
      case  2: lua_pushnumber(L,0);	              break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,width(n));	      break;
      case  5: lua_pushnumber(L,depth(n));	      break;
      case  6: lua_pushnumber(L,height(n));	      break;
      case  7: lua_pushnumber(L,rule_dir(n));         break;
      default: lua_pushnil(L);
      }
      break;
    case ins_node:
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,float_cost(n));	      break;
      case  5: lua_pushnumber(L,depth(n));	      break;
      case  6: lua_pushnumber(L,height(n));	      break;
      case  7: nodelib_pushspec(L,split_top_ptr(n));  break;
      case  8: nodelib_pushlist(L,ins_ptr(n));        break;
      default: lua_pushnil(L);
      }
      break;
    case mark_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,mark_class(n));	      break;
      case  5: tokenlist_to_lua(L,mark_ptr(n));       break;
      default: lua_pushnil(L);
      }
      break;
    case adjust_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: nodelib_pushlist(L,adjust_ptr(n));     break;
      default: lua_pushnil(L);
      }
      break;
    case disc_node: 
      switch (field) {
      case  2: lua_pushnumber(L,replace_count(n));    break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: nodelib_pushlist(L,pre_break(n));      break;
      case  5: nodelib_pushlist(L,post_break(n));      break;
      default: lua_pushnil(L);
      }
      break;
    case math_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,surround(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case glue_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: nodelib_pushspec(L,glue_ptr(n));       break;
      case  5: nodelib_pushlist(L,leader_ptr(n));     break;
      default: lua_pushnil(L);
      }
      break;
    case glue_spec_node: 
      switch (field) {
      case  2: lua_pushnumber(L,0);	              break;
      case  3: lua_pushnumber(L,width(n));	      break;
      case  4: lua_pushnumber(L,stretch(n));	      break;
      case  5: lua_pushnumber(L,shrink(n));	      break;
      case  6: lua_pushnumber(L,stretch_order(n));    break;
      case  7: lua_pushnumber(L,shrink_order(n));     break;
      case  8: lua_pushnumber(L,glue_ref_count(n));   break;
      default: lua_pushnil(L);
      }
      break;
    case kern_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,width(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case penalty_node: 
      switch (field) {
      case  2: lua_pushnumber(L,0);	              break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,penalty(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case glyph_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,character(n));	      break;
      case  5: lua_pushnumber(L,font(n));	      break;
      case  6: nodelib_pushlist(L,lig_ptr(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case ligature_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n)+1);	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,character(n));	      break;
      case  5: lua_pushnumber(L,font(n));	      break;
      case  6: nodelib_pushlist(L,lig_ptr(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case margin_kern_node :
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,width(n));	      break;
      case  5: nodelib_pushlist(L,margin_char(n));    break;
      default: lua_pushnil(L);
      }
      break;
    case attribute_list_node :
      switch (field) {
      case  2: lua_pushnumber(L,0);	               break;
      case  3: lua_pushnumber(L,attr_list_ref(n));     break;
      default: lua_pushnil(L);
      }
      break;
    case attribute_node :
      switch (field) {
      case  2: lua_pushnumber(L,0);	              break;
      case  3: lua_pushnumber(L,attribute_id(n));     break;
      case  4: lua_pushnumber(L,attribute_value(n));  break;
      default: lua_pushnil(L);
      }
      break;
    case whatsit_node:
      lua_nodelib_getfield_whatsit(L,n,field);
      break;
    default: 
      lua_pushnil(L); 
      break;
    }
  }
  return 1;
}


static int nodelib_getlist(lua_State *L, int n) {
  halfword *m;
  if (lua_isuserdata(L,n)) {
    m = check_isnode(L,n);
    return *m;
  } else {
    return null ;
  }
}

#define nodelib_getattr        nodelib_getlist
#define nodelib_getspec        nodelib_getlist
#define nodelib_getstring(L,a) maketexstring(lua_tostring(L,a))
#define nodelib_gettoks(L,a)   tokenlist_from_lua(L)

static int nodelib_cantset (lua_State *L, int field, int n) {
  lua_pushfstring(L,"You cannot set field %d in a node of type %s",
		  field, node_names[type(n)]);
  lua_error(L);
  return 0;
}

static int
lua_nodelib_setfield_whatsit(lua_State *L, int n, int field) {
  switch (subtype(n)) {
  case open_node:
    switch (field) {
    case  4: write_stream(n) = lua_tointeger(L,3);          break;
    case  5: open_name(n) = nodelib_getstring(L,3);         break;
    case  6: open_area(n) = nodelib_getstring(L,3);         break;
    case  7: open_ext(n)  = nodelib_getstring(L,3);         break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case write_node:              
    switch (field) {
    case  4: write_stream(n) = lua_tointeger(L,3);          break;
    case  5: write_tokens(n) = nodelib_gettoks(L,3);        break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case close_node:              
    switch (field) {
    case  4: write_stream(n) = lua_tointeger(L,3);          break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case special_node:            
    switch (field) {
    case  5: write_tokens(n) = nodelib_gettoks(L,3);        break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case language_node:
    switch (field) {
    case  4: what_lang(n) = lua_tointeger(L,3);             break;
    case  5: what_lhm(n) = lua_tointeger(L,3);              break;
    case  6: what_rhm(n) = lua_tointeger(L,3);              break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case local_par_node:          
    switch (field) {
    case  4: local_pen_inter(n) = lua_tointeger(L,3);       break;
    case  5: local_pen_broken(n) = lua_tointeger(L,3);      break;
    case  6: local_par_dir(n) = lua_tointeger(L,3);         break;
    case  7: local_box_left(n) = nodelib_getlist(L,3);      break;
    case  8: local_box_left_width(n) = lua_tointeger(L,3);  break;
    case  9: local_box_right(n) = nodelib_getlist(L,3);     break;
    case 10: local_box_right_width(n) = lua_tointeger(L,3); break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case dir_node:                
    switch (field) {
    case  4: dir_dir(n) = lua_tointeger(L,3);               break;
    case  5: dir_level(n) = lua_tointeger(L,3);             break;
    case  6: dir_dvi_ptr(n) = lua_tointeger(L,3);           break;
    case  7: dir_dvi_h(n) = lua_tointeger(L,3);             break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_literal_node:
    switch (field) {
    case  4: pdf_literal_mode(n) = lua_tointeger(L,3);      break;
    case  5: pdf_literal_data(n) = nodelib_gettoks(L,3);    break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_refobj_node:         
    switch (field) {
    case  4: pdf_obj_objnum(n) = lua_tointeger(L,3);        break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_refxform_node:       
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_xform_objnum(n) = lua_tointeger(L,3);      break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_refximage_node:      
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_ximage_objnum(n) = lua_tointeger(L,3);     break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_annot_node:          
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_annot_objnum(n) = lua_tointeger(L,3);      break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_start_link_node:     
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_link_objnum(n) = lua_tointeger(L,3);       break;
    case  8: pdf_link_attr(n) = nodelib_gettoks(L,3);       break;
    case  9: pdf_link_action(n) = nodelib_getlist(L,3);     break; /* action */
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_end_link_node:       
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_dest_node:           
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_dest_named_id(n) = lua_tointeger(L,3);     break;
    case  8: if (pdf_dest_named_id(n)==1)
	pdf_dest_id(n) = nodelib_gettoks(L,3);
      else
	pdf_dest_id(n) = lua_tointeger(L,3);                break;
    case  9: pdf_dest_type(n) = lua_tointeger(L,3);         break;
    case 10: pdf_dest_xyz_zoom(n) = lua_tointeger(L,3);     break;
    case 11: pdf_dest_objnum(n) = lua_tointeger(L,3);       break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_thread_node:         
  case pdf_start_thread_node:   
    switch (field) {
    case  4: pdf_width(n) = lua_tointeger(L,3);             break;
    case  5: pdf_height(n) = lua_tointeger(L,3);            break;
    case  6: pdf_depth(n) = lua_tointeger(L,3);             break;
    case  7: pdf_thread_named_id(n) = lua_tointeger(L,3);   break;
    case  8: if (pdf_thread_named_id(n)==1)
	pdf_thread_id(n) = nodelib_gettoks(L,3);
      else
	pdf_thread_id(n) = lua_tointeger(L,3);              break;
    case  9: pdf_thread_attr(n) = nodelib_gettoks(L,3);     break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_end_thread_node:     
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_save_pos_node:       
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_snap_ref_point_node: 
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_snapy_node:          
    switch (field) {
    case  4: final_skip(n) = lua_tointeger(L,3);            break;
    case  5: snap_glue_ptr(n) = nodelib_getspec(L,3);       break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_snapy_comp_node:     
    switch (field) {
    case  4: snapy_comp_ratio(n) = lua_tointeger(L,3);      break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case late_lua_node:           
    switch (field) {
    case  4: late_lua_reg(n) = lua_tointeger(L,3);          break;
    case  5: late_lua_data(n) = nodelib_gettoks(L,3);       break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case close_lua_node:          
    switch (field) {
    case  4: late_lua_reg(n) = lua_tointeger(L,3);          break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_colorstack_node:     
    switch (field) {
    case  4: pdf_colorstack_stack(n) = lua_tointeger(L,3);  break;
    case  5: pdf_colorstack_cmd(n) = lua_tointeger(L,3);    break;
    case  6: pdf_colorstack_data(n) = nodelib_gettoks(L,3); break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_setmatrix_node:      
    switch (field) {
    case  4: pdf_setmatrix_data(n) = nodelib_gettoks(L,3);  break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_save_node:           
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case pdf_restore_node:        
    switch (field) {
    default: return nodelib_cantset(L,field,n);
    }
    break;
  case user_defined_node:       
    switch (field) {
    case  4: user_node_id(n) = lua_tointeger(L,3);          break;
    case  5: user_node_type(n) = lua_tointeger(L,3);        break;
    case  6: 
      switch(user_node_type(n)) {
      case 'd': user_node_value(n) = lua_tointeger(L,3); break;
      case 'n': user_node_value(n) = nodelib_getlist(L,3); break;
      case 's': user_node_value(n) = nodelib_getstring(L,3); break;
      case 't': user_node_value(n) = nodelib_gettoks(L,3); break;
      default: user_node_value(n) = lua_tointeger(L,3); break;
      }                                                     break;
    default: return nodelib_cantset(L,field,n);
    }
    break;
  default:
    /* do nothing */
    break;
  }
}

static int
lua_nodelib_setfield  (lua_State *L) {
  int i;
  halfword *m, *n_ptr;
  halfword n;
  int field;
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  field = get_node_field_id(L,2,n);
  if (field==0) {
    vlink(n) = nodelib_getlist(L,3);
  } else {
    switch (type(n)) {
    case hlist_node:
    case vlist_node:
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: width(n) = lua_tointeger(L,3);	        break;
      case  5: depth(n) = lua_tointeger(L,3);	        break;
      case  6: height(n) = lua_tointeger(L,3);	        break;
      case  7: shift_amount(n) = lua_tointeger(L,3);    break;
      case  8: list_ptr(n) = nodelib_getlist(L,3);      break;
      case  9: glue_order(n) = lua_tointeger(L,3);      break;
      case 10: glue_sign(n) = lua_tointeger(L,3);       break;
      case 11: glue_set(n) = (double)lua_tonumber(L,3); break;
      case 12: box_dir(n) = lua_tointeger(L,3);         break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case unset_node:
      switch (field) {
      case  2: /* dummy subtype */                      break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: width(n) = lua_tointeger(L,3);	        break;
      case  5: depth(n) = lua_tointeger(L,3);	        break;
      case  6: height(n) = lua_tointeger(L,3);	        break;
      case  7: glue_shrink(n) = lua_tointeger(L,3);     break;
      case  8: list_ptr(n) = nodelib_getlist(L,3);      break;
      case  9: glue_order(n) = lua_tointeger(L,3);      break;
      case 10: glue_sign(n) = lua_tointeger(L,3);       break;
      case 11: glue_stretch(n) = lua_tointeger(L,3);    break;
      case 12: box_dir(n) = lua_tointeger(L,3);         break;
      case 13: span_count(n) = lua_tointeger(L,3);	break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case rule_node:
      switch (field) {
      case  2: /* dummy subtype */                      break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: width(n) = lua_tointeger(L,3);	        break;
      case  5: depth(n) = lua_tointeger(L,3);	        break;
      case  6: height(n) = lua_tointeger(L,3);	        break;
      case  7: rule_dir(n) = lua_tointeger(L,3);        break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case ins_node:
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: float_cost(n) = lua_tointeger(L,3);	break;
      case  5: depth(n) = lua_tointeger(L,3);	        break;
      case  6: height(n) = lua_tointeger(L,3);	        break;
      case  7: split_top_ptr(n) = nodelib_getspec(L,3); break;
      case  8: ins_ptr(n) = nodelib_getlist(L,3);       break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case mark_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: mark_class(n) = lua_tointeger(L,3);	break;
      case  5: mark_ptr(n) = nodelib_gettoks(L,3);      break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case adjust_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: adjust_ptr(n) = nodelib_getlist(L,3);    break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case disc_node: 
      switch (field) {
      case  2: replace_count(n) = lua_tointeger(L,3);   break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: pre_break(n) = nodelib_getlist(L,3);     break;
      case  5: post_break(n) = nodelib_getlist(L,3);    break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case math_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: surround(n) = lua_tointeger(L,3);        break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case glue_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);         break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: glue_ptr(n) = nodelib_getspec(L,3);      break;
      case  5: leader_ptr(n) = nodelib_getlist(L,3);    break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case glue_spec_node: 
      switch (field) {
      case  2: /* dummy subtype */                      break;
      case  3: width(n) = lua_tointeger(L,3);	        break;
      case  4: stretch(n) = lua_tointeger(L,3);	        break;
      case  5: shrink(n) = lua_tointeger(L,3);	        break;
      case  6: stretch_order(n) = lua_tointeger(L,3);	break;
      case  7: shrink_order(n) = lua_tointeger(L,3);	break;
      case  8: glue_ref_count(n) = lua_tointeger(L,3);  break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case kern_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: width(n) = lua_tointeger(L,3);           break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case penalty_node: 
      switch (field) {
      case  2: /* dummy subtype */                      break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: penalty(n) = lua_tointeger(L,3);         break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case ligature_node: 
      switch (field) {
      case  2: subtype(n) = (lua_tointeger(L,3)-1);     break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: character(n) = lua_tointeger(L,3);	break;
      case  5: font(n) = lua_tointeger(L,3);	        break;
      case  6: lig_ptr(n) = nodelib_getlist(L,3);       break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case glyph_node: 
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: character(n) = lua_tointeger(L,3);	break;
      case  5: font(n) = lua_tointeger(L,3);	        break;
      case  6: lig_ptr(n) = nodelib_getlist(L,3);       break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case margin_kern_node:
      switch (field) {
      case  2: subtype(n) = lua_tointeger(L,3);	        break;
      case  3: node_attr(n) = nodelib_getattr(L,3);     break;
      case  4: width(n) = lua_tointeger(L,3);           break;
      case  5: margin_char(n) = nodelib_getlist(L,3);   break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case attribute_list_node:
      switch (field) {
      case  2: /* dummy subtype */                      break;
      case  3: attr_list_ref(n) = lua_tointeger(L,3);   break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case attribute_node:
      switch (field) {
      case  2: /* dummy subtype */                       break;
      case  3: attribute_id(n) = lua_tointeger(L,3);     break;
      case  4: attribute_value(n) = lua_tointeger(L,3);  break;
      default: return nodelib_cantset(L,field,n);
      }
      break;
    case whatsit_node:
      lua_nodelib_setfield_whatsit(L,n,field);
      break;
    default:
      /* do nothing */
      break;
    }
  }
  return 0;
}

static int
lua_nodelib_print  (lua_State *L) {
  halfword *n_ptr;
  halfword n;
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  lua_checkstack(L,1);
  lua_pushfstring(L,"<node %d: %s>",n, node_names[type(n)]);
  return 1;
}


static const struct luaL_reg nodelib_f [] = {
  {"id",            lua_nodelib_id},
  {"new",           lua_nodelib_new},
  {"types",         lua_nodelib_types},
  {"whatsits",      lua_nodelib_whatsits},
  {"fields",        lua_nodelib_fields},
  {"free",          lua_nodelib_free},
  {"flush_list",    lua_nodelib_flush_list},
  {"copy",          lua_nodelib_copy},
  {"copy_list",     lua_nodelib_copy_list},
  {"has_attribute", lua_nodelib_has_attribute},
  {NULL, NULL}  /* sentinel */
};

static const struct luaL_reg nodelib_m [] = {
  {"__index",    lua_nodelib_getfield},
  {"__newindex", lua_nodelib_setfield},
  {"__tostring", lua_nodelib_print},
  {NULL, NULL}  /* sentinel */
};



int 
luaopen_node (lua_State *L) 
{
  luaL_newmetatable(L,NODE_METATABLE);
  luaL_register(L, NULL, nodelib_m);
  luaL_register(L, "node", nodelib_f);
  return 1;
}

void 
unodelist_to_lua (lua_State *L, halfword n) { 
  lua_pushnumber(L,n);
  lua_nodelib_push(L);
}

halfword
unodelist_from_lua (lua_State *L) { 
  halfword *n = check_isnode(L,-1);
  return *n;
}


