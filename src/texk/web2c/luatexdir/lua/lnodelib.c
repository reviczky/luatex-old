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
    lua_pushstring(L, "Invalid node type id");
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
lua_nodelib_id (lua_State *L) {
  integer i;
  i = get_node_type_id(L,1);
  if (i>=0) {
    lua_pushnumber(L,i);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

/* converts id numbers to type names */

static int
lua_nodelib_type (lua_State *L) {
  integer i;
  i = get_node_type_id(L,1);
  if (i>=0) {
    lua_pushstring(L,node_names[i]);
  } else {
    lua_pushnil(L);
  }
  return 1;
}


/* allocate a new node */

static int
lua_nodelib_new(lua_State *L) {
  int i,j;
  halfword n  = null;
  i = get_node_type_id(L,1);
  if (i<0)
    return 0;
  if (i==whatsit_node) {
    j = -1;
    if (lua_gettop(L)>1) {  j = get_node_subtype_id(L,2);  }
    if (j<0) {
      lua_pushstring(L, "Creating a whatsit requires the subtype number as a second argument");
      lua_error(L);
    }
  } else {
    j = 0;
    if (lua_gettop(L)>1) { j = lua_tointeger(L,2); }
  }
  n = lua_node_new(i,j);
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
  halfword *n;
  halfword m;
  if (lua_isnil(L,1))
    return 1; /* the nil itself */
  n = check_isnode(L,1);
  m = copy_node_list(*n);
  lua_pushnumber(L,m);
  lua_nodelib_push(L);
  return 1;
}

/* (Deep) copy a node */

static int
lua_nodelib_copy(lua_State *L) {
  halfword *n;
  halfword m,p;
  if (lua_isnil(L,1))
    return 1; /* the nil itself */
  n = check_isnode(L,1);
  p = vlink(*n); vlink(*n) = null;
  m = copy_node_list(*n);
  vlink(*n) = p;
  lua_pushnumber(L,m);
  lua_nodelib_push(L);
  return 1;
}

/* A whole set of static data for field information */
/* every node is supposed to have at least "next", 
   "id", and "subtype" */

/* ordinary nodes */
static char * node_fields_list        [] = { "attr", "width", "depth", "height", "shift", "list", 
					     "glue_order", "glue_sign", "glue_set" , "dir" ,  NULL };
static char * node_fields_rule        [] = { "attr", "width", "depth", "height", "dir", NULL };
static char * node_fields_insert      [] = { "attr", "cost",  "depth", "height", "top_skip", "insert", NULL };
static char * node_fields_mark        [] = { "attr", "class", "mark", NULL }; 
static char * node_fields_adjust      [] = { "attr", "list", NULL }; 
static char * node_fields_disc        [] = { "attr", "pre", "post", "replace", NULL };
static char * node_fields_math        [] = { "attr", "surround", NULL }; 
static char * node_fields_glue        [] = { "attr", "spec", "leader", NULL }; 
static char * node_fields_kern        [] = { "attr", "width", NULL };
static char * node_fields_penalty     [] = { "attr", "penalty", NULL };
static char * node_fields_unset       [] = { "attr", "width", "depth", "height", "shrink", "list", 
					     "glue_order", "glue_sign", "stretch" , "dir" , "span",  NULL };
static char * node_fields_margin_kern [] = { "attr", "width", "glyph", NULL };
static char * node_fields_glyph       [] = { "attr", "char", "font", "components", NULL };

/* math nodes and noads */
static char * node_fields_style    [] = { NULL };
static char * node_fields_choice   [] = { NULL };
static char * node_fields_ord      [] = { NULL };
static char * node_fields_op       [] = { NULL };
static char * node_fields_bin      [] = { NULL };
static char * node_fields_rel      [] = { NULL };
static char * node_fields_open     [] = { NULL };
static char * node_fields_close    [] = { NULL };
static char * node_fields_punct    [] = { NULL };
static char * node_fields_inner    [] = { NULL };
static char * node_fields_radical  [] = { NULL };
static char * node_fields_fraction [] = { NULL };
static char * node_fields_under    [] = { NULL };
static char * node_fields_over     [] = { NULL };
static char * node_fields_accent   [] = { NULL };
static char * node_fields_vcenter  [] = { NULL };
static char * node_fields_left     [] = { NULL };
static char * node_fields_right    [] = { NULL };

/* terminal objects */
static char * node_fields_action         [] = { "action_type", "named_id", "action_id", 
						"file", "new_window", "data", "ref_count", NULL };
static char * node_fields_attribute      []  = { "number", "value", NULL };
static char * node_fields_glue_spec      []  = { "width", "stretch", "shrink", 
						 "stretch_order", "shrink_order", "ref_count", NULL };
static char * node_fields_attribute_list []  = { "ref_count", NULL };

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
  NULL,  NULL,  NULL,
  node_fields_action,
  node_fields_margin_kern, /* 40 */
  node_fields_glyph,
  node_fields_attribute,
  node_fields_glue_spec,
  node_fields_attribute_list,
  NULL };


static char * node_fields_whatsit_open               [] = { "attr", "stream", "name", "area", "ext", NULL };
static char * node_fields_whatsit_write              [] = { "attr", "stream", "data", NULL };
static char * node_fields_whatsit_close              [] = { "attr", "stream", NULL };
static char * node_fields_whatsit_special            [] = { "attr", "data", NULL };
static char * node_fields_whatsit_language           [] = { "attr", "lang", "left", "right", NULL };
static char * node_fields_whatsit_local_par          [] = { "attr", "pen_inter", "pen_broken", "dir", 
							    "box_left", "box_left_width", "box_right", "box_right_width", NULL };
static char * node_fields_whatsit_dir                [] = { "attr", "dir", "level", "dvi_ptr", "dvi_h", NULL };

static char * node_fields_whatsit_pdf_literal        [] = { "attr", "mode", "data", NULL };
static char * node_fields_whatsit_pdf_refobj         [] = { "attr", "objnum", NULL };
static char * node_fields_whatsit_pdf_refxform       [] = { "attr", "width", "height", "depth", "objnum", NULL };
static char * node_fields_whatsit_pdf_refximage      [] = { "attr", "width", "height", "depth", "objnum", NULL };
static char * node_fields_whatsit_pdf_annot          [] = { "attr", "width", "height", "depth", "objnum", "data", NULL };
static char * node_fields_whatsit_pdf_start_link     [] = { "attr", "width", "height", "depth", 
							    "objnum", "link_attr", "action", NULL };
static char * node_fields_whatsit_pdf_end_link       [] = { "attr", NULL };
static char * node_fields_whatsit_pdf_dest           [] = { "attr", "width", "height", "depth", 
							    "named_id", "dest_id", "dest_type", "xyz_zoom", "objnum",  NULL };
static char * node_fields_whatsit_pdf_thread         [] = { "attr", "width", "height", "depth", 
							    "named_id", "thread_id", "thread_attr", NULL };
static char * node_fields_whatsit_pdf_start_thread   [] = { "attr", "width", "height", "depth", 
							    "named_id", "thread_id", "thread_attr", NULL };
static char * node_fields_whatsit_pdf_end_thread     [] = { "attr", NULL };
static char * node_fields_whatsit_pdf_save_pos       [] = { "attr", NULL };
static char * node_fields_whatsit_pdf_snap_ref_point [] = { "attr", NULL };
static char * node_fields_whatsit_pdf_snapy          [] = { "attr", "final_skip", "spec", NULL };
static char * node_fields_whatsit_pdf_snapy_comp     [] = { "attr", "comp_ratio", NULL };
static char * node_fields_whatsit_late_lua           [] = { "attr", "reg", "data", NULL };
static char * node_fields_whatsit_close_lua          [] = { "attr", "reg", NULL };
static char * node_fields_whatsit_pdf_colorstack     [] = { "attr", "stack", "cmd", "data", NULL };
static char * node_fields_whatsit_pdf_setmatrix      [] = { "attr", "data", NULL };
static char * node_fields_whatsit_pdf_save           [] = { "attr", NULL };
static char * node_fields_whatsit_pdf_restore        [] = { "attr", NULL };
static char * node_fields_whatsit_user_defined       [] = { "attr", "user_id", "type", "value", NULL };

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
  int j;
  char *s = NULL;
  int i = -2;
  int t = type(node);
  char *** fields = node_fields;
  if (lua_type(L,n)==LUA_TSTRING) {
    s = (char *)lua_tostring(L,n);
    if      (strcmp(s,"prev")    == 0) {  i = -1; } 
    else if (strcmp(s,"next")    == 0) {  i = 0; } 
    else if (strcmp(s,"id")      == 0) {  i = 1; } 
    else if (strcmp(s,"subtype") == 0) {  i = 2; }
    else {
      if (t==whatsit_node) {
	t = subtype(node);
	fields = node_fields_whatsits;
      }
      for (j=0;fields[t][j]!=NULL;j++) {
	if (strcmp(s,fields[t][j])==0) {
	  i=j+3;
	  break;
	}
      }
      if (fields[t][j]==NULL)
	i=-2;
    }
  } else if (lua_isnumber(L,n)) {
    i = lua_tonumber(L,n);
    /* do some test here as well !*/
  }
  if (i==-2) {
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
  lua_pushstring(L,"prev");
  lua_rawseti(L,-2,-1);
  lua_pushstring(L,"next");
  lua_rawseti(L,-2,0);
  lua_pushstring(L,"id");
  lua_rawseti(L,-2,1);
  lua_pushstring(L,"subtype");
  lua_rawseti(L,-2,2);
  for (i=0;fields[t][i]!=NULL;i++) {
    lua_pushstring(L,fields[t][i]);
    lua_rawseti(L,-2,(i+3));
  }
  return 1;
}

/* find the end of a list */

static int
lua_nodelib_tail (lua_State *L) {
  halfword *n;
  halfword t;
  if (lua_isnil(L,1))
    return 1; /* the nil itself */
  n = check_isnode(L,1);
  t=*n;
  if (t==null)
    return 1; /* the old userdata */
  alink(t) = null;
  while (vlink(t)!=null) {
    alink(vlink(t)) = t;
    t = vlink(t);
  }
  lua_pushnumber(L,t);
  lua_nodelib_push(L);
  return 1;
}



/* a utility function for attribute stuff */

static int
lua_nodelib_has_attribute (lua_State *L) {
  int i;
  halfword *n;
  halfword t;
  if (lua_isnil(L,1))
    return 1; /* the nil itself */
  n = check_isnode(L,1);
  t=*n;
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
#define nodelib_pushaction(L,n) { lua_pushnumber(L,n); lua_nodelib_push(L); }
#define nodelib_pushstring(L,n) { lua_pushstring(L,makecstring(n)); }

static void
lua_nodelib_getfield_whatsit  (lua_State *L, int n, int field) {
  if (field==2) {
    lua_pushnumber(L,subtype(n));
  } else if (field==3){
    nodelib_pushattr(L,node_attr(n));
  } else {
  switch (subtype(n)) {
  case open_node:               
    switch (field) {
    case  4: lua_pushnumber(L,write_stream(n));              break;
    case  5: nodelib_pushstring(L,open_name(n));             break;
    case  6: nodelib_pushstring(L,open_area(n));             break;
    case  7: nodelib_pushstring(L,open_ext(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case write_node:              
    switch (field) {
    case  4: lua_pushnumber(L,write_stream(n));              break;
    case  5: tokenlist_to_lua(L,write_tokens(n));            break;
    default: lua_pushnil(L); 
    }
    break;
  case close_node:              
    switch (field) {
    case  4: lua_pushnumber(L,write_stream(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case special_node:            
    switch (field) {
    case  4: tokenlist_to_luastring(L,write_tokens(n));      break;
    default: lua_pushnil(L); 
    }
    break;
  case language_node:           
    switch (field) {
    case  4: lua_pushnumber(L,what_lang(n));                 break;
    case  5: lua_pushnumber(L,what_lhm(n));                  break;
    case  6: lua_pushnumber(L,what_rhm(n));                  break;
    default: lua_pushnil(L); 
    }
    break;
  case local_par_node:          
    switch (field) {
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
    case  4: lua_pushnumber(L,dir_dir(n));                   break;
    case  5: lua_pushnumber(L,dir_level(n));                 break;
    case  6: lua_pushnumber(L,dir_dvi_ptr(n));               break;
    case  7: lua_pushnumber(L,dir_dvi_h(n));                 break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_literal_node:
    switch (field) {
    case  4: lua_pushnumber(L,pdf_literal_mode(n));          break;
    case  5: tokenlist_to_luastring(L,pdf_literal_data(n));  break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refobj_node:         
    switch (field) {
    case  4: lua_pushnumber(L,pdf_obj_objnum(n));            break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refxform_node:       
    switch (field) {
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_xform_objnum(n));          break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_refximage_node:      
    switch (field) {
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_ximage_objnum(n));         break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_annot_node:          
    switch (field) {
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_annot_objnum(n));          break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_start_link_node:     
    switch (field) {
    case  4: lua_pushnumber(L,pdf_width(n));                 break;
    case  5: lua_pushnumber(L,pdf_height(n));                break;
    case  6: lua_pushnumber(L,pdf_depth(n));                 break;
    case  7: lua_pushnumber(L,pdf_link_objnum(n));           break;
    case  8: tokenlist_to_luastring(L,pdf_link_attr(n));     break;
    case  9: nodelib_pushaction(L,pdf_link_action(n));       break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_end_link_node:       
    lua_pushnil(L); 
    break;
  case pdf_dest_node:           
    switch (field) {
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
  case pdf_save_pos_node:       
  case pdf_snap_ref_point_node: 
    lua_pushnil(L); 
    break;
  case pdf_snapy_node:          
    switch (field) {
    case  4: lua_pushnumber(L,final_skip(n));	             break;
    case  5: nodelib_pushspec(L,snap_glue_ptr(n));	     break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_snapy_comp_node:     
    switch (field) {
    case  4: lua_pushnumber(L,snapy_comp_ratio(n));	     break;
    default: lua_pushnil(L); 
    }
    break;
  case late_lua_node:           
    switch (field) {
    case  4: lua_pushnumber(L,late_lua_reg(n));              break;
    case  5: tokenlist_to_luastring(L,late_lua_data(n));     break;
    default: lua_pushnil(L); 
    }
    break;
  case close_lua_node:          
    switch (field) {
    case  4: lua_pushnumber(L,late_lua_reg(n));              break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_colorstack_node:     
    switch (field) {
    case  4: lua_pushnumber(L,pdf_colorstack_stack(n));      break;
    case  5: lua_pushnumber(L,pdf_colorstack_cmd(n));        break;
    case  6: tokenlist_to_luastring(L,pdf_colorstack_data(n)); break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_setmatrix_node:      
    switch (field) {
    case  4: tokenlist_to_luastring(L,pdf_setmatrix_data(n)); break;
    default: lua_pushnil(L); 
    }
    break;
  case pdf_save_node:           
  case pdf_restore_node:        
    lua_pushnil(L); 
    break;
  case user_defined_node:       
    switch (field) {
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
}


static int
lua_nodelib_getfield  (lua_State *L) {
  halfword *n_ptr, n;
  int field;
  if (lua_isnil(L,1))
    return 1; /* a nil */
  n_ptr = check_isnode(L,1);
  n = *n_ptr;
  field = get_node_field_id(L,2, n);
  if (field<-1)
    return;
  if (field==0) {
    lua_pushnumber(L,vlink(n));
    lua_nodelib_push(L);
  } else if (field==1) {
    lua_pushnumber(L,type(n));
  } else if (field==-1) {
    lua_pushnumber(L,alink(n));
    lua_nodelib_push(L);
  } else {
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
      case  2: lua_pushnumber(L,0);                   break;
      case  3: nodelib_pushattr(L,node_attr(n));      break;
      case  4: nodelib_pushlist(L,pre_break(n));      break;
      case  5: nodelib_pushlist(L,post_break(n));     break;
      case  6: lua_pushnumber(L,replace_count(n));    break;
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
    case action_node:
      switch (field) {
      case  2: /* dummy subtype */                            break;
      case  3: lua_pushnumber(L,pdf_action_type(n));          break;
      case  4: lua_pushnumber(L,pdf_action_named_id(n));      break;
      case  5: if (pdf_action_named_id(n)==1) {
	  tokenlist_to_luastring(L,pdf_action_id(n));
	} else {
	  lua_pushnumber(L,pdf_action_id(n));
	}                                                     break;
      case  6: tokenlist_to_luastring(L,pdf_action_file(n));  break;
      case  7: lua_pushnumber(L,pdf_action_new_window(n));    break;
      case  8: tokenlist_to_luastring(L,pdf_action_tokens(n));break;
      case  9: lua_pushnumber(L,pdf_action_refcount(n));      break;
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
#define nodelib_getaction      nodelib_getlist
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
  if (field==3) {
    node_attr(n) = nodelib_getattr(L,3); 
  } else {
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
    case  4: write_tokens(n) = nodelib_gettoks(L,3);        break;
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
    case  9: pdf_link_action(n) = nodelib_getaction(L,3);   break;
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
  case pdf_save_pos_node:       
  case pdf_snap_ref_point_node: 
    return nodelib_cantset(L,field,n);
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
  case pdf_restore_node:        
    return nodelib_cantset(L,field,n);
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
  return 0;
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
  if (field<-1)
    return 0;
  if (field==0) {
    vlink(n) = nodelib_getlist(L,3);
  } else if (field==-1) {
    alink(n) = nodelib_getlist(L,3);
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
    case action_node:
      switch (field) {
      case  2: /* dummy subtype */                            break;
      case  3: pdf_action_type(n)     = lua_tointeger(L,3);   break;
      case  4: pdf_action_named_id(n) = lua_tointeger(L,3);   break;
      case  5: if (pdf_action_named_id(n)==1) {
	  pdf_action_id(n) = nodelib_gettoks(L,3);         
	} else {
	  pdf_action_id(n) = lua_tointeger(L,3);         
	}                                                     break;
      case  6: pdf_action_file(n) =  nodelib_gettoks(L,3);    break;
      case  7: pdf_action_new_window(n) = lua_tointeger(L,3); break;
      case  8: pdf_action_tokens(n) =  nodelib_gettoks(L,3);  break;
      case  9: pdf_action_refcount(n) = lua_tointeger(L,3);   break;
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
  halfword *n;
  n = check_isnode(L,1);
  lua_pushfstring(L,"<node (%d<) %d (>%d): %s>", 
		  (alink(*n)==null ? -1 : alink(*n)),
		  *n,
		  (vlink(*n)==null ? -1 : vlink(*n)),
		  node_names[type(*n)]);
  return 1;
}


static const struct luaL_reg nodelib_f [] = {
  {"id",            lua_nodelib_id},
  {"type",          lua_nodelib_type},
  {"new",           lua_nodelib_new},
  {"slide",         lua_nodelib_tail},
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
nodelist_to_lua (lua_State *L, halfword n) { 
  lua_pushnumber(L,n);
  lua_nodelib_push(L);
}

halfword
nodelist_from_lua (lua_State *L) { 
  halfword *n;
  if (lua_isnil(L,-1))
    return null;
  n = check_isnode(L,-1);
  return *n;
}


