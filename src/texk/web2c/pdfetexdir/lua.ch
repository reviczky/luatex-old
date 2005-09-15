%$Id: lua.ch,v 1.3 2005/08/09 20:17:58 hahe Exp hahe $
%
% This implements primitives \lua and \latelua.
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
  uniform_deviate_code:     print_esc("pdfuniformdeviate");
  normal_deviate_code:      print_esc("pdfnormaldeviate");
@y
  uniform_deviate_code:     print_esc("pdfuniformdeviate");
  normal_deviate_code:      print_esc("pdfnormaldeviate");
  lua_code:                 print_esc("lua");
@z

%***********************************************************************

@x 11121
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
    link(garbage) := str_toks(b);
	ins_list(link(temp_head));
    flush_str(s);
    return;
  end;
@z

%***********************************************************************

@x 15633
    while j<str_start[s+1] do begin
       pdf_out(str_pool[j]);
       incr(j);
    end;
    pdf_print_nl;
end;
@y
    while j<str_start[s+1] do begin
       pdf_out(str_pool[j]);
       incr(j);
    end;
    pdf_print_nl;
end;

procedure latelua(s: str_number; lua_mode: integer; warn: boolean);
var b, j: pool_pointer; {current character code position}
begin
    j:=str_start[s];
    case lua_mode of
    reset_origin: begin
        pdf_end_text;
        pdf_set_origin;
        end;
    direct_page:
        pdf_end_text;
    direct_always: begin
        pdf_first_space_corr := 0;
        pdf_end_string;
        pdf_print_nl;
        end;
    othercases confusion("latelua1")
    endcases;
    b := pool_ptr;
    luacall(s);
    while b<pool_ptr do begin
       pdf_out(str_pool[b]);
       incr(b);
    end;
    pdf_print_nl;
end;
@z

%***********************************************************************

@x 16171
@d scan_special == 3          {look into special text}
@y
@d scan_special == 3          {look into special text}

@# {data structure for \.{\\latelua}}
@d late_lua_data(#)        == link(#+1) {data}
@d late_lua_mode(#)        == info(#+1) {mode of resetting the text matrix
                              while writing data to the page stream}
@z

%***********************************************************************

@x 17656
procedure pdf_special(p: pointer);
var old_setting:0..max_selector; {holds print |selector|}
    s: str_number;
begin
    old_setting:=selector; selector:=new_string;
    show_token_list(link(write_tokens(p)),null,pool_size-pool_ptr);
    selector:=old_setting;
    s := make_string;
    literal(s, scan_special, true);
    flush_str(s);
end;
@y
procedure pdf_special(p: pointer);
var old_setting:0..max_selector; {holds print |selector|}
    s: str_number;
begin
    old_setting:=selector; selector:=new_string;
    show_token_list(link(write_tokens(p)),null,pool_size-pool_ptr);
    selector:=old_setting;
    s := make_string;
    literal(s, scan_special, true);
    flush_str(s);
end;

procedure do_late_lua(p: pointer);
var old_setting:0..max_selector; {holds print |selector|}
    s: str_number;
begin
    old_setting:=selector; selector:=new_string;
    show_token_list(link(write_tokens(p)),null,pool_size-pool_ptr);
    selector:=old_setting;
    s := make_string;
    latelua(s, late_lua_mode(p), false);
    flush_str(s);
end;
@z

%***********************************************************************

@x 32911
@d pdftex_last_extension_code  == pdftex_first_extension_code + 26
@y
@d late_lua_node               == pdftex_first_extension_code + 27
@d pdftex_last_extension_code  == pdftex_first_extension_code + 27
@z

%***********************************************************************

@x 32979
primitive("pdfsetrandomseed",extension,set_random_seed_code);@/
@!@:set_random_seed_code}{\.{\\pdfsetrandomseed} primitive@>
@y
primitive("pdfsetrandomseed",extension,set_random_seed_code);@/
@!@:set_random_seed_code}{\.{\\pdfsetrandomseed} primitive@>
primitive("latelua",extension,late_lua_node);@/
@!@:late_lua_node_}{\.{\\latelua} primitive@>
@z

%***********************************************************************

@x 33023
  othercases print("[unknown extension!]")
@y
  late_lua_node: print_esc("latelua");
  othercases print("[unknown extension!]")
@z

%***********************************************************************

@x 33071
othercases confusion("ext1")
@y
late_lua_node: @<Implement \.{\\latelua}@>;
othercases confusion("ext1")
@z

%***********************************************************************

@x 33240
    pdf_literal_data(tail) := def_ref;
end
@y
    pdf_literal_data(tail) := def_ref;
end

@ @<Implement \.{\\latelua}@>=
begin
    check_pdfoutput("\latelua", true);
    new_whatsit(late_lua_node, write_node_size);
    if scan_keyword("direct") then
        late_lua_mode(tail) := direct_always
    else if scan_keyword("page") then
        late_lua_mode(tail) := direct_page
    else
        late_lua_mode(tail) := reset_origin;
    scan_pdf_ext_toks;
    late_lua_data(tail) := def_ref;
end
@z

%***********************************************************************

@x 34358
    print_mark(pdf_literal_data(p));
end;
@y
    print_mark(pdf_literal_data(p));
end;
late_lua_node: begin
    print_esc("latelua");
    case late_lua_mode(p) of
    reset_origin:
        do_nothing;
    direct_page:
        print(" page");
    direct_always:
        print(" direct");
    othercases confusion("latelua2")
    endcases;
    print_mark(late_lua_data(p));
end;
@z

%***********************************************************************

@x 34512
pdf_literal_node: begin
    r := get_node(write_node_size);
    add_token_ref(pdf_literal_data(p));
    words := write_node_size;
end;
@y
pdf_literal_node: begin
    r := get_node(write_node_size);
    add_token_ref(pdf_literal_data(p));
    words := write_node_size;
end;
late_lua_node: begin
    r := get_node(write_node_size);
    add_token_ref(late_lua_data(p));
    words := write_node_size;
end;
@z

%***********************************************************************

@x 34577
pdf_literal_node: begin
    delete_token_ref(pdf_literal_data(p));
    free_node(p, write_node_size);
end;
@y
pdf_literal_node: begin
    delete_token_ref(pdf_literal_data(p));
    free_node(p, write_node_size);
end;
late_lua_node: begin
    delete_token_ref(late_lua_data(p));
    free_node(p, write_node_size);
end;
@z

%***********************************************************************

@x 35450
pdf_literal_node:
    pdf_out_literal(p);
@y
pdf_literal_node:
    pdf_out_literal(p);
late_lua_node:
    do_late_lua(p);
@z

%***********************************************************************

@x 35506
pdf_literal_node:
    pdf_out_literal(p);
@y
pdf_literal_node:
    pdf_out_literal(p);
late_lua_node:
    do_late_lua(p);
@z

%***********************************************************************


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

