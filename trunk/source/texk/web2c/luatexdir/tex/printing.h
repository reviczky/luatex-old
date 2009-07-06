/* printing.h
   
   Copyright 2009 Taco Hoekwater <taco@luatex.org>

   This file is part of LuaTeX.

   LuaTeX is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   LuaTeX is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with LuaTeX; if not, see <http://www.gnu.org/licenses/>. */

/* $Id$ */

#ifndef PRINTING_H
#  define PRINTING_H

typedef enum {
    no_print = 16,              /* |selector| setting that makes data disappear */
    term_only = 17,             /* printing is destined for the terminal only */
    log_only = 18,              /* printing is destined for the transcript file only */
    term_and_log = 19,          /* normal |selector| setting */
    pseudo = 20,                /* special |selector| setting for |show_context| */
    new_string = 21,            /* printing is deflected to the string pool */
} selector_settings;

#  define ssup_error_line 255
#  define max_selector new_string
                                /* highest selector setting */

extern alpha_file log_file;
extern int selector;
extern int dig[23];
extern integer tally;
extern int term_offset;
extern int file_offset;
extern packed_ASCII_code trick_buf[(ssup_error_line + 1)];
extern integer trick_count;
extern integer first_count;
extern boolean inhibit_par_tokens;

/*
Macro abbreviations for output to the terminal and to the log file are
defined here for convenience. Some systems need special conventions
for terminal output, and it is possible to adhere to those conventions
by changing |wterm|, |wterm_ln|, and |wterm_cr| in this section.
@^system dependencies@>
*/

#  define wterm_cr()   fprintf(term_out,"\n")
#  define wlog_cr()    fprintf(log_file,"\n")

extern void print_ln(void);
extern void print_char(int s);
extern void print(integer s);
extern void print_nl(str_number s);
extern void print_nlp(void);
extern void slow_print(integer s);
extern void print_banner(char *, int);
extern void log_banner(char *, int);
extern void print_version_banner(void);
extern void print_esc(str_number s);
extern void print_the_digs(eight_bits k);
extern void print_int(longinteger n);
extern void print_two(integer n);
extern void print_hex(integer n);
extern void print_roman_int(integer n);
extern void print_current_string(void);

#  define print_font_name(A) tprint(font_name(A))

extern void print_cs(integer p);
extern void sprint_cs(pointer p);
extern void tprint(char *s);
extern void tprint_nl(char *s);
extern void tprint_esc(char *s);

extern void prompt_input(char *s);


#  define single_letter(A)					\
  ((str_length(A)==1)||						\
   ((str_length(A)==4)&&(str_pool[str_start_macro(A)]>=0xF0))||	\
   ((str_length(A)==3)&&(str_pool[str_start_macro(A)]>=0xE0))||	\
   ((str_length(A)==2)&&(str_pool[str_start_macro(A)]>=0xC0)))

#  define is_active_cs(a) (str_length(a)>3 &&			      \
                         (str_pool[str_start_macro(a)]   == 0xEF) &&  \
                         (str_pool[str_start_macro(a)+1] == 0xBF) &&  \
                         (str_pool[str_start_macro(a)+2] == 0xBF))


#  define active_cs_value(A) pool_to_unichar(str_start_macro(A)+3)

extern void print_glue(scaled d, integer order, char *s);       /* prints a glue component */
extern void print_spec(integer p, char *s);     /* prints a glue specification */

#endif
