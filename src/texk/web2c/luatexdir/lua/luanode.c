/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#define append_node(t,a)   { if (a!=null) { link(a) = null; link(t) = a;  t = a;  } }


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
generic_node_to_lua (lua_State *L, char *name, char *fmt, ...) {
  va_list args;
  int val;
  double fval;
  int i = 1;
  lua_createtable(L,(strlen(fmt)+1),0);
  lua_pushstring(L,name);
  lua_rawseti(L,-2,i++);
  va_start(args,fmt);
  while (*fmt) {
    switch(*fmt++) {
    case 'a':           /* int */
      val = va_arg(args, int);
      action_node_to_lua(L,val);
      lua_rawseti(L,-2,i++);
      break;
    case 'd':           /* int */
      val = va_arg(args, int);
      lua_pushnumber(L,val);
      lua_rawseti(L,-2,i++);
      break;
    case 'f':           /* float */
      fval = va_arg(args, double);
      lua_pushnumber(L,fval);
      lua_rawseti(L,-2,i++);
      break;
    case 'n':           /* nodelist */
      val = va_arg(args, int);
      if (val!=null) {
	nodelist_to_lua(L,val);
	lua_rawseti(L,-2,i++);
      } else {
	i++;
      }
      break;
    case 's':           /* strnumber */
      val = va_arg(args, int);
      lua_pushstring(L,makecstring(val));
      lua_rawseti(L,-2,i++);
      break;
    case 't':           /* tokenlist */
      val = va_arg(args, int);
      if (val == null) {
	lua_pushnil(L);
      } else {
	tokenlist_to_lua(L,link(val));
      }
      lua_rawseti(L,-2,i++);
      break;
    }
  }
  va_end(args);
}



void 
glyph_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"glyph","ddd",0,character(p),font(p));
}

void 
ligature_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"glyph","dddn",(subtype(p)+1),
		      character(lignode_char(p)),font(lignode_char(p)),lig_ptr(p));
}

halfword 
glyph_node_from_lua (lua_State *L) {
  int f,c,t,l;
  halfword p;
  int i = 2;
  numeric_field(t,i++);
  numeric_field(c,i++);
  numeric_field(f,i++);
  if (t==0) { /* char node */
    p = get_avail();
    character(p) = c;
    font(p) = f;
  } else {
    nodelist_field(l,i++); 
    p = new_ligature(f,c,l);
    subtype(p) = (t-1);
  }
  link(p) = null;
  return p;
}

void
rule_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"rule","ddddd",0,width(p),depth(p),height(p),rule_dir(p));
}

halfword
rule_node_from_lua(lua_State *L) {
  int i = 3;
  halfword p = new_rule();
  link(p)=null; 
  numeric_field(width(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(rule_dir(p),i++);
  return p;
}

void
disc_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"disc","dnn",subtype(p),pre_break(p),post_break(p));
}

halfword
disc_node_from_lua(lua_State *L) {
  int i = 2;
  halfword p = new_disc();
  link(p)=null; 
  numeric_field(subtype(p),i++);
  nodelist_field(pre_break(p),i++);
  nodelist_field(post_break(p),i++);
  return p;
}

void
list_node_to_lua(lua_State *L, int list_type_, halfword p) {
  generic_node_to_lua(L,node_names[list_type_],"dddddnddfd",subtype(p),width(p),depth(p),height(p),
		      shift_amount(p),list_ptr(p),glue_order(p),glue_sign(p),glue_set(p),box_dir(p));
}


halfword
list_node_from_lua(lua_State *L, int list_type_) {
  int i = 2;
  int p = new_null_box();
  link(p)=null; 
  type(p)    = list_type_;
  numeric_field(subtype(p),i++);
  numeric_field(width(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(shift_amount(p),i++);
  nodelist_field(list_ptr(p),i++);
  numeric_field(glue_order(p),i++);
  numeric_field(glue_sign(p),i++);
  float_field(glue_set(p),i++);
  numeric_field(box_dir(p),i++);
  return p;
}

void
penalty_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"penalty","dd",0,penalty(p));
}

halfword
penalty_node_from_lua(lua_State *L) {
  halfword p = new_penalty(0);
  link(p)=null; 
  numeric_field(penalty(p),3);
  return p;
}

void 
glue_node_to_lua (lua_State *L, halfword p) {
  halfword q = glue_ptr(p);
  generic_node_to_lua(L,"glue","ddddddn",subtype(p),
		      width(q),stretch(q),stretch_order(q),
		      shrink(q),shrink_order(q),leader_ptr(p));
}

halfword 
glue_node_from_lua (lua_State *L) {
  int i = 2;
  halfword q = new_spec(0); /* 0 == the null glue at zmem[0] */
  halfword p = new_glue(q);
  link(p)=null; 
  numeric_field (subtype(p),i++);
  numeric_field (width(q),i++);
  numeric_field (stretch(q),i++);
  numeric_field (stretch_order(q),i++);
  numeric_field (shrink(q),i++);
  numeric_field (shrink_order(q),i++);
  nodelist_field(leader_ptr(p),i++);
  return p;
}

void 
kern_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"kern","dd",subtype(p),width(p));
}

halfword 
kern_node_from_lua (lua_State *L) {
  halfword p;
  p = new_kern(0);
  link(p)=null; 
  numeric_field(subtype(p),2);
  numeric_field(width(p),3);
  return p;
}

void 
margin_kern_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"margin_kern","dddd",subtype(p),width(p),
		      font(margin_char(p)),character(margin_char(p)));
}


halfword 
margin_kern_node_from_lua (lua_State *L) {
  int i = 2;
  halfword p = get_node(margin_kern_node_size);
  link(p)=null; 
  type(p) = margin_kern_node;
  numeric_field(subtype(p),i++);
  numeric_field(width(p),i++);
  numeric_field(font(margin_char(p)),i++);
  numeric_field(character(margin_char(p)),i++);
  return p;
}

void 
mark_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"mark","ddt",subtype(p),mark_class(p),mark_ptr(p));
}

halfword 
mark_node_from_lua (lua_State *L) {
  int i = 2;
  halfword p = get_node(small_node_size);
  link(p)=null; 
  type(p) = mark_node;
  numeric_field  (subtype(p),i++);
  numeric_field  (mark_class(p),i++);
  tokenlist_field(mark_ptr(p),i++);
  return p;
}

void 
math_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"math","dd",subtype(p),width(p));
}

halfword
math_node_from_lua  (lua_State *L) {
  halfword p = new_math(0,0);
  link(p)=null; 
  numeric_field(subtype(p),2);
  numeric_field(width(p),3);
  return p;
}

void 
adjust_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"adjust","dn",subtype(p),adjust_ptr(p));
}

halfword
adjust_node_from_lua  (lua_State *L) {
  halfword p = get_node(small_node_size);
  link(p)=null; 
  type(p)= adjust_node;
  numeric_field(subtype(p),2);
  nodelist_field(adjust_ptr(p),3);
  return p;
}


void 
nodelist_to_lua (lua_State *L, halfword t) {
  halfword v = t;
  int i = 1;
  luaL_checkstack(L,2,"out of stack space");
  if (t==null) {
    lua_createtable(L,0,0);
    return;
  }
  while (link(v)!=null) { i++;  v = link(v);  }
  lua_createtable(L,i,0);
  i = 0;
  do {
    if (is_char_node(t)) {
      glyph_node_to_lua(L, t);
      lua_rawseti(L,-2,++i);
    } else {
      switch (type(t)) {
      case hlist_node:
      case vlist_node:
	list_node_to_lua(L,type(t),t); 
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
	if (type(t)<=right_noad) {
	  fprintf(stdout,"<noad type %s not supported>\n",node_names[type(t)]);
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
  int t, u, head;
  if (!lua_istable(L,-1)) {
    return null;
  }
  t = get_avail();
  link(t) = null;
  head = t;
  luaL_checkstack(L,3,"out of lua memory");
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
	  case margin_kern_node: 
	    u = margin_kern_node_from_lua(L);
	    append_node(t,u);
	    break;
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
lua_node_filter (char *filtername, halfword head_node) {
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
  /*
  depth_threshold = 100000;
  breadth_max = 100000;
  print_ln();
  fprintf(stdout,"\n=>%s",filtername);
  fprintf(log_file,"\n=>%s",filtername);
  show_node_list(head_node);
  */
  nodelist_to_lua(L,head_node);
  if (lua_pcall(L,1,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  /* destroy the old one */
  flush_node_list(head_node);
  if ((ret = nodelist_from_lua(L)) != 0) {
    lua_pop(L,2); /* result and callback container table */
    return ret;
  } 
  lua_pop(L,2); /* result and callback container table */
  fprintf(stdout,"error: node list has gone to the moon\n");
  error();
  return null;
}
