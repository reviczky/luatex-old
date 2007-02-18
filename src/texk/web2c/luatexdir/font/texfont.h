/*
  Copyright (c) 1996-2006 Taco Hoekwater <taco@luatex.org>
  
  This file is part of LuaTeX.
  
  LuaTeX is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  LuaTeX is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with LuaTeX; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  $Id$ */

/* Here we have the interface to LuaTeX's font system, as seen from the
   main pascal program. There is a companion list in luatex.defines to
   keep web2c happy */

/* this file is read at the end of ptexlib.h, which is called for at
   the end of luatexcoerce.h, as well as from the C sources 
*/

#define pointer halfword

#define do_realloc(a,b,d)    a = xrealloc(a,(b)*sizeof(d))

typedef struct characterinfo {
  char * _name;
  unsigned int _index;
  integer _lig_index;
  integer _kern_index;
  integer _packet_index;
  integer _remainder;
  unsigned short _width_index ;
  unsigned short _height_index;
  unsigned short _depth_index ;
  unsigned short _italic_index;
  char _tag ;
  char _used ;
} characterinfo;

typedef struct liginfo {
  integer type_char;
  integer lig;
} liginfo;

typedef struct kerninfo {
  integer adj;
  scaled sc;
} kerninfo;

typedef struct texfont {
  integer _font_size ;
  integer _font_dsize ;
  char * _font_name ;
  char * _font_area ;
  char * _font_filename ;
  char * _font_fullname ;
  char * _font_encodingname ;
  integer _font_ec ;
  integer _font_checksum ;   /* internal information */
  char    _font_used ;       /* internal information */
  char    _font_touched ;    /* internal information */
  integer _font_cache_id ;   /* internal information */
  fm_entry_ptr _font_map;
  integer _font_type;
  integer _font_format;
  integer _font_embedding;
  integer _font_bc ;
  integer _hyphen_char ;
  integer _skew_char ;
  integer _bchar_label;
  integer _font_bchar;
  integer _font_false_bchar;
  integer _font_natural_dir;

  integer _char_infos;
  characterinfo *_char_base;

  integer _font_params;
  scaled  *_param_base;

  integer _font_widths;
  integer *_width_base;

  integer _font_depths;
  integer *_depth_base;

  integer _font_heights;
  integer *_height_base;

  integer _font_italics;
  integer *_italic_base;

  integer _font_ligs;
  liginfo *_lig_base;

  integer _font_kerns;
  kerninfo *_kern_base;
  
  integer _font_extens;
  fourquarters *_exten_base;

  /* virtual packet commands. this is a byte sequence because 
     literal strings can appear inside */
  integer _font_packets;
  real_eight_bits *_packet_base;

} texfont;

typedef enum {
  unknown_font_type=0, /* new font (has not been used yet) */
  virtual_font_type, /* virtual font */
  real_font_type, /* real font */
} font_types;

typedef enum {
  unknown_format=0, 
  type1_format, 
  type3_format, 
  truetype_format, 
  opentype_format, 
} font_formats;

typedef enum {
  unknown_embedding=0, 
  no_embedding, 
  subset_embedding, 
  full_embedding, 
} font_embedding_option;

#define font_checksum(a)          font_tables[a]->_font_checksum
#define set_font_checksum(a,b)    font_checksum(a) = b

#define font_check_0(a)           ((font_tables[a]->_font_checksum&0xFF000000)>>24)
#define font_check_1(a)           ((font_tables[a]->_font_checksum&0x00FF0000)>>16)
#define font_check_2(a)           ((font_tables[a]->_font_checksum&0x0000FF00)>>8)
#define font_check_3(a)            (font_tables[a]->_font_checksum&0x000000FF)

#define font_size(a)              font_tables[a]->_font_size
#define set_font_size(a,b)        font_size(a) = b
#define font_dsize(a)             font_tables[a]->_font_dsize
#define get_font_dsize            font_dsize
#define set_font_dsize(a,b)       font_dsize(a) = b

#define font_name(a)              font_tables[a]->_font_name
#define get_font_name             font_name
#define set_font_name(f,b)        font_name(f) = b
#define tex_font_name(a)          maketexstring(font_name(a))
#define set_tex_font_name(a,b)    font_name(a) = makecstring(b)

boolean cmp_font_name (integer, strnumber);

#define font_area(a)              font_tables[a]->_font_area
#define set_font_area(f,b)        font_area(f) = b
#define tex_font_area(a)          maketexstring(font_area(a))
#define set_tex_font_area(a,b)    font_area(a) = makecstring(b)

boolean cmp_font_area (integer, strnumber);

#define font_filename(a)           font_tables[a]->_font_filename
#define set_font_filename(f,b)     { if (font_filename(f)!=NULL) \
	  free(font_filename(f)); font_filename(f) = b; }

#define font_fullname(a)           font_tables[a]->_font_fullname
#define set_font_fullname(f,b)     { if (font_fullname(f)!=NULL) \
	  free(font_fullname(f)); font_fullname(f) = b; }


#define font_encodingname(a)       font_tables[a]->_font_encodingname
#define set_font_encodingname(f,b) { if (font_encodingname(f)!=NULL) \
	  free(font_encodingname(f)); font_encodingname(f) = b; }


#define font_bc(a)                font_tables[a]->_font_bc
#define get_font_bc               font_bc
#define set_font_bc(f,b)          font_bc(f) = b

#define font_ec(a)                font_tables[a]->_font_ec
#define get_font_ec               font_ec
#define set_font_ec(f,b)          font_ec(f) = b

#define font_used(a)              font_tables[a]->_font_used
#define get_font_used             font_used
#define set_font_used(a,b)        font_used(a) = b

#define font_touched(a)           font_tables[a]->_font_touched
#define set_font_touched(a,b)     font_touched(a) = b

#define font_type(a)              font_tables[a]->_font_type
#define set_font_type(a,b)        font_type(a) = b

#define font_format(a)              font_tables[a]->_font_format
#define set_font_format(a,b)        font_format(a) = b

#define font_embedding(a)           font_tables[a]->_font_embedding
#define set_font_embedding(a,b)     font_embedding(a) = b

#define font_map(a)              font_tables[a]->_font_map
#define set_font_map(a,b)        font_map(a) = b

#define font_cache_id(a)              font_tables[a]->_font_cache_id
#define set_font_cache_id(a,b)        font_cache_id(a) = b

#define hyphen_char(a)            font_tables[a]->_hyphen_char
#define set_hyphen_char(a,b)      hyphen_char(a) = b
#define skew_char(a)              font_tables[a]->_skew_char
#define set_skew_char(a,b)        skew_char(a) = b
#define bchar_label(a)            font_tables[a]->_bchar_label
#define set_bchar_label(a,b)      bchar_label(a) = b
#define font_bchar(a)             font_tables[a]->_font_bchar
#define set_font_bchar(a,b)       font_bchar(a) = b
#define font_false_bchar(a)       font_tables[a]->_font_false_bchar
#define set_font_false_bchar(a,b) font_false_bchar(a) = b
#define font_natural_dir(a)       font_tables[a]->_font_natural_dir
#define set_font_natural_dir(a,b) font_natural_dir(a) = b

/* character information */

#define char_infos(a)             font_tables[a]->_char_infos
#define char_base(a)              font_tables[a]->_char_base
#define char_info(f,b)            font_tables[f]->_char_base[b]

/* too hard to do inline, new space needs to be zeroed */
extern void set_char_infos(internal_font_number f, int b);

#define set_char_info(f,n,b)							   \
  { if (char_infos(f)<n) set_char_infos(f,n);		       \
    char_info(f,n) = b; }

/* font parameters */

#define font_params(a)       font_tables[a]->_font_params
#define param_base(a)        font_tables[a]->_param_base
#define font_param(a,b)      font_tables[a]->_param_base[b]

/* too hard to do inline, new space needs to be zeroed */
extern void set_font_params(internal_font_number f, int b);

#define set_font_param(f,n,b)                                   \
  { if (font_params(f)<n) set_font_params(f,n);                 \
    font_param(f,n) = b; }


/* character widths */

#define font_widths(a)       font_tables[a]->_font_widths
#define width_base(a)        font_tables[a]->_width_base
#define font_width(a,b)      font_tables[a]->_width_base[b]

#define set_font_widths(f,b)						\
  { if (font_widths(f)!=b) {						\
      font_bytes += (b-font_widths(f))*sizeof(integer);			\
      do_realloc(width_base(f), b, integer);				\
      font_widths(f) = b;  } }

#define set_font_width(f,n,b)                                   \
  { if (font_widths(f)<n) set_font_widths(f,n);                 \
    font_width(f,n) = (b); }


/* character heights */

#define font_heights(a)       font_tables[a]->_font_heights
#define height_base(a)        font_tables[a]->_height_base
#define font_height(a,b)      font_tables[a]->_height_base[b]

#define set_font_heights(f,b)						\
  { if (font_heights(f)!=b) {						\
      font_bytes += (b-font_heights(f))*sizeof(integer);			\
      do_realloc(height_base(f), b, integer);				\
      font_heights(f) = b;  } }

#define set_font_height(f,n,b)                                   \
  { if (font_heights(f)<n) set_font_heights(f,n);		 \
    font_height(f,n) = b; }

/* character depths */

#define font_depths(a)       font_tables[a]->_font_depths
#define depth_base(a)        font_tables[a]->_depth_base
#define font_depth(a,b)      font_tables[a]->_depth_base[b]

#define set_font_depths(f,b)						\
  { if (font_depths(f)!=b) {						\
      font_bytes += (b-font_depths(f))*sizeof(integer);			\
      do_realloc(depth_base(f), b, integer);				\
      font_depths(f) = b;  } }

#define set_font_depth(f,n,b)					\
  { if (font_depths(f)<n) set_font_depths(f,n);			\
    font_depth(f,n) = b; }


/* character italics */

#define font_italics(a)       font_tables[a]->_font_italics
#define italic_base(a)        font_tables[a]->_italic_base
#define font_italic(a,b)      font_tables[a]->_italic_base[b]

#define set_font_italics(f,b)						\
  { if (font_italics(f)!=b) {						\
      font_bytes += (b-font_italics(f))*sizeof(integer);			\
      do_realloc(italic_base(f), b, integer);				\
      font_italics(f) = b;  } }

#define set_font_italic(f,n,b)                                   \
  { if (font_italics(f)<n) set_font_italics(f,n);		 \
    font_italic(f,n) = b; }

/* character kerns */

#define font_kerns(a)       font_tables[a]->_font_kerns
#define kern_base(a)        font_tables[a]->_kern_base
#define font_kern(a,b)      font_tables[a]->_kern_base[b]
#define font_kern_sc(a,b)   font_tables[a]->_kern_base[b].sc

#define set_font_kerns(f,b)					 \
  { if (font_kerns(f)!=b) {					 \
      font_bytes += (b-font_kerns(f))*sizeof(kerninfo);		 \
      do_realloc(kern_base(f), b, kerninfo);			 \
      font_kerns(f) = b;  } }

#define set_font_kern(f,n,b,c)				 \
  { if (font_kerns(f)<n) set_font_kerns(f,n);		 \
    font_kern(f,n).adj = b;				 \
    font_kern(f,n).sc = c; }

#define adjust_font_kern(f,n,c)				 \
  { font_kern(f,n).sc = c; }

#define kern_char(f,b)       font_kern(f,b).adj
#define kern_kern(f,b)       font_kern(f,b).sc

 /* disabled item has bit 24 set */ 
#define kern_disabled(f,b)   (font_kern(f,b).adj > end_ligkern)
#define kern_end(f,b)        (font_kern(f,b).adj == end_ligkern)

/* character ligatures */

#define font_ligs(a)       font_tables[a]->_font_ligs
#define lig_base(a)        font_tables[a]->_lig_base
#define font_lig(a,b)      font_tables[a]->_lig_base[b]

#define set_font_ligs(f,b)					 \
  { if (font_ligs(f)!=b) {					 \
      font_bytes += (b-font_ligs(f))*sizeof(liginfo);		 \
      do_realloc(lig_base(f), b, liginfo);			 \
      font_ligs(f) = b;  } }

#define set_font_lig(f,n,b,c,d)				 \
  { if (font_ligs(f)<n) set_font_ligs(f,n);		 \
    font_lig(f,n).type_char = ((b<<24)+c);		 \
    font_lig(f,n).lig = d; }

#define is_ligature(a)        (a.type_char&0xFF000000!=0)
#define lig_type(a)           ((a.type_char&0xFF000000)>>25)
#define lig_char(a)           (a.type_char&0x00FFFFFF)
#define lig_replacement(a)     a.lig

#define lig_end(u)          (lig_char(u) == end_ligkern)

 /* disabled item has bit 24 set */ 
#define lig_disabled(u)     (lig_char(u) > end_ligkern)


/* extensibles */

#define font_extens(a)       font_tables[a]->_font_extens
#define exten_base(a)        font_tables[a]->_exten_base
#define font_exten(a,b)      font_tables[a]->_exten_base[b]

#define set_font_extens(f,b)						\
  { if (font_extens(f)!=b) {						\
      font_bytes += (b-font_extens(f))*sizeof(fourquarters);		\
      do_realloc(exten_base(f), b, fourquarters);			\
      font_extens(f) = b;  } }

#define set_font_exten(f,n,b)					\
  { if (font_extens(f)<n) set_font_extens(f,n);			\
    font_exten(f,n) = b; }


/* character packets */

#define font_packets(a)       font_tables[a]->_font_packets
#define packet_base(a)        font_tables[a]->_packet_base
#define font_packet(a,b)      font_tables[a]->_packet_base[b]

#define set_font_packets(f,b)						\
  { if (font_packets(f)!=b) {						\
      font_bytes += (b-font_packets(f))*sizeof(fourquarters);		\
      do_realloc(packet_base(f), b, fourquarters);			\
      font_packets(f) = b;  } }

#define set_font_packet(f,n,b)						\
  { if (font_packets(f)<n) set_font_packets(f,n);			\
    font_packet(f,n) = b; }


/* Font parameters are sometimes referred to as |slant(f)|, |space(f)|, etc.*/

typedef enum {
  slant_code = 1,
  space_code = 2,
  space_stretch_code = 3,
  space_shrink_code = 4,
  x_height_code = 5,
  quad_code = 6,
  extra_space_code = 7 } font_parameter_codes ;

#define slant(f)         font_param(f,slant_code)
#define space(f)         font_param(f,space_code)
#define space_stretch(f) font_param(f,space_stretch_code)
#define space_shrink(f)  font_param(f,space_shrink_code)
#define x_height(f)      font_param(f,x_height_code)
#define quad(f)          font_param(f,quad_code)
#define extra_space(f)   font_param(f,extra_space_code)

/* now for characters  */

#define non_char 65536 /* a code that can't match a real character */
#define non_address 0  /* a spurious |bchar_label| */

#define end_ligkern     0x7FFFFF /* otherchar value meaning "stop" */
#define ignored_ligkern 0x800000 /* otherchar value meaning "disabled" */


#define no_tag 0   /* vanilla character */
#define lig_tag 1  /* character has a ligature/kerning program */
#define list_tag 2 /* character has a successor in a charlist */
#define ext_tag 3  /* character is extensible */

#define width_index(f,c)      (char_info(f,c))._width_index
#define height_index(f,c)     (char_info(f,c))._height_index
#define depth_index(f,c)      (char_info(f,c))._depth_index
#define italic_index(f,c)     (char_info(f,c))._italic_index
#define kern_index(f,c)       (char_info(f,c))._kern_index
#define lig_index(f,c)        (char_info(f,c))._lig_index
#define packet_index(f,c)     (char_info(f,c))._packet_index

#define char_width(f,b)       font_width (f,width_index(f,b))
#define char_height(f,b)      font_height(f,height_index(f,b))
#define char_depth(f,b)       font_depth (f,depth_index(f,b))
#define char_italic(f,b)      font_italic(f,italic_index(f,b))

#define char_kern(f,b)        font_kern  (f,kern_index(f,b))
#define char_lig(f,b)         font_lig   (f,lig_index(f,b))
#define char_packet(f,b)      font_packet(f,packet_index(f,b))

#define char_remainder(f,b)   (char_info(f,b))._remainder
#define char_tag(f,b)         (char_info(f,b))._tag
#define char_used(f,b)        (char_info(f,b))._used
#define char_name(f,b)        (char_info(f,b))._name
#define char_index(f,b)       (char_info(f,b))._index

#define set_char_tag(f,b,c)       char_tag(f,b) = c
#define set_char_remainder(f,b,c) char_remainder(f,b) = c
#define set_char_used(f,b,c)      char_used(f,b) = c
#define set_char_index(f,b,c)     char_index(f,b) = c
#define set_char_name(f,b,c)     { if (char_name(f,b)!=NULL)	\
	  free(char_name(f,b)); char_name(f,b) = c; }

#define set_char_kern(f,b,c)   kern_index(f,b) = c
#define set_char_lig(f,b,c)    lig_index(f,b) = c
#define set_char_packet(f,b,c) packet_index(f,b) = c

#define char_exists(f,b)     ((b<=font_ec(f))&&(b>=font_bc(f))&&	\
							  (width_index(f,b)>0))
#define has_lig(f,b)          (char_exists(f,b) && lig_index(f,b)>0)
#define has_kern(f,b)         (char_exists(f,b) && kern_index(f,b)>0)
#define has_packet(f,b)       (char_exists(f,b) && packet_index(f,b)>0)

scaled get_kern(internalfontnumber f, integer lc, integer rc);
liginfo get_ligature(internalfontnumber f, integer lc, integer rc);

#define ext_top(f,c)          font_exten(f,char_remainder(f,c)).b0
#define ext_mid(f,c)          font_exten(f,char_remainder(f,c)).b1
#define ext_bot(f,c)          font_exten(f,char_remainder(f,c)).b2
#define ext_rep(f,c)          font_exten(f,char_remainder(f,c)).b3

extern texfont **font_tables;

integer new_font (void) ;
integer copy_font (integer id) ;
integer scale_font (integer id, integer atsize) ;
integer max_font_id (void);
void set_max_font_id (integer id);
integer new_font_id (void);
void create_null_font (void);
void delete_font(integer id);
boolean is_valid_font (integer id);


void dump_font (int font_number);
void undump_font (int font_number);

integer test_no_ligatures (internalfontnumber f) ;
void set_no_ligatures (internal_font_number f) ;
integer get_tag_code (internalfontnumber f, eight_bits c);

int read_tfm_info(internalfontnumber f, char *nom, char *aire, scaled s);


/* from dofont.c */

extern int read_font_info (pointer u, strnumber nom, strnumber aire, scaled s, integer ndir);
extern int find_font_id (char *nom, char *aire, scaled s);


/* for and from vfpacket.c */

#define scan_special 3 /* look into special text */ /* this is a hack */

typedef enum {  packet_char_code,
		packet_font_code,
		packet_pop_code,
		packet_push_code,
		packet_special_code,
		packet_right_code,
		packet_down_code,
		packet_rule_code,
		packet_nop_code,
		packet_end_code } packet_command_codes;

extern scaled sqxfw (scaled sq, integer fw);

extern void do_vf_packet (internal_font_number vf_f, eight_bits c);

