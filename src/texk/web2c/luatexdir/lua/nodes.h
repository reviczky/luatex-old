
#define null -0x3FFFFFFF

#undef link /* defined by cpascal.h */
#define info(a)    zmem[(a)].hh.v.LH 
#define link(a)    zmem[(a)].hh.v.RH 

#define llink(a)   zmem[(a+1)].hh.v.LH 
#define rlink(a)   zmem[(a+1)].hh.v.RH 

#define type(a)    zmem[(a)].hh.u.B0
#define subtype(a) zmem[(a)].hh.u.B1

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
#define glue_sign(a)     type((a)+5)
#define glue_set(a)      zmem[(a+6)].gr
#define box_dir(a)       zmem[(a+7)].cint

#define pdf_literal_data(a)  link(a+1)
#define pdf_literal_mode(a)  info(a+1)

#define mark_ptr(a)    link(a+1)
#define mark_class(a)  info(a+1)

#define write_tokens(a)  link(a+1)
#define write_stream(a)  info(a+1)

#define stretch(a) zmem[(a+2)].cint
#define shrink(a)  zmem[(a+3)].cint
#define stretch_order type
#define shrink_order  subtype

#define lignode_char(a)   ((a)+1)
#define lig_ptr(a)    link(lignode_char(a))

#define adjust_ptr(a) zmem[(a+1)].cint

#define margin_char(a) info((a)+2)

#define font      type
#define character subtype
#define is_char_node(a) ((a)>=hi_mem_min)

#define free_avail(a) { link(a)=avail; avail=a; decr(dyn_used); }


typedef enum {
  hlist_node = 0,
  vlist_node,
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
  right_node = 31,
  char_node = 37,
  font_node,
  glyph_node,
  margin_kern_node = 40 } node_types ;

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
  language_node, /* id 5 is MIA */
  local_par_node = 6,
  dir_node,
  pdf_literal_node,
  pdf_obj_node,
  pdf_refobj_node,
  pdf_xform_node,
  pdf_refxform_node,
  pdf_ximage_node,
  pdf_refximage_node,
  pdf_annot_node,
  pdf_start_link_node,
  pdf_end_link_node,
  pdf_outline_node,
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
  late_lua_code,
  close_lua_code,
  save_cat_code_table_code,
  init_cat_code_table_code,
  pdf_colorstack_node,
  pdf_setmatrix_node,
  pdf_save_node,
  pdf_restore_node,
  pdftex_last_extension_code } whatsit_node_types ;


extern void      whatsit_node_to_lua (lua_State *L, halfword p);
extern halfword  whatsit_node_from_lua (lua_State *L);
