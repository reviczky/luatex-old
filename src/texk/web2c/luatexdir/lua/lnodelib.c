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


static int
lua_nodelib_new(lua_State *L) {
  integer i,j;
  char *s;
  halfword *a;
  halfword n  = null;
  i = get_node_type_id(L,1);
  if (i<0)
    return 0;
  switch (i) {
  case hlist_node:
  case vlist_node:
	n = bare_null_box(); 
	type(n)=i;	subtype(n)=0;
	break;
  case rule_node:
	n = bare_rule();
	type(n)=i;	subtype(n)=0;
	break;
  case ins_node:
	n = get_node(ins_node_size); 
	float_cost(n)=0; height(n)=0; depth(n)=0;
	ins_ptr(n)=null; split_top_ptr(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	break;
  case mark_node: 
	n = get_node(mark_node_size);  
	mark_ptr(n)=null;
	mark_class(n)=0;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	break;
  case adjust_node: 
	n = get_node(adjust_node_size); 
	adjust_ptr(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	break;
  case disc_node: 
	n = get_node(disc_node_size);  
	replace_count(n)=0;
	pre_break(n)=null;
	post_break(n)=null;
	node_attr(n) = null;
	type(n)=i;	subtype(n)=0;
	break;
  case math_node: 
	n = get_node(math_node_size);  
	node_attr(n) = null;
	surround(n) = 0;
	type(n)=i;	subtype(n)=0;
	break;
  case glue_node: 
	n = get_node(glue_node_size);  
	glue_ptr(n)=null;
	leader_ptr(n)=null;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
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
	break;
  case margin_kern_node:          
	n = get_node(margin_kern_node_size); 
	margin_char(n) = null; 
	width(n) = 0;
	node_attr(n)=null;
	type(n)=i;	subtype(n)=0;
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
	break; 
  /*case whatsit_node:         break; */
  default: 
    fprintf(stdout,"<node type %s not supported yet>\n",node_names[i]);
    break;
  }  
  lua_pushnumber(L,n);
  lua_nodelib_push(L);
  return 1;
}

/* as a bonus, this function returns the 'next' node */

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

static int
lua_nodelib_flush_list(lua_State *L) {
  halfword *n_ptr = check_isnode(L,-1);
  flush_node_list(*n_ptr); 
  return 0;
}


static int
lua_nodelib_copy_list (lua_State *L) {
  halfword *n = check_isnode(L,-1);
  halfword m = copy_node_list(*n);
  lua_pushnumber(L,m);
  lua_nodelib_push(L);
  return 1;
}

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

static char * node_fields_list    [] = { "next", "type", "subtype", "attr", "width", "depth", "height", "shift", "list", 
                                          "glue_order", "glue_sign", "glue_set" , "dir" ,  NULL };
static char * node_fields_rule     [] = { "next", "type", "subtupe", "attr", "width", "depth", "height", "dir", NULL };
static char * node_fields_insert   [] = { "next", "type", "subtype", "attr", "cost",  "depth", "height", "top_skip", "insert", NULL };
static char * node_fields_mark     [] = { "next", "type", "subtype", "attr", "class", "mark", NULL }; 
static char * node_fields_adjust   [] = { "next", "type", "subtype", "attr", "list", NULL }; 
static char * node_fields_disc     [] = { "next", "type", "replace", "attr", "pre", "post", NULL };
static char * node_fields_whatsit  [] = { "next", "type", NULL };
static char * node_fields_math     [] = { "next", "type", "subtype", "attr", "surround", NULL }; 
static char * node_fields_glue     [] = { "next", "type", "subtype", "attr", "glue_spec", "leader", NULL }; 
static char * node_fields_kern     [] = { "next", "type", "subtype", "attr", "width", NULL };
static char * node_fields_penalty  [] = { "next", "type", "subtype", "attr", "penalty", NULL };
static char * node_fields_unset    [] = { "next", "type", "span", "attr", "width", "depth", "height", "shrink", "list", 
                                          "glue_order", "glue_sign", "stretch" , "dir" ,  NULL };
static char * node_fields_style    [] = { "next", "type", NULL };
static char * node_fields_choice   [] = { "next", "type", NULL };
static char * node_fields_ord      [] = { "next", "type", NULL };
static char * node_fields_op       [] = { "next", "type", NULL };
static char * node_fields_bin      [] = { "next", "type", NULL };
static char * node_fields_rel      [] = { "next", "type", NULL };
static char * node_fields_open     [] = { "next", "type", NULL };
static char * node_fields_close    [] = { "next", "type", NULL };
static char * node_fields_punct    [] = { "next", "type", NULL };
static char * node_fields_inner    [] = { "next", "type", NULL };
static char * node_fields_radical  [] = { "next", "type", NULL };
static char * node_fields_fraction [] = { "next", "type", NULL };
static char * node_fields_under    [] = { "next", "type", NULL };
static char * node_fields_over     [] = { "next", "type", NULL };
static char * node_fields_accent   [] = { "next", "type", NULL };
static char * node_fields_vcenter  [] = { "next", "type", NULL };
static char * node_fields_left     [] = { "next", "type", NULL };
static char * node_fields_right    [] = { "next", "type", NULL };
static char * node_fields_margin_kern []  = { "next", "type", "subtype", "attr",  "width", "glyph", NULL };
static char * node_fields_glyph     []  = { "next", "type", "subtype", "attr", "character", "font", "lig", NULL };
static char * node_fields_attribute []  = { "next", "type", "id", "value", NULL };
static char * node_fields_glue_spec []  = { "next", "type", "width", "stretch", "shrink", "stretch_order", "shrink_order", "ref_count", NULL };
static char * node_fields_attribute_list []  = { "next", "type", "ref_count", NULL };

static char ** node_fields[] = { 
  node_fields_list, 
  node_fields_list,
  node_fields_rule,
  node_fields_insert,
  node_fields_mark,
  node_fields_adjust,
  node_fields_glyph,
  node_fields_disc,
  node_fields_whatsit,
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
  NULL,  NULL,  NULL,  NULL,  NULL,
  NULL,  NULL,  NULL,
  node_fields_margin_kern, /* 40 */
  node_fields_glyph,
  node_fields_attribute,
  node_fields_glue_spec,
  node_fields_attribute_list,
  NULL };


/* This function is similar to |get_node_type_id|, for field
   identifiers.  It has to do some more work, because not all
   identifiers are valid for all types of nodes.
*/


static int
get_node_field_id (lua_State *L, int n, int node ) {
  char *s = NULL;
  int i = -1;
  int t = type(node);
  if (lua_type(L,n)==LUA_TSTRING) {
    s = (char *)lua_tostring(L,n);
    for (i=0;node_fields[t][i]!=NULL;i++) {
      if (strcmp(s,node_fields[t][i])==0)
      break;
    }
    if (node_fields[t][i]==NULL)
      i=-1;
  } else if (lua_isnumber(L,n)) {
    i = lua_tonumber(L,n);
    /* do some test here as well !*/
  }
  if (i==-1) {
    lua_pushfstring(L, "Invalid field type %s for node type %s" , s , node_names[type(node)]);
    lua_error(L);
  }
  return i;
}


/* fetch the list of valid fields */

static int
lua_nodelib_fields (lua_State *L) {
  int i = -1;
  int t = get_node_type_id(L,1);
  lua_checkstack(L,2);
  lua_newtable(L);
  for (i=0;node_fields[t][i]!=NULL;i++) {
    lua_pushstring(L,node_fields[t][i]);
    lua_rawseti(L,-2,i);
  }
  return 1;
}


/* fetching a field from a node */

#define nodelib_pushlist(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushattr(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushspec(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }

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
      case  2: lua_pushnumber(L,span_count(n));	      break;
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
      default: lua_pushnil(L);
      }
      break;
    case rule_node:
      switch (field) {
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
      case  5: nodelib_pushlist(L,mark_ptr(n));       break;
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
      case  2: lua_pushnumber(L,width(n));	      break;
      case  3: lua_pushnumber(L,stretch(n));	      break;
      case  4: lua_pushnumber(L,shrink(n));	      break;
      case  5: lua_pushnumber(L,stretch_order(n));    break;
      case  6: lua_pushnumber(L,shrink_order(n));     break;
      case  7: lua_pushnumber(L,glue_ref_count(n));   break;
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
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: lua_pushnumber(L,penalty(n));	      break;
      default: lua_pushnil(L);
      }
      break;
    case glyph_node: 
    case ligature_node: 
      switch (field) {
      case  2: lua_pushnumber(L,subtype(n));	      break;
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
      case  2: lua_pushnumber(L,attr_list_ref(n));     break;
      default: lua_pushnil(L);
      }
      break;
    case attribute_node :
      switch (field) {
      case  2: lua_pushnumber(L,attribute_id(n));     break;
      case  3: lua_pushnumber(L,attribute_value(n));  break;
      default: lua_pushnil(L);
      }
      break;
    default: 
      lua_pushnil(L); 
      break;
    }
  }
  return 1;
}


static int
lua_nodelib_setfield  (lua_State *L) {
  int i;
  halfword *m, *n_ptr;
  halfword n;
  char *field;
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  field = (char *)lua_tostring(L,2);
  if (strcmp(field,"subtype")==0) {
	subtype(n)=lua_tointeger(L,3);
  } else if (strcmp(field,"link")==0) {
	if (lua_isnil(L,3)) {
	  vlink(n)==null;
	} else if (lua_isuserdata(L,3)) {
	  m = check_isnode(L,3);
	  vlink(n) = *m;
	}
  } else {
	switch (type(n)) {

  case hlist_node:
  case vlist_node:
	break;
  case rule_node:
	break;
  case ins_node:
	break;
  case mark_node: 
	break;
  case adjust_node: 
	break;
  case disc_node: 
	break;
  case math_node: 
	break;
  case glue_node: 
	break;
  case glue_spec_node: 
	break;
  case kern_node: 
	break;
  case penalty_node: 
	break;
	case glyph_node: 
	  if (strcmp(field,"character")==0) { 
		character(n)=lua_tointeger(L,3);
	  } else if (strcmp(field,"font")==0) { 
		font(n)=lua_tointeger(L,3);
	  }
	  break;
	default:
	  /* do nothing */
	  break;
	}
  }
  return 0;
}


static int
lua_nodelib_setfield_new  (lua_State *L) {
  int i;
  halfword *m, *n_ptr;
  halfword n;
  int field;
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  field = lua_tonumber(L,2);
  if (field==1) {
    subtype(n)=lua_tointeger(L,3);
  } else if (field==0) {
    if (lua_isnil(L,3)) {
      vlink(n)==null;
    } else if (lua_isuserdata(L,3)) {
      m = check_isnode(L,3);
      vlink(n) = *m;
    }
  } else {
    switch (type(n)) {
  case hlist_node:
  case vlist_node:
	break;
  case rule_node:
	break;
  case ins_node:
	break;
  case mark_node: 
	break;
  case adjust_node: 
	break;
  case disc_node: 
	break;
  case math_node: 
	break;
  case glue_node: 
	break;
  case glue_spec_node: 
	break;
  case kern_node: 
	break;
  case penalty_node: 
	break;
    case glyph_node: 
      if (field==4) { 
	character(n)=lua_tointeger(L,3);
      } else if (field==5) { 
	font(n)=lua_tointeger(L,3);
      }
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
  {"id",          lua_nodelib_id},
  {"new",         lua_nodelib_new},
  {"fields",      lua_nodelib_fields},
  {"free",        lua_nodelib_free},
  {"flush_list",  lua_nodelib_flush_list},
  {"copy",        lua_nodelib_copy},
  {"copy_list",   lua_nodelib_copy_list},
  {NULL, NULL}  /* sentinel */
};

static const struct luaL_reg nodelib_m [] = {
  {"__index",    lua_nodelib_getfield},
  {"__newindex", lua_nodelib_setfield_new},
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


