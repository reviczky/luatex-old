
#include <stdarg.h>

#define null -0x3FFFFFFF

#undef link /* defined by cpascal.h */
#define info(a)    zmem[(a)].hh.v.LH 
#define link(a)    zmem[(a)].hh.v.RH 

#define llink(a)   zmem[(a+1)].hh.v.LH 
#define rlink(a)   zmem[(a+1)].hh.v.RH 

#define status(a)       (zmem[(a)].hh.u.B0 >> 15)
#define set_status(a)   zmem[(a)].hh.u.B0 |= 32768
#define unset_status(a) zmem[(a)].hh.u.B0 &= 32767

#define type(a)       (zmem[(a)].hh.u.B0 % 32768)
#define type_field(a)  zmem[(a)].hh.u.B0
#define subtype(a)     zmem[(a)].hh.u.B1

#define penalty(a) zmem[(a+1)].cint

#define glue_ptr   llink
#define leader_ptr rlink

#define pre_break  llink
#define post_break rlink

#define width(a)         zmem[(a+1)].cint
#define depth(a)         zmem[(a+2)].cint
#define height(a)        zmem[(a+3)].cint
#define rule_dir(a)      zmem[(a+4)].cint
#define shift_amount(a)  zmem[(a+4)].cint
#define list_ptr(a)      link((a)+5)
#define glue_order(a)    subtype((a)+5)
#define glue_sign(a)     type_field((a)+5)
#define glue_set(a)      zmem[(a+6)].gr
#define box_dir(a)       zmem[(a+7)].cint

#define glue_stretch(a)  zmem[(a)+6].cint
#define glue_shrink      shift_amount
#define span_count       subtype

#define pdf_literal_data(a)  link(a+1)
#define pdf_literal_mode(a)  info(a+1)

#define mark_ptr(a)    link(a+1)
#define mark_class(a)  info(a+1)

#define write_tokens(a)  link(a+1)
#define write_stream(a)  info(a+1)

#define stretch(a) zmem[(a+2)].cint
#define shrink(a)  zmem[(a+3)].cint
#define stretch_order type_field
#define shrink_order  subtype

#define adjust_ptr(a) zmem[(a+1)].cint

#define margin_char(a) info((a)+2)

#define font(a)         link(a+1)
#define character(a)    info(a+1)
#define lig_ptr(a)    link((a+2))
#define is_char_node(a) (type(a)==glyph_node)

#define free_avail(a) { link(a)=avail; avail=a; decr(dyn_used); }


typedef enum {
  hlist_node = 0,
  vlist_node = 1,
  rule_node,
  ins_node,
  mark_node,
  adjust_node,
  ligature_node,
  disc_node,
  whatsit_node,
  math_node,
  glue_node,
  kern_node,
  penalty_node,
  unset_node,
  right_noad = 31,
  margin_kern_node = 40,
  glyph_node = 41,
  last_known_node = 42  } node_types ;

#define small_node_size 2
#define margin_kern_node_size 3

extern void  nodelist_to_lua (lua_State *L, halfword t);
extern halfword nodelist_from_lua (lua_State *L) ;

#define local_pen_inter(a)       zmem[a+1].cint
#define local_pen_broken(a)      zmem[a+2].cint
#define local_box_left(a)        zmem[a+3].cint
#define local_box_left_width(a)  zmem[a+4].cint
#define local_box_right(a)       zmem[a+5].cint
#define local_box_right_width(a) zmem[a+6].cint
#define local_par_dir(a)         zmem[a+7].cint

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


#define open_name(a) link((a)+1)
#define open_area(a) info((a)+2)
#define open_ext(a)  link((a)+2)

#define what_lang(a) link((a)+1)
#define what_lhm(a)  type_field((a)+1)
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

#define pdf_dest_type(a)          type_field((a) + 5)
#define pdf_dest_named_id(a)      subtype((a) + 5)
#define pdf_dest_id(a)            link((a) + 5)
#define pdf_dest_xyz_zoom(a)      info((a) + 6)
#define pdf_dest_objnum(a)        link((a) + 6)

#define pdf_thread_named_id(a)    subtype((a) + 5)
#define pdf_thread_id(a)          link((a) + 5)
#define pdf_thread_attr(a)        info((a) + 6)

#define dir_dir(a)     info((a)+1)
#define dir_level(a)   link((a)+1)
#define dir_dvi_ptr(a) info((a)+2)
#define dir_dvi_h(a)   info((a)+3)

#define late_lua_data(a)        link((a)+1)
#define late_lua_reg(a)         subtype((a)+1)

#define snap_glue_ptr(a)    info((a) + 1)
#define final_skip(a)       zmem[(a) + 2].cint
#define snapy_comp_ratio(a) zmem[(a) + 1].cint

#define pdf_colorstack_stack(a)  link((a)+1)
#define pdf_colorstack_cmd(a)    info((a)+1)
#define pdf_colorstack_data(a)   link((a)+2)
#define pdf_setmatrix_data(a)    link((a)+1)

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


#define write_node_size 2 
#define small_node_size 2 
#define dir_node_size 4
#define open_node_size 2 
#define local_par_node_size 8 
#define pdf_refximage_node_size 5
#define pdf_refxform_node_size  5
#define pdf_refobj_node_size 2
#define pdf_annot_node_size 7
#define pdf_dest_node_size 7
#define pdf_thread_node_size 7
#define snap_node_size 3
#define pdf_colorstack_node_size 3
#define pdf_setmatrix_node_size 2
#define pdf_save_node_size     2
#define pdf_restore_node_size  2

#define make_whatsit(p,b)    { p = get_node(b);  type_field(p)=whatsit_node;  link(p)=null; }

#define numeric_field(a,b)   { lua_rawgeti(L,-1,b); a = lua_tonumber(L,-1); lua_pop(L,1); }
#define status_field(a,b)    { lua_rawgeti(L,-1,b);						\
	if (lua_toboolean(L,-1)) { set_status(a); }							\
	else { unset_status(a);}; lua_pop(L,1); }
#define float_field(a,b)     { lua_rawgeti(L,-1,b); a = lua_tonumber(L,-1); lua_pop(L,1); }
#define nodelist_field(a,b)  { lua_rawgeti(L,-1,b); a = nodelist_from_lua(L); lua_pop(L,1); }
#define tokenlist_field(a,b) { lua_rawgeti(L,-1,b); a = tokenlist_from_lua(L); lua_pop(L,1); }
#define action_field(a,b)    { lua_rawgeti(L,-1,b); a = action_node_from_lua(L); lua_pop(L,1); }
#define string_field(a,b)    { lua_rawgeti(L,-1,b); a = maketexstring(lua_tostring(L,-1)); lua_pop(L,1); }

#define pdf_action_size 3
#define pdf_action_type           type_field
#define pdf_action_named_id       subtype
#define pdf_action_id             link
#define pdf_action_file(a)        info((a) + 1)
#define pdf_action_new_window(a)  link((a) + 1)
#define pdf_action_page_tokens(a) info((a) + 2)
#define pdf_action_user_tokens(a) info((a) + 2)
#define pdf_action_refcount(a)    link((a) + 2)

extern void generic_node_to_lua (lua_State *L, char *name, char *fmt, ...);

extern void action_node_to_lua (lua_State *L, halfword p);
