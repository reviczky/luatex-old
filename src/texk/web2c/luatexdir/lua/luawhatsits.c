/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"
#include <stdarg.h>

#define open_name(a) link((a)+1)
#define open_area(a) info((a)+2)
#define open_ext(a)  link((a)+2)

#define what_lang(a) link((a)+1)
#define what_lhm(a)  type((a)+1)
#define what_rhm(a)  subtype((a)+1)

#define pdf_width(a)         zmem[(a) + 1].cint
#define pdf_height(a)        zmem[(a) + 2].cint
#define pdf_depth(a)         zmem[(a) + 3].cint
#define pdf_ximage_objnum(a) info((a) + 4)
#define pdf_obj_objnum(a)    info((a) + 1)
#define pdf_xform_objnum(a)  info((a) + 4)

#define pdf_annot_data(a)       info((a) + 5)
#define pdf_link_attr(a)        info((a) + 5)
#define pdf_link_action(a)      link((a) + 5)
#define pdf_annot_objnum(a)     zmem[(a) + 6].cint
#define pdf_link_objnum(a)      zmem[(a) + 6].cint

#define dir_dir(a)     info((a)+1)
#define dir_level(a)   link((a)+1)
#define dir_dvi_ptr(a) info((a)+2)
#define dir_dvi_h(a)   info((a)+3)

extern void tokenlist_to_lua(lua_State *L, halfword p) ;
extern halfword tokenlist_from_lua(lua_State *L) ;

#define write_node_size 2 
#define small_node_size 2 
#define dir_node_size 4
#define open_node_size 2 
#define local_par_node_size 8 
#define pdf_refximage_node_size 5
#define pdf_refxform_node_size  5
#define pdf_refobj_node_size 2
#define pdf_annot_node_size 7



char *whatsit_node_names[] = {
  "open",
  "write",
  "close",
  "special",
  "language",
  "!" /* "set_language" */,
  "local_par",
  "dir",
  "pdf_literal",
  "!" /* "pdf_obj" */,
  "pdf_refobj",
  "!" /* "pdf_xform" */,
  "pdf_refxform",
  "!" /* "pdf_ximage" */,
  "pdf_refximage",
  "pdf_annot",
  "pdf_start_link",
  "pdf_end_link",
  "!" /* "pdf_outline" */,
  "pdf_dest",
  "pdf_thread",
  "pdf_start_thread",
  "pdf_end_thread",
  "pdf_save_pos",
  "!" /* "pdf_info" */,
  "!" /* "pdf_catalog" */,
  "!" /* "pdf_names" */,
  "!" /* "pdf_font_attr" */,
  "!" /* "pdf_include_chars" */,
  "!" /* "pdf_map_file" */,
  "!" /* "pdf_map_line" */,
  "!" /* "pdf_trailer" */,
  "!" /* "pdf_font_expand" */,
  "!" /* "set_random_seed" */,
  "pdf_snap_ref_point",
  "pdf_snapy",
  "pdf_snapy_comp",
  "!" /* "pdf_glyph_to_unicode" */,
  "late_lua",
  "close_lua",
  "!" /* "save_cat_code_table" */,
  "!" /* "init_cat_code_table" */,
  "pdf_colorstack", 
  "pdf_setmatrix",
  "pdf_save",
  "pdf_restore",
  NULL };

#define make_whatsit(p,b)    { p = get_node(b);  type(p)=whatsit_node;  link(p)=null; }

#define numeric_field(a,b)   { lua_rawgeti(L,-1,b); a = lua_tonumber(L,-1); lua_pop(L,1); }
#define nodelist_field(a,b)  { lua_rawgeti(L,-1,b); a = nodelist_from_lua(L); lua_pop(L,1); }
#define tokenlist_field(a,b) { lua_rawgeti(L,-1,b); a = tokenlist_from_lua(L); lua_pop(L,1); }
#define string_field(a,b)    { lua_rawgeti(L,-1,b); a = maketexstring(lua_tostring(L,-1)); lua_pop(L,1); }

void
generic_node_to_lua (lua_State *L, char *name, char *fmt, ...) {
  va_list args;
  int val;
  int i = 1;
  lua_createtable(L,(strlen(fmt)+1),0);
  lua_pushstring(L,name);
  lua_rawseti(L,-2,i++);
  va_start(args,fmt);
  while (*fmt) {
    switch(*fmt++) {
    case 'd':           /* int */
      val = va_arg(args, int);
      lua_pushnumber(L,val);
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
      tokenlist_to_lua(L,val);
      lua_rawseti(L,-2,i++);
      break;
    }
  }
  va_end(args);
}

void
whatsit_open_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddsss", subtype(p),write_stream(p), open_name(p),open_area(p),open_ext(p));
}

halfword 
whatsit_open_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,open_node_size);  

  numeric_field(subtype(p),i++);
  numeric_field(write_stream(p),i++);
  string_field(open_name(p),i++);
  string_field(open_area(p),i++);
  string_field(open_ext(p),i++);
  return p;
}

void
whatsit_write_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddt",subtype(p),write_stream(p),link(write_tokens(p)));
}


halfword 
whatsit_write_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);

  numeric_field(subtype(p),i++);
  numeric_field(write_stream(p),i++);
  tokenlist_field(write_tokens(p),i++);
  return p;
}

void
whatsit_close_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dd",subtype(p),write_stream(p));
}

halfword 
whatsit_close_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,small_node_size);

  numeric_field(subtype(p),i++);
  numeric_field(write_stream(p),i++);
  return p;
}

void
whatsit_special_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dt",subtype(p),link(write_tokens(p)));
}

halfword 
whatsit_special_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);
  write_stream(p) = null;

  numeric_field(subtype(p),i++);
  tokenlist_field(write_tokens(p),i++);
  return p;
}

void
whatsit_language_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dddd",subtype(p),what_lang(p),what_lhm(p),what_rhm(p));
}

halfword 
whatsit_language_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,small_node_size);

  numeric_field(subtype(p),i++);
  numeric_field(what_lang(p),i++);
  numeric_field(what_lhm(p),i++);
  numeric_field(what_rhm(p),i++);
  return p;
}



void 
whatsit_local_par_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddddndnd",subtype(p),local_pen_inter(p),local_pen_broken(p),
		      local_par_dir(p),local_box_left(p),local_box_left_width(p),
		      local_box_right(p),local_box_right_width(p));
}

halfword 
whatsit_local_par_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,local_par_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (local_pen_inter(p),i++);
  numeric_field  (local_pen_broken(p),i++);
  numeric_field  (local_par_dir(p),i++);
  nodelist_field (local_box_left(p),i++);
  numeric_field  (local_box_left_width(p),i++);
  nodelist_field (local_box_right(p),i++);
  numeric_field  (local_box_right_width(p),i++);
  return p;
}

void
whatsit_dir_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddddd",subtype(p),dir_dir(p),dir_level(p),dir_dvi_ptr(p),dir_dvi_h(p));
}

halfword 
whatsit_dir_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,dir_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (dir_dir(p),i++);
  numeric_field  (dir_level(p),i++);
  numeric_field  (dir_dvi_ptr(p),i++);
  numeric_field  (dir_dvi_h(p),i++);
  return p;
}


void
whatsit_pdf_literal_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddt",subtype(p),pdf_literal_mode(p),link(pdf_literal_data(p)));
}

halfword 
whatsit_pdf_literal_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);
  numeric_field  (subtype(p),i++);
  numeric_field  (pdf_literal_mode(p),i++);
  tokenlist_field(pdf_literal_data(p),i++);
  return p;
}

void
whatsit_pdf_refobj_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dd",subtype(p),pdf_obj_objnum(p));
}

halfword 
whatsit_pdf_refobj_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refobj_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (pdf_obj_objnum(p),i++);
  return p;
}

void
whatsit_pdf_refxform_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddddd",subtype(p),pdf_width(p),pdf_height(p),pdf_depth(p),pdf_xform_objnum(p));
}

halfword 
whatsit_pdf_refxform_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refxform_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_xform_objnum(p),i++);
  return p;
}

void
whatsit_pdf_refximage_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","ddddd",subtype(p),pdf_width(p),pdf_height(p),pdf_depth(p),pdf_ximage_objnum(p));
}

halfword 
whatsit_pdf_refximage_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refximage_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_ximage_objnum(p),i++);
  return p;
}

void
whatsit_pdf_annot_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dddddt",subtype(p),pdf_width(p),pdf_height(p),pdf_depth(p),
		      pdf_annot_objnum(p),pdf_annot_data(p));
}

halfword 
whatsit_pdf_annot_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_annot_node_size);

  numeric_field  (subtype(p),i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_annot_objnum(p),i++);
  tokenlist_field(pdf_annot_data(p),i++);
  return p;
}

halfword 
whatsit_node_from_lua (lua_State *L) {
  int t;
  halfword p = null;
  lua_rawgeti(L,-1,2);
  t = lua_tonumber(L,-1);
  lua_pop(L,1);
  switch(t) {
  case open_node:   
    p = whatsit_open_from_lua(L); 
    break;
  case write_node:   
    p = whatsit_write_from_lua(L); 
    break;
  case close_node:   
    p = whatsit_close_from_lua(L); 
    break;
  case special_node:   
    p = whatsit_special_from_lua(L); 
    break;
  case language_node:   
    p = whatsit_language_from_lua(L); 
    break;
  case local_par_node:   
    p = whatsit_local_par_from_lua(L); 
    break;
  case dir_node:   
    p = whatsit_dir_from_lua(L); 
    break;
  case pdf_literal_node:   
    p = whatsit_pdf_literal_from_lua(L); 
    break;
  case pdf_refobj_node:   
    p = whatsit_pdf_refobj_from_lua(L); 
    break;
  case pdf_refxform_node:   
    p = whatsit_pdf_refxform_from_lua(L); 
    break;
  case pdf_refximage_node:   
    p = whatsit_pdf_refximage_from_lua(L); 
    break;
  case pdf_annot_node:   
    p = whatsit_pdf_annot_from_lua(L); 
    break;
    /*
      "pdf_start_link",
      "pdf_end_link",
      "pdf_dest",
      "pdf_thread",
      "pdf_start_thread",
      "pdf_end_thread",
      "pdf_save_pos",
      "pdf_snap_ref_point",
      "pdf_snapy",
      "pdf_snapy_comp",
      "late_lua",
      "close_lua",
      "pdf_colorstack", 
      "pdf_setmatrix",
      "pdf_save",
      "pdf_restore", */
  default:
    fprintf(stdout,"<reading whatsits not fully supported yet (%d)>\n",t);
  }
  return p;
}


void 
whatsit_node_to_lua (lua_State *L, halfword p) {

  luaL_checkstack(L,2,"out of stack space");

  switch(subtype(p)) {
  case open_node:       
    whatsit_open_to_lua(L,p); 
    break;
  case write_node:       
    whatsit_write_to_lua(L,p); 
    break;
  case close_node:       
    whatsit_close_to_lua(L,p); 
    break;
  case special_node:       
    whatsit_special_to_lua(L,p); 
    break;
  case language_node:   
    whatsit_language_to_lua(L,p); 
    break;
  case local_par_node:   
    whatsit_local_par_to_lua(L,p); 
    break;
  case dir_node:   
    whatsit_dir_to_lua(L,p); 
    break;
  case pdf_literal_node: 
    whatsit_pdf_literal_to_lua(L,p); 
    break;
  case pdf_refobj_node:       
    whatsit_pdf_refobj_to_lua(L,p);
    break;
  case pdf_refxform_node:       
    whatsit_pdf_refxform_to_lua(L,p);
    break;
  case pdf_refximage_node:       
    whatsit_pdf_refximage_to_lua(L,p);
    break;
  case pdf_annot_node:       
    whatsit_pdf_annot_to_lua(L,p);
    break;
    /*
      "pdf_start_link",
      "pdf_end_link",
      "pdf_dest",
      "pdf_thread",
      "pdf_start_thread",
      "pdf_end_thread",
      "pdf_save_pos",
      "pdf_snap_ref_point",
      "pdf_snapy",
      "pdf_snapy_comp",
      "late_lua",
      "close_lua",
      "pdf_colorstack", 
      "pdf_setmatrix",
      "pdf_save",
      "pdf_restore", */
  default:
    fprintf(stdout,"<writing whatsits not fully supported yet (%d)>\n",subtype(p));
  }
}

