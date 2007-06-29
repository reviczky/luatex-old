/* Copyright (C) 2000-2007 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pfaeditui.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI

#ifndef _CursorsMustBe16x16
# define _CursorsMustBe16x16	0	/* X on Mac OSX can't handle anything bigger than 16x16 cursors (I think, it seems to manage smaller ones) */
#endif

#ifdef FONTFORGE_CONFIG_NO_WINDOWING_UI
 /* this file is pointless */
#elif defined(FONTFORGE_CONFIG_GTK)
GdkCursor *ct_magplus, *ct_magminus, *ct_mypointer,
	*ct_circle, *ct_square, *ct_triangle, *ct_pen,
	*ct_ruler, *ct_knife, *ct_rotate, *ct_skew, *ct_scale, *ct_flip,
	*ct_3drotate, *ct_perspective,
	*ct_updown, *ct_leftright, *ct_nesw, *ct_nwse,
	*ct_rect, *ct_elipse, *ct_poly, *ct_star, *ct_filledrect, *ct_filledelipse,
	*ct_pencil, *ct_shift, *ct_line, *ct_myhand, *ct_setwidth,
	*ct_kerning, *ct_rbearing, *ct_lbearing, *ct_eyedropper,
	*ct_prohibition, *ct_ddcursor;
#else
GCursor ct_magplus, ct_magminus, ct_mypointer, ct_circle, ct_square, ct_triangle,
	ct_ruler, ct_pen, ct_knife, ct_rotate, ct_skew, ct_scale, ct_flip,
	ct_3drotate, ct_perspective;
GCursor ct_rect, ct_elipse, ct_poly, ct_star, ct_pencil, ct_shift, ct_line,
	ct_myhand, ct_filledrect, ct_filledelipse, ct_setwidth, ct_eyedropper;
GCursor ct_updown, ct_leftright, ct_nesw, ct_nwse;
GCursor ct_rbearing, ct_kerning, ct_lbearing;
GCursor ct_prohibition, ct_ddcursor;
GWindow logo_icon;
#endif

#define magplus_width 16
#define magplus_height 16
#define magplus_x_hot 5
#define magplus_y_hot 5
static unsigned char magplus_bits[] = {
   0xf8, 0x00, 0x06, 0x03, 0x22, 0x02, 0x21, 0x04, 0x21, 0x04, 0xfd, 0x05,
   0x21, 0x04, 0x21, 0x04, 0x22, 0x02, 0x06, 0x03, 0xf8, 0x04, 0x00, 0x08,
   0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};
#define magminus_width 16
#define magminus_height 16
#define magminus_x_hot 5
#define magminus_y_hot 5
static unsigned char magminus_bits[] = {
   0xf8, 0x00, 0x06, 0x03, 0x02, 0x02, 0x01, 0x04, 0x01, 0x04, 0xfd, 0x05,
   0x01, 0x04, 0x01, 0x04, 0x02, 0x02, 0x06, 0x03, 0xf8, 0x04, 0x00, 0x08,
   0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};
#if 0
#define magmask_width 16
#define magmask_height 16
static unsigned char magmask_bits[] = {
   0xf8, 0x00, 0xfe, 0x03, 0x26, 0x03, 0x23, 0x06, 0x23, 0x06, 0xff, 0x07,
   0x23, 0x06, 0x23, 0x06, 0x26, 0x03, 0xfe, 0x07, 0xf8, 0x0e, 0x00, 0x1c,
   0x00, 0x38, 0x00, 0x70, 0x00, 0xe0, 0x00, 0xc0};
#endif
#if _CursorsMustBe16x16
#define pointertri_width 16
#define pointertri_height 16
#define pointertri_x_hot 1
#define pointertri_y_hot 0
static unsigned char pointertri_bits[] = {
   0x02, 0x00, 0x06, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x3e, 0x00, 0x7e, 0x00,
   0xfe, 0x00, 0xfe, 0x01, 0x3e, 0x00, 0x36, 0x10, 0x62, 0x30, 0x60, 0x70,
   0xc0, 0xf0, 0xc0, 0x70, 0x80, 0x31, 0x80, 0x11};
#define pointertrimask_width 16
#define pointertrimask_height 16
static unsigned char pointertrimask_bits[] = {
   0x02, 0x00, 0x06, 0x00, 0x1f, 0x00, 0x3f, 0x00, 0x7f, 0x00, 0xff, 0x00,
   0xff, 0x01, 0xff, 0x03, 0xff, 0x03, 0x3f, 0x10, 0x67, 0x30, 0x63, 0x70,
   0xc0, 0xf0, 0xc0, 0x70, 0x80, 0x31, 0x80, 0x11};
#define pointersqr_width 16
#define pointersqr_height 16
#define pointersqr_x_hot 2
#define pointersqr_y_hot 0
static unsigned char pointersqr_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x00,
   0x80, 0xf1, 0x80, 0xf1, 0x00, 0xf3, 0x00, 0xf3};
#define pointersqrmask_width 16
#define pointersqrmask_height 16
static unsigned char pointersqrmask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0x7e, 0x00, 0xde, 0x00, 0xc6, 0x00,
   0x80, 0xf1, 0x80, 0xf1, 0x00, 0xf3, 0x00, 0xf3};
#define pointercic_width 16
#define pointercic_height 16
#define pointercic_x_hot 2
#define pointercic_y_hot 0
static unsigned char pointercic_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x70,
   0x80, 0xf9, 0x80, 0xf9, 0x00, 0xfb, 0x00, 0x73};
#define pointercicmask_width 16
#define pointercicmask_height 16
static unsigned char pointercicmask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0x7e, 0x00, 0xce, 0x00, 0xc6, 0x70,
   0x80, 0xf9, 0x80, 0xf9, 0x00, 0xfb, 0x00, 0x73};
#else
#define pointertri_width 16
#define pointertri_height 24
#define pointertri_x_hot 2
#define pointertri_y_hot 0
static unsigned char pointertri_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x00,
   0x80, 0x01, 0x80, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x08, 0x00, 0x1c,
   0x00, 0x3e, 0x00, 0x7f, 0x80, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define pointertrimask_width 16
#define pointertrimask_height 24
static unsigned char pointertrimask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0xfe, 0x00, 0xee, 0x01, 0xe6, 0x01,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x07, 0x80, 0x07, 0x00, 0x0b, 0x00, 0x1c,
   0x00, 0x3e, 0x00, 0x7f, 0x80, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define pointersqr_width 16
#define pointersqr_height 24
#define pointersqr_x_hot 2
#define pointersqr_y_hot 0
static unsigned char pointersqr_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x00,
   0x80, 0x01, 0x80, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x7c,
   0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00};
#define pointersqrmask_width 16
#define pointersqrmask_height 24
static unsigned char pointersqrmask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0xfe, 0x00, 0xe6, 0x01, 0xe0, 0x01,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x07, 0x80, 0x07, 0x00, 0x03, 0x00, 0x7c,
   0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00};
#define pointercic_width 16
#define pointercic_height 24
#define pointercic_x_hot 2
#define pointercic_y_hot 0
static unsigned char pointercic_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x00,
   0x80, 0x01, 0x80, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x38, 0x00, 0x7c,
   0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x00};
#define pointercicmask_width 16
#define pointercicmask_height 24
static unsigned char pointercicmask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3e, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0xfe, 0x00, 0xee, 0x01, 0xe6, 0x01,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x07, 0x80, 0x07, 0x00, 0x3b, 0x00, 0x7c,
   0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x00};
#endif
#define pointercur_width 16
#define pointercur_height 16
#define pointercur_x_hot 2
#define pointercur_y_hot 0
static unsigned char pointercur_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xfc, 0x03, 0x7c, 0x00, 0x6c, 0x00, 0xc4, 0x00, 0xc0, 0x00,
   0x80, 0x01, 0x80, 0x01, 0x00, 0x03, 0x00, 0x03};
#define pointercurmask_width 16
#define pointercurmask_height 16
static unsigned char pointercurmask_bits[] = {
   0x04, 0x00, 0x0c, 0x00, 0x3c, 0x00, 0x7e, 0x00, 0xfe, 0x00, 0xfe, 0x01,
   0xfe, 0x03, 0xfe, 0x07, 0xfe, 0x07, 0xfe, 0x00, 0xee, 0x01, 0xe6, 0x01,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x07, 0x80, 0x07};
#if _CursorsMustBe16x16
#define rulercur_width 16
#define rulercur_height 16
#define rulercur_x_hot 0
#define rulercur_y_hot 3
static unsigned char rulercur_bits[] = {
   0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x3f, 0x40, 0x22, 0x40, 0x24, 0x40,
   0x20, 0x40, 0x20, 0x49, 0x20, 0x49, 0x20, 0x49, 0xe0, 0x7f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else
#define rulercur_width 24
#define rulercur_height 12
#define rulercur_x_hot 0
#define rulercur_y_hot 3
static unsigned char rulercur_bits[] = {
   0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x80,
   0x22, 0x00, 0x80, 0x24, 0x40, 0x80, 0x20, 0x40, 0x80, 0x20, 0x49, 0x92,
   0x20, 0x49, 0x92, 0x20, 0x49, 0x92, 0xe0, 0xff, 0xff, 0x00, 0x00, 0x00};
#endif
#define pencur_width 16
#define pencur_height 16
#define pencur_x_hot 6
#define pencur_y_hot 0
static unsigned char pencur_bits[] = {
   0x40, 0x00, 0xe0, 0x00, 0xe0, 0x00, 0x50, 0x01, 0x50, 0x01, 0x50, 0x01,
   0x48, 0x02, 0x48, 0x02, 0x44, 0x04, 0xa4, 0x04, 0x44, 0x04, 0x08, 0x02,
   0xf0, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01};
#define pencurmask_width 16
#define pencurmask_height 16
static unsigned char pencurmask_bits[] = {
   0x40, 0x00, 0xe0, 0x00, 0xf0, 0x01, 0x58, 0x03, 0x58, 0x03, 0x58, 0x03,
   0x4c, 0x06, 0x4c, 0x06, 0x46, 0x0c, 0xe6, 0x0c, 0x46, 0x0c, 0x0c, 0x06,
   0xf8, 0x03, 0x18, 0x03, 0x18, 0x03, 0x18, 0x03};
#define knifecur_width 16
#define knifecur_height 16
#define knifecur_x_hot 7
#define knifecur_y_hot 4
static unsigned char knifecur_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x38, 0x20, 0x40, 0x21, 0x00, 0xee,
   0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define knifecurmask_width 16
#define knifecurmask_height 16
static unsigned char knifecurmask_bits[] = {
   0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xfe, 0xf8, 0xff, 0x00, 0xfe,
   0x00, 0x7e, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define flipcur_width 16
#define flipcur_height 16
#define flipcur_x_hot 8
#define flipcur_y_hot 8
static unsigned char flipcur_bits[] = {
   0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x20, 0x09,
   0x20, 0x09, 0x30, 0x11, 0x30, 0x09, 0x28, 0x21, 0x28, 0x09, 0x24, 0x41,
   0x3c, 0x69, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};
#define flipcurmask_width 16
#define flipcurmask_height 16
static unsigned char flipcurmask_bits[] = {
   0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x20, 0x09,
   0x20, 0x09, 0x30, 0x19, 0x30, 0x19, 0x28, 0x29, 0x28, 0x29, 0x24, 0x49,
   0x3c, 0x79, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};
#define rotatecur_width 16
#define rotatecur_height 16
#define rotatecur_x_hot 3
#define rotatecur_y_hot 12
static unsigned char rotatecur_bits[] = {
   0x08, 0x01, 0x88, 0x07, 0x08, 0x09, 0x08, 0x10, 0x08, 0x20, 0x08, 0x20,
   0x08, 0x80, 0x08, 0x60, 0x08, 0x18, 0x08, 0x06, 0x88, 0x01, 0x68, 0x00,
   0xff, 0xff, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00};
#define scalecur_width 16
#define scalecur_height 16
#define scalecur_x_hot 0
#define scalecur_y_hot 15
static unsigned char scalecur_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x05, 0x00, 0x00, 0x01, 0x04,
   0x00, 0x00, 0x7f, 0x04, 0x41, 0x00, 0x41, 0x04, 0x41, 0x00, 0x41, 0x04,
   0x41, 0x00, 0x41, 0x04, 0x41, 0x00, 0x7f, 0x05};
#define scalecurmask_width 16
#define scalecurmask_height 16
static unsigned char scalecurmask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x01, 0x04, 0x01, 0x04,
   0x01, 0x04, 0x7f, 0x04, 0x41, 0x04, 0x41, 0x04, 0x41, 0x04, 0x41, 0x04,
   0x41, 0x04, 0x41, 0x04, 0x41, 0x04, 0xff, 0x07};
#define skewcur_width 16
#define skewcur_height 16
#define skewcur_x_hot 0
#define skewcur_y_hot 15
static unsigned char skewcur_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x7f, 0x05, 0x41, 0x00, 0x49, 0x02, 0x41, 0x00, 0x45, 0x01,
   0x41, 0x00, 0xc3, 0x00, 0x41, 0x00, 0x7f, 0x00};
#define skewcurmask_width 16
#define skewcurmask_height 16
static unsigned char skewcurmask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xff, 0x07, 0x49, 0x02, 0x49, 0x02, 0x45, 0x01, 0x45, 0x01,
   0xc3, 0x00, 0xc3, 0x00, 0x41, 0x00, 0x7f, 0x00};
#define rotate3dcur_width 16
#define rotate3dcur_height 16
#define rotate3dcur_x_hot 1
#define rotate3dcur_y_hot 13
static unsigned char rotate3dcur_bits[] = {
   0x02, 0x00, 0xfa, 0x00, 0x8a, 0x00, 0x8a, 0x00, 0x8a, 0x00, 0x8a, 0x00,
   0x8a, 0x00, 0xfa, 0x00, 0x02, 0x1c, 0xaa, 0x22, 0x02, 0xaa, 0xaa, 0x72,
   0x02, 0x22, 0xff, 0x8f, 0x02, 0x02, 0x02, 0x3c};
#define perspectivecur_width 16
#define perspectivecur_height 16
#define perspectivecur_x_hot 0
#define perspectivecur_y_hot 15
static unsigned char perspectivecur_bits[] = {
   0x41, 0x04, 0x41, 0x04, 0x21, 0x08, 0x21, 0x08, 0x11, 0x10, 0xf1, 0x1f,
   0x31, 0x19, 0x29, 0x28, 0xa9, 0x2a, 0x25, 0x48, 0x65, 0x4c, 0x25, 0x48,
   0xe3, 0x8f, 0x03, 0x80, 0x01, 0x00, 0xff, 0xff};
#define setwidthcur_width 16
#define setwidthcur_height 16
#define setwidthcur_x_hot 14
#define setwidthcur_y_hot 7
static unsigned char setwidthcur_bits[] = {
   0x00, 0x00, 0x01, 0x40, 0x00, 0x40, 0x01, 0x40, 0x00, 0x40, 0x09, 0x48,
   0x04, 0x50, 0xab, 0x6a, 0x04, 0x50, 0x09, 0x48, 0x00, 0x40, 0x01, 0x40,
   0x00, 0x40, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00};
/*static unsigned char setwidthmask_bits[] = {				     */
/*   0x00, 0x00, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x01, 0x40, 0x09, 0x48, */
/*   0x05, 0x50, 0xff, 0x7f, 0x05, 0x50, 0x09, 0x48, 0x01, 0x40, 0x01, 0x40, */
/*   0x01, 0x40, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00};			     */

#define updown_width 8
#define updown_height 15
#define updown_x_hot 3
#define updown_y_hot 7
static unsigned char updown_bits[] = {
   0x08, 0x14, 0x36, 0x55, 0x14, 0x14, 0x14, 0x7f, 0x14, 0x14, 0x14, 0x55,
   0x36, 0x14, 0x08};
#define leftright_width 16
#define leftright_height 9
#define leftright_x_hot 7
#define leftright_y_hot 4
static unsigned char leftright_bits[] = {
   0x80, 0x00, 0x88, 0x08, 0x84, 0x10, 0xfe, 0x3f, 0x81, 0x40, 0xfe, 0x3f,
   0x84, 0x10, 0x88, 0x08, 0x80, 0x00};
#define nesw_width 16
#define nesw_height 15
#define nesw_x_hot 8
#define nesw_y_hot 7
static unsigned char nesw_bits[] = {
   0x00, 0xfc, 0x00, 0x90, 0x00, 0x88, 0x10, 0xc4, 0x20, 0xa2, 0x40, 0x91,
   0x80, 0x08, 0x40, 0x05, 0x20, 0x02, 0x12, 0x05, 0x8a, 0x08, 0x46, 0x10,
   0x22, 0x00, 0x12, 0x00, 0x7e, 0x00};
#define nwse_width 16
#define nwse_height 15
#define nwse_x_hot 8
#define nwse_y_hot 7
static unsigned char nwse_bits[] = {
   0x7e, 0x00, 0x12, 0x00, 0x22, 0x00, 0x46, 0x10, 0x8a, 0x08, 0x12, 0x05,
   0x20, 0x02, 0x40, 0x05, 0x80, 0x08, 0x40, 0x91, 0x20, 0xa2, 0x10, 0xc4,
   0x00, 0x88, 0x00, 0x90, 0x00, 0xfc};
#define rectcur_width 16
#define rectcur_height 8
#define rectcur_x_hot 0
#define rectcur_y_hot 2
static unsigned char rectcur_bits[] = {
   0x04, 0x00, 0x02, 0x00, 0xff, 0x7f, 0x12, 0x40, 0x14, 0x40, 0x10, 0x40,
   0xf0, 0x7f, 0x00, 0x00};
#define elipsecur_width 16
#define elipsecur_height 8
#define elipsecur_x_hot 0
#define elipsecur_y_hot 2
static unsigned char elipsecur_bits[] = {
   0x04, 0x00, 0x02, 0x00, 0x3f, 0x1f, 0xc2, 0x60, 0x24, 0x80, 0xc0, 0x60,
   0x00, 0x1f, 0x00, 0x00};
#define polycur_width 16
#define polycur_height 16
#define polycur_x_hot 1
#define polycur_y_hot 7
static unsigned char polycur_bits[] = {
   0x80, 0x01, 0x60, 0x06, 0x18, 0x18, 0x06, 0x60, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x06, 0x60,
   0x18, 0x18, 0x60, 0x06, 0x80, 0x01, 0x00, 0x00};
#define starcur_width 16
#define starcur_height 16
#define starcur_x_hot 0
#define starcur_y_hot 6
static unsigned char starcur_bits[] = {
   0x10, 0x08, 0x70, 0x0e, 0x90, 0x09, 0x10, 0x08, 0x08, 0x10, 0x06, 0x60,
   0x01, 0x80, 0x06, 0x60, 0x08, 0x10, 0x10, 0x08, 0x90, 0x09, 0x70, 0x0e,
   0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define pencil_width 16
#define pencil_height 16
#define pencil_x_hot 1
#define pencil_y_hot 1
static unsigned char pencil_bits[] = {
   0x00, 0x00, 0x0e, 0x00, 0x16, 0x00, 0x22, 0x00, 0x44, 0x00, 0x88, 0x00,
   0x10, 0x01, 0x20, 0x02, 0x40, 0x04, 0x80, 0x08, 0x00, 0x11, 0x00, 0x22,
   0x00, 0x64, 0x00, 0x98, 0x00, 0x50, 0x00, 0x20};
#define pencilmask_width 16
#define pencilmask_height 16
static unsigned char pencilmask_bits[] = {
   0x00, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x3e, 0x00, 0x7c, 0x00, 0xf8, 0x00,
   0xf0, 0x01, 0xe0, 0x03, 0xc0, 0x07, 0x80, 0x0f, 0x00, 0x1f, 0x00, 0x3e,
   0x00, 0x7c, 0x00, 0x98, 0x00, 0x50, 0x00, 0x20};
#define eyedropper_width 16
#define eyedropper_height 16
#define eyedropper_x_hot 0
#define eyedropper_y_hot 0
static unsigned char eyedropper_bits[] = {
   0x02, 0x00, 0x0d, 0x00, 0x12, 0x00, 0x22, 0x00, 0x44, 0x00, 0x88, 0x00,
   0x10, 0x09, 0x20, 0x0e, 0x40, 0x0e, 0x80, 0x1f, 0x80, 0x7f, 0xc0, 0xff,
   0x00, 0xfe, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0x78};
#define eyedroppermask_width 16
#define eyedroppermask_height 16
static unsigned char eyedroppermask_bits[] = {
   0x03, 0x00, 0x0f, 0x00, 0x1e, 0x00, 0x3e, 0x00, 0x7c, 0x00, 0xf8, 0x00,
   0xf0, 0x09, 0xe0, 0x0f, 0xc0, 0x0f, 0x80, 0x1f, 0x80, 0x7f, 0xc0, 0xff,
   0x00, 0xfe, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0x78};
#define shift_width 16
#define shift_height 16
#define shift_x_hot 7
#define shift_y_hot 7
static unsigned char shift_bits[] = {
   0x80, 0x00, 0xc0, 0x01, 0xa0, 0x02, 0x80, 0x00, 0x80, 0x00, 0x84, 0x10,
   0x82, 0x20, 0xff, 0x7f, 0x82, 0x20, 0x84, 0x10, 0x80, 0x00, 0x80, 0x00,
   0xa0, 0x02, 0xc0, 0x01, 0x80, 0x00, 0x00, 0x00};
#define linecur_width 16
#define linecur_height 16
#define linecur_x_hot 0
#define linecur_y_hot 0
static unsigned char linecur_bits[] = {
   0xc1, 0xff, 0x62, 0xa0, 0xc4, 0xff, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00,
   0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08,
   0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};
#if _CursorsMustBe16x16
#define hand_width 16
#define hand_height 16
#define hand_x_hot 8
#define hand_y_hot 12
static unsigned char hand_bits[] = {
   0x80, 0x01, 0x40, 0x02, 0x48, 0x0e, 0x54, 0x12, 0x54, 0x52, 0x64, 0xb2,
   0x68, 0xb2, 0x48, 0x92, 0x17, 0x80, 0x19, 0x40, 0x11, 0x40, 0x02, 0x40,
   0x04, 0x20, 0x08, 0x20, 0x08, 0x20, 0x10, 0x10};
#define handmask_width 16
#define handmask_height 16
static unsigned char handmask_bits[] = {
   0x80, 0x01, 0xc0, 0x03, 0xc8, 0x0f, 0xdc, 0x1f, 0xdc, 0x5f, 0xfc, 0xff,
   0xf8, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x7f,
   0xfc, 0x3f, 0xf8, 0x3f, 0xf8, 0x3f, 0xf0, 0x1f};
#else
#define hand_width 24
#define hand_height 24
#define hand_x_hot 11
#define hand_y_hot 15
static unsigned char hand_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00,
   0x00, 0x12, 0x00, 0x40, 0x72, 0x00, 0xa0, 0x92, 0x00, 0xa0, 0x92, 0x02,
   0x20, 0x93, 0x05, 0x40, 0x93, 0x05, 0x40, 0x92, 0x04, 0xb8, 0x00, 0x04,
   0xc8, 0x00, 0x02, 0x88, 0x00, 0x02, 0x10, 0x00, 0x02, 0x20, 0x00, 0x01,
   0x40, 0x00, 0x01, 0x40, 0x00, 0x01, 0x80, 0x80, 0x00, 0x00, 0x81, 0x00,
   0x00, 0x82, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define handmask_width 24
#define handmask_height 24
static unsigned char handmask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00,
   0x00, 0x1e, 0x00, 0x40, 0x7e, 0x00, 0xe0, 0xfe, 0x00, 0xe0, 0xfe, 0x02,
   0xe0, 0xff, 0x07, 0xc0, 0xff, 0x07, 0xc0, 0xff, 0x07, 0xb8, 0xff, 0x07,
   0xf8, 0xff, 0x03, 0xf8, 0xff, 0x03, 0xf0, 0xff, 0x03, 0xe0, 0xff, 0x01,
   0xc0, 0xff, 0x01, 0xc0, 0xff, 0x01, 0x80, 0xff, 0x00, 0x00, 0xff, 0x00,
   0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif
#define filledrectcur_width 16
#define filledrectcur_height 8
#define filledrectcur_x_hot 0
#define filledrectcur_y_hot 2
static unsigned char filledrectcur_bits[] = {
   0x04, 0x00, 0x02, 0x00, 0xff, 0xff, 0xe2, 0xff, 0xe4, 0xff, 0xe0, 0xff,
   0xe0, 0xff, 0x00, 0x00};
#define filledelipsecur_width 16
#define filledelipsecur_height 8
#define filledelipsecur_x_hot 0
#define filledelipsecur_y_hot 2
static unsigned char filledelipsecur_bits[] = {
   0x04, 0x00, 0x02, 0x00, 0x3f, 0x1f, 0xc2, 0x7f, 0xe4, 0xff, 0xc0, 0x7f,
   0x00, 0x1f, 0x00, 0x00};
#define kerncur_width 16
#define kerncur_height 15
#define kerncur_x_hot 7
#define kerncur_y_hot 7
static unsigned char kerncur_bits[] = {
   0x80, 0x00, 0x80, 0x00, 0xf8, 0x0f, 0x08, 0x08, 0x28, 0x0a, 0x28, 0x09,
   0xaa, 0x28, 0x6f, 0x78, 0xaa, 0x28, 0x28, 0x09, 0x28, 0x0a, 0x08, 0x08,
   0xf8, 0x0f, 0x80, 0x00, 0x80, 0x00};
#define rbearcur_width 16
#define rbearcur_height 15
#define rbearcur_x_hot 7
#define rbearcur_y_hot 7
static unsigned char rbearcur_bits[] = {
   0x80, 0x00, 0x80, 0x00, 0xf8, 0x0f, 0x08, 0x08, 0xe8, 0x09, 0x28, 0x0a,
   0x2a, 0x2a, 0xef, 0x79, 0x2a, 0x29, 0x28, 0x09, 0x28, 0x0a, 0x08, 0x08,
   0xf8, 0x0f, 0x80, 0x00, 0x80, 0x00};
#define rbearmask_width 16
#define rbearmask_height 15
static unsigned char rbearmask_bits[] = {
   0x80, 0x00, 0xfc, 0x1f, 0xfc, 0x1f, 0xfc, 0x1f, 0xfc, 0x1f, 0xfe, 0x3f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xfc, 0x1f,
   0xfc, 0x1f, 0xfc, 0x1f, 0x80, 0x00};
#define lbearcur_width 16
#define lbearcur_height 15
#define lbearcur_x_hot 7
#define lbearcur_y_hot 7
static unsigned char lbearcur_bits[] = {
   0x80, 0x00, 0x80, 0x00, 0xf0, 0x07, 0x10, 0x04, 0x50, 0x04, 0x54, 0x14,
   0x52, 0x24, 0x5f, 0x7c, 0x52, 0x24, 0x54, 0x14, 0xd0, 0x05, 0x10, 0x04,
   0xf0, 0x07, 0x80, 0x00, 0x80, 0x00};
#define lbearmask_width 16
#define lbearmask_height 15
static unsigned char lbearmask_bits[] = {
   0x80, 0x00, 0xf8, 0x0f, 0xf8, 0x0f, 0xf8, 0x0f, 0xfc, 0x1f, 0xfe, 0x3f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f,
   0xf8, 0x0f, 0xf8, 0x0f, 0x80, 0x00};
   
#define logo_width 32
#define logo_height 32
static unsigned char logo_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00,
   0xf0, 0x07, 0x00, 0x00, 0xf0, 0x07, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00,
   0xe0, 0x0f, 0x00, 0x00, 0xc0, 0x1f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00,
   0xfc, 0x3f, 0x00, 0x00, 0x9f, 0x3f, 0x00, 0x00, 0x07, 0x0f, 0x38, 0x00,
   0x00, 0xc3, 0xff, 0x01, 0x00, 0x60, 0xf0, 0x1f, 0x00, 0xbc, 0xed, 0x7f,
   0x80, 0x07, 0xf8, 0x7f, 0xe0, 0x6f, 0xff, 0x7f, 0xf8, 0xb7, 0xff, 0x1f,
   0xfe, 0x01, 0xfe, 0x05, 0xff, 0xff, 0xff, 0x00, 0xf8, 0xff, 0x23, 0x00,
   0x80, 0x71, 0x7c, 0x00, 0x00, 0x8e, 0xff, 0x01, 0x00, 0xff, 0xff, 0x03,
   0xc0, 0xdf, 0xff, 0x03, 0xc0, 0xdf, 0xff, 0x03, 0x80, 0xef, 0xff, 0x00,
   0x00, 0xef, 0x3f, 0x00, 0x00, 0xec, 0x03, 0x00, 0x00, 0x30, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define prohibition_width 16
#define prohibition_height 16
#define prohibition_x_hot 7
#define prohibition_y_hot 7
static unsigned char prohibition_bits[] = {
   0xc0, 0x01, 0x30, 0x06, 0x0c, 0x18, 0x0c, 0x10, 0x12, 0x20, 0x22, 0x20,
   0x41, 0x40, 0x81, 0x40, 0x01, 0x41, 0x02, 0x22, 0x02, 0x24, 0x04, 0x18,
   0x0c, 0x18, 0x30, 0x06, 0xc0, 0x01, 0x00, 0x00};
static unsigned char prohibitionmask_bits[] = {
   0xc0, 0x01, 0xf0, 0x07, 0xfc, 0x1f, 0xfc, 0x1f, 0xfe, 0x3f, 0xfe, 0x3f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfc, 0x1f,
   0xfc, 0x1f, 0xf0, 0x07, 0xc0, 0x01, 0x00, 0x00};
#define ddcursor_width 16
#define ddcursor_height 16
#define ddcursor_x_hot 0
#define ddcursor_y_hot 0
static unsigned char ddcursor_bits[] = {
   0x0f, 0x00, 0x03, 0x00, 0x05, 0x00, 0x09, 0x00, 0x10, 0x00, 0x00, 0x00,
   0x00, 0x02, 0x00, 0x02, 0x60, 0xc6, 0x50, 0x2a, 0x50, 0x2a, 0xa0, 0xc6,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


void InitCursors(void) {
#ifdef FONTFORGE_CONFIG_NO_WINDOWING_UI
 /* this file is pointless */
#elif defined(FONTFORGE_CONFIG_GTK)
    GdkPixmap *image, *mask;
    static GdkColor white = { 0xffffffff, 0xffff, 0xffff, 0xffff },
		    black = { 0xffffffff, 0	, 0	, 0	 },
		    red   = { 0xffffffff, 0xffff, 0	, 0	 },

    image = gdk_pixmap_create_from_data(NULL,magplus_bits,magplus_width,magplus_height,
	    24, &black, &white);
    ct_magplus = gdk_cursor_new_from_pixmap( image,image,&back,&white,magplus_x_hot,
	    magplus_y_hot);
    image = gdk_pixmap_create_from_data(NULL,magminus_bits,magminus_width,magminus_height,
	    24, &black, &white);
    ct_magminus = gdk_cursor_new_from_pixmap( image,image,&back,&white,magminus_x_hot,
	    magminus_y_hot);

    image = gdk_pixmap_create_from_data(NULL,pointercur_bits,pointercur_width,pointercur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pointercurmask_bits,pointercurmask_width,pointercurmask_height,
	    24, &black, &white);
    ct_mypointer = gdk_cursor_new_from_pixmap( image,mask,&back,&white,pointercur_x_hot,
	    pointercur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,pointercirc_bits,pointercirc_width,pointercirc_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pointercircmask_bits,pointercircmask_width,pointercircmask_height,
	    24, &black, &white);
    ct_circle = gdk_cursor_new_from_pixmap( image,mask,&back,&white,pointercirc_x_hot,
	    pointercirc_y_hot);
    image = gdk_pixmap_create_from_data(NULL,pointertri_bits,pointertri_width,pointertri_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pointertrimask_bits,pointertrimask_width,pointertrimask_height,
	    24, &black, &white);
    ct_triangle = gdk_cursor_new_from_pixmap( image,mask,&back,&white,pointertri_x_hot,
	    pointertri_y_hot);
    image = gdk_pixmap_create_from_data(NULL,pointersqr_bits,pointersqr_width,pointersqr_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pointersqrmask_bits,pointersqrmask_width,pointersqrmask_height,
	    24, &black, &white);
    ct_square = gdk_cursor_new_from_pixmap( image,mask,&back,&white,pointersqr_x_hot,
	    pointersqr_y_hot);
    image = gdk_pixmap_create_from_data(NULL,pencur_bits,pencur_width,pencur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pencurmask_bits,pencurmask_width,pencurmask_height,
	    24, &black, &white);
    ct_pen = gdk_cursor_new_from_pixmap( image,mask,&back,&white,pencur_x_hot,
	    pencur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,setwidthcur_bits,setwidthcur_width,setwidthcur_height,
	    24, &black, &white);
/*    mask = gdk_pixmap_create_from_data(NULL,setwidthcurmask_bits,setwidthcurmask_width,setwidthcurmask_height,
	    24, &black, &white);*/
    ct_setwidth = gdk_cursor_new_from_pixmap( image,image,&back,&white,setwidthcur_x_hot,
	    setwidthcur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,rulercur_bits,rulercur_width,rulercur_height,
	    24, &black, &white);
    ct_ruler = gdk_cursor_new_from_pixmap( image,image,&back,&white,rulercur_x_hot,
	    rulercur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,knifecur_bits,knifecur_width,knifecur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,knifecurmask_bits,knifecurmask_width,knifecurmask_height,
	    24, &black, &white);
    ct_knife = gdk_cursor_new_from_pixmap( image,mask,&back,&white,knifecur_x_hot,
	    knifecur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,flipcur_bits,flipcur_width,flipcur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,flipcurmask_bits,flipcurmask_width,flipcurmask_height,
	    24, &black, &white);
    ct_flip = gdk_cursor_new_from_pixmap( image,mask,&red,&white,flipcur_x_hot,
	    flipcur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,rotatecur_bits,rotatecur_width,rotatecur_height,
	    24, &black, &white);
    ct_rotate = gdk_cursor_new_from_pixmap( image,image,&red,&white,rotatecur_x_hot,
	    rotatecur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,scalecur_bits,scalecur_width,scalecur_height,
	    24, &black, &white);
    ct_scale = gdk_cursor_new_from_pixmap( image,image,&red,&white,scalecur_x_hot,
	    scalecur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,skewcur_bits,skewcur_width,skewcur_height,
	    24, &black, &white);
    ct_skew = gdk_cursor_new_from_pixmap( image,image,&red,&white,skewcur_x_hot,
	    skewcur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,rotate3dcur_bits,rotate3dcur_width,rotate3dcur_height,
	    24, &black, &white);
    ct_3drotate = gdk_cursor_new_from_pixmap( image,image,&red,&white,rotate3dcur_x_hot,
	    rotate3dcur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,perspectivecur_bits,perspectivecur_width,perspectivecur_height,
	    24, &black, &white);
    ct_perspective = gdk_cursor_new_from_pixmap( image,image,&red,&white,perspectivecur_x_hot,
	    perspectivecur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,rectcur_bits,rectcur_width,rectcur_height,
	    24, &black, &white);
    ct_rect = gdk_cursor_new_from_pixmap( image,image,&red,&white,rectcur_x_hot,
	    rectcur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,elipsecur_bits,elipsecur_width,elipsecur_height,
	    24, &black, &white);
    ct_elipse = gdk_cursor_new_from_pixmap( image,image,&red,&white,elipsecur_x_hot,
	    elipsecur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,polycur_bits,polycur_width,polycur_height,
	    24, &black, &white);
    ct_poly = gdk_cursor_new_from_pixmap( image,image,&red,&white,polycur_x_hot,
	    polycur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,starcur_bits,starcur_width,starcur_height,
	    24, &black, &white);
    ct_star = gdk_cursor_new_from_pixmap( image,image,&red,&white,starcur_x_hot,
	    starcur_y_hot);


    image = gdk_pixmap_create_from_data(NULL,nwse_bits,nwse_width,nwse_height,
	    24, &black, &white);
    ct_nwse = gdk_cursor_new_from_pixmap( image,image,&red,&white,nwse_x_hot,
	    nwse_y_hot);
    image = gdk_pixmap_create_from_data(NULL,nesw_bits,nesw_width,nesw_height,
	    24, &black, &white);
    ct_nesw = gdk_cursor_new_from_pixmap( image,image,&red,&white,nesw_x_hot,
	    nesw_y_hot);
    image = gdk_pixmap_create_from_data(NULL,leftright_bits,leftright_width,leftright_height,
	    24, &black, &white);
    ct_leftright = gdk_cursor_new_from_pixmap( image,image,&red,&white,leftright_x_hot,
	    leftright_y_hot);
    image = gdk_pixmap_create_from_data(NULL,updown_bits,updown_width,updown_height,
	    24, &black, &white);
    ct_updown = gdk_cursor_new_from_pixmap( image,image,&red,&white,updown_x_hot,
	    updown_y_hot);

    image = gdk_pixmap_create_from_data(NULL,pencil_bits,pencil_width,pencil_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,pencilmask_bits,pencil_width,pencil_height,
	    24, &black, &white);
    ct_pencil = gdk_cursor_new_from_pixmap( image,mask,&red,&white,pencil_x_hot,
	    pencil_y_hot);
    image = gdk_pixmap_create_from_data(NULL,eyedropper_bits,eyedropper_width,eyedropper_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,eyedroppermask_bits,eyedropper_width,eyedropper_height,
	    24, &black, &white);
    ct_eyedropper = gdk_cursor_new_from_pixmap( image,mask,&red,&white,eyedropper_x_hot,
	    eyedropper_y_hot);
    image = gdk_pixmap_create_from_data(NULL,shift_bits,shift_width,shift_height,
	    24, &black, &white);
    ct_shift = gdk_cursor_new_from_pixmap( image,image,&red,&white,shift_x_hot,
	    shift_y_hot);
    image = gdk_pixmap_create_from_data(NULL,linecur_bits,linecur_width,linecur_height,
	    24, &black, &white);
    ct_linecur = gdk_cursor_new_from_pixmap( image,image,&red,&white,linecur_x_hot,
	    linecur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,hand_bits,hand_width,hand_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,handmask_bits,hand_width,hand_height,
	    24, &black, &white);
    ct_myhand = gdk_cursor_new_from_pixmap( image,mask,&red,&white,hand_x_hot,
	    hand_y_hot);
    image = gdk_pixmap_create_from_data(NULL,filledrectcur_bits,filledrectcur_width,filledrectcur_height,
	    24, &black, &white);
    ct_filledrect = gdk_cursor_new_from_pixmap( image,image,&red,&white,filledrectcur_x_hot,
	    filledrectcur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,filledelipsecur_bits,filledelipsecur_width,filledelipsecur_height,
	    24, &black, &white);
    ct_filledelipse = gdk_cursor_new_from_pixmap( image,image,&red,&white,filledelipsecur_x_hot,
	    filledelipsecur_y_hot);

    image = gdk_pixmap_create_from_data(NULL,kerncur_bits,kerncur_width,kerncur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,rbearmask_bits,kerncur_width,kerncur_height,
	    24, &black, &white);
    ct_kerning = gdk_cursor_new_from_pixmap( image,mask,&red,&white,kerncur_x_hot,
	    kerncur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,rbearcur_bits,rbearcur_width,rbearcur_height,
	    24, &black, &white);
    ct_rbearing = gdk_cursor_new_from_pixmap( image,mask,&red,&white,rbearcur_x_hot,
	    rbearcur_y_hot);
    image = gdk_pixmap_create_from_data(NULL,lbearcur_bits,lbearcur_width,lbearcur_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,lbearmask_bits,lbearcur_width,lbearcur_height,
	    24, &black, &white);
    ct_lbearing = gdk_cursor_new_from_pixmap( image,mask,&red,&white,lbearcur_x_hot,
	    lbearcur_y_hot);
/*
    logo_icon = GDrawCreateBitmap(NULL,logo_width,logo_height,logo_bits);
    GDrawSetDefaultIcon(logo_icon);
*/

    image = gdk_pixmap_create_from_data(NULL,prohibition_bits,prohibition_width,prohibition_height,
	    24, &black, &white);
    mask = gdk_pixmap_create_from_data(NULL,prohibitionmask_bits,prohibition_width,prohibition_height,
	    24, &black, &white);
    ct_prohibition = gdk_cursor_new_from_pixmap( image,mask,&red,&white,prohibition_x_hot,
	    prohibition_y_hot);
    image = gdk_pixmap_create_from_data(NULL,ddcursor_bits,ddcursor_width,ddcursor_height,
	    24, &black, &white);
    ct_ddcursor = gdk_cursor_new_from_pixmap( image,image,&red,&white,ddcursor_x_hot,
	    ddcursor_y_hot);
#else
    GWindow mask, image;
    /* The XServer shipping with redhat 7.1 seems to suffer a protocol change */
    /*  with the red and blue members of XColor structure reversed */
    /* So all of my cursors which should be red (to match the points drawn */
    /*  on the screen) come out blue. If I run the same image on another server*/
    /*  it's red. Oh well. */

    image = GDrawCreateBitmap(NULL,magplus_width,magplus_height,magplus_bits);
    ct_magplus = GDrawCreateCursor(image,image,0x000000,0xffffff,magplus_x_hot,
	    magplus_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,magminus_width,magminus_height,magminus_bits);
    ct_magminus = GDrawCreateCursor(image,image,0x000000,0xffffff,magminus_x_hot,
	    magminus_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,pointercur_width,pointercur_height,pointercur_bits);
    mask = GDrawCreateBitmap(NULL,pointercurmask_width,pointercurmask_height,pointercurmask_bits);
    ct_mypointer = GDrawCreateCursor(image,mask,0x000000,0xffffff,pointercur_x_hot,
	    pointercur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);

    image = GDrawCreateBitmap(NULL,pointercic_width,pointercic_height,pointercic_bits);
    mask = GDrawCreateBitmap(NULL,pointercic_width,pointercic_height,pointercicmask_bits);
    ct_circle = GDrawCreateCursor(image,mask,0xff0000,0xffffff,pointercic_x_hot,
	    pointercic_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,pointertri_width,pointertri_height,pointertri_bits);
    mask = GDrawCreateBitmap(NULL,pointertri_width,pointertri_height,pointertrimask_bits);
    ct_triangle = GDrawCreateCursor(image,mask,0xff0000,0xffffff,pointertri_x_hot,
	    pointertri_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,pointersqr_width,pointersqr_height,pointersqr_bits);
    mask = GDrawCreateBitmap(NULL,pointersqr_width,pointersqr_height,pointersqrmask_bits);
    ct_square = GDrawCreateCursor(image,mask,0xff0000,0xffffff,pointersqr_x_hot,
	    pointersqr_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,pencur_width,pencur_height,pencur_bits);
    mask = GDrawCreateBitmap(NULL,pencur_width,pencur_height,pencurmask_bits);
    ct_pen = GDrawCreateCursor(image,mask,0xff0000,0xffffff,pencur_x_hot,
	    pencur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,setwidthcur_width,setwidthcur_height,setwidthcur_bits);
    /*mask = GDrawCreateBitmap(NULL,setwidthcur_width,setwidthcur_height,setwidthmask_bits);*/
    ct_setwidth = GDrawCreateCursor(image,image,0xff0000,0xffffff,setwidthcur_x_hot,
	    setwidthcur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,rulercur_width,rulercur_height,rulercur_bits);
    ct_ruler = GDrawCreateCursor(image,image,0x000000,0xffffff,rulercur_x_hot,
	    rulercur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,knifecur_width,knifecur_height,knifecur_bits);
    mask = GDrawCreateBitmap(NULL,knifecur_width,knifecur_height,knifecurmask_bits);
    ct_knife = GDrawCreateCursor(image,mask,0xff0000,0xffffff,knifecur_x_hot,
	    knifecur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);

    image = GDrawCreateBitmap(NULL,flipcur_width,flipcur_height,flipcur_bits);
    mask = GDrawCreateBitmap(NULL,flipcur_width,flipcur_height,flipcurmask_bits);
    ct_flip = GDrawCreateCursor(image,image,0xff0000,0xffffff,flipcur_x_hot,
	    flipcur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);

    image = GDrawCreateBitmap(NULL,rotatecur_width,rotatecur_height,rotatecur_bits);
    ct_rotate = GDrawCreateCursor(image,image,0x000000,0xffffff,rotatecur_x_hot,
	    rotatecur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,scalecur_width,scalecur_height,scalecur_bits);
    mask = GDrawCreateBitmap(NULL,scalecur_width,scalecur_height,scalecurmask_bits);
    ct_scale = GDrawCreateCursor(image,image,0xff0000,0xffffff,scalecur_x_hot,
	    scalecur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);

    image = GDrawCreateBitmap(NULL,skewcur_width,skewcur_height,skewcur_bits);
    mask = GDrawCreateBitmap(NULL,skewcur_width,skewcur_height,skewcurmask_bits);
    ct_skew = GDrawCreateCursor(image,image,0xff0000,0xffffff,skewcur_x_hot,
	    skewcur_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);

    image = GDrawCreateBitmap(NULL,rotate3dcur_width,rotate3dcur_height,rotate3dcur_bits);
    ct_3drotate = GDrawCreateCursor(image,image,0xff0000,0xffffff,rotate3dcur_x_hot,
	    rotate3dcur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,perspectivecur_width,perspectivecur_height,perspectivecur_bits);
    ct_perspective = GDrawCreateCursor(image,image,0xff0000,0xffffff,perspectivecur_x_hot,
	    perspectivecur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,rectcur_width,rectcur_height,rectcur_bits);
    ct_rect = GDrawCreateCursor(image,image,0xff0000,0xffffff,rectcur_x_hot,
	    rectcur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,elipsecur_width,elipsecur_height,elipsecur_bits);
    ct_elipse = GDrawCreateCursor(image,image,0xff0000,0xffffff,elipsecur_x_hot,
	    elipsecur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,polycur_width,polycur_height,polycur_bits);
    ct_poly = GDrawCreateCursor(image,image,0xff0000,0xffffff,polycur_x_hot,
	    polycur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,starcur_width,starcur_height,starcur_bits);
    ct_star = GDrawCreateCursor(image,image,0xff0000,0xffffff,starcur_x_hot,
	    starcur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,nwse_width,nwse_height,nwse_bits);
    ct_nwse = GDrawCreateCursor(image,image,0xff0000,0xffffff,nwse_x_hot,
	    nwse_y_hot);
    GDrawDestroyWindow(image); 
    image = GDrawCreateBitmap(NULL,nesw_width,nesw_height,nesw_bits);
    ct_nesw = GDrawCreateCursor(image,image,0xff0000,0xffffff,nesw_x_hot,
	    nesw_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,leftright_width,leftright_height,leftright_bits);
    ct_leftright = GDrawCreateCursor(image,image,0xff0000,0xffffff,leftright_x_hot,
	    leftright_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,updown_width,updown_height,updown_bits);
    ct_updown = GDrawCreateCursor(image,image,0xff0000,0xffffff,updown_x_hot,
	    updown_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,pencil_width,pencil_height,pencil_bits);
    mask = GDrawCreateBitmap(NULL,pencil_width,pencil_height,pencilmask_bits);
    ct_pencil = GDrawCreateCursor(image,mask,0xff0000,0xffffff,pencil_x_hot,
	    pencil_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,eyedropper_width,eyedropper_height,eyedropper_bits);
    mask = GDrawCreateBitmap(NULL,eyedropper_width,eyedropper_height,eyedroppermask_bits);
    ct_eyedropper = GDrawCreateCursor(image,mask,0xff0000,0xffffff,eyedropper_x_hot,
	    eyedropper_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,shift_width,shift_height,shift_bits);
    ct_shift = GDrawCreateCursor(image,image,0xff0000,0xffffff,shift_x_hot,
	    shift_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,linecur_width,linecur_height,linecur_bits);
    ct_line = GDrawCreateCursor(image,image,0xff0000,0xffffff,linecur_x_hot,
	    linecur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,hand_width,hand_height,hand_bits);
    mask = GDrawCreateBitmap(NULL,hand_width,hand_height,handmask_bits);
    ct_myhand = GDrawCreateCursor(image,mask,0xff0000,0xffffff,hand_x_hot,
	    hand_y_hot);
    GDrawDestroyWindow(image); GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,filledrectcur_width,filledrectcur_height,filledrectcur_bits);
    ct_filledrect = GDrawCreateCursor(image,image,0xff0000,0xffffff,filledrectcur_x_hot,
	    filledrectcur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,filledelipsecur_width,filledelipsecur_height,filledelipsecur_bits);
    ct_filledelipse = GDrawCreateCursor(image,image,0xff0000,0xffffff,filledelipsecur_x_hot,
	    filledelipsecur_y_hot);
    GDrawDestroyWindow(image);

    image = GDrawCreateBitmap(NULL,kerncur_width,kerncur_height,kerncur_bits);
    mask = GDrawCreateBitmap(NULL,rbearmask_width,rbearmask_height,rbearmask_bits);
    ct_kerning = GDrawCreateCursor(image,mask,0xff0000,0xffffff,kerncur_x_hot,
	    kerncur_y_hot);
    GDrawDestroyWindow(image);
    image = GDrawCreateBitmap(NULL,rbearcur_width,rbearcur_height,rbearcur_bits);
    ct_rbearing = GDrawCreateCursor(image,mask,0xff0000,0xffffff,rbearcur_x_hot,
	    rbearcur_y_hot);
    GDrawDestroyWindow(image);
    GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,lbearcur_width,lbearcur_height,lbearcur_bits);
    mask = GDrawCreateBitmap(NULL,lbearmask_width,lbearmask_height,lbearmask_bits);
    ct_lbearing = GDrawCreateCursor(image,mask,0xff0000,0xffffff,lbearcur_x_hot,
	    lbearcur_y_hot);
    GDrawDestroyWindow(image);
    GDrawDestroyWindow(mask);

    logo_icon = GDrawCreateBitmap(NULL,logo_width,logo_height,logo_bits);
    GDrawSetDefaultIcon(logo_icon);

    image = GDrawCreateBitmap(NULL,prohibition_width,prohibition_height,prohibition_bits);
    mask = GDrawCreateBitmap(NULL,prohibition_width,prohibition_height,prohibitionmask_bits);
    ct_prohibition = GDrawCreateCursor(image,mask,0xff0000,0xffffff,prohibition_x_hot,
	    prohibition_y_hot);
    GDrawDestroyWindow(image);
    GDrawDestroyWindow(mask);
    image = GDrawCreateBitmap(NULL,ddcursor_width,ddcursor_height,ddcursor_bits);
    ct_ddcursor = GDrawCreateCursor(image,image,0xff0000,0xffffff,ddcursor_x_hot,
	    ddcursor_y_hot);
    GDrawDestroyWindow(image);
#endif
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
