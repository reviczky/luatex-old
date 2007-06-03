
#include <stdarg.h>

#define null -0x3FFFFFFF
#define zero_glue 0

#define vinfo(a)           varmem[(a)].hh.v.LH 
#define vlink(a)           varmem[(a)].hh.v.RH 
#define type(a)            varmem[(a)].hh.u.B0
#define subtype(a)         varmem[(a)].hh.u.B1
#define node_attr(a)       vinfo((a)+1)

#define attribute_id(a)    vlink((a)+1)
#define attribute_value(a) vinfo((a)+1)

#define temp_node_size 1

#define penalty_node_size 3 
#define penalty(a)       vlink((a)+1)

#define glue_node_size 3
#define glue_ptr(a)      vinfo((a)+2)
#define leader_ptr(a)    vlink((a)+2)

#define disc_node_size 3
#define pre_break(a)     vinfo((a)+2)
#define post_break(a)    vlink((a)+2)

#define kern_node_size 3
#define margin_kern_node_size 3
#define box_node_size 8
#define width(a)         varmem[(a+2)].cint
#define depth(a)         varmem[(a+3)].cint
#define height(a)        varmem[(a+4)].cint
#define shift_amount(a)  vlink((a)+5)
#define box_dir(a)       vinfo((a)+5)
#define list_ptr(a)      vlink((a)+6)
#define glue_order(a)    subtype((a)+6)
#define glue_sign(a)     type((a)+6)
#define glue_set(a)      varmem[(a+7)].gr

/* unset nodes */
#define glue_stretch(a)  varmem[(a)+7].cint
#define glue_shrink      shift_amount
#define span_count       subtype

#define rule_node_size 5
#define rule_dir(a)      vlink((a)+1)

#define mark_node_size 3
#define mark_ptr(a)      vlink((a)+2)
#define mark_class(a)    vinfo((a)+2)

/* a glue spec */
#define glue_spec_size 3
#define stretch(a)       vlink((a)+1)
#define shrink(a)        vinfo((a)+1)
#define stretch_order    type
#define shrink_order     subtype
#define glue_ref_count   vlink

#define adjust_node_size 2
#define adjust_ptr(a)    vlink(a+1)

#define glyph_node_size 3 /* and ligatures */
#define margin_char(a)  vlink((a)+1)
#define font(a)         vlink((a)+1)
#define character(a)    vinfo((a)+2)
#define lig_ptr(a)      vlink((a)+2)
#define is_char_node(a) (a!=null && type(a)==glyph_node)

#define math_node_size 2
#define surround(a)      vlink((a)+1)

#define ins_node_size 6
#define float_cost(a)    varmem[(a)+2].cint
#define ins_ptr(a)       vinfo((a)+5)
#define split_top_ptr(a) vlink((a)+5)

typedef enum {
  hlist_node = 0, //
  vlist_node = 1, //
  rule_node,      //
  ins_node,       //
  mark_node,      //
  adjust_node,    //
  ligature_node,  //
  disc_node,      //
  whatsit_node,
  math_node,      //
  glue_node,      //
  kern_node,      //
  penalty_node,   //
  unset_node,     //
  right_noad = 31,
  margin_kern_node = 40, //
  glyph_node = 41,
  attribute_node = 42,
  last_known_node = 43,
  unhyphenated_node = 50, 
  hyphenated_node = 51,
  delta_node = 52,
  passive_node = 53 } node_types ;

extern int node_sizes[];

extern void  nodelist_to_lua (lua_State *L, halfword t);
extern halfword nodelist_from_lua (lua_State *L) ;

#define local_pen_inter(a)       varmem[a+1].cint
#define local_pen_broken(a)      varmem[a+2].cint
#define local_box_left(a)        varmem[a+3].cint
#define local_box_left_width(a)  varmem[a+4].cint
#define local_box_right(a)       varmem[a+5].cint
#define local_box_right_width(a) varmem[a+6].cint
#define local_par_dir(a)         varmem[a+7].cint

#define pdf_literal_data(a)  vlink(a+1)
#define pdf_literal_mode(a)  vinfo(a+1)

#define write_tokens(a)  vlink(a+1)
#define write_stream(a)  vinfo(a+1)


typedef enum {
  open_node = 0,
  write_node,
  close_node,
  special_node,
  language_node,
  set_language_code,
  local_par_node,
  dir_node,
  pdf_literal_node,
  pdf_obj_code,
  pdf_refobj_node,
  pdf_xform_code,
  pdf_refxform_node,
  pdf_ximage_code,
  pdf_refximage_node,
  pdf_annot_node,
  pdf_start_link_node,
  pdf_end_link_node,
  pdf_outline_code,
  pdf_dest_node,
  pdf_thread_node,
  pdf_start_thread_node,
  pdf_end_thread_node,
  pdf_save_pos_node,
  pdf_info_code,
  pdf_catalog_code,
  pdf_names_code,
  pdf_font_attr_code,
  pdf_include_chars_code,
  pdf_map_file_code,
  pdf_map_line_code,
  pdf_trailer_code,
  pdf_font_expand_code,
  set_random_seed_code,
  pdf_snap_ref_point_node,
  pdf_snapy_node,
  pdf_snapy_comp_node,
  pdf_glyph_to_unicode_code,
  late_lua_node,
  close_lua_node,
  save_cat_code_table_code,
  init_cat_code_table_code,
  pdf_colorstack_node,
  pdf_setmatrix_node,
  pdf_save_node,
  pdf_restore_node,
  user_defined_node } whatsit_types ;


extern void      whatsit_node_to_lua (lua_State *L, halfword p);
extern halfword  whatsit_node_from_lua (lua_State *L);


#define open_name(a) vlink((a)+1)
#define open_area(a) vinfo((a)+2)
#define open_ext(a)  vlink((a)+2)

#define what_lang(a) vlink((a)+1)
#define what_lhm(a)  type((a)+1)
#define what_rhm(a)  subtype((a)+1)

#define pdf_width(a)         varmem[(a) + 1].cint
#define pdf_height(a)        varmem[(a) + 2].cint
#define pdf_depth(a)         varmem[(a) + 3].cint
#define pdf_ximage_objnum(a) vinfo((a) + 4)
#define pdf_obj_objnum(a)    vinfo((a) + 1)
#define pdf_xform_objnum(a)  vinfo((a) + 4)

#define pdf_annot_data(a)       vinfo((a) + 5)
#define pdf_link_attr(a)        vinfo((a) + 5)
#define pdf_link_action(a)      vlink((a) + 5)
#define pdf_annot_objnum(a)     varmem[(a) + 6].cint
#define pdf_link_objnum(a)      varmem[(a) + 6].cint

#define pdf_dest_type(a)          type((a) + 5)
#define pdf_dest_named_id(a)      subtype((a) + 5)
#define pdf_dest_id(a)            vlink((a) + 5)
#define pdf_dest_xyz_zoom(a)      vinfo((a) + 6)
#define pdf_dest_objnum(a)        vlink((a) + 6)

#define pdf_thread_named_id(a)    subtype((a) + 5)
#define pdf_thread_id(a)          vlink((a) + 5)
#define pdf_thread_attr(a)        vinfo((a) + 6)

#define dir_dir(a)     vinfo((a)+1)
#define dir_level(a)   vlink((a)+1)
#define dir_dvi_ptr(a) vinfo((a)+2)
#define dir_dvi_h(a)   vinfo((a)+3)

#define late_lua_data(a)        vlink((a)+1)
#define late_lua_reg(a)         subtype((a)+1)

#define snap_glue_ptr(a)    vinfo((a) + 1)
#define final_skip(a)       varmem[(a) + 2].cint
#define snapy_comp_ratio(a) varmem[(a) + 1].cint

#define pdf_colorstack_stack(a)  vlink((a)+1)
#define pdf_colorstack_cmd(a)    vinfo((a)+1)
#define pdf_colorstack_data(a)   vlink((a)+2)
#define pdf_setmatrix_data(a)    vlink((a)+1)

typedef enum {
  colorstack_set=0,
  colorstack_push,
  colorstack_pop,
  colorstack_current } colorstack_commands;

extern void tokenlist_to_lua(lua_State *L, halfword p) ;
extern halfword tokenlist_from_lua(lua_State *L) ;

typedef enum {
  pdf_action_page = 0,
  pdf_action_goto,
  pdf_action_thread,
  pdf_action_user } pdf_action_types;

#define open_node_size 3 
#define write_node_size 3
#define close_node_size 3
#define special_node_size 3
#define language_node_size 3
#define dir_node_size 3
#define pdf_end_link_node_size 3
#define pdf_end_thread_node_size 3
#define pdf_save_pos_node_size 3
#define pdf_snap_ref_point_node_size 3
#define pdf_snapy_comp_node_size 3
#define local_par_size 8

#define pdf_colorstack_node_size 3
#define pdf_setmatrix_node_size 3
#define pdf_save_node_size     3
#define pdf_restore_node_size  3
#define pdf_refobj_node_size 3
#define pdf_refxform_node_size  5
#define pdf_refximage_node_size 5
#define pdf_annot_node_size 7
#define pdf_action_size 3
#define pdf_dest_node_size 7
#define pdf_thread_node_size 7
#define snap_node_size 3

#define make_whatsit(p,b)    { p = get_node(b);  type(p)=whatsit_node; }

#define numeric_field(a,b)    { lua_rawgeti(L,-1,b); a = lua_tonumber(L,-1); lua_pop(L,1); }
#define float_field(a,b)      { lua_rawgeti(L,-1,b); a = lua_tonumber(L,-1); lua_pop(L,1); }
#define nodelist_field(a,b)   { lua_rawgeti(L,-1,b); a = nodelist_from_lua(L); lua_pop(L,1); }
#define tokenlist_field(a,b)  { lua_rawgeti(L,-1,b); a = tokenlist_from_lua(L); lua_pop(L,1); }
#define action_field(a,b)     { lua_rawgeti(L,-1,b); a = action_node_from_lua(L); lua_pop(L,1); }
#define attributes_field(a,b) { lua_rawgeti(L,-1,b); a = attribute_list_from_lua(L); lua_pop(L,1); }
#define string_field(a,b)     { lua_rawgeti(L,-1,b); a = maketexstring(lua_tostring(L,-1)); lua_pop(L,1); }

#define pdf_action_type           type
#define pdf_action_named_id       subtype
#define pdf_action_id             vlink
#define pdf_action_file(a)        vinfo((a) + 1)
#define pdf_action_new_window(a)  vlink((a) + 1)
#define pdf_action_page_tokens(a) vinfo((a) + 2)
#define pdf_action_user_tokens(a) vinfo((a) + 2)
#define pdf_action_refcount(a)    vlink((a) + 2)

extern void generic_node_to_lua (lua_State *L, char *name, char *fmt, ...);

extern void action_node_to_lua (lua_State *L, halfword p);
extern void attribute_list_to_lua (lua_State *L, halfword p);
