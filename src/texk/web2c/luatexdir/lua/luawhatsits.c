/* $Id$ */

#include "luatex-api.h"
#include <ptexlib.h>
#include "nodes.h"

/* this is a temporary hack */
#define status(a) null
#define status_field(a,b) b

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
  "user_defined",
  NULL };

void 
action_node_to_lua (lua_State *L, halfword p) {
  lua_newtable(L);
  switch (pdf_action_type(p)) {
  case pdf_action_user:
    generic_node_to_lua(L,"action","dt", pdf_action_type(p), pdf_action_user_tokens(p));
    break;
  case pdf_action_goto:
  case pdf_action_thread:
  case pdf_action_page:
    if (pdf_action_named_id(p) == 1) {
      generic_node_to_lua(L,"action","ddttdt", pdf_action_type(p),pdf_action_named_id(p),
			  pdf_action_id(p),pdf_action_file(p),pdf_action_new_window(p),
			  pdf_action_page_tokens(p));
    } else {
      generic_node_to_lua(L,"action","dddtdt", pdf_action_type(p),pdf_action_named_id(p),
			  pdf_action_id(p),pdf_action_file(p),pdf_action_new_window(p),
			  pdf_action_page_tokens(p));
    }
    break;
  }
}

halfword 
action_node_from_lua (lua_State *L) {
  int p, i = 2;
  p = get_node(pdf_action_size);  
  numeric_field(pdf_action_type(p),i++);
  switch (pdf_action_type(p)) {
  case pdf_action_user:
    tokenlist_field(pdf_action_user_tokens(p),i++);
    break;
  case pdf_action_goto:
  case pdf_action_thread:
  case pdf_action_page:
    numeric_field(pdf_action_named_id(p),i++);
    if (pdf_action_named_id(p)==1) {
      tokenlist_field(pdf_action_id(p),i++);
    } else {
      numeric_field  (pdf_action_id(p),i++);
    }
    tokenlist_field(pdf_action_file(p),i++);
    numeric_field  (pdf_action_new_window(p),i++);
    tokenlist_field(pdf_action_page_tokens(p),i++);
    break;
  }
  return p;
}

void
whatsit_open_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadsss", subtype(p),status(p),write_stream(p), 
		      open_name(p),open_area(p),open_ext(p));
}

halfword 
whatsit_open_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,open_node_size);  

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(write_stream(p),i++);
  string_field(open_name(p),i++);
  string_field(open_area(p),i++);
  string_field(open_ext(p),i++);
  return p;
}

void
whatsit_write_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadt",subtype(p),status(p),write_stream(p),write_tokens(p));
}


halfword 
whatsit_write_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(write_stream(p),i++);
  tokenlist_field(write_tokens(p),i++);
  return p;
}

void
whatsit_close_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dad",subtype(p),status(p),write_stream(p));
}

halfword 
whatsit_close_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,close_node_size);

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(write_stream(p),i++);
  return p;
}

void
whatsit_special_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dat",subtype(p),status(p),write_tokens(p));
}

halfword 
whatsit_special_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);
  write_stream(p) = null;
  numeric_field(subtype(p),i++);
  status_field(p,i++);
  tokenlist_field(write_tokens(p),i++);
  return p;
}

void
whatsit_language_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","daddd",subtype(p),status(p),what_lang(p),what_lhm(p),what_rhm(p));
}

halfword 
whatsit_language_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,language_node_size);

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(what_lang(p),i++);
  numeric_field(what_lhm(p),i++);
  numeric_field(what_rhm(p),i++);
  return p;
}



void 
whatsit_local_par_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadddndnd",subtype(p),status(p),local_pen_inter(p),local_pen_broken(p),
		      local_par_dir(p),local_box_left(p),local_box_left_width(p),
		      local_box_right(p),local_box_right_width(p));
}

halfword 
whatsit_local_par_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,local_par_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
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
  generic_node_to_lua(L,"whatsit","dadddd",subtype(p),status(p),dir_dir(p),dir_level(p),dir_dvi_ptr(p),dir_dvi_h(p));
}

halfword 
whatsit_dir_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,dir_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (dir_dir(p),i++);
  numeric_field  (dir_level(p),i++);
  numeric_field  (dir_dvi_ptr(p),i++);
  numeric_field  (dir_dvi_h(p),i++);
  return p;
}


void
whatsit_pdf_literal_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadt",subtype(p),status(p),pdf_literal_mode(p),pdf_literal_data(p));
}

halfword 
whatsit_pdf_literal_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_literal_mode(p),i++);
  tokenlist_field(pdf_literal_data(p),i++);
  return p;
}

void
whatsit_pdf_refobj_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dad",subtype(p),status(p),pdf_obj_objnum(p));
}

halfword 
whatsit_pdf_refobj_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refobj_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_obj_objnum(p),i++);
  return p;
}

void
whatsit_pdf_refxform_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadddd",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
		      pdf_xform_objnum(p));
}

halfword 
whatsit_pdf_refxform_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refxform_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_xform_objnum(p),i++);
  return p;
}

void
whatsit_pdf_refximage_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadddd",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
		      pdf_ximage_objnum(p));
}

halfword 
whatsit_pdf_refximage_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_refximage_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_ximage_objnum(p),i++);
  return p;
}

void
whatsit_pdf_annot_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","daddddt",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
		      pdf_annot_objnum(p),pdf_annot_data(p));
}

halfword 
whatsit_pdf_annot_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_annot_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_annot_objnum(p),i++);
  tokenlist_field(pdf_annot_data(p),i++);
  return p;
}

void
whatsit_pdf_start_link_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","daddddtc",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
		      pdf_link_objnum(p),pdf_link_attr(p),pdf_link_action(p));
}

halfword 
whatsit_pdf_start_link_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_annot_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_link_objnum(p),i++);
  tokenlist_field(pdf_link_attr(p),i++);
  action_field   (pdf_link_action(p),i++);
  return p;
}

void
whatsit_pdf_end_link_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_end_link_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_end_link_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}

void
whatsit_pdf_dest_to_lua (lua_State *L, halfword p) {
  if(pdf_dest_named_id(p)==1) {
    generic_node_to_lua(L,"whatsit","daddddtddd",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
			pdf_dest_named_id(p),pdf_dest_id(p),pdf_dest_type(p),pdf_dest_xyz_zoom(p),
                        pdf_dest_objnum(p));
  } else {
    generic_node_to_lua(L,"whatsit","dadddddddd",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
			pdf_dest_named_id(p),pdf_dest_id(p),pdf_dest_type(p),pdf_dest_xyz_zoom(p),
                        pdf_dest_objnum(p));
  }
}

halfword 
whatsit_pdf_dest_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_dest_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_dest_named_id(p),i++);
  if(pdf_dest_named_id(p)==1) {
    tokenlist_field  (pdf_dest_id(p),i++);
  } else {
    numeric_field  (pdf_dest_id(p),i++);
  }
  numeric_field  (pdf_dest_type(p),i++);
  numeric_field  (pdf_dest_xyz_zoom(p),i++);
  numeric_field  (pdf_dest_objnum(p),i++);
  return p;
}

void
whatsit_pdf_thread_to_lua (lua_State *L, halfword p) {
  if (pdf_thread_named_id(p)==1) {
    generic_node_to_lua(L,"whatsit","daddddtt",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
			pdf_thread_named_id(p),pdf_thread_id(p),pdf_thread_attr(p));
  } else {
    generic_node_to_lua(L,"whatsit","dadddddt",subtype(p),status(p),pdf_width(p),pdf_height(p),pdf_depth(p),
			pdf_thread_named_id(p),pdf_thread_id(p),pdf_thread_attr(p));
  }
}

halfword
whatsit_pdf_thread_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_thread_node_size);

  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_width(p),i++);
  numeric_field  (pdf_height(p),i++);
  numeric_field  (pdf_depth(p),i++);
  numeric_field  (pdf_thread_named_id(p),i++);
  if(pdf_thread_named_id(p)==1) {
    tokenlist_field  (pdf_thread_id(p),i++);
  } else {
    numeric_field  (pdf_thread_id(p),i++);
  }
  tokenlist_field  (pdf_thread_attr(p),i++);
  return p;
}

void
whatsit_pdf_end_thread_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_end_thread_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_end_thread_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}

void
whatsit_pdf_save_pos_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_save_pos_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_save_pos_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}

void
whatsit_pdf_snap_ref_point_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_snap_ref_point_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_snap_ref_point_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}

void
whatsit_pdf_snapy_to_lua (lua_State *L, halfword p) {
  halfword q;
  q = snap_glue_ptr(p);
  generic_node_to_lua(L,"whatsit","dadddddd",subtype(p),status(p),final_skip(p),
		      width(q),stretch(q),stretch_order(q),shrink(q),shrink_order(q));
}

halfword 
whatsit_pdf_snapy_from_lua (lua_State *L) {
  int p, i;
  halfword q;
  i = 2;
  q = new_spec(0); /* 0 == the null glue at zmem[0] */
  make_whatsit(p,snap_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (final_skip(p),i++);
  numeric_field  (width(q),i++);
  numeric_field  (stretch(q),i++);
  numeric_field  (stretch_order(q),i++);
  numeric_field  (shrink(q),i++);
  numeric_field  (shrink_order(q),i++);
  snap_glue_ptr(p) = q;
  return p;
}


void
whatsit_pdf_snapy_comp_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dad",subtype(p),status(p),snapy_comp_ratio(p));
}

halfword 
whatsit_pdf_snapy_comp_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,snap_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (snapy_comp_ratio(p),i++);
  return p;
}


void
whatsit_late_lua_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dadt",subtype(p),status(p),late_lua_reg(p),late_lua_data(p));
}

halfword 
whatsit_late_lua_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,write_node_size);

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(late_lua_reg(p),i++);
  tokenlist_field(late_lua_data(p),i++);
  return p;
}

void
whatsit_close_lua_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dad",subtype(p),status(p),late_lua_reg(p));
}

halfword 
whatsit_close_lua_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,close_lua_node_size);

  numeric_field(subtype(p),i++);
  status_field(p,i++);
  numeric_field(late_lua_reg(p),i++);
  return p;
}

void
whatsit_pdf_colorstack_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","daddt",subtype(p),status(p),
		      pdf_colorstack_stack(p),
		      pdf_colorstack_cmd(p),
		      pdf_colorstack_data(p));
}

halfword 
whatsit_pdf_colorstack_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_colorstack_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  numeric_field  (pdf_colorstack_stack(p),i++);
  numeric_field  (pdf_colorstack_cmd(p),i++);
  tokenlist_field(pdf_colorstack_data(p),i++);
  return p;
}

void
whatsit_pdf_setmatrix_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","dat",subtype(p), status(p),pdf_setmatrix_data(p));
}

halfword 
whatsit_pdf_setmatrix_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_setmatrix_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  tokenlist_field(pdf_setmatrix_data(p),i++);
  return p;
}


void
whatsit_pdf_save_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_save_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_save_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}


void
whatsit_pdf_restore_to_lua (lua_State *L, halfword p) {
  generic_node_to_lua(L,"whatsit","da",subtype(p),status(p));
}

halfword 
whatsit_pdf_restore_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,pdf_restore_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
  return p;
}

#define user_defined_node_size 2
#define user_node_type(a)  vinfo((a)+1)
#define user_node_value(a) vlink((a)+1)

void
whatsit_user_defined_to_lua (lua_State *L, halfword p) {
  switch (user_node_type(p)) {
  case 'b': 
	generic_node_to_lua(L,"whatsit","danb", subtype(p),status(p),
						user_node_type(p),user_node_value(p)); 
	break;
  case 'd': 
	generic_node_to_lua(L,"whatsit","dand", subtype(p),status(p),
						user_node_type(p),user_node_value(p)); 
	break;
  }
  
}

halfword 
whatsit_user_defined_from_lua (lua_State *L) {
  int p, i = 2;
  make_whatsit(p,user_defined_node_size);
  numeric_field  (subtype(p),i++);
  status_field(p,i++);
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
  case open_node:               p = whatsit_open_from_lua(L);               break;
  case write_node:              p = whatsit_write_from_lua(L);              break;
  case close_node:              p = whatsit_close_from_lua(L);              break;
  case special_node:            p = whatsit_special_from_lua(L);            break;
  case language_node:           p = whatsit_language_from_lua(L);           break;
  case local_par_node:          p = whatsit_local_par_from_lua(L);          break;
  case dir_node:                p = whatsit_dir_from_lua(L);                break;
  case pdf_literal_node:        p = whatsit_pdf_literal_from_lua(L);        break;
  case pdf_refobj_node:         p = whatsit_pdf_refobj_from_lua(L);         break;
  case pdf_refxform_node:       p = whatsit_pdf_refxform_from_lua(L);       break;
  case pdf_refximage_node:      p = whatsit_pdf_refximage_from_lua(L);      break;
  case pdf_annot_node:          p = whatsit_pdf_annot_from_lua(L);          break;
  case pdf_start_link_node:     p = whatsit_pdf_start_link_from_lua(L);     break;
  case pdf_end_link_node:       p = whatsit_pdf_end_link_from_lua(L);       break;
  case pdf_dest_node:           p = whatsit_pdf_dest_from_lua(L);           break;
  case pdf_thread_node:         p = whatsit_pdf_thread_from_lua(L);         break;
  case pdf_start_thread_node:   p = whatsit_pdf_thread_from_lua(L);         break;
  case pdf_end_thread_node:     p = whatsit_pdf_end_thread_from_lua(L);     break;
  case pdf_save_pos_node:       p = whatsit_pdf_save_pos_from_lua(L);       break;
  case pdf_snap_ref_point_node: p = whatsit_pdf_snap_ref_point_from_lua(L); break;
  case pdf_snapy_node:          p = whatsit_pdf_snapy_from_lua(L);          break;
  case pdf_snapy_comp_node:     p = whatsit_pdf_snapy_comp_from_lua(L);     break;
  case late_lua_node:           p = whatsit_late_lua_from_lua(L);           break;
  case close_lua_node:          p = whatsit_close_lua_from_lua(L);          break;
  case pdf_colorstack_node:     p = whatsit_pdf_colorstack_from_lua(L);     break;
  case pdf_setmatrix_node:      p = whatsit_pdf_setmatrix_from_lua(L);      break;
  case pdf_save_node:           p = whatsit_pdf_save_from_lua(L);           break;
  case pdf_restore_node:        p = whatsit_pdf_restore_from_lua(L);        break;
  case user_defined_node:       p = whatsit_user_defined_from_lua(L);       break;
  default:
    fprintf(stdout,"<unknown whatsit type cannot be de-lua-fied (%d)>\n",t);
    error();
  }
  return p;
}

void 
whatsit_node_to_lua (lua_State *L, halfword p) {
  luaL_checkstack(L,2,"out of stack space");
  switch(subtype(p)) {
  case open_node:                whatsit_open_to_lua(L,p);               break;
  case write_node:               whatsit_write_to_lua(L,p);              break;
  case close_node:               whatsit_close_to_lua(L,p);              break;
  case special_node:             whatsit_special_to_lua(L,p);            break;
  case language_node:            whatsit_language_to_lua(L,p);           break;
  case local_par_node:           whatsit_local_par_to_lua(L,p);          break;
  case dir_node:                 whatsit_dir_to_lua(L,p);                break;
  case pdf_literal_node:         whatsit_pdf_literal_to_lua(L,p);        break;
  case pdf_refobj_node:          whatsit_pdf_refobj_to_lua(L,p);         break;
  case pdf_refxform_node:        whatsit_pdf_refxform_to_lua(L,p);       break;
  case pdf_refximage_node:       whatsit_pdf_refximage_to_lua(L,p);      break;
  case pdf_annot_node:           whatsit_pdf_annot_to_lua(L,p);          break;
  case pdf_start_link_node:      whatsit_pdf_start_link_to_lua(L,p);     break;
  case pdf_end_link_node:        whatsit_pdf_end_link_to_lua(L,p);       break;
  case pdf_dest_node:            whatsit_pdf_dest_to_lua(L,p);           break;
  case pdf_thread_node:          whatsit_pdf_thread_to_lua(L,p);         break;
  case pdf_start_thread_node:    whatsit_pdf_thread_to_lua(L,p);         break;
  case pdf_end_thread_node:      whatsit_pdf_end_thread_to_lua(L,p);     break;
  case pdf_save_pos_node:        whatsit_pdf_save_pos_to_lua(L,p);       break;
  case pdf_snap_ref_point_node:  whatsit_pdf_snap_ref_point_to_lua(L,p); break;
  case pdf_snapy_node:           whatsit_pdf_snapy_to_lua(L,p);          break;
  case pdf_snapy_comp_node:      whatsit_pdf_snapy_comp_to_lua(L,p);     break;
  case late_lua_node:            whatsit_late_lua_to_lua(L,p);           break;
  case close_lua_node:           whatsit_close_lua_to_lua(L,p);          break;
  case pdf_colorstack_node:      whatsit_pdf_colorstack_to_lua(L,p);     break;
  case pdf_setmatrix_node:       whatsit_pdf_setmatrix_to_lua(L,p);      break;
  case pdf_save_node:            whatsit_pdf_save_to_lua(L,p);           break;
  case pdf_restore_node:         whatsit_pdf_restore_to_lua(L,p);        break;
  case user_defined_node:        whatsit_user_defined_to_lua(L,p);       break;
  default:
    fprintf(stdout,"<unknown whatsit type cannot be lua-fied (%d)>\n",subtype(p));
    error();
  }
}

