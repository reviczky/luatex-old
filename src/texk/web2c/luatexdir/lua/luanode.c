/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#undef link /* defined by cpascal.h */
#define info(a)    fixmem[(a)].hhlh
#define link(a)    fixmem[(a)].hhrh

#define append_node(t,a)   { if (a!=null) { vlink(a) = null; vlink(t) = a;  t = a;  } }

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
  "!",  
  "margin_kern", /* 40 */
  "glyph",
  "attribute",
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
    case 'a':           /* action, int */
      val = va_arg(args, int);
      if (val==null) {
        lua_pushnil(L);
      } else {
        attribute_list_to_lua(L,val);
      }
      lua_rawseti(L,-2,i++);
      break;
    case 'b':           /* boolean, int */
      val = va_arg(args, int);
      lua_pushboolean(L,val);
      lua_rawseti(L,-2,i++);
      break;
    case 'c':           /* action, int */
      val = va_arg(args, int);
      action_node_to_lua(L,val);
      lua_rawseti(L,-2,i++);
      break;
    case 'd':           /* number, int */
      val = va_arg(args, int);
      lua_pushnumber(L,val);
      lua_rawseti(L,-2,i++);
      break;
    case 'f':           /* float, double */
      fval = va_arg(args, double);
      lua_pushnumber(L,fval);
      lua_rawseti(L,-2,i++);
      break;
    case 'n':           /* nodelist */
      val = va_arg(args, int);
      if (val==null) {
        lua_pushnil(L);
      } else {
        nodelist_to_lua(L,val);
      }
      lua_rawseti(L,-2,i++);
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
attribute_list_to_lua (lua_State *L, halfword p) {
  integer q=vlink(p);
  lua_newtable(L);
  while (q!=null && type(q)==attribute_node) {
    lua_pushnumber(L,attribute_value(q));
    lua_rawseti(L,-2,attribute_id(q)); 
    q = vlink(q);
  }
}

halfword 
attribute_list_from_lua (lua_State *L) {
  halfword p,q,s;
  int k;
  integer v;
  if (!lua_istable(L,-1))
    return null;
  p = get_node(temp_node_size);
  vinfo(p)=null; /* refcount=0*/
  q = p;
  for (k=0;k<256;k++) {
    lua_rawgeti(L,-1,k);
    if (lua_isnumber(L,-1)) {
      v =lua_tonumber(L,-1);
      if (v>=0) {
        s = new_attribute_node(k,v);
        vlink(p) = s;
        p = vlink(p);
      }
    }
    lua_pop(L,1);
  }
  if (q==p) {
    free_node(p,temp_node_size);
    q = null;
  }
  return q;
}

void 
glyph_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"glyph","daddn",0,node_attr(p),character(p),font(p),lig_ptr(p));
}

void 
ligature_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"glyph","daddn",(subtype(p)+1),node_attr(p),
                      character(p),font(p),lig_ptr(p));
}

halfword 
glyph_node_from_lua (lua_State *L) {
  int f,c,t,l,a;
  halfword p;
  int i = 2;
  numeric_field(t,i++);
  attributes_field(a,i++);
  numeric_field(c,i++);
  numeric_field(f,i++);
  if (t==0) { /* char node */
    i++;
    p = new_glyph(f,c);
  } else {
    nodelist_field(l,i++); 
    p = new_ligature(f,c,l);
    subtype(p) = (t-1);
  }
  delete_attribute_ref(node_attr(p)); 
  node_attr(p)=a; 
  return p;
}

void
rule_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"rule","dadddd",0,node_attr(p),
                      width(p),depth(p),height(p),rule_dir(p));
}

halfword
rule_node_from_lua(lua_State *L) {
  int a;
  int i = 3;
  halfword p = new_rule();
  delete_attribute_ref(node_attr(p)); 
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(width(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(rule_dir(p),i++);
  return p;
}

void
ins_node_to_lua(lua_State *L, halfword p) {
  halfword q = split_top_ptr(p);
  generic_node_to_lua(L,"rule","daddddddddn",subtype(p),node_attr(p),
                      float_cost(p),depth(p),height(p),
		      width(q),stretch(q),stretch_order(q),
                      shrink(q),shrink_order(q),ins_ptr(p));
}

halfword
ins_node_from_lua(lua_State *L) {
  int a;
  int i = 2;
  halfword p = get_node(ins_node_size);
  halfword q = new_spec(zero_glue);
  type(p) = ins_node;
  numeric_field(subtype(p),i++);
  attributes_field(a,i++); node_attr(p)=a;  
  numeric_field(float_cost(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(width(q),i++);
  numeric_field(stretch(q),i++);
  numeric_field(stretch_order(q),i++);
  numeric_field(shrink(q),i++);
  numeric_field(shrink_order(q),i++);
  split_top_ptr(p) = q;
  nodelist_field(a,i++); ins_ptr(p)=a;
  return p;
}


void
disc_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"disc","dann",subtype(p),node_attr(p),
                      pre_break(p),post_break(p));
}

halfword
disc_node_from_lua(lua_State *L) {
  int a;
  int i = 2;
  halfword p = new_disc();
  delete_attribute_ref(node_attr(p)); 
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  nodelist_field(a,i++); pre_break(p)=a;
  nodelist_field(a,i++); post_break(p)=a;
  return p;
}

void
list_node_to_lua(lua_State *L, int list_type_, halfword p) {
  generic_node_to_lua(L,node_names[list_type_],"daddddnddfd",subtype(p),node_attr(p),width(p),depth(p),height(p),
                      shift_amount(p),list_ptr(p),glue_order(p),glue_sign(p),glue_set(p),box_dir(p));
}



halfword
list_node_from_lua(lua_State *L, int list_type_) {
  int a;
  int i = 2;
  int p = new_null_box();
  delete_attribute_ref(node_attr(p)); 
  type(p) = list_type_;
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(width(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(shift_amount(p),i++);
  nodelist_field(a,i++); list_ptr(p)=a;
  numeric_field(glue_order(p),i++);
  numeric_field(glue_sign(p),i++);
  float_field(glue_set(p),i++);
  numeric_field(box_dir(p),i++);
  return p;
}

void
unset_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"unset","daddddndddd",span_count(p),node_attr(p),width(p),depth(p),height(p),
                      glue_shrink(p),list_ptr(p),glue_order(p),glue_sign(p),glue_stretch(p),box_dir(p));
}


halfword
unset_node_from_lua(lua_State *L) {
  int a;
  int i = 2;
  int p = new_null_box();
  delete_attribute_ref(node_attr(p)); 
  type(p) = unset_node;
  numeric_field(span_count(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(width(p),i++);
  numeric_field(depth(p),i++);
  numeric_field(height(p),i++);
  numeric_field(glue_shrink(p),i++);
  nodelist_field(a,i++); list_ptr(p)=a;
  numeric_field(glue_order(p),i++);
  numeric_field(glue_sign(p),i++);
  numeric_field(glue_stretch(p),i++);
  numeric_field(box_dir(p),i++);
  return p;
}

void
penalty_node_to_lua(lua_State *L, halfword p) {
  generic_node_to_lua(L,"penalty","dad",0,node_attr(p),penalty(p));
}

halfword
penalty_node_from_lua(lua_State *L) {
  int a;
  int i = 3;
  halfword p = new_penalty(0);
  delete_attribute_ref(node_attr(p)); 
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(penalty(p),i++);
  return p;
}

void 
glue_node_to_lua (lua_State *L, halfword p) {
  halfword q = glue_ptr(p);
  generic_node_to_lua(L,"glue","dadddddn",subtype(p),node_attr(p),
                      width(q),stretch(q),stretch_order(q),
                      shrink(q),shrink_order(q),leader_ptr(p));
}

halfword 
glue_node_from_lua (lua_State *L) {
  int a;
  int i = 2;
  halfword q = get_node(glue_spec_size);
  halfword p = get_node(glue_node_size); 
  type(p)=glue_node; 
  leader_ptr(p)=null;
  glue_ptr(p)=q;
  numeric_field (subtype(p),i++);
  attributes_field(a,i++);   node_attr(p)=a;
  numeric_field (width(q),i++);
  numeric_field (stretch(q),i++);
  numeric_field (stretch_order(q),i++);
  numeric_field (shrink(q),i++);
  numeric_field (shrink_order(q),i++);
  nodelist_field(a,i++); leader_ptr(p)=a;
  glue_ref_count(q) = null;
  return p;
}

void 
kern_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"kern","dad",subtype(p),node_attr(p),width(p));
}

halfword 
kern_node_from_lua (lua_State *L) {
  int a;
  halfword p;
  int i = 2;
  p = new_kern(0);
  delete_attribute_ref(node_attr(p)); 
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(width(p),i++);
  return p;
}

void 
margin_kern_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"margin_kern","daddd",subtype(p),node_attr(p),width(p),
                      font(margin_char(p)),character(margin_char(p)));
}


halfword 
margin_kern_node_from_lua (lua_State *L) {
  int a;
  int i = 2;
  halfword p = get_node(margin_kern_node_size);
  type(p) = margin_kern_node;
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);    node_attr(p)=a;
  numeric_field(width(p),i++);
  margin_char(p)=new_glyph(0,0);
  a=copy_node_list(node_attr(p));
  node_attr(margin_char(p))=a;
  numeric_field(font(margin_char(p)),i++);
  numeric_field(character(margin_char(p)),i++);
  return p;
}

void 
mark_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"mark","dadt",subtype(p),node_attr(p),mark_class(p),mark_ptr(p));
}

halfword 
mark_node_from_lua (lua_State *L) {
  int a;
  int i = 2;
  halfword p = get_node(mark_node_size);
  type(p) = mark_node;
  numeric_field  (subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field  (mark_class(p),i++);
  tokenlist_field(a,i++); mark_ptr(p)=a;
  return p;
}

void 
math_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"math","dad",subtype(p),node_attr(p),surround(p));
}

halfword
math_node_from_lua  (lua_State *L) {
  int a;
  int i = 2;
  halfword p = new_math(0,0);
  delete_attribute_ref(node_attr(p)); 
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  numeric_field(surround(p),i++);
  return p;
}

void 
adjust_node_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"adjust","dan",subtype(p),node_attr(p),adjust_ptr(p));
}

halfword
adjust_node_from_lua  (lua_State *L) {
  int a;
  int i = 2;
  halfword p = get_node(adjust_node_size);
  type(p)= adjust_node;
  numeric_field(subtype(p),i++);
  attributes_field(a,i++);  node_attr(p)=a;
  nodelist_field(a,i++); adjust_ptr(p)=a;
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
  while (vlink(v)!=null) { i++;  v = vlink(v);  }
  lua_createtable(L,i,0);
  i = 0;
  while (t!=null) {
      switch (type(t)) {
      case glyph_node:
        glyph_node_to_lua(L, t);
        lua_rawseti(L,-2,++i);
        break;
      case hlist_node:
      case vlist_node:
        list_node_to_lua(L,type(t),t); 
        lua_rawseti(L,-2,++i); 
        break;
      case rule_node: 
        rule_node_to_lua(L,t); 
        lua_rawseti(L,-2,++i);
        break;
      case ins_node: 
        ins_node_to_lua(L,t); 
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
      case unset_node: 
        unset_node_to_lua(L,t); 
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
    t = vlink(t);
  };
}

halfword
nodelist_from_lua (lua_State *L) {
  char *s; 
  int i; /* general counter */
  int t, u, head;
  if (!lua_istable(L,-1)) {
    return null;
  }
  t = get_node(temp_node_size);
  head = t;
  luaL_checkstack(L,3,"out of lua memory");
  lua_pushnil(L);
  while (lua_next(L,-2) != 0) {
    /* it is more sensible here to ignore rubbish, instead of
       generating an error, because nodes may be deleted on the lua
       side in various ways. Requiring a table to return as a proper
       array would likely result in speed penalties */
    if (lua_istable(L,-1)) {
      lua_rawgeti(L,-1,1); 
      if (lua_isstring(L,-1)) { /* it is a node */
        s = (char *)lua_tostring(L,-1);
        for (i=0;node_names[i]!=NULL;i++) {
          if (strcmp(s,node_names[i])==0)
            break;
        }
        lua_pop(L,1); /* the string */
        if (i<last_known_node) {
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
          case ins_node: 
            u = ins_node_from_lua(L);
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
          case unset_node: 
            u = unset_node_from_lua(L);
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
  t = vlink(head);
  vlink(head) = null;
  free_node(head,temp_node_size);
  return t;
}

char *group_code_names[] = {
  "",
  "simple",
  "hbox",
  "adjusted_hbox",
  "vbox",
  "vtop",
  "align",
  "no_align",
  "output",
  "math",
  "disc",
  "insert",
  "vcenter",
  "math_choice",
  "semi_simple",
  "math_shift",
  "math_left",
  "local_box" ,
  "split_off",
  "split_keep",
  "preamble",
  "align_set",
  "fin_row"};


#define LUA_GC_STEP_SIZE 1

void
lua_node_filter (int filterid, int extrainfo, halfword head_node, halfword *tail_node) {
  halfword ret;  
  int a;
  integer callback_id ; 
  lua_State *L = Luas[0];
  callback_id = callback_defined(filterid);
  if (callback_id==0) {
    return;
  }
  lua_rawgeti(L,LUA_REGISTRYINDEX,callback_callbacks_id);
  lua_rawgeti(L,-1, callback_id);
  if (!lua_isfunction(L,-1)) {
    lua_pop(L,2);
    return;
  }
  /*
   breadth_max=100000;
   depth_threshold=100;
   show_node_list(vlink(head_node));
  */
  nodelist_to_lua(L,vlink(head_node));
  lua_pushstring(L,group_code_names[extrainfo]);
  if (lua_pcall(L,2,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return;
  }
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(vlink(head_node));
      vlink(head_node) = null;
    }
  } else {
    flush_node_list(vlink(head_node));
    a = nodelist_from_lua(L);
    vlink(head_node)= a;
    /*show_node_list(vlink(head_node));*/
  }
  lua_pop(L,2); /* result and callback container table */
  lua_gc(L,LUA_GCSTEP, LUA_GC_STEP_SIZE);
  ret = vlink(head_node); 
  if (ret!=null) {
    while (vlink(ret)!=null)
      ret=vlink(ret); 
  }
  *tail_node=ret;
  return ;
}

char *pack_type_name[] = { "exactly", "additional"};


halfword
lua_hpack_filter (halfword head_node, scaled size, int pack_type, int extrainfo) {
  halfword ret;  
  integer callback_id ; 
  lua_State *L = Luas[0];
  callback_id = callback_defined(hpack_filter_callback);
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
  lua_pushnumber(L,size);
  lua_pushstring(L,pack_type_name[pack_type]);
  lua_pushstring(L,group_code_names[extrainfo]);
  if (lua_pcall(L,4,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  ret = head_node;
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(head_node);
      ret = null;
    }
  } else {
    flush_node_list(head_node);
    ret = nodelist_from_lua(L);
  }
  lua_pop(L,2); /* result and callback container table */
  lua_gc(L,LUA_GCSTEP, LUA_GC_STEP_SIZE);
  return ret;
}

halfword
lua_vpack_filter (halfword head_node, scaled size, int pack_type, scaled maxd, int extrainfo) {
  halfword ret;  
  integer callback_id ; 
  lua_State *L = Luas[0];
  if (strcmp("output",group_code_names[extrainfo])==0) {
    callback_id = callback_defined(pre_output_filter_callback);
  } else {
    callback_id = callback_defined(vpack_filter_callback);
  }
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
  lua_pushnumber(L,size);
  lua_pushstring(L,pack_type_name[pack_type]);
  lua_pushnumber(L,maxd);
  lua_pushstring(L,group_code_names[extrainfo]);
  if (lua_pcall(L,5,1,0) != 0) { /* no arg, 1 result */
    fprintf(stdout,"error: %s\n",lua_tostring(L,-1));
    lua_pop(L,2);
    error();
    return head_node;
  }
  ret = head_node;
  if (lua_isboolean(L,-1)) {
    if (lua_toboolean(L,-1)!=1) {
      flush_node_list(head_node);
      ret = null;
    }
  } else {
    flush_node_list(head_node);
    ret = nodelist_from_lua(L);
  }
  lua_pop(L,2); /* result and callback container table */
  lua_gc(L,LUA_GCSTEP, LUA_GC_STEP_SIZE);
  return ret;
}
