/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#define append_node(t,a)   { if (a) { link(a) = null; link(t) = a;  t = a;  } }
#define small_node_size 2


static char * node_names[] = {
  "hlist", /* 0 */
  "vlist",  
  "rule",
  "ins", 
  "mark", 
  "adjust",
  "ligature", 
  "disc", 
  "whatsit",
  "math", 
  "glue",  /* 10 */
  "kern",
  "penalty", 
  "unset", 
  "style",
  "choice",  
  "ord",
  "op",
  "bin",
  "rel",
  "open", /* 20 */
  "close",
  "punct",
  "inner",
  "radical",
  "fraction",
  "under",
  "over",
  "accent",
  "vcenter",
  "left", /* 30 */
  "right",
  "!","!","!","!","!",
  /* Thanh lost count, and set margin kern to 40 arbitrarily.
    I am now misusing that as a spine for my own new nodes*/
  "!",
  "!",
  "glyph",  
  "margin_kern", /* 40 */
   NULL };


void 
glyph_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"glyph");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,0);
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,character(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,font(p));
  lua_rawseti(L,-2,i++);
}

void 
ligature_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"glyph");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,(subtype(p)+1));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,character(lignode_char(p)));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,font(lignode_char(p)));
  lua_rawseti(L,-2,i++);
  nodelist_to_lua(L,lig_ptr(p));
  lua_rawseti(L,-2,i++);
}


halfword 
glyph_node_from_lua (lua_State *L) {
  int f,c,t;
  halfword p;
  int i = 2;
  lua_rawgeti(L,-1,i++);
  t = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  f = lua_tonumber(L,-1);
  lua_pop(L,1);
  if (t==0) { /* char node */
    p = get_avail();
    link(p) = null;
    character(p) = c;
    font(p) = f;
  } else {
    lua_rawgeti(L,-1,i++);
    l = nodelist_from_lua(L);
    lua_pop(L,1);
    p = new_ligature(f,c,l);
    subtype(p) = (t-1);
  }
  return p;
}


void
rule_node_to_lua(lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"rule");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,0);
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,width(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,depth(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,height(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,rule_dir(p));
  lua_rawseti(L,-2,i++);
}

halfword
rule_node_from_lua(lua_State *L) {
  halfword p;
  p = new_rule();
  lua_rawgeti(L,-1,3);
  width(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,4);
  depth(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,5);
  height(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,6);
  rule_dir(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  return p;
}


void
disc_node_to_lua(lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"disc");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  nodelist_to_lua(L,pre_break(p));
  lua_rawseti(L,-2,i++);
  nodelist_to_lua(L,post_break(p));
  lua_rawseti(L,-2,i++);
}

halfword
disc_node_from_lua(lua_State *L) {
  halfword p;
  p = new_disc();
  lua_rawgeti(L,-1,2);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,3);
  pre_break(p) = nodelist_from_lua(L);
  lua_pop(L,1);
  lua_rawgeti(L,-1,4);
  post_break(p) = nodelist_from_lua(L);
  lua_pop(L,1);
  return p;
}

void
list_node_to_lua(lua_State *L, int list_type_, halfword p) {
  int i = 1;
  luaL_checkstack(L,2,"out of stack space");
  lua_newtable(L);
  lua_pushstring(L,node_names[list_type_]);
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,width(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,depth(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,height(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,shift_amount(p));
  lua_rawseti(L,-2,i); i++;
  
  nodelist_to_lua(L,list_ptr(p));
  lua_rawseti(L,-2,i); i++;
  
  lua_pushnumber(L,glue_order(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,glue_sign(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,glue_set(p));
  lua_rawseti(L,-2,i); i++;
  lua_pushnumber(L,box_dir(p));
  lua_rawseti(L,-2,i); i++;
}


halfword
list_node_from_lua(lua_State *L, int list_type_) {
  int i = 2;
  int p = new_null_box();
  type(p)    = list_type_;
  lua_rawgeti(L,-1,i++);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  width(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  depth(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  height(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  shift_amount(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  list_ptr(p) = nodelist_from_lua(L);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  glue_order(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  glue_sign(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  glue_set(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  box_dir(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  return p;
}


void
penalty_node_to_lua(lua_State *L, halfword p) {
  lua_newtable(L);
  lua_pushstring(L,"penalty");
  lua_rawseti(L,-2,1);
  lua_pushnumber(L,0);
  lua_rawseti(L,-2,2);
  lua_pushnumber(L,penalty(p));
  lua_rawseti(L,-2,3);
}

halfword
penalty_node_from_lua(lua_State *L) {
  halfword p;
  p = new_penalty(0);
  lua_rawgeti(L,-1,3);
  penalty(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  return p;
}


void 
glue_node_to_lua (lua_State *L, halfword p) {
  halfword q;
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"glue");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);

  q = glue_ptr(p);
  lua_pushnumber(L,width(q));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,stretch(q));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,stretch_order(q));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,shrink(q));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,shrink_order(q));
  lua_rawseti(L,-2,i++);
  if (leader_ptr(p)!=null) {
    nodelist_to_lua(L,leader_ptr(p));
    lua_rawseti(L,-2,i++);
  }
}

halfword 
glue_node_from_lua (lua_State *L) {
  halfword n,q;
  q = new_spec(0); /* 0 == the null glue at zmem[0] */
  n = new_glue(q);
  lua_rawgeti(L,-1,2);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  subtype(n) = c;

  lua_rawgeti(L,-1,3);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  width(q) = c;

  lua_rawgeti(L,-1,4);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  stretch(q) = c;

  lua_rawgeti(L,-1,5);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  stretch_order(q) = c;

  lua_rawgeti(L,-1,6);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  shrink(q) = c;

  lua_rawgeti(L,-1,7);
  c = lua_tonumber(L,-1);
  lua_pop(L,1);
  shrink_order(q) = c;

  glue_ptr(n) = q;

  /* nodelist_to_lua(L,leader_ptr(p));*/
  lua_rawgeti(L,-1,8);
  if (lua_istable(L,-1)) {
    fprintf(stdout,"<node type leader not readable yet>\n");
  }
  lua_pop(L,1);
  return n;
}


void 
kern_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"kern");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,width(p));
  lua_rawseti(L,-2,i++);
}

halfword 
kern_node_from_lua (lua_State *L) {
  halfword p;
  p = new_kern(0);
  lua_rawgeti(L,-1,2);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,3);
  width(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  return p;
}


void 
margin_kern_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"margin_kern");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,width(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,font(margin_char(p)));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,character(margin_char(p)));
  lua_rawseti(L,-2,i++);
}


extern void tokenlist_to_lua(lua_State *L, halfword p) ;

void 
mark_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"mark");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,mark_class(p));
  lua_rawseti(L,-2,i++);
  tokenlist_to_lua(L,mark_ptr(p));
  lua_rawseti(L,-2,i++);
}

halfword 
mark_node_from_lua (lua_State *L) {
  halfword p;
  p = get_node(small_node_size);
  lua_rawgeti(L,-1,2);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,3);
  mark_class(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,4);
  mark_ptr(p) = tokenlist_from_lua(L);
  lua_pop(L,1);
  return p;
}

void 
math_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"math");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,width(p));
  lua_rawseti(L,-2,i++);
}

halfword
math_node_from_lua  (lua_State *L) {
  halfword p;
  p = new_math(0,0);
  lua_rawgeti(L,-1,2);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,3);
  width(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  return p;
}

void 
adjust_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"adjust");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  list_node_to_lua(L,vlist_node,adjust_ptr(p));
  lua_rawseti(L,-2,i++);
}

halfword
adjust_node_from_lua  (lua_State *L) {
  halfword p;
  p = get_node(small_node_size);
  lua_rawgeti(L,-1,2);
  subtype(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,3);
  adjust_ptr(p) = nodelist_from_lua(L);
  lua_pop(L,1);
  return p;
}


void 
nodelist_to_lua (lua_State *L, halfword t) {
  int i = 0;
  int f = 0;
  lua_newtable(L);
  if (t == null || t == 0)
    return;
  luaL_checkstack(L,2,"out of stack space");
  do {
    if (is_char_node(t)) {
      glyph_node_to_lua(L, t);
      lua_rawseti(L,-2,++i);
    } else {
      switch (type(t)) {
      case hlist_node:
      case vlist_node:
	list_node_to_lua(L,hlist_node,t); 
	lua_rawseti(L,-2,++i); 
	break;
      case rule_node: 
	rule_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case mark_node: 
	mark_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case adjust_node: 
	adjust_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case ligature_node: 
	ligature_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case disc_node: 
	disc_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case whatsit_node: 
	whatsit_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case math_node: 
	math_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case glue_node: 
	glue_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case kern_node: 
	kern_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case penalty_node: 
	penalty_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      case margin_kern_node: 
	margin_kern_node_to_lua(L,t); 
	lua_rawseti(L,-2,++i);
	break;
      default: 
	if (type(t)<=right_node) {
	  fprintf(stdout,"<node type %s not supported yet>\n",node_names[type(t)]);
	} else {
	  fprintf(stdout,"<unknown node type %d>\n", type(t));
	}
	break;
      }
    }
    t = link(t);

  } while (t>mem_min);
}


halfword
nodelist_from_lua (lua_State *L) {
  char *s; 
  int i; /* general counter */
  int f; /* current font */
  int t, u, head;
  if (!lua_istable(L,-1)) {
    return 0;
  }
  t = get_avail();
  link(t) = null;
  head = t;
  lua_pushnil(L);
  while (lua_next(L,-2) != 0) {
    if (lua_istable(L,-1)) {
      lua_rawgeti(L,-1,1);
      if (lua_isstring(L,-1)) { /* it is a node */
	s = (char *)lua_tostring(L,-1);
	for (i=0;node_names[i]!=NULL;i++) {
	  if (strcmp(s,node_names[i])==0)
	    break;
	}
	lua_pop(L,1); /* the string */
	if (i<=margin_kern_node) {
	  switch (i) {
 	  case hlist_node:
	  case vlist_node:
	    u = list_node_from_lua(L,i);
	    append_node(t,u);
	    break;
	  case rule_node: 
	    u = rule_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case mark_node: 
	    u = mark_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case adjust_node: 
	    u = adjust_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case disc_node: 
 	    u = disc_node_from_lua(L);
 	    append_node(t,u);
 	    break;
	  case whatsit_node: 
	    u = whatsit_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case math_node: 
	    u = math_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case glue_node: 
	    u = glue_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case kern_node: 
	    u = kern_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case penalty_node: 
	    u = penalty_node_from_lua(L);
	    append_node(t,u);
	    break;
	  case glyph_node: 
	    u = glyph_node_from_lua(L);
	    append_node(t,u);
	    break;
            /*
	  case margin_kern_node: 
	    break;
	    */
	  default: 
	    fprintf(stdout,"<node type %s not supported yet>\n",node_names[i]);
	    break;
	  }
	}
      } else {
	/* not decided yet */
	lua_pop(L,1); /* the not-a-string */
      }
    }
    lua_pop(L, 1);
  }
  t = link(head);
  link(head) = null;
  free_avail(head);
  return t;
}


halfword
pre_linebreak_filter (halfword head_node) {
  halfword ret;  
  integer callback_id ; 
  lua_State *L = Luas[0];
  callback_id = callback_defined("linebreak_filter");
  if (callback_id==0) {
    return head_node;
  }
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return head_node;
  }
  nodelist_to_lua(L,head_node);
  if (lua_pcall(L,1,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  if ((ret = nodelist_from_lua(L)) != 0) {
    lua_pop(L,1); /* result */
    lua_pop(L,1); /* callback container table */
    /* destroy the old one */
    /* return head_node;*/
    show_node_list(ret);
    return ret;
  } 
  lua_pop(L,1); /* result */
  lua_pop(L,1); /* callback container table */
  return head_node;
}
