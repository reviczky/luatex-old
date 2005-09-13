%$Id: lua.ch,v 1.3 2005/08/09 20:17:58 hahe Exp hahe $
%
% This implements primitive \pdfpkmode as a token register.
%
%***********************************************************************

@x 10721
@d uniform_deviate_code     = pdftex_first_expand_code + 21 {end of \pdfTeX's command codes}
@d normal_deviate_code      = pdftex_first_expand_code + 22 {end of \pdfTeX's command codes}
@d pdftex_convert_codes     = pdftex_first_expand_code + 23 {end of \pdfTeX's command codes}
@y
@d uniform_deviate_code     = pdftex_first_expand_code + 21 {command code for \.{\\uniformdeviate}}
@d normal_deviate_code      = pdftex_first_expand_code + 22 {command code for \.{\\normaldeviate}}
@d lua_code                 = pdftex_first_expand_code + 23 {command code for \.{\\lua}}
@d pdftex_convert_codes     = pdftex_first_expand_code + 24 {end of \pdfTeX's command codes}
@z

%***********************************************************************

@x 10783
primitive("pdfnormaldeviate",convert,normal_deviate_code);@/
@!@:normal_deviate_}{\.{\\pdfnormaldeviate} primitive@>
@y
primitive("pdfnormaldeviate",convert,normal_deviate_code);@/
@!@:normal_deviate_}{\.{\\pdfnormaldeviate} primitive@>
primitive("lua",convert,lua_code);@/
@!@:lua_}{\.{\\lua} primitive@>
@z

%***********************************************************************

@x 10818
  normal_deviate_code:      print_esc("pdfnormaldeviate");
@y
  normal_deviate_code:      print_esc("pdfnormaldeviate");
  lua_code:                 print_esc("lua");
@z

%***********************************************************************

@x 11120
normal_deviate_code:      do_nothing;
@y
normal_deviate_code:      do_nothing;
lua_code:
  begin
    save_scanner_status := scanner_status;
    save_def_ref := def_ref;
    save_warning_index := warning_index;
    scan_pdf_ext_toks;
    s := tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    def_ref := save_def_ref;
    warning_index := save_warning_index;
    scanner_status := save_scanner_status;
    b := pool_ptr;
    luacall(s);
    link(garbage):=str_toks(b);
	ins_list(link(temp_head));
    flush_str(s);
    return;
  end;
@z

@x
@* \[54] System-dependent changes.
@y

@ The lua interface needs some extra pascal functions. The functions
themselves are quite boring, but they are handy because otherwise this
internal stuff has to be accessed from C directly, where lots of the
pascal defines are not available.

@p function get_tex_dimen_register (j:integer):scaled;
begin
  get_tex_dimen_register := dimen(j);
end;

function set_tex_dimen_register (j:integer;v:scaled):integer;
begin  {return non-nil for error}
dimen(j) := v;
set_tex_dimen_register := 0;
end;

function get_tex_count_register (j:integer):scaled;
begin
  get_tex_count_register := count(j);
end;

function set_tex_count_register (j:integer;v:scaled):integer;
begin  {return non-nil for error}
count(j) := v;
set_tex_count_register := 0;
end;

function get_tex_toks_register (j:integer):str_number;
var s:str_number;
begin
  s:="";
  if toks(j) <> min_halfword then begin
	s := tokens_to_string(toks(j));
  end;
  get_tex_toks_register := s;
end;

function set_tex_toks_register (j:integer;s:str_number):integer;
var save_pool_ptr:pool_pointer;
  ref:pointer;
begin
  set_tex_toks_register := 0;
  save_pool_ptr := pool_ptr;
  pool_ptr := str_start[s+1];
  ref := get_avail;
  link(garbage) := str_toks(str_start[s]);
  pool_ptr := save_pool_ptr;
  token_ref_count(ref) := 0;
  link(ref) := link(temp_head);
  toks(j) := ref;
  flush_str(s);
end;

function get_tex_box_width (j:integer):scaled;
var q:pointer;
begin
 q := box(j);
 get_tex_box_width := 0;
 if q <> min_halfword then
    get_tex_box_width := width(q);
end;

function set_tex_box_width (j:integer;v:scaled):integer;
var q:pointer;
begin 
  q := box(j);
  set_tex_box_width := 0;
  if q <> min_halfword then
    width(q) := v
  else
    set_tex_box_width := 1;
end;

function get_tex_box_height (j:integer):scaled;
var q:pointer;
begin
 q := box(j);
 get_tex_box_height := 0;
 if q <> min_halfword then
    get_tex_box_height := height(q);
end;

function set_tex_box_height (j:integer;v:scaled):integer;
var q:pointer;
begin 
  q := box(j);
  set_tex_box_height := 0;
  if q <> min_halfword then
    height(q) := v
  else
    set_tex_box_height := 1;
end;

function get_tex_box_depth (j:integer):scaled;
var q:pointer;
begin
 q := box(j);
 get_tex_box_depth := 0;
 if q <> min_halfword then
    get_tex_box_depth := depth(q);
end;

function set_tex_box_depth (j:integer;v:scaled):integer;
var q:pointer;
begin 
  q := box(j);
  set_tex_box_depth := 0;
  if q <> min_halfword then
    depth(q) := v
  else
    set_tex_box_depth := 1;
end;


@* \[54] System-dependent changes.
@z
%***********************************************************************

