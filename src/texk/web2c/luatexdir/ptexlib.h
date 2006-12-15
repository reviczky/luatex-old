/*
Copyright (c) 1996-2006 Han The Thanh, <thanh@pdftex.org>

This file is part of luaTeX.

pdfTeX is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pdfTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pdfTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id: //depot/Build/source.development/TeX/texk/web2c/pdftexdir/ptexlib.h#26 $
*/

#ifndef LUATEXLIB
#  define LUATEXLIB

/* WEB2C macros and prototypes */
#  if !defined(LUATEXCOERCE) && !defined(PDFETEXCOERCE)
#    ifdef luaTeX
#      undef luaTeX             /* to avoid warning about redefining pdfTeX in pdftexd.h */
#    endif                      /* pdfTeX */
#    define EXTERN extern
#    include "luatexd.h"
#  endif

/* pdftexlib macros */
#  include "ptexmac.h"

/* avl */
#  include "avlstuff.h"

#  include "openbsd-compat.h"

/* pdftexlib type declarations */
typedef struct {
    const char *pdfname;
    const char *t1name;
    float value;
    boolean valid;
} key_entry;

typedef struct {
    boolean loaded;             /* the encoding has been loaded? */
    char *name;                 /* encoding file name */
    integer objnum;             /* object number */
    char **glyph_names;
    integer tounicode;          /* object number of associated ToUnicode entry */
} enc_entry;

struct _subfont_entry;
typedef struct _subfont_entry subfont_entry;

struct _subfont_entry {
    char *infix;                /* infix for this subfont, eg "01" */
    long charcodes[256];        /* the mapping for this subfont as read from sfd */
    subfont_entry *next;
};

typedef struct {
    char *name;                 /* sfd name, eg "Unicode" */
    subfont_entry *subfont;     /* linked list of subfonts */
} sfd_entry;

typedef struct {
    char *name;                 /* glyph name */
    long code;                  /* -1 = undefined; -2 = multiple codes, stored
                                   as string in unicode_seq; otherwise unicode value */
    char *unicode_seq;          /* multiple unicode sequence */
} glyph_unicode_entry;

typedef struct {
    char *tfm_name;             /* TFM file name */
    char *ps_name;              /* PostScript name */
    integer flags;              /* font flags */
    char *ff_name;              /* font file name */
    char *subset_tag;           /* pseudoUniqueTag for subsetted font */
    enc_entry *encoding;        /* pointer to corresponding encoding */
    internalfontnumber tfm_num; /* number of the TFM refering this entry */
    unsigned short type;        /* font type (T1/TTF/...) */
    short slant;                /* SlantFont */
    short extend;               /* ExtendFont */
    integer ff_objnum;          /* FontFile object number */
    integer fn_objnum;          /* FontName/BaseName object number */
    integer fd_objnum;          /* FontDescriptor object number */
    char *charset;              /* string containing used glyphs */
    boolean all_glyphs;         /* embed all glyphs? */
    unsigned short links;       /* link flags from tfm_tree and ps_tree */
    short tfm_avail;            /* flags whether a tfm is available */
    short pid;                  /* Pid for truetype fonts */
    short eid;                  /* Eid for truetype fonts */
    subfont_entry *subfont;     /* subfont mapping */
} fm_entry;

typedef struct {
    char *ff_name;              /* base name of font file */
    char *ff_path;              /* full path to font file */
} ff_entry;

typedef short shalfword;
typedef struct {
    integer charcode, cwidth, cheight, xoff, yoff, xescape, rastersize;
    halfword *raster;
} chardesc;

/* pdftexlib variable declarations */
extern boolean fontfile_found;
extern boolean is_otf_font;
extern boolean true_dimen;
extern boolean write_ttf_glyph_names;
extern char **t1_glyph_names, *t1_builtin_glyph_names[];
extern char *cur_file_name;
extern const char notdef[];
extern fm_entry *fm_cur;
extern integer t1_length1, t1_length2, t1_length3;
extern integer ttf_length;
extern internalfontnumber tex_font;
extern key_entry font_keys[];
extern strnumber last_tex_string;
extern size_t last_ptr_index;
extern char fontname_buf[];

/* pdftexlib function prototypes */

/* epdf.c */
extern integer get_fontfile_num (int);
extern integer get_fontname_num (int);
extern void epdf_free (void);
extern int is_type1 (fm_entry *);

/* mapfile.c */
extern char *mk_basename (char *);
extern char *mk_exname (char *, int);
extern fm_entry *lookup_fontmap (char *);
extern boolean hasfmentry (internalfontnumber);
extern internalfontnumber tfmoffm (fmentryptr);
extern void checkextfm (strnumber, integer);
extern void fm_free (void);
extern void fm_read_info (void);
extern ff_entry *check_ff_exist (fm_entry *);
extern void pdfmapfile (integer);
extern void pdfmapline (integer);
extern void pdfinitmapfile (string map_name);
extern fm_entry *new_fm_entry (void);
extern void delete_fm_entry (fm_entry *);
extern int avl_do_entry (fm_entry *, int);

/* papersiz.c */
extern integer myatodim (char **);
extern integer myatol (char **);

/* pkin.c */
extern int readchar (boolean, chardesc *);

/* subfont.c */
extern void sfd_free (void);
extern boolean handle_subfont_fm (fm_entry *, int);

/* tounicode.c */
extern void glyph_unicode_free (void);
extern void deftounicode (strnumber, strnumber);
extern integer write_tounicode (char **, char *);

/* utils.c */
extern boolean str_eq_cstr (strnumber, char *);
extern char *makecstring (integer);
extern char *makeclstring (integer, int *);
extern int xfflush (FILE *);
extern int xgetc (FILE *);
extern int xputc (int, FILE *);
extern scaled extxnoverd (scaled, scaled, scaled);
extern size_t xfwrite (void *, size_t size, size_t nmemb, FILE *);
extern strnumber getresnameprefix (void);
extern strnumber maketexstring (const char *);
extern strnumber maketexlstring (const char *, size_t);
extern integer fb_offset (void);
extern void fb_flush (void);
extern void fb_putchar (eightbits b);
extern void fb_seek (integer);
extern void libpdffinish (void);
extern char *makecfilename (strnumber s);
extern void make_subset_tag (fm_entry *, char **);
__attribute__ ((format (printf, 1, 2))) 
extern void pdf_printf (const char *, ...);
extern void pdf_puts (const char *);
__attribute__ ((noreturn, format (printf, 1, 2)))
extern void pdftex_fail (const char *, ...);
__attribute__ ((format (printf, 1, 2)))
extern void pdftex_warn (const char *, ...);
extern void setjobid (int, int, int, int);
__attribute__ ((format (printf, 1, 2)))
extern void tex_printf (const char *, ...);
extern void writestreamlength (integer, integer);
extern char *convertStringToPDFString (const char *in, int len);
extern void printID (strnumber);
extern void printcreationdate ();
extern void printmoddate ();
extern void escapename (poolpointer in);
extern void escapestring (poolpointer in);
extern void escapehex (poolpointer in);
extern void unescapehex (poolpointer in);
extern void getcreationdate ();
extern void getfilemoddate (strnumber s);
extern void getfilesize (strnumber s);
extern void getmd5sum (strnumber s, boolean file);
extern void getfiledump (strnumber s, int offset, int length);
extern void matchstrings (strnumber s, strnumber t, int subcount,
                          boolean icase);
extern void getmatch (int i);
extern void makepdftexbanner (void);
extern void initstarttime ();
extern void removepdffile (void);
extern void garbagewarning (void);
extern void stripspaces (char *p);

/* vfpacket.c */
extern realeightbits packetbyte (void);
extern integer newvfpacket (internalfontnumber);
extern void poppacketstate (void);
extern void pushpacketstate (void);
extern void startpacket (internalfontnumber, integer);
extern void storepacket (integer, integer, integer);
extern void vf_free (void);

/* writeenc.c */
extern boolean indexed_enc (fm_entry *);
extern enc_entry *add_enc (char *);
extern void enc_free (void);
extern void read_enc (enc_entry *);
extern void write_enc (char **, enc_entry *, integer);

/* writefont.c */
extern void dopdffont (integer, internalfontnumber);

/* writeimg.c */
extern boolean checkimageb (integer);
extern boolean checkimagec (integer);
extern boolean checkimagei (integer);
extern boolean ispdfimage (integer);
extern integer epdforigx (integer);
extern integer epdforigy (integer);
extern integer imageheight (integer);
extern integer imagepages (integer);
extern integer imagewidth (integer);
extern integer imagexres (integer);
extern integer imageyres (integer);
extern integer readimage (strnumber, integer, strnumber, integer, integer,
                          integer, integer);
extern void deleteimage (integer);
extern void img_free (void);
extern void updateimageprocset (integer);
extern void writeimage (integer);
extern integer imagecolordepth (integer img);

/* writejbig2.c */
extern void flushjbig2page0objects ();

/* writet1.c */
extern boolean t1_subset (char *, char *, unsigned char *);
extern void load_enc (char *, char **);
extern void writet1 (void);
extern void t1_free (void);

/* writet3.c */
extern void writet3 (int, internalfontnumber);
extern scaled getpkcharwidth (internalfontnumber, scaled);

/* writettf.c */
extern void writettf (void);
extern void writeotf (void);
extern void ttf_free (void);

/* writezip.c */
extern void writezip (boolean);

/* avlstuff.c */
extern void avlputobj (integer, integer);
extern integer avlfindobj (integer, integer, integer);

#include "font/texfont.h"

#endif                          /* LUATEXLIB */
