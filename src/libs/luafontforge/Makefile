# $Id$

SRCS= src/luafontforge.c
OBJS= src/luafontforge.o

FF_OBJS=autohint.o fontinfo.o fvfonts.o \
parsepfa.o parsettf.o prefs.o psread.o namelist.o \
splinefill.o splineoverlap.o splinestroke.o splineutil.o splineutil2.o \
stamp.o start.o encoding.o splineorder2.o kernclass.o ttfspecial.o \
parsettfatt.o contextchain.o macenc.o splinerefigure.o \
parsettfvar.o pua.o stemdb.o \
lookups.o featurefile.o charview.o fontview.o macbinary.o 

# stemdb.o: needed by autohint.o
# splinestroke.o: needed by psread.o

# mm.o
# fvcomposit.o cvundoes.o bvedit.o parsepdf.o  combinations.o 
# parsettfbmf.o sftextfield.o statemachine.o unicoderange.o 
# cvgetinfo.o autowidth.o uiutil.o utils.o othersubrs.o 
# anchorsaway.o charinfo.o crctab.o cvhints.o cvpointer.o 
# splinesaveafm.o tottfgpos.o zapfnomen.o fvmetrics.o 
# showatt.o winfonts.o bitmapview.o search.o openfontdlg.o 
# ofl.o sfd1.o effects.o genttfinstrs.o ttfinstrs.o freetype.o 
# bdfinfo.o splinesave.o autohint.c tottfaat.o tottfvar.o mm.o
# cvdgloss.o glyphcomp.o tilepath.o displayfonts.o tottf.o 
# problems.o metafont.o alignment.o savefontdlg.o palmfonts.o 
# cvtranstools.o images.o bitmapdlg.o metricsview.o transform.o 
# cvknife.o cvaddpoints.o cvruler.o cvshapes.o cvstroke.o 
# cvgridfit.o cvdebug.o cvhand.o cvfreehand.o cvimages.o cvpalettes.o 
# cursors.o windowmenu.o histograms.o simplifydlg.o gotodlg.o 
# print.o dumppfa.o autosave.o cvexport.o  dumpbdf.o autotrace.o splashimage.o 
# svg.o ufo.o scripting.o plugins.o python.o ikarus.o sfd.o 

FF_DUMMY=src/ffdummies.o
FF_DUMMY_O=../../src/ffdummies.o 

FF_LIB=src/luafflib.o
FF_LIB_O=../../src/luafflib.o 

FF_UNI_OBJS =  ArabicForms.o alphabet.o backtrns.o char.o \
 cjk.o memory.o ucharmap.o unialt.o ustring.o \
 utype.o usprintf.o gwwiconv.o

FF_UNI_OBJS_O =  ../Unicode/ArabicForms.o ../Unicode/alphabet.o  ../Unicode/char.o \
 ../Unicode/memory.o ../Unicode/cjk.o  ../Unicode/ustring.o \
 ../Unicode/usprintf.o ../Unicode/gwwiconv.o 

# cjk.o and gwwiconv.o are stripped off all _from_unicode entries
# cjk.o could be shortened even more by breaking the big arrays into
# smaller blocks divided by zero-segments, but that is not much fun to do
# (the same is true for namelist.o)
# ../Unicode/utype.o   /* relevant bits reimplemented */
# ../Unicode/ucharmap.o /* only needed for message */
# ../Unicode/unialt.o /* uses quite a lot of room compared to functionality */

FF_CONF_OPTIONS=--without-python --without-iconv --without-bytecode-interpreter --without-freetype-src --without-x \
--without-devicetables --without-pic --with-regular-link --enable-static

FF_NEEDED_LIBS= -lm

FF_CFLAGS='-g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 '

MY_CFLAGS=-g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1

.PHONY: newfile clean

# dependancies are a problem, but let's assume that ff remains stable

fontforge: $(FF_DUMMY) $(FF_LIB) $(FF_OBJS) $(FF_UNI_OBJS)
	rm -f libff.a
	cd fontforge/fontforge && ar cru  ../../libff.a  $(FF_OBJS) $(FF_UNI_OBJS_O) $(FF_DUMMY_O) $(FF_LIB_O)

$(FF_UNI_OBJS):
	cd fontforge/Unicode && make $@

$(FF_OBJS):
	cd fontforge/fontforge && make $@

$(FF_DUMMY): src/ffdummies.c
	cd src; gcc $(MY_CFLAGS) -c -o ffdummies.o -I../fontforge/fontforge -I../fontforge/inc ffdummies.c

$(FF_LIB): src/luafflib.c
	cd src; gcc $(MY_CFLAGS) -c -o luafflib.o -I../../lua51 -I.. -I../fontforge/inc -I../fontforge/fontforge luafflib.c

config: newfile
	cd fontforge && env CFLAGS=$(FF_CFLAGS) ./configure $(FF_CONF_OPTIONS)

clean:
	rm  -f libff.a $(FF_DUMMY) $(FF_LIB)
	cd fontforge; make clean