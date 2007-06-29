prefix = /usr/local
exec_prefix = ${prefix}

srcdir = .
top_srcdir = ..
top_builddir = ..

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib

GD_VERSION=2
GD_REVISION=0
GD_AGE=0

LIBTOOL = $(SHELL) $(top_builddir)/libtool
CC = gcc

X_CFLAGS = 
X_LIBS = 
X_11LIB = 
X_EXTRA_LIBS = 
X_PRE_LIBS = 
STATIC_LIBS =  -lpng -lz -ljpeg -lfreetype -lxml2 -luninameslist

libgdraw_OBJECTS =  choosericons.lo divisors.lo drawboxborder.lo \
 fsys.lo gaskdlg.lo gbuttons.lo gchardlg.lo gcontainer.lo gdraw.lo \
 gdrawbuildchars.lo gdrawerror.lo gdrawtxt.lo gdrawtxtinit.lo \
 genkeysym.lo gfilechooser.lo gfiledlg.lo ggadgets.lo ggroupbox.lo \
 gimage.lo gimageclut.lo gimagecvt.lo gimagepsdraw.lo gimageread.lo \
 gimagereadbmp.lo gimagereadgif.lo gimagereadjpeg.lo gimagereadpng.lo \
 gimagereadras.lo gimagereadrgb.lo gimagereadtiff.lo gimagereadxbm.lo \
 gimagereadxpm.lo gimagewritebmp.lo gimagewriteeps.lo \
 gimagewritegimage.lo gimagewritejpeg.lo gimagewritepng.lo \
 gimagewritexbm.lo gimagewritexpm.lo gimagexdraw.lo gio.lo giofile.lo \
 giohosts.lo giomime.lo giothread.lo giotrans.lo gkeysym.lo glist.lo \
 gmenu.lo gprogress.lo gpsdraw.lo gpstxtinit.lo gradio.lo gresource.lo \
 gresourceimage.lo gsavefiledlg.lo gscrollbar.lo gtabset.lo \
 gtextfield.lo gtextinfo.lo gwidgets.lo gxdraw.lo dynamic.lo \
 ghvbox.lo gmatrixedit.lo gdrawable.lo gspacer.lo

Incs = -I$(top_srcdir)/inc -I/usr/pkg/include -I/usr/pkg/include/giflib
CFLAGS = -g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 -UHAVE_LIBINTL_H  -I/usr/include/freetype2/ -I/usr/include/libxml2/  $(Incs) -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts $(X_CFLAGS) -DNOTHREADS -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -D_NO_PYTHON=1 -D_NO_LIBUNGIF=1 -D_NO_LIBTIFF=1 -D_NO_LIBTIFF=1 -DHAVE_LANGINFO_H=1 -DHAVE_STDINT_H=1 -DX_DISPLAY_MISSING=1 -DHAVE_LONG_DOUBLE=1 -D_HAS_LONGLONG=1 -DHAVE_LIBINTL_H=1 -D_STATIC_LIBINTL=1 -DNODYNAMIC=1 -D_STATIC_LIBUNINAMESLIST=1  -DLIBDIR='"$(libdir)"'

../libgdraw.la: $(libgdraw_OBJECTS)
	$(LIBTOOL) --mode=link $(CC) -no-undefined $(CFLAGS)  -L/usr/local/lib -o ../libgdraw.la $(libgdraw_OBJECTS) \
	    ../libgunicode.la -ldl  -lpthread \
	    $(STATIC_LIBS) $(X_LIBS) $(X_PRE_LIBS) $(X_11LIB) $(X_EXTRA_LIBS)  \
	    -rpath $(libdir) -version-info $(GD_VERSION):$(GD_REVISION):$(GD_AGE)

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.lo *.o

distclean cleaner:
	-rm -f *.lo *.o ../libgdraw.la Makefile
