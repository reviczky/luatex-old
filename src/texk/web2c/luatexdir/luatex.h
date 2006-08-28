/*
Copyright (c) 1996-2006 Han The Thanh, <thanh@pdftex.org>

This file is part of pdfTeX.

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

$Id $
*/

/* some code array functions */

extern void     setmathcode (integer n, halfword v, quarterword grouplevel);
extern halfword getmathcode (integer n);

extern void     setdelcode (integer n, halfword v,  halfword w, quarterword grouplevel);
extern halfword getdelcodea (integer n);
extern halfword getdelcodeb (integer n);

extern void unsavemathcodes (quarterword grouplevel);
extern void initializemathcodes ();
extern void dumpmathcodes ();
extern void undumpmathcodes ();

extern void     setlccode  (integer n, halfword v, quarterword grouplevel);
extern halfword getlccode  (integer n);
extern void     setuccode  (integer n, halfword v, quarterword grouplevel);
extern halfword getuccode  (integer n);
extern void     setsfcode  (integer n, halfword v, quarterword grouplevel);
extern halfword getsfcode  (integer n);
extern void     setcatcode (integer n, halfword v, quarterword grouplevel);
extern halfword getcatcode (integer n);

extern void unsavetextcodes (quarterword grouplevel);
extern void initializetextcodes ();
extern void dumptextcodes ();
extern void undumptextcodes ();


/* this is needed for delcodes */ 

extern memoryword **fonttables;
extern void allocatefonttable();
extern void dumpfonttable();
extern void undumpfonttable();

extern int **ocptables;
extern int ocptemp;

extern void allocateocptable();
extern void dumpocptable();
extern void undumpocptable();

extern void runexternalocp();
extern void btestin();

/* Additions to texmfmp.h for pdfTeX */

/* mark a char in font */
#define pdfmarkchar(f, c) pdfcharused[f][c/16] |= (1<<(c%16))

/* test whether a char in font is marked */
#define pdfcharmarked(f, c) (boolean)(pdfcharused[f][c/16] & (1<<(c%16)))

/* writepdf() always writes by fwrite() */
#define       writepdf(a, b) \
  (void) fwrite ((char *) &pdfbuf[a], sizeof (pdfbuf[a]), \
                 (int) ((b) - (a) + 1), pdffile)

#define getlpcode(f, c) \
    (pdffontlpbase[f] == 0 ? 0 : pdfmem[pdffontlpbase[f] + c])

#define getrpcode(f, c) \
    (pdffontrpbase[f] == 0 ? 0 : pdfmem[pdffontrpbase[f] + c])

#define getefcode(f, c) \
    (pdffontefbase[f] == 0 ? 1000 : pdfmem[pdffontefbase[f] + c])

#define getknbscode(f, c) \
    (pdffontknbsbase[f] == 0 ? 0 : pdfmem[pdffontknbsbase[f] + c])

#define getstbscode(f, c) \
    (pdffontstbsbase[f] == 0 ? 0 : pdfmem[pdffontstbsbase[f] + c])

#define getshbscode(f, c) \
    (pdffontshbsbase[f] == 0 ? 0 : pdfmem[pdffontshbsbase[f] + c])

#define getknbccode(f, c) \
    (pdffontknbcbase[f] == 0 ? 0 : pdfmem[pdffontknbcbase[f] + c])

#define getknaccode(f, c) \
    (pdffontknacbase[f] == 0 ? 0 : pdfmem[pdffontknacbase[f] + c])

#define texbopenin(f) \
    open_input (&(f), kpse_tex_format, FOPEN_RBIN_MODE)
#define vfbopenin(f) \
    open_input (&(f), kpse_vf_format, FOPEN_RBIN_MODE)

#include <luatexdir/ptexlib.h>
