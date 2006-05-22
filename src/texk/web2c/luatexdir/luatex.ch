%
% Copyright (c) 1996-2006 Han The Thanh, <thanh@pdftex.org>
%
% This file is part of pdfTeX.
%
% pdfTeX is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) any later version.
%
% pdfTeX is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with pdfTeX; if not, write to the Free Software
% Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
%
% $Id $
%
@x [0.0] l.83 - WEAVE: we want the whole file
  \def\?##1]{\hbox{Changes to \hbox to 1em{\hfil##1}.\ }}
  }
\let\maybe=\iffalse
@y 83
  \def\?##1]{\hbox{Changes to \hbox to 1em{\hfil##1}.\ }}
  }
\let\maybe=\iftrue
@z

% Change file to assist in creating the web2c-specific change file.
% This one resolves inconsistencies between tex.ch and tex.ech.
% Public domain.

@x 
@d eTeX_banner=='This is e-TeX, Version 3.141592',eTeX_version_string
@y
@z

@x 
@d pdfeTeX_banner=='This is pdfeTeX, Version 3.141592',pdftex_version_string,eTeX_version_string
@y
@z

@x 
@d pdfTeX_banner=='This is pdfTeX, Version 3.141592',pdftex_version_string
@y
@d luaTeX_banner=='This is luaTeX, Version 3.141592',luatex_version_string
@z

@x [1] m.2 l.188 - banner
@d banner==TeX_banner
@d banner_k==TeX_banner_k
@y
@d banner==luaTeX_banner
@d banner_k==luaTeX_banner
@z

@x [17.236] l.4960 - first web2c, then e-TeX additional integer parameters
@d int_pars=web2c_int_pars {total number of integer parameters}
@#
@d etex_int_base=tex_int_pars {base for \eTeX's integer parameters}
@y
@d pdftex_first_integer_code = web2c_int_pars {base for \pdfTeX's integer parameters}
@z

@x [49.1259] 
    begin if s>0 then
      begin if s=font_size[f] then goto common_ending;
      end
    else if font_size[f]=xn_over_d(font_dsize[f],-s,1000) then
      goto common_ending;
    end
@y
    begin
    if pdf_font_step[f] = 0 then begin
       if s>0 then
         begin if s=font_size[f] then goto common_ending;
         end
       else if font_size[f]=xn_over_d(font_dsize[f],-s,1000) then
         goto common_ending;
       end
    end
@z

@x (WEB2C!)
param_base:=xmalloc_array(integer, font_max);

undump_things(font_check[null_font], font_ptr+1-null_font);
@y
param_base:=xmalloc_array(integer, font_max);

pdf_char_used:=xmalloc_array(char_used_array, font_max);
pdf_font_size:=xmalloc_array(scaled, font_max);
pdf_font_num:=xmalloc_array(integer, font_max);
pdf_font_map:=xmalloc_array(fm_entry_ptr, font_max);
pdf_font_type:=xmalloc_array(eight_bits, font_max);
pdf_font_attr:=xmalloc_array(str_number, font_max);
pdf_font_blink:=xmalloc_array(internal_font_number, font_max);
pdf_font_elink:=xmalloc_array(internal_font_number, font_max);
pdf_font_stretch:=xmalloc_array(integer, font_max);
pdf_font_shrink:=xmalloc_array(integer, font_max);
pdf_font_step:=xmalloc_array(integer, font_max);
pdf_font_expand_ratio:=xmalloc_array(integer, font_max);
pdf_font_auto_expand:=xmalloc_array(boolean, font_max);
pdf_font_lp_base:=xmalloc_array(integer, font_max);
pdf_font_rp_base:=xmalloc_array(integer, font_max);
pdf_font_ef_base:=xmalloc_array(integer, font_max);
pdf_font_kn_bs_base:=xmalloc_array(integer, font_max);
pdf_font_st_bs_base:=xmalloc_array(integer, font_max);
pdf_font_sh_bs_base:=xmalloc_array(integer, font_max);
pdf_font_kn_bc_base:=xmalloc_array(integer, font_max);
pdf_font_kn_ac_base:=xmalloc_array(integer, font_max);
vf_packet_base:=xmalloc_array(integer, font_max);
vf_default_font:=xmalloc_array(internal_font_number, font_max);
vf_local_font_num:=xmalloc_array(internal_font_number, font_max);
vf_e_fnts:=xmalloc_array(integer, font_max);
vf_i_fnts:=xmalloc_array(internal_font_number, font_max);

for font_k := font_base to font_max do begin
    for k := 0 to 31 do
        pdf_char_used[font_k, k] := 0;
    pdf_font_size[font_k] := 0;
    pdf_font_num[font_k] := 0;
    pdf_font_map[font_k] := 0;
    pdf_font_type[font_k] := new_font_type;
    pdf_font_attr[font_k] := "";
    pdf_font_blink[font_k] := null_font;
    pdf_font_elink[font_k] := null_font;
    pdf_font_stretch[font_k] := null_font;
    pdf_font_shrink[font_k] := null_font;
    pdf_font_step[font_k] := 0;
    pdf_font_expand_ratio[font_k] := 0;
    pdf_font_auto_expand[font_k] := false;
    pdf_font_lp_base[font_k] := 0;
    pdf_font_rp_base[font_k] := 0;
    pdf_font_ef_base[font_k] := 0;
    pdf_font_kn_bs_base[font_k] := 0;
    pdf_font_st_bs_base[font_k] := 0;
    pdf_font_sh_bs_base[font_k] := 0;
    pdf_font_kn_bc_base[font_k] := 0;
    pdf_font_kn_ac_base[font_k] := 0;
end;

make_pdftex_banner;
undump_things(font_check[null_font], font_ptr+1-null_font);
@z

@x [51.1]
  setup_bound_var (0)('hash_extra')(hash_extra);

@y
  setup_bound_var (0)('hash_extra')(hash_extra);
  setup_bound_var (72)('pk_dpi')(pk_dpi);
@z

@x [51.2]
  const_chk (hash_extra);
  if error_line > ssup_error_line then error_line := ssup_error_line;
@y
  const_chk (hash_extra);
  const_chk (obj_tab_size);
  const_chk (pdf_mem_size);
  const_chk (dest_names_size);
  const_chk (pk_dpi);
  if error_line > ssup_error_line then error_line := ssup_error_line;
@z

@x [51.1332] l.24203 (ca.) texarray
  line_stack:=xmalloc_array (integer, max_in_open);
@y
  line_stack:=xmalloc_array (integer, max_in_open);
  eof_seen:=xmalloc_array (boolean, max_in_open);
  grp_stack:=xmalloc_array (save_pointer, max_in_open);
  if_stack:=xmalloc_array (pointer, max_in_open);
@z

@x [51.3]
  hyph_link :=xmalloc_array (hyph_pointer, hyph_size);
@y
  hyph_link :=xmalloc_array (hyph_pointer, hyph_size);
  obj_tab:=xmalloc_array (obj_entry, inf_obj_tab_size); {will grow dynamically}
  pdf_mem:=xmalloc_array (integer, inf_pdf_mem_size); {will grow dynamically}
  dest_names:=xmalloc_array (dest_name_entry, inf_dest_names_size); {will grow dynamically}
  pdf_op_buf:=xmalloc_array (eight_bits, pdf_op_buf_size);
  pdf_os_buf:=xmalloc_array (eight_bits, inf_pdf_os_buf_size); {will grow dynamically}
  pdf_os_objnum:=xmalloc_array (integer, pdf_os_max_objs);
  pdf_os_objoff:=xmalloc_array (integer, pdf_os_max_objs);
@z

@x [51.1337] l.24371 (ca.) texarray
  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
@y
  trie_root:=0; trie_c[0]:=si(0); trie_ptr:=0;
  hyph_root:=0; hyph_start:=0;
@z


@x (WEB2C!)
  param_base:=xmalloc_array(integer, font_max);

  font_ptr:=null_font; fmem_ptr:=7;
@y
param_base:=xmalloc_array(integer, font_max);

pdf_char_used:=xmalloc_array(char_used_array,font_max);
pdf_font_size:=xmalloc_array(scaled,font_max);
pdf_font_num:=xmalloc_array(integer,font_max);
pdf_font_map:=xmalloc_array(fm_entry_ptr,font_max);
pdf_font_type:=xmalloc_array(eight_bits,font_max);
pdf_font_attr:=xmalloc_array(str_number,font_max);
pdf_font_blink:=xmalloc_array(internal_font_number,font_max);
pdf_font_elink:=xmalloc_array(internal_font_number,font_max);
pdf_font_stretch:=xmalloc_array(integer,font_max);
pdf_font_shrink:=xmalloc_array(integer,font_max);
pdf_font_step:=xmalloc_array(integer,font_max);
pdf_font_expand_ratio:=xmalloc_array(integer,font_max);
pdf_font_auto_expand:=xmalloc_array(boolean,font_max);
pdf_font_lp_base:=xmalloc_array(integer,font_max);
pdf_font_rp_base:=xmalloc_array(integer,font_max);
pdf_font_ef_base:=xmalloc_array(integer,font_max);
pdf_font_kn_bs_base:=xmalloc_array(integer, font_max);
pdf_font_st_bs_base:=xmalloc_array(integer, font_max);
pdf_font_sh_bs_base:=xmalloc_array(integer, font_max);
pdf_font_kn_bc_base:=xmalloc_array(integer, font_max);
pdf_font_kn_ac_base:=xmalloc_array(integer, font_max);
vf_packet_base:=xmalloc_array(integer,font_max);
vf_default_font:=xmalloc_array(internal_font_number,font_max);
vf_local_font_num:=xmalloc_array(internal_font_number,font_max);
vf_e_fnts:=xmalloc_array(integer,font_max);
vf_i_fnts:=xmalloc_array(internal_font_number,font_max);

for font_k := font_base to font_max do begin
    for k := 0 to 31 do
        pdf_char_used[font_k, k] := 0;
    pdf_font_size[font_k] := 0;
    pdf_font_num[font_k] := 0;
    pdf_font_map[font_k] := 0;
    pdf_font_type[font_k] := new_font_type;
    pdf_font_attr[font_k] := "";
    pdf_font_blink[font_k] := null_font;
    pdf_font_elink[font_k] := null_font;
    pdf_font_stretch[font_k] := null_font;
    pdf_font_shrink[font_k] := null_font;
    pdf_font_step[font_k] := 0;
    pdf_font_expand_ratio[font_k] := 0;
    pdf_font_auto_expand[font_k] := false;
    pdf_font_lp_base[font_k] := 0;
    pdf_font_rp_base[font_k] := 0;
    pdf_font_ef_base[font_k] := 0;
    pdf_font_kn_bs_base[font_k] := 0;
    pdf_font_st_bs_base[font_k] := 0;
    pdf_font_sh_bs_base[font_k] := 0;
    pdf_font_kn_bc_base[font_k] := 0;
    pdf_font_kn_ac_base[font_k] := 0;
end;

font_ptr:=null_font; fmem_ptr:=7;
make_pdftex_banner;
@z

@x [53a.1379] l.??? -etex command line switch
@!init if (buffer[loc]="*")and(format_ident=" (INITEX)") then
@y
@!init if (etex_p or(buffer[loc]="*"))and(format_ident=" (INITEX)") then
@z

@x [53a.1379] l.??? -etex command line switch
  incr(loc); eTeX_mode:=1; {enter extended mode}
@y
  if (buffer[loc]="*") then incr(loc);
  eTeX_mode:=1; {enter extended mode}
@z

@x [53a.1383] l.??? -etex command line switch
@!eTeX_mode: 0..1; {identifies compatibility and extended mode}
@y
@!eTeX_mode: 0..1; {identifies compatibility and extended mode}
@!etex_p: boolean; {was the -etex option specified}
@z

@x [53a.1391] l.??? texarray
@!eof_seen : array[1..max_in_open] of boolean; {has eof been seen?}
@y
@!eof_seen : ^boolean; {has eof been seen?}
@z

@x [53a.1506] l.??? texarray
@!grp_stack : array[0..max_in_open] of save_pointer; {initial |cur_boundary|}
@!if_stack : array[0..max_in_open] of pointer; {initial |cond_ptr|}
@y
@!grp_stack : ^save_pointer; {initial |cur_boundary|}
@!if_stack : ^pointer; {initial |cond_ptr|}
@z

@x [53a.1587] l.??? texarray
hyph_root:=0; hyph_start:=0;
@y
@z

