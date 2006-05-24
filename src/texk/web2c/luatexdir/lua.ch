%$Id: lua.ch,v 1.3 2005/08/09 20:17:58 hahe Exp hahe $
%
% This implements primitives \luaescapestring, \lua and \latelua.
%

%********************************* kill off the pool file

@x
@ When the \.{WEB} system program called \.{TANGLE} processes the \.{TEX.WEB}
description that you are now reading, it outputs the \PASCAL\ program
\.{TEX.PAS} and also a string pool file called \.{TEX.POOL}. The \.{INITEX}
@.WEB@>@.INITEX@>
program reads the latter file, where each string appears as a two-digit decimal
length followed by the string itself, and the information is recorded in
\TeX's string memory.

@<Glob...@>=
@!init @!pool_file:alpha_file; {the string-pool file output by \.{TANGLE}}
tini

@ @d bad_pool(#)==begin wake_up_terminal; write_ln(term_out,#);
  a_close(pool_file); get_strings_started:=false; return;
  end
@<Read the other strings...@>=
name_length := strlen (pool_name);
name_of_file := xmalloc_array (ASCII_code, name_length + 1);
strcpy (stringcast(name_of_file+1), pool_name); {copy the string}
if a_open_in (pool_file, kpse_texpool_format) then
  begin c:=false;
  repeat @<Read one string, but return |false| if the
    string memory space is getting too tight for comfort@>;
  until c;
  a_close(pool_file); get_strings_started:=true;
  end
else  bad_pool('! I can''t read ', pool_name, '; bad path?')
@.I can't read TEX.POOL@>

@ @<Read one string...@>=
begin if eof(pool_file) then bad_pool('! ', pool_name, ' has no check sum.');
@.TEX.POOL has no check sum@>
read(pool_file,m); read(pool_file,n); {read two digits of string length}
if m='*' then @<Check the pool check sum@>
else  begin if (xord[m]<"0")or(xord[m]>"9")or@|
      (xord[n]<"0")or(xord[n]>"9") then
    bad_pool('! ', pool_name, ' line doesn''t begin with two digits.');
@.TEX.POOL line doesn't...@>
  l:=xord[m]*10+xord[n]-"0"*11; {compute the length}
  if pool_ptr+l+string_vacancies>pool_size then
    bad_pool('! You have to increase POOLSIZE.');
@.You have to increase POOLSIZE@>
  for k:=1 to l do
    begin if eoln(pool_file) then m:=' '@+else read(pool_file,m);
    append_char(xord[m]);
    end;
  read_ln(pool_file); g:=make_string;
  end;
end

@ The \.{WEB} operation \.{@@\$} denotes the value that should be at the
end of this \.{TEX.POOL} file; any other value means that the wrong pool
file has been loaded.
@^check sum@>

@<Check the pool check sum@>=
begin a:=0; k:=1;
loop@+  begin if (xord[n]<"0")or(xord[n]>"9") then
  bad_pool('! ', pool_name, ' check sum doesn''t have nine digits.');
@.TEX.POOL check sum...@>
  a:=10*a+xord[n]-"0";
  if k=9 then goto done;
  incr(k); read(pool_file,n);
  end;
done: if a<>@$ then
  bad_pool('! ', pool_name, ' doesn''t match; tangle me again (or fix the path).');
@.TEX.POOL doesn't match@>
c:=true;
end
@y

@ @<Read the other strings...@>=
  g := loadpoolstrings((pool_size-string_vacancies));
  if c=0 then begin 
     wake_up_terminal; write_ln(term_out,'! You have to increase POOLSIZE.');
     get_strings_started:=false; 
     return;
  end;
  get_strings_started:=true;
@z


%***********************************************************************

@x

@p @t\4@>@<Declare \eTeX\ procedures for token lists@>@;@/
function str_toks(@!b:pool_pointer):pointer;
@y
|lua_str_toks| is almost identical, but it also escapes the three
symbols that |lua| considers special while scanning a literal string

@p @t\4@>@<Declare \eTeX\ procedures for token lists@>@;@/
function lua_str_toks(@!b:pool_pointer):pointer;
  {changes the string |str_pool[b..pool_ptr]| to a token list}
var p:pointer; {tail of the token list}
@!q:pointer; {new node being added to the token list via |store_new_token|}
@!t:halfword; {token being appended}
@!k:pool_pointer; {index into |str_pool|}
begin str_room(1);
p:=temp_head; link(p):=null; k:=b;
while k<pool_ptr do
  begin t:=so(str_pool[k]);
  if t=" " then t:=space_token
  else 
    begin 
    if (t="\") or (t="""") or (t="'") then 
	  fast_store_new_token(other_token+"\");
    t:=other_token+t;  
    end;
  fast_store_new_token(t);
  incr(k);
  end;
pool_ptr:=b; lua_str_toks:=p;
end;

function str_toks(@!b:pool_pointer):pointer;
@z

%***********************************************************************

@x 10721
@d pdftex_convert_codes     = pdftex_first_expand_code + 25 {end of \pdfTeX's command codes}
@y
@d lua_code                 = pdftex_first_expand_code + 25 {command code for \.{\\lua}}
@d lua_escape_string_code   = pdftex_first_expand_code + 26 {command code for \.{\\luaescapestring}}
@d pdftex_convert_codes     = pdftex_first_expand_code + 27 {end of \pdfTeX's command codes}
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
primitive("luaescapestring",convert,lua_escape_string_code);@/
@!@:lua_}{\.{\\luaescapestring} primitive@>
@z

%***********************************************************************

@x 10818
  pdf_ximage_bbox_code: print_esc("pdfximagebbox");
@y
  pdf_ximage_bbox_code:    print_esc("pdfximagebbox");
  lua_code:                print_esc("lua");
  lua_escape_string_code:  print_esc("luaescapestring");
@z

%***********************************************************************


@x 11121
normal_deviate_code:      do_nothing;
@y
normal_deviate_code:      do_nothing;
lua_escape_string_code: 
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
    link(garbage) := lua_str_toks(str_start[s]);
	ins_list(link(temp_head));
    flush_str(s);
    return;
  end;
lua_code:
  begin
    save_scanner_status := scanner_status;
    save_def_ref := def_ref;
    save_warning_index := warning_index;
	scan_register_num;	
    scan_pdf_ext_toks;
    s := tokens_to_string(def_ref);
    delete_token_ref(def_ref);
    def_ref := save_def_ref;
    warning_index := save_warning_index;
    scanner_status := save_scanner_status;
    b := pool_ptr;
    luacall(cur_val,s);
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

procedure latelua(lua_id: quarterword; s: str_number; lua_mode: integer; warn: boolean);
var b, j: pool_pointer; {current character code position}
begin
    j:=str_start[s];
    case lua_mode of
    set_origin: begin
        pdf_end_text;
        pdf_set_origin(cur_h, cur_v);
        end;
    direct_page:
        pdf_end_text;
    direct_always:
        pdf_end_string_nl;
    othercases confusion("latelua1")
    endcases;
    b := pool_ptr;
    luacall(lua_id,s);
    while b<pool_ptr do begin
       pdf_out(str_pool[b]);
       incr(b);
    end;
    pdf_print_nl;
end;
@z

%***********************************************************************

@x 16171
@d scan_special            == 3 {look into special text}
@y
@d scan_special            == 3 {look into special text}

@# {data structure for \.{\\latelua}}
@d late_lua_data(#)        == link(#+1) {data}
@# {a bit sneaky, since these are quarterwords that are assumed to hold 16bits}
@d late_lua_mode(#)        == type(#+1) {mode of resetting the text matrix
                              while writing data to the page stream}
@d late_lua_reg(#)         == subtype(#+1) {register id}
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
    latelua(late_lua_reg(p), s, late_lua_mode(p), false);
    flush_str(s);
end;
@z

%***********************************************************************

@x 32911
@d pdftex_last_extension_code  == pdftex_first_extension_code + 29
@y
@d late_lua_node               == pdftex_first_extension_code + 30
@d pdftex_last_extension_code  == pdftex_first_extension_code + 30
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
        late_lua_mode(tail) := set_origin;
	scan_register_num;
	late_lua_reg(tail) := cur_val;
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
    set_origin:
        do_nothing;
    direct_page:
        print(" page");
    direct_always:
        print(" direct");
    othercases confusion("latelua2")
    endcases;
	print_int(late_lua_reg(p));
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

function get_cur_v: integer;
begin
    if is_shipping_page then
        get_cur_v := cur_page_height - cur_v
    else
        get_cur_v := pdf_xform_height + pdf_xform_depth - cur_v;
end;

function get_cur_h: integer;
begin
    get_cur_h := cur_h;
end;


@* \[54] System-dependent changes.
@z
%***********************************************************************

