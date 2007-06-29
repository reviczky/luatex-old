prefix = /usr/local
exec_prefix = ${prefix}

sharedir = /usr/local/share/fontforge
srcdir = .
top_srcdir = ..
top_builddir = ..

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
plugindir = $(sharedir)/plugins

LIBTOOL = $(SHELL) $(top_builddir)/libtool
CC = gcc

X_CFLAGS = 
X_LIBS = 
X_11LIB = 
X_EXTRA_LIBS = 
X_PRE_LIBS = 
STATIC_LIBS =  -lpng -lz -ljpeg -lfreetype -lxml2 -luninameslist

fontforge_LIBOBJECTS =  autohint.o autosave.o autowidth.o bitmapdlg.o \
 bitmapview.o bvedit.o charview.o cursors.o \
 cvaddpoints.o cvexport.o cvgetinfo.o cvhints.o cvimages.o cvknife.o \
 cvpalettes.o cvpointer.o cvruler.o cvshapes.o cvstroke.o cvtranstools.o \
 cvundoes.o dumpbdf.o dumppfa.o fontinfo.o fontview.o fvcomposit.o \
 fvfonts.o fvimportbdf.o fvmetrics.o images.o metricsview.o \
 parsepfa.o parsettf.o prefs.o psread.o namelist.o savefontdlg.o \
 sfd.o splashimage.o splinefill.o splineoverlap.o splinesave.o \
 splinesaveafm.o splinestroke.o splineutil.o splineutil2.o stamp.o \
 start.o tottf.o transform.o uiutil.o utils.o windowmenu.o  \
 zapfnomen.o othersubrs.o autotrace.o openfontdlg.o encoding.o print.o \
 problems.o metafont.o alignment.o parsettfbmf.o \
 macbinary.o crctab.o scripting.o freetype.o gotodlg.o search.o \
 combinations.o tilepath.o sftextfield.o displayfonts.o ikarus.o \
 cvhand.o cvfreehand.o simplifydlg.o winfonts.o tottfgpos.o tottfaat.o \
 charinfo.o splineorder2.o genttfinstrs.o ttfinstrs.o cvgridfit.o \
 cvdebug.o showatt.o kernclass.o nonlineartrans.o effects.o \
 histograms.o ttfspecial.o svg.o parsettfatt.o contextchain.o \
 macenc.o statemachine.o splinerefigure.o mm.o \
 parsettfvar.o tottfvar.o pua.o stemdb.o anchorsaway.o palmfonts.o \
 cvdgloss.o groups.o parsepdf.o plugins.o bdfinfo.o \
 glyphcomp.o unicoderange.o ufo.o ofl.o lookups.o sfd1.o  \
 python.o featurefile.o
DIFFOBJS = sfddiff.o sfd.o diffstubs.o stamp.o namelist.o pua.o
ACORNOBJS = acorn2sfd.o sfd.o diffstubs.o psunicodenames.o stamp.o

_CFLAGS = -I$(top_srcdir)/inc -I$(srcdir) -I. -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts $(X_CFLAGS) \
    -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -D_NO_PYTHON=1 -D_NO_LIBUNGIF=1 -D_NO_LIBTIFF=1 -D_NO_LIBTIFF=1 -DHAVE_LANGINFO_H=1 -DHAVE_STDINT_H=1 -DX_DISPLAY_MISSING=1 -DHAVE_LONG_DOUBLE=1 -D_HAS_LONGLONG=1 -DHAVE_LIBINTL_H=1 -D_STATIC_LIBINTL=1 -DNODYNAMIC=1 -D_STATIC_LIBUNINAMESLIST=1  '-DSHAREDIR="$(sharedir)"' -DLIBDIR='"$(libdir)"' \
     '-DPREFIX="/usr/local"'
CFLAGS = -g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 -UHAVE_LIBINTL_H  -I/usr/include/freetype2/ -I/usr/include/libxml2/ $(_CFLAGS)

#I used to include -L/usr/lib explicitly after static_libs. Not sure why. Should be implied & breaks 64bit compiles
LDFLAGS =  -L/usr/local/lib
LIBS = -rpath $(libdir) $(X_LIBS) ../libgdraw.la ../libgunicode.la \
    $(X_PRE_LIBS) $(X_11LIB) $(X_EXTRA_LIBS) -ldl  -lpthread $(STATIC_LIBS) -lm
DLIBS = -rpath $(libdir) ../libgunicode.la -ldl  -lpthread $(STATIC_LIBS) -lm

all: fontforge sfddiff fontimage

fontforge: main.o $(fontforge_LIBOBJECTS)
	$(LIBTOOL) --mode=link $(CC) -export-dynamic $(LDFLAGS) -o fontforge main.o $(fontforge_LIBOBJECTS) $(LIBS)

sfddiff: sfddiff.pe
	echo "#!$(bindir)/fontforge" >sfddiff
	cat sfddiff.pe >>sfddiff
	chmod +x sfddiff

fontimage: fontimage.pe
	echo "#!$(bindir)/fontforge" >fontimage
	cat fontimage.pe >>fontimage
	chmod +x fontimage

sfddiff.old: $(DIFFOBJS)
	$(LIBTOOL) --mode=link $(CC) -o sfddiff.old $(DIFFOBJS) $(DLIBS)

acorn2sfd: $(ACORNOBJS)
	$(LIBTOOL) --mode=link $(CC) -o acorn2sfd $(ACORNOBJS) $(DLIBS)

# The slight error introduced occasionally by optimization turns out to have
# disasterous effects. This file may not be compiled with the optimizer.
# On the other hand we do need CPPFLAGS to get the right includes...
splinerefigure.o: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(CC) -g -c -o splinerefigure.o $(_CFLAGS) $(srcdir)/splinerefigure.c

splinerefigure.lo: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(LIBTOOL) --mode=compile $(CC) -g -c -o splinerefigure.lo $(_CFLAGS) $(srcdir)/splinerefigure.c

%.o: %.c

%.o: %.c $(srcdir)/splinefont.h $(srcdir)/views.h $(srcdir)/stemdb.h $(srcdir)/ttf.h $(srcdir)/unicoderange.h $(srcdir)/psfont.h $(srcdir)/groups.h $(srcdir)/edgelist.h $(srcdir)/edgelist2.h
	$(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o fontforge sfddiff fontimage
	-rm -rf .libs

distclean cleaner: clean
	-rm -f Makefile

install:
	( cd .. ; make install )

FontForge-MenuShortCuts.pot: *.c
	xgettext --from-code=UTF-8 -kH_ -oFontForge-MenuShortCuts.pot *.c ../gdraw/*.c
	patch < potmstitle.patch

FontForge.pot: *.c utf8.pot
	echo "If this doesn't work try make fontforge-old.pot instead"
	xgettext --from-code=UTF-8 -k_ -kN_ -kS_ -kU_ -kNU_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../gdraw/*.c
	patch < pottitle.patch

# xgettext has caniptions and strips any non-ASCII characters from my strings
# but I do occasionally want to use unicode characters (TeX, arrows, Increment)
# so I have put these messages in utf8.pot which I manage by hand. non-ASCII
# strings work just fine as keys. It does mean that my pot files should be
# viewed in utf8 though.

# I am told that new versions of xgettext support an option --from-code=UTF-8
# unfortunately this works on none of the systems I use, and I haven't found
# it documented. If it worked I would not need utf8.pot.
FontForge-old.pot: *.c utf8.pot
	xgettext -k_ -kN_ -kS_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../gdraw/*.c
	cat utf8.pot >> FontForge.pot
	patch < pottitle.patch
