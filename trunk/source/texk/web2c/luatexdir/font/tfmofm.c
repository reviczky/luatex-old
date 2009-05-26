/* tfmofm.c
   
   Copyright 2006-2008 Taco Hoekwater <taco@luatex.org>

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

#include "ptexlib.h"
#include "luatex-api.h"
#include "luatexfont.h"

static const char _svn_version[] =
    "$Id$ $URL$";

/* Here are some macros that help process ligatures and kerns */

#define lig_kern_start(f,c)   char_remainder(f,c)
#define stop_flag 128           /* value indicating `\.{STOP}' in a lig/kern program */
#define kern_flag 128           /* op code for a kern step */

#define skip_byte(z)         lig_kerns[z].b0
#define next_char(z)         lig_kerns[z].b1
#define op_byte(z)           lig_kerns[z].b2
#define rem_byte(z)          lig_kerns[z].b3
#define lig_kern_restart(c)  (256*op_byte(c)+rem_byte(c))

/*

The information in a \.{TFM} file appears in a sequence of 8-bit bytes.
Since the number of bytes is always a multiple of 4, we could
also regard the file as a sequence of 32-bit words, but \TeX\ uses the
byte interpretation. The format of \.{TFM} files was designed by
Lyle Ramshaw in 1980. The intent is to convey a lot of different kinds
@^Ramshaw, Lyle Harold@>
of information in a compact but useful form.

$\Omega$ is capable of reading not only \.{TFM} files, but also
\.{OFM} files, which can describe fonts with up to 65536 characters
and with huge lig/kern tables.  These fonts will often be virtual
fonts built up from real fonts with 256 characters, but $\Omega$
is not aware of this.  

The documentation below describes \.{TFM} files, with slight additions
to show where \.{OFM} files differ.
                                         */
/*
@ The first 24 bytes (6 words) of a \.{TFM} file contain twelve 16-bit
integers that give the lengths of the various subsequent portions
of the file. These twelve integers are, in order:
$$\vbox{\halign{\hfil#&$\null=\null$#\hfil\cr
|lf|&length of the entire file, in words;\cr
|lh|&length of the header data, in words;\cr
|bc|&smallest character code in the font;\cr
|ec|&largest character code in the font;\cr
|nw|&number of words in the width table;\cr
|nh|&number of words in the height table;\cr
|nd|&number of words in the depth table;\cr
|ni|&number of words in the italic correction table;\cr
|nl|&number of words in the lig/kern table;\cr
|nk|&number of words in the kern table;\cr
|ne|&number of words in the extensible character table;\cr
|np|&number of font parameter words.\cr}}$$
They are all nonnegative and less than $2^{15}$. We must have |bc-1<=ec<=255|,
and
$$\hbox{|lf=6+lh+(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np|.}$$
Note that a \.{TFM} font may contain as many as 256 characters 
(if |bc=0| and |ec=255|), and as few as 0 characters (if |bc=ec+1|).

Incidentally, when two or more 8-bit bytes are combined to form an integer of
16 or more bits, the most significant bytes appear first in the file.
This is called BigEndian order.
@!@^BigEndian order@>

The first 52 bytes (13 words) of an \.{OFM} file contains thirteen
32-bit integers that give the lengths of the various subsequent
portions of the file.  The first word is 0 (future versions of
\.{OFM} files could have different values;  what is important is that
the first two bytes be 0 to differentiate \.{TFM} and \.{OFM} files).
The next twelve integers are as above, all nonegative and less
than~$2^{31}$.  We must have |bc-1<=ec<=65535|, and
$$\hbox{|lf=13+lh+2*(ec-bc+1)+nw+nh+nd+ni+nl+nk+ne+np|.}$$
Note that an \.{OFM} font may contain as many as 65536 characters 
(if |bc=0| and |ec=65535|), and as few as 0 characters (if |bc=ec+1|).

@ The rest of the \.{TFM} file may be regarded as a sequence of ten data
arrays having the informal specification
$$\def\arr$[#1]#2${\&{array} $[#1]$ \&{of} #2}
\vbox{\halign{\hfil\\{#}&$\,:\,$\arr#\hfil\cr
header&|[0..lh-1]@t\\{stuff}@>|\cr
char\_info&|[bc..ec]char_info_word|\cr
width&|[0..nw-1]fix_word|\cr
height&|[0..nh-1]fix_word|\cr
depth&|[0..nd-1]fix_word|\cr
italic&|[0..ni-1]fix_word|\cr
lig\_kern&|[0..nl-1]lig_kern_command|\cr
kern&|[0..nk-1]fix_word|\cr
exten&|[0..ne-1]extensible_recipe|\cr
param&|[1..np]fix_word|\cr}}$$
The most important data type used here is a |@!fix_word|, which is
a 32-bit representation of a binary fraction. A |fix_word| is a signed
quantity, with the two's complement of the entire word used to represent
negation. Of the 32 bits in a |fix_word|, exactly 12 are to the left of the
binary point; thus, the largest |fix_word| value is $2048-2^{-20}$, and
the smallest is $-2048$. We will see below, however, that all but two of
the |fix_word| values must lie between $-16$ and $+16$.

@ The first data array is a block of header information, which contains
general facts about the font. The header must contain at least two words,
|header[0]| and |header[1]|, whose meaning is explained below.
Additional header information of use to other software routines might
also be included, but \TeX82 does not need to know about such details.
For example, 16 more words of header information are in use at the Xerox
Palo Alto Research Center; the first ten specify the character coding
scheme used (e.g., `\.{XEROX text}' or `\.{TeX math symbols}'), the next five
give the font identifier (e.g., `\.{HELVETICA}' or `\.{CMSY}'), and the
last gives the ``face byte.'' The program that converts \.{DVI} files
to Xerox printing format gets this information by looking at the \.{TFM}
file, which it needs to read anyway because of other information that
is not explicitly repeated in \.{DVI}~format.

\yskip\hang|header[0]| is a 32-bit check sum that \TeX\ will copy into
the \.{DVI} output file. Later on when the \.{DVI} file is printed,
possibly on another computer, the actual font that gets used is supposed
to have a check sum that agrees with the one in the \.{TFM} file used by
\TeX. In this way, users will be warned about potential incompatibilities.
(However, if the check sum is zero in either the font file or the \.{TFM}
file, no check is made.)  The actual relation between this check sum and
the rest of the \.{TFM} file is not important; the check sum is simply an
identification number with the property that incompatible fonts almost
always have distinct check sums.
@^check sum@>

\yskip\hang|header[1]| is a |fix_word| containing the design size of
the font, in units of \TeX\ points. This number must be at least 1.0; it is
fairly arbitrary, but usually the design size is 10.0 for a ``10 point''
font, i.e., a font that was designed to look best at a 10-point size,
whatever that really means. When a \TeX\ user asks for a font
`\.{at} $\delta$ \.{pt}', the effect is to override the design size
and replace it by $\delta$, and to multiply the $x$ and~$y$ coordinates
of the points in the font image by a factor of $\delta$ divided by the
design size.  {\sl All other dimensions in the\/ \.{TFM} file are
|fix_word|\kern-1pt\ numbers in design-size units}, with the exception of
|param[1]| (which denotes the slant ratio). Thus, for example, the value
of |param[6]|, which defines the \.{em} unit, is often the |fix_word| value
$2^{20}=1.0$, since many fonts have a design size equal to one em.
The other dimensions must be less than 16 design-size units in absolute
value; thus, |header[1]| and |param[1]| are the only |fix_word|
entries in the whole \.{TFM} file whose first byte might be something
besides 0 or 255.

@ Next comes the |char_info| array, which contains one |@!char_info_word|
per character. Each word in this part of a \.{TFM} file contains six fields
packed into four bytes as follows.

\yskip\hang first byte: |@!width_index| (8 bits)\par
\hang second byte: |@!height_index| (4 bits) times 16, plus |@!depth_index|
  (4~bits)\par
\hang third byte: |@!italic_index| (6 bits) times 4, plus |@!tag|
  (2~bits)\par
\hang fourth byte: |@!remainder| (8 bits)\par
\yskip\noindent
The actual width of a character is \\{width}|[width_index]|, in design-size
units; this is a device for compressing information, since many characters
have the same width. Since it is quite common for many characters
to have the same height, depth, or italic correction, the \.{TFM} format
imposes a limit of 16 different heights, 16 different depths, and
64 different italic corrections.

For \.{OFM} files, two words (eight bytes) are used. 
The arrangement is as follows.

\yskip\hang first and second bytes: |@!width_index| (16 bits)\par
\hang third byte: |@!height_index| (8 bits)\par
\hang fourth byte: |@!depth_index| (8~bits)\par
\hang fifth and sixth bytes: 
|@!italic_index| (14 bits) times 4, plus |@!tag| (2~bits)\par
\hang seventh and eighth bytes: |@!remainder| (16 bits)\par
\yskip\noindent
Therefore the \.{OFM} format imposes a limit of 256 different heights,
256 different depths, and 16384 different italic corrections.

@!@^italic correction@>
The italic correction of a character has two different uses.
(a)~In ordinary text, the italic correction is added to the width only if
the \TeX\ user specifies `\.{\\/}' after the character.
(b)~In math formulas, the italic correction is always added to the width,
except with respect to the positioning of subscripts.

Incidentally, the relation $\\{width}[0]=\\{height}[0]=\\{depth}[0]=
\\{italic}[0]=0$ should always hold, so that an index of zero implies a
value of zero.  The |width_index| should never be zero unless the
character does not exist in the font, since a character is valid if and
only if it lies between |bc| and |ec| and has a nonzero |width_index|.
*/

/*
@ \TeX\ checks the information of a \.{TFM} file for validity as the
file is being read in, so that no further checks will be needed when
typesetting is going on. The somewhat tedious subroutine that does this
is called |read_font_info|. It has four parameters: the user font
identifier~|u|, the file name and area strings |nom| and |aire|, and the
``at'' size~|s|. If |s|~is negative, it's the negative of a scale factor
to be applied to the design size; |s=-1000| is the normal case.
Otherwise |s| will be substituted for the design size; in this
case, |s| must be positive and less than $2048\rm\,pt$
(i.e., it must be less than $2^{27}$ when considered as an integer).

The subroutine opens and closes a global file variable called |tfm_file|.
It returns the value of the internal font number that was just loaded.
If an error is detected, an error message is issued and no font
information is stored; |null_font| is returned in this case.

@d bad_tfm=11 {label for |read_font_info|}
@d abort==goto bad_tfm {do this when the \.{TFM} data is wrong}

*/

/* 
The |tag| field in a |char_info_word| has four values that explain how to
interpret the |remainder| field.

\yskip\hangg|tag=0| (|no_tag|) means that |remainder| is unused.\par
\hangg|tag=1| (|lig_tag|) means that this character has a ligature/kerning
program starting at position |remainder| in the |lig_kern| array.\par
\hangg|tag=2| (|list_tag|) means that this character is part of a chain of
characters of ascending sizes, and not the largest in the chain.  The
|remainder| field gives the character code of the next larger character.\par
\hangg|tag=3| (|ext_tag|) means that this character code represents an
extensible character, i.e., a character that is built up of smaller pieces
so that it can be made arbitrarily large. The pieces are specified in
|@!exten[remainder]|.\par
\yskip\noindent
Characters with |tag=2| and |tag=3| are treated as characters with |tag=0|
unless they are used in special circumstances in math formulas. For example,
the \.{\\sum} operation looks for a |list_tag|, and the \.{\\left}
operation looks for both |list_tag| and |ext_tag|.
*/

/* The |lig_kern| array contains instructions in a simple programming language
that explains what to do for special letter pairs. Each word in this array,
in a \.{TFM} file, is a |@!lig_kern_command| of four bytes.

\yskip\hang first byte: |skip_byte|, indicates that this is the final program
  step if the byte is 128 or more, otherwise the next step is obtained by
  skipping this number of intervening steps.\par
\hang second byte: |next_char|, ``if |next_char| follows the current character,
  then perform the operation and stop, otherwise continue.''\par
\hang third byte: |op_byte|, indicates a ligature step if less than~128,
  a kern step otherwise.\par
\hang fourth byte: |remainder|.\par
\yskip\noindent
In an \.{OFM} file, eight bytes are used, two bytes for each field.

In a kern step, an
additional space equal to |kern[256*(op_byte-128)+remainder]| is inserted
between the current character and |next_char|. This amount is
often negative, so that the characters are brought closer together
by kerning; but it might be positive.

There are eight kinds of ligature steps, having |op_byte| codes $4a+2b+c$ where
$0\le a\le b+c$ and $0\le b,c\le1$. The character whose code is
|remainder| is inserted between the current character and |next_char|;
then the current character is deleted if $b=0$, and |next_char| is
deleted if $c=0$; then we pass over $a$~characters to reach the next
current character (which may have a ligature/kerning program of its own).

If the very first instruction of the |lig_kern| array has |skip_byte=255|,
the |next_char| byte is the so-called right boundary character of this font;
the value of |next_char| need not lie between |bc| and~|ec|.
If the very last instruction of the |lig_kern| array has |skip_byte=255|,
there is a special ligature/kerning program for a left boundary character,
beginning at location |256*op_byte+remainder|.
The interpretation is that \TeX\ puts implicit boundary characters
before and after each consecutive string of characters from the same font.
These implicit characters do not appear in the output, but they can affect
ligatures and kerning.

If the very first instruction of a character's |lig_kern| program has
|skip_byte>128|, the program actually begins in location
|256*op_byte+remainder|. This feature allows access to large |lig_kern|
arrays, because the first instruction must otherwise
appear in a location |<=255| in a \.{TFM} file, |<=65535| in an \.{OFM} file.

Any instruction with |skip_byte>128| in the |lig_kern| array must satisfy
the condition
$$\hbox{|256*op_byte+remainder<nl|.}$$
If such an instruction is encountered during
normal program execution, it denotes an unconditional halt; no ligature
or kerning command is performed.
*/

/*
 Extensible characters are specified by an |@!extensible_recipe|, which
consists of four bytes in a \.{TFM} file, 
called |@!top|, |@!mid|, |@!bot|, and |@!rep| (in this order). 
In an \.{OFM} file, each field takes two bytes, for eight in total.
These bytes are the character codes of individual pieces used to
build up a large symbol.  If |top|, |mid|, or |bot| are zero, they are not
present in the built-up result. For example, an extensible vertical line is
like an extensible bracket, except that the top and bottom pieces are missing.

Let $T$, $M$, $B$, and $R$ denote the respective pieces, or an empty box
if the piece isn't present. Then the extensible characters have the form
$TR^kMR^kB$ from top to bottom, for some |k>=0|, unless $M$ is absent;
in the latter case we can have $TR^kB$ for both even and odd values of~|k|.
The width of the extensible character is the width of $R$; and the
height-plus-depth is the sum of the individual height-plus-depths of the
components used, since the pieces are butted together in a vertical list.
*/

/*
The final portion of a \.{TFM} file is the |param| array, which is another
sequence of |fix_word| values.

\yskip\hang|param[1]=slant| is the amount of italic slant, which is used
to help position accents. For example, |slant=.25| means that when you go
up one unit, you also go .25 units to the right. The |slant| is a pure
number; it's the only |fix_word| other than the design size itself that is
not scaled by the design size.

\hang|param[2]=space| is the normal spacing between words in text.
Note that character |" "| in the font need not have anything to do with
blank spaces.

\hang|param[3]=space_stretch| is the amount of glue stretching between words.

\hang|param[4]=space_shrink| is the amount of glue shrinking between words.

\hang|param[5]=x_height| is the size of one ex in the font; it is also
the height of letters for which accents don't have to be raised or lowered.

\hang|param[6]=quad| is the size of one em in the font.

\hang|param[7]=extra_space| is the amount added to |param[2]| at the
ends of sentences.

\yskip\noindent
If fewer than seven parameters are present, \TeX\ sets the missing parameters
to zero. Fonts used for math symbols are required to have
additional parameter information, which is explained later.
*/

/*
 There are programs called \.{TFtoPL} and \.{PLtoTF} that convert
 between the \.{TFM} format and a symbolic property-list format
 that can be easily edited. These programs contain extensive
 diagnostic information, so \TeX\ does not have to bother giving
 precise details about why it rejects a particular \.{TFM} file.

*/

#define tfm_abort { font_tables[f]->_font_name = NULL;      \
                    font_tables[f]->_font_area = NULL;      \
                    xfree(tfm_buffer); xfree(kerns);      \
        xfree(widths);  xfree(heights);  xfree(depths);     \
        xfree(italics);  xfree(extens);  xfree(lig_kerns);  \
        xfree(xligs);  xfree(xkerns);           \
      return 0; }

#define tfm_success { xfree(tfm_buffer); xfree(kerns);       \
                xfree(widths);  xfree(heights);  xfree(depths);    \
          xfree(italics);  xfree(extens);  xfree(lig_kerns); \
          xfree(xligs);  xfree(xkerns); return 1; }


int
open_tfm_file(char *nom, unsigned char **tfm_buf, integer * tfm_siz)
{
    boolean res;                /* was the callback successful? */
    boolean opened;             /* was |tfm_file| successfully opened? */
    integer callback_id;
    FILE *tfm_file;
    /* packfilename(nom,aire,getnullstr()); */
    if (nameoffile != NULL)
        xfree(nameoffile);
    nameoffile = malloc(strlen(nom) + 2);
    strcpy(stringcast(nameoffile + 1), nom);
    namelength = strlen(nom);
    callback_id = callback_defined(read_font_file_callback);
    if (callback_id > 0) {
        res = run_callback(callback_id, "S->bSd", stringcast(nameoffile + 1),
                           &opened, tfm_buf, tfm_siz);
        if (res && opened && (*tfm_siz > 0)) {
            return 1;
        }
        if (!opened)
            return -1;
    } else {
        if (ofm_open_in(tfm_file)) {
            res = read_tfm_file(tfm_file, tfm_buf, tfm_siz);
            b_close(tfm_file);
            if (res) {
                return 1;
            }
        } else {
            return -1;
        }
    }
    return 0;
}


/*
  Note: A malformed \.{TFM} file might be shorter than it claims to be;
  thus |eof(tfm_file)| might be true when |read_font_info| refers to
  |tfm_file^| or when it says |get(tfm_file)|. If such circumstances
  cause system error messages, you will have to defeat them somehow,
  for example by defining |fget| to be `\ignorespaces|begin get(tfm_file);|
  |if eof(tfm_file) then abort; end|\unskip'.
  @^system dependencies@>
*/

#define fget  tfm_byte++
#define fbyte tfm_buffer[tfm_byte]

#define read_sixteen(a)                                                 \
  { a=tfm_buffer[tfm_byte++];                                           \
    if (a>127) { tfm_abort; }                                               \
    a=(a*256)+tfm_buffer[tfm_byte]; }

#define read_sixteen_unsigned(a)                                        \
  { a=tfm_buffer[tfm_byte++];                                           \
    a=(a*256)+tfm_buffer[tfm_byte]; }

#define read_thirtytwo(a)                                               \
  { a=tfm_buffer[++tfm_byte];                                           \
    if (a>127) { tfm_abort; }                                               \
    a=(a*256)+tfm_buffer[++tfm_byte];                                   \
    a=(a*256)+tfm_buffer[++tfm_byte];                                   \
    a=(a*256)+tfm_buffer[++tfm_byte]; }

#define store_four_bytes(z)                                             \
  { a=tfm_buffer[++tfm_byte];           \
    a=(a*256)+tfm_buffer[++tfm_byte];         \
    a=(a*256)+tfm_buffer[++tfm_byte];         \
    a=(a*256)+tfm_buffer[++tfm_byte];         \
    z = a; }

#define store_char_info(z)                                              \
  { if (font_level!=-1) {                                               \
      fget; read_sixteen_unsigned(a);         \
      ci._width_index=a;            \
      fget; read_sixteen_unsigned(b);         \
      ci._height_index=b>>8;            \
      ci._depth_index=b%256;            \
      fget; read_sixteen_unsigned(c);         \
      ci._italic_index=c>>8;            \
      ci._tag=c%4;              \
      fget; read_sixteen_unsigned(d);         \
      ci._remainder=d;              \
    } else {                                                            \
      a=tfm_buffer[++tfm_byte];           \
      ci._width_index=a;            \
      b=tfm_buffer[++tfm_byte];           \
      ci._height_index=b>>4;            \
      ci._depth_index=b%16;           \
      c=tfm_buffer[++tfm_byte];           \
      ci._italic_index=c>>2;            \
      ci._tag=c%4;              \
      d=tfm_buffer[++tfm_byte];           \
      ci._remainder=d;              \
    } }

#define read_four_quarters(q)           \
  { if (font_level!=-1) {                                               \
      fget; read_sixteen_unsigned(a); q.b0=a;       \
      fget; read_sixteen_unsigned(b); q.b1=b;       \
      fget; read_sixteen_unsigned(c); q.b2=c;       \
      fget; read_sixteen_unsigned(d); q.b3=d;       \
    } else {                                                            \
      a=tfm_buffer[++tfm_byte]; q.b0=a;         \
      b=tfm_buffer[++tfm_byte]; q.b1=b;         \
      c=tfm_buffer[++tfm_byte]; q.b2=c;         \
      d=tfm_buffer[++tfm_byte]; q.b3=d;                                \
    } }

#define check_byte_range(z)  { if ((z<bc)||(z>ec)) tfm_abort ; }


/* A |fix_word| whose four bytes are $(a,b,c,d)$ from left to right represents
   the number
   $$x=\left\{\vcenter{\halign{$#$,\hfil\qquad&if $#$\hfil\cr
   b\cdot2^{-4}+c\cdot2^{-12}+d\cdot2^{-20}&a=0;\cr
   -16+b\cdot2^{-4}+c\cdot2^{-12}+d\cdot2^{-20}&a=255.\cr}}\right.$$
   (No other choices of |a| are allowed, since the magnitude of a number in
   design-size units must be less than 16.)  We want to multiply this
   quantity by the integer~|z|, which is known to be less than $2^{27}$.
   If $|z|<2^{23}$, the individual multiplications $b\cdot z$,
   $c\cdot z$, $d\cdot z$ cannot overflow; otherwise we will divide |z| by 2,
   4, 8, or 16, to obtain a multiplier less than $2^{23}$, and we can
   compensate for this later. If |z| has thereby been replaced by
   $|z|^\prime=|z|/2^e$, let $\beta=2^{4-e}$; we shall compute
   $$\lfloor(b+c\cdot2^{-8}+d\cdot2^{-16})\,z^\prime/\beta\rfloor$$
   if $a=0$, or the same quantity minus $\alpha=2^{4+e}z^\prime$ if $a=255$.
   This calculation must be done exactly, in order to guarantee portability
   of \TeX\ between computers.
*/

#define store_scaled(zz)                                                   \
  { fget; a=fbyte; fget; b=fbyte;                                          \
    fget; c=fbyte; fget; d=fbyte;                                          \
    sw=(((((d*z)>>8)+(c*z))>>8)+(b*z)) / beta;                             \
    if (a==0) { zz=sw; } else if (a==255) { zz=sw-alpha; } else tfm_abort; \
  }

scaled store_scaled_f(scaled sq, scaled z_in)
{
    eight_bits a, b, c, d;
    scaled sw;
    static integer alpha, beta; /* beta:1..16 */
    static scaled z, z_prev = 0;
    /* @<Replace |z| by $|z|^\prime$ and compute $\alpha,\beta$@>; */
    if (z_in != z_prev || z_prev == 0) {
        z = z_prev = z_in;
        alpha = 16;
        while (z >= 0x800000) {
            z /= 2;
            alpha += alpha;
        }
        beta = 256 / alpha;
        alpha *= z;
    };
    if (sq >= 0) {
        d = sq % 256;           /* any "mod 256" not really needed, would typecast alone be safe? */
        sq = sq / 256;
        c = sq % 256;
        sq = sq / 256;
        b = sq % 256;
        sq = sq / 256;
        a = sq % 256;
    } else {
        sq = (sq + 1073741824) + 1073741824;    /* braces for optimizing compiler */
        d = sq % 256;
        sq = sq / 256;
        c = sq % 256;
        sq = sq / 256;
        b = sq % 256;
        sq = sq / 256;
        a = (sq + 128) % 256;
    }
    sw = (((((d * z) >> 8) + (c * z)) >> 8) + (b * z)) / beta;
    if (a == 0)
        return sw;
    else if (a == 255)
        return (sw - alpha);
    else
        pdf_error(maketexstring("vf"), maketexstring("vf scaling"));
}

#define  check_existence(z)                                             \
  { check_byte_range(z);                                                \
    if (!char_exists(f,z)) tfm_abort;         \
  }

typedef struct tfmcharacterinfo {
    integer _kern_index;
    integer _lig_index;
    integer _width_index;
    integer _height_index;
    integer _depth_index;
    integer _italic_index;
    integer _remainder;
    unsigned char _tag;
} tfmcharacterinfo;

int read_tfm_info(internalfontnumber f, char *cnom, scaled s)
{
    integer k;                  /* index into |font_info| */
    halfword lf, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np, slh;       /* sizes of subfiles */
    scaled *widths, *heights, *depths, *italics, *kerns;
    halfword font_dir;
    integer a, b, c, d;         /* byte variables */
    integer i;                  /* counter */
    integer font_level, header_length;
    integer nco, ncw, npc, nlw, neew;
    tfmcharacterinfo ci;
    charinfo *co;
    four_quarters qw;
    four_quarters *lig_kerns, *extens;
    scaled sw;                  /* accumulators */
    integer bch_label;          /* left boundary start location, or infinity */
    int bchar;                  /* :0..too_big_char; *//* right boundary character, or |too_big_char| */
    integer first_two;
    scaled z;                   /* the design size or the ``at'' size */
    integer alpha;
    char beta;                  /* :1..16 */
    integer *xligs, *xkerns;    /* aux. for ligkern processing */
    liginfo *cligs;
    kerninfo *ckerns;
    int fligs, fkerns;
    char *tmpnam;
    integer tfm_byte = 0;       /* index into |tfm_buffer| */
    integer saved_tfm_byte = 0; /* saved index into |tfm_buffer| */
    unsigned char *tfm_buffer = NULL;   /* byte buffer for tfm files */
    integer tfm_size = 0;       /* total size of the tfm file */

    widths = NULL;
    heights = NULL;
    depths = NULL;
    italics = NULL;
    kerns = NULL;
    lig_kerns = NULL;
    extens = NULL;
    xkerns = NULL;
    ckerns = NULL;
    xligs = NULL;
    cligs = NULL;

    font_dir = 0;

    memset(&ci, 0, sizeof(tfmcharacterinfo));

    if (open_tfm_file(cnom, &tfm_buffer, &tfm_size) != 1)
        tfm_abort;

    /* cnom can be an absolute filename, xbasename() fixes that. */

    tmpnam = strdup(xbasename(cnom));
    if (strcmp(tmpnam + strlen(tmpnam) - 4, ".tfm") == 0) {
        *(tmpnam + strlen(tmpnam) - 4) = 0;
    }
    set_font_name(f, tmpnam);
    set_font_area(f, NULL);

    /* @<Read the {\.{TFM}} size fields@>; */
    nco = 0;
    ncw = 0;
    npc = 0;
    read_sixteen(first_two);
    if (first_two != 0) {
        font_level = -1;
        lf = first_two;
        fget;
        read_sixteen(lh);
        fget;
        read_sixteen(bc);
        fget;
        read_sixteen(ec);
        if ((bc > ec + 1) || (ec > 255))
            tfm_abort;
        if (bc > 255) {         /* |bc=256| and |ec=255| */
            bc = 1;
            ec = 0;
        };
        fget;
        read_sixteen(nw);
        fget;
        read_sixteen(nh);
        fget;
        read_sixteen(nd);
        fget;
        read_sixteen(ni);
        fget;
        read_sixteen(nl);
        fget;
        read_sixteen(nk);
        fget;
        read_sixteen(ne);
        fget;
        read_sixteen(np);
        header_length = 6;
        ncw = (ec - bc + 1);
        nlw = nl;
        neew = ne;
    } else {
        fget;
        read_sixteen(font_level);
        if (font_level != 0)
            tfm_abort;
        read_thirtytwo(lf);
        read_thirtytwo(lh);
        read_thirtytwo(bc);
        read_thirtytwo(ec);
        if ((bc > ec + 1) || (ec > 65535))
            tfm_abort;
        if (bc > 65535) {       /* |bc=65536| and |ec=65535| */
            bc = 1;
            ec = 0;
        };
        read_thirtytwo(nw);
        read_thirtytwo(nh);
        read_thirtytwo(nd);
        read_thirtytwo(ni);
        read_thirtytwo(nl);
        read_thirtytwo(nk);
        read_thirtytwo(ne);
        read_thirtytwo(np);
        read_thirtytwo(font_dir);       /* junk */
        nlw = 2 * nl;
        neew = 2 * ne;
        header_length = 14;
        ncw = 2 * (ec - bc + 1);
    };
    if (lf !=
        (header_length + lh + ncw + nw + nh + nd + ni + nlw + nk + neew + np))
        tfm_abort;
    if ((nw == 0) || (nh == 0) || (nd == 0) || (ni == 0))
        tfm_abort;

    /* 
       We check to see that the \.{TFM} file doesn't end prematurely; but
       no error message is given for files having more than |lf| words.
     */
    if (lf * 4 > tfm_size)
        tfm_abort;

    /* @<Use size fields to allocate font information@>; */

    set_font_natural_dir(f, font_dir);
    set_font_bc(f, bc);
    set_font_ec(f, ec);

    /* read the arrays first */
    widths = xmalloc(nw * sizeof(scaled));
    heights = xmalloc(nh * sizeof(scaled));
    depths = xmalloc(nd * sizeof(scaled));
    italics = xmalloc(ni * sizeof(scaled));
    extens = xmalloc(ne * sizeof(four_quarters));
    lig_kerns = xmalloc(nl * sizeof(four_quarters));
    kerns = xmalloc(nk * sizeof(scaled));

    /* @<Read the {\.{TFM}} header@>; */

    /* Only the first two words of the header are needed by \TeX82. */
    slh = lh;
    if (lh < 2)
        tfm_abort;
    store_four_bytes(font_checksum(f));
    fget;
    read_sixteen(z);            /* this rejects a negative design size */
    fget;
    z = z * 256 + fbyte;
    fget;
    z = (z * 16) + (fbyte >> 4);
    if (z < unity)
        tfm_abort;
    while (lh > 2) {
        fget;
        fget;
        fget;
        fget;
        lh--;                   /* ignore the rest of the header */
    };

    /* read the arrays before the character info */

    set_font_dsize(f, z);
    if (s != -1000) {
        z = (s >= 0 ? s : xn_over_d(z, -s, 1000));
    }
    set_font_size(f, z);

    if (np > 7)
        set_font_params(f, np);

    saved_tfm_byte = tfm_byte;
    tfm_byte = (header_length + slh + ncw) * 4 - 1;

    /* @<Replace |z| by $|z|^\prime$ and compute $\alpha,\beta$@>; */

    alpha = 16;
    while (z >= 040000000) {
        z = z >> 1;
        alpha = alpha + alpha;
    };
    beta = 256 / alpha;
    alpha = alpha * z;

    /* @<Read box dimensions@>; */

    for (k = 0; k < nw; k++) {
        store_scaled(sw);
        widths[k] = sw;
    }
    if (widths[0] != 0)         /* \\{width}[0] must be zero */
        tfm_abort;
    for (k = 0; k < nh; k++) {
        store_scaled(sw);
        heights[k] = sw;
    }
    if (heights[0] != 0)
        tfm_abort;              /* \\{height}[0] must be zero */
    for (k = 0; k < nd; k++) {
        store_scaled(sw);
        depths[k] = sw;
    }
    if (depths[0] != 0)
        tfm_abort;              /* \\{depth}[0] must be zero */
    for (k = 0; k < ni; k++) {
        store_scaled(sw);
        italics[k] = sw;
    }
    if (italics[0] != 0)
        tfm_abort;              /* \\{italic}[0] must be zero */


    /* @<Read ligature/kern program@>; */

    bch_label = nl;             /* infinity */
    bchar = 65536;
    if (nl > 0) {
        for (k = 0; k < nl; k++) {
            read_four_quarters(qw);
            lig_kerns[k] = qw;
            if (a > 128) {
                if (256 * c + d >= nl)
                    tfm_abort;
                if (a == 255 && k == 0)
                    bchar = b;
            } else {
                /* if (b!=bchar) check_existence(b); */
                if (c < 128) {
                    /* check_existence(d); *//* check ligature */
                } else if (256 * (c - 128) + d >= nk) {
                    tfm_abort;  /* check kern */
                }
                if ((a < 128) && (k - 0 + a + 1 >= nl))
                    tfm_abort;
            };
        };
        if (a == 255)
            bch_label = 256 * c + d;
    };

    /* the actual kerns */
    for (k = 0; k < nk; k++) {
        store_scaled(sw);
        kerns[k] = sw;
    }

    /* @<Read extensible character recipes@>; */
    for (k = 0; k < ne; k++) {
        read_four_quarters(qw);
        extens[k] = qw;
    }

    /* @<Read font parameters@>; */

    if (np > 7) {
        set_font_params(f, np);
    }
    for (k = 1; k <= np; k++) {
        if (k == 1) {           /* the |slant| parameter is a pure number */
            fget;
            sw = fbyte;
            if (sw > 127)
                sw = sw - 256;
            fget;
            sw = sw * 256 + fbyte;
            fget;
            sw = sw * 256 + fbyte;
            fget;
            sw = (sw * 16) + (fbyte >> 4);
            set_font_param(f, k, sw);
        } else {
            store_scaled(font_param(f, k));
        }
    }

    tfm_byte = saved_tfm_byte;

    /* fix up the left boundary character */
    fligs = 0;
    fkerns = 0;
    if (bch_label != nl) {
        k = bch_label;
        /*
           if (skip_byte(k) > stop_flag)
           k = lig_kern_restart(k);
         */
        while (1) {
            if (skip_byte(k) <= stop_flag) {
                if (op_byte(k) >= kern_flag) {  /* kern */
                    fkerns++;
                } else {        /* lig */
                    fligs++;
                }
            }
            if (skip_byte(k) == 0) {
                k++;
            } else {
                if (skip_byte(k) >= stop_flag)
                    break;
                k += skip_byte(k) + 1;
            }
        }
    }
    if (fkerns > 0 || fligs > 0) {
        if (fligs > 0)
            cligs = xcalloc((fligs + 1), sizeof(liginfo));
        if (fkerns > 0)
            ckerns = xcalloc((fkerns + 1), sizeof(kerninfo));
        fligs = 0;
        fkerns = 0;
        k = bch_label;
        /*
           if (skip_byte(k) > stop_flag)
           k = lig_kern_restart(k);
         */
        while (1) {
            if (skip_byte(k) <= stop_flag) {
                if (op_byte(k) >= kern_flag) {  /* kern */
                    set_kern_item(ckerns[fkerns], next_char(k),
                                  kerns[256 * (op_byte(k) - 128) +
                                        rem_byte(k)]);
                    fkerns++;
                } else {        /* lig */
                    set_ligature_item(cligs[fligs], (op_byte(k) * 2 + 1),
                                      next_char(k), rem_byte(k));
                    fligs++;
                }
            }
            if (skip_byte(k) == 0) {
                k++;
            } else {
                if (skip_byte(k) >= stop_flag)
                    break;
                k += skip_byte(k) + 1;
            }
        }
        if (fkerns > 0 || fligs > 0) {
            co = get_charinfo(f, left_boundarychar);
            if (fkerns > 0) {
                set_kern_item(ckerns[fkerns], end_kern, 0);
                fkerns++;
                set_charinfo_kerns(co, ckerns);
            }
            if (fligs > 0) {
                set_ligature_item(cligs[fligs], 0, end_ligature, 0);
                fligs++;
                set_charinfo_ligatures(co, cligs);
            }
            set_charinfo_remainder(co, 0);
        }
    }

    /* @<Read character data@>; */
    for (k = bc; k <= ec; k++) {
        store_char_info(k);
        if (ci._width_index == 0)
            continue;
        if (ci._width_index >= nw || ci._height_index >= nh ||
            ci._depth_index >= nd || ci._italic_index >= ni)
            tfm_abort;
        d = ci._remainder;
        switch (ci._tag) {
        case lig_tag:
            if (d >= nl)
                tfm_abort;
            break;
        case ext_tag:
            if (d >= ne)
                tfm_abort;
            break;
        case list_tag:
            /* We want to make sure that there is no cycle of characters linked together
               by |list_tag| entries, since such a cycle would get \TeX\ into an endless
               loop. If such a cycle exists, the routine here detects it when processing
               the largest character code in the cycle.
             */
            check_byte_range(d);
            while (d < k) {     /* current_character == k */
                if (char_tag(f, d) != list_tag)
                    goto NOT_FOUND;     /* not a cycle */
                d = char_remainder(f, d);       /* next character on the list */
            };
            if (d == k)
                tfm_abort;      /* yes, there's a cycle */
          NOT_FOUND:
            break;
        }
        /* put it in the actual font */
        co = get_charinfo(f, k);
        set_charinfo_index(co, k);
        set_charinfo_tag(co, ci._tag);
        if (ci._tag == ext_tag) {
            set_charinfo_extensible(co, extens[ci._remainder].b0,       /* top */
                                    extens[ci._remainder].b2,   /* bot */
                                    extens[ci._remainder].b1,   /* mid */
                                    extens[ci._remainder].b3);  /* rep */
            set_charinfo_remainder(co, 0);
        } else {
            set_charinfo_remainder(co, ci._remainder);
        }
        set_charinfo_width(co, widths[ci._width_index]);
        set_charinfo_height(co, heights[ci._height_index]);
        set_charinfo_depth(co, depths[ci._depth_index]);
        set_charinfo_italic(co, italics[ci._italic_index]);
    };

    /* first pass: count ligs and kerns */

    xligs = xcalloc((ec + 1), sizeof(integer));
    xkerns = xcalloc((ec + 1), sizeof(integer));

    for (i = bc; i <= ec; i++) {
        if (char_tag(f, i) == lig_tag) {
            k = lig_kern_start(f, i);
            if (skip_byte(k) > stop_flag)
                k = lig_kern_restart(k);
            /* now k is the start index */
            while (1) {
                if (skip_byte(k) <= stop_flag) {
                    if (op_byte(k) >= kern_flag) {      /* kern */
                        xkerns[i]++;
                        if (next_char(k) == bchar)
                            xkerns[i]++;
                    } else {    /* lig */
                        xligs[i]++;
                        if (next_char(k) == bchar)
                            xligs[i]++;
                    }
                }
                if (skip_byte(k) == 0) {
                    k++;
                } else {
                    if (skip_byte(k) >= stop_flag)
                        break;
                    k += skip_byte(k) + 1;
                }
            }
        }
    }

    cligs = NULL;
    ckerns = NULL;

    for (i = bc; i <= ec; i++) {
        fligs = 0;
        fkerns = 0;
        if (char_tag(f, i) == lig_tag) {
            k = lig_kern_start(f, i);
            if (skip_byte(k) > stop_flag)
                k = lig_kern_restart(k);
            /* now k is the start index */
            if (xligs[i] > 0)
                cligs = xcalloc((xligs[i] + 1), sizeof(liginfo));
            if (xkerns[i] > 0)
                ckerns = xcalloc((xkerns[i] + 1), sizeof(kerninfo));
            while (1) {
                if (skip_byte(k) <= stop_flag) {
                    if (op_byte(k) >= kern_flag) {      /* kern */
                        if (next_char(k) == bchar) {
                            set_kern_item(ckerns[fkerns], right_boundarychar,
                                          kerns[256 * (op_byte(k) - 128) +
                                                rem_byte(k)]);
                            fkerns++;
                        }
                        set_kern_item(ckerns[fkerns], next_char(k),
                                      kerns[256 * (op_byte(k) - 128) +
                                            rem_byte(k)]);
                        fkerns++;
                    } else {    /* lig */
                        if (next_char(k) == bchar) {
                            set_ligature_item(cligs[fligs],
                                              (op_byte(k) * 2 + 1),
                                              right_boundarychar, rem_byte(k));
                            fligs++;
                        }
                        set_ligature_item(cligs[fligs], (op_byte(k) * 2 + 1),
                                          next_char(k), rem_byte(k));
                        fligs++;
                    }
                }
                if (skip_byte(k) == 0) {
                    k++;
                } else {
                    if (skip_byte(k) >= stop_flag)
                        break;
                    k += skip_byte(k) + 1;
                }
            }
            if (fkerns > 0 || fligs > 0) {
                co = get_charinfo(f, i);
                if (fkerns > 0) {
                    set_kern_item(ckerns[fkerns], end_kern, 0);
                    fkerns++;
                    set_charinfo_kerns(co, ckerns);
                }
                if (fligs > 0) {
                    set_ligature_item(cligs[fligs], 0, end_ligature, 0);
                    fligs++;
                    set_charinfo_ligatures(co, cligs);
                }
                set_charinfo_remainder(co, 0);
            }
        }
    }


    /* @<Make final adjustments and |goto done|@> */

    /* Now to wrap it up, we have checked all the necessary things about the \.{TFM}
       file, and all we need to do is put the finishing touches on the data for
       the new font.
     */

    if (bchar != 65536) {
        co = copy_charinfo(char_info(f, bchar));
        set_right_boundary(f, co);
    }

    tfm_success;
}
