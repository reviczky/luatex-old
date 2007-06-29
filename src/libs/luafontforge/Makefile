# $Id$

SRCS= src/luafontforge.c
OBJS= src/luafontforge.o

FF_OBJS=autohint.o autowidth.o bvedit.o bdfinfo.o cvaddpoints.o \
cvgetinfo.o cvhints.o cvimages.o cvknife.o cvpalettes.o \
cvpointer.o cvruler.o cvshapes.o cvstroke.o cvtranstools.o	\
cvundoes.o fontinfo.o fvcomposit.o \
fvfonts.o fvimportbdf.o fvmetrics.o images.o \
parsepfa.o parsettf.o prefs.o psread.o namelist.o \
splinefill.o splineoverlap.o \
splinestroke.o splineutil.o splineutil2.o \
stamp.o start.o transform.o uiutil.o utils.o zapfnomen.o 	\
othersubrs.o encoding.o problems.o metafont.o alignment.o \
parsettfbmf.o macbinary.o crctab.o freetype.o \
search.o combinations.o tilepath.o sftextfield.o \
displayfonts.o cvhand.o cvfreehand.o \
winfonts.o charinfo.o splineorder2.o \
genttfinstrs.o ttfinstrs.o cvgridfit.o cvdebug.o showatt.o kernclass.o 	\
nonlineartrans.o effects.o ttfspecial.o \
parsettfatt.o contextchain.o macenc.o statemachine.o splinerefigure.o mm.o	 \
parsettfvar.o pua.o stemdb.o anchorsaway.o \
palmfonts.o cvdgloss.o parsepdf.o glyphcomp.o \
unicoderange.o ofl.o lookups.o sfd1.o featurefile.o \
bitmapdlg.o openfontdlg.o \
bitmapview.o charview.o fontview.o metricsview.o \
tottfgpos.o tottfaat.o tottfvar.o tottf.o \
splinesave.o splinesaveafm.o  savefontdlg.o 

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

FF_UNI_OBJS_O =  ../Unicode/ArabicForms.o ../Unicode/alphabet.o ../Unicode/backtrns.o ../Unicode/char.o \
 ../Unicode/cjk.o ../Unicode/memory.o ../Unicode/ucharmap.o ../Unicode/unialt.o ../Unicode/ustring.o \
 ../Unicode/utype.o ../Unicode/usprintf.o ../Unicode/gwwiconv.o

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