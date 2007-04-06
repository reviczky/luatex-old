/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

#define pdf_width(a)         zmem[(a) + 1].cint
#define pdf_height(a)        zmem[(a) + 2].cint
#define pdf_depth(a)         zmem[(a) + 3].cint
#define pdf_ximage_objnum(a) info((a)+4)

char *whatsit_node_names[] = {
  "open",
  "write",
  "close",
  "special",
  "language",
  "",
  "local_par",
  "dir",
  "pdf_literal",
  "pdf_obj",
  "pdf_refobj",
  "pdf_xform",
  "pdf_refxform",
  "pdf_ximage",
  "pdf_refximage",
  "pdf_annot",
  "pdf_start_link",
  "pdf_end_link",
  "pdf_outline",
  "pdf_dest",
  "pdf_thread",
  "pdf_start_thread",
  "pdf_end_thread",
  "pdf_save_pos",
  "pdf_info",
  "pdf_catalog",
  "pdf_names",
  "pdf_font_attr",
  "pdf_include_chars",
  "pdf_map_file",
  "pdf_map_line",
  "pdf_trailer",
  "pdf_font_expand",
  "set_random_seed",
  "pdf_snap_ref_point",
  "pdf_snapy",
  "pdf_snapy_comp",
  "pdf_glyph_to_unicode",
  "late_lua",
  "close_lua",
  "save_cat_code_table",
  "init_cat_code_table",
  "pdf_colorstack",
  "pdf_setmatrix",
  "pdf_save",
  "pdf_restore",
  NULL };

void 
whatsit_local_par_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"whatsit");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,local_pen_inter(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,local_pen_broken(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,local_par_dir(p));
  lua_rawseti(L,-2,i++);

  if (local_box_left(p)!=null) {
    nodelist_to_lua(L,local_box_left(p));
  } else {
    lua_pushnil(L);
  }
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,local_box_left_width(p));
  lua_rawseti(L,-2,i++);

  if (local_box_right(p)!=null) {
    nodelist_to_lua(L,local_box_right(p));
  } else {
    lua_pushnil(L);
  }
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,local_box_right_width(p));
  lua_rawseti(L,-2,i++);
}

#define local_par_size 8 

halfword 
whatsit_local_par_from_lua (lua_State *L) {
  int p;
  int i = 3;
  p = get_node(local_par_size);
  type(p)=whatsit_node;
  subtype(p)=local_par_node; 
  link(p)=null;

  lua_rawgeti(L,-1,i++);
  local_pen_inter(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  local_pen_broken(p) = lua_tonumber(L,-1);
  lua_pop(L,1);
  lua_rawgeti(L,-1,i++);
  local_par_dir(p) = lua_tonumber(L,-1);
  lua_pop(L,1);

  /* not handled yet */
  local_box_left(p)=null;
  local_box_left_width(p)=0;
  local_box_right(p)=null;
  local_box_right_width(p)=0;

  return p;
}



void
whatsit_pdf_literal_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"whatsit");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_literal_mode(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_literal_data(p));
  lua_rawseti(L,-2,i++);
}

void
whatsit_special_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"whatsit");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,write_tokens(p));
  lua_rawseti(L,-2,i++);
}


void
whatsit_write_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"whatsit");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,write_stream(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,write_tokens(p));
  lua_rawseti(L,-2,i++);
}

void
whatsit_pdf_refximage_to_lua (lua_State *L, halfword p) {
  int i = 1;
  lua_newtable(L);
  lua_pushstring(L,"whatsit");
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,subtype(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_width(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_height(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_depth(p));
  lua_rawseti(L,-2,i++);
  lua_pushnumber(L,pdf_ximage_objnum(p));
  lua_rawseti(L,-2,i++);
}

halfword 
whatsit_node_from_lua (lua_State *L) {
  int t;
  halfword p = null;
  lua_rawgeti(L,-1,2);
  t = lua_tonumber(L,-1);
  lua_pop(L,1);
  switch(t) {
  case local_par_node:   
    p = whatsit_local_par_from_lua(L); 
    break;
  default:
    fprintf(stdout,"<whatsits not fully supported yet (%s)>\n",whatsit_node_names[t]);
  }
  return p;
}


void 
whatsit_node_to_lua (lua_State *L, halfword p) {
  int i = 1;
  luaL_checkstack(L,2,"out of stack space");
  switch(subtype(p)) {
  case local_par_node:   
	whatsit_local_par_to_lua(L,p); 
	break;
  case pdf_literal_node: 
	whatsit_pdf_literal_to_lua(L,p); 
	break;
  case write_node:       
	whatsit_write_to_lua(L,p); 
	break;
  case special_node:       
	whatsit_special_to_lua(L,p); 
	break;
  case pdf_refximage_node:       
	whatsit_pdf_refximage_to_lua(L,p);
	break;
  default:
    fprintf(stdout,"<whatsits not fully supported yet (%s)>\n",whatsit_node_names[subtype(p)]);
  }
}

