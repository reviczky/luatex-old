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
INSTALL = /usr/bin/install -c
CC = gcc

_CFLAGS = -I$(top_srcdir)/inc -I$(srcdir) -I. -I$(top_srcdir)/fontforge \
    -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts $(X_CFLAGS) \
    -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -D_NO_PYTHON=1 -D_NO_LIBUNGIF=1 -D_NO_LIBTIFF=1 -D_NO_LIBTIFF=1 -DHAVE_LANGINFO_H=1 -DHAVE_STDINT_H=1 -DX_DISPLAY_MISSING=1 -DHAVE_LONG_DOUBLE=1 -D_HAS_LONGLONG=1 -DHAVE_LIBINTL_H=1 -D_STATIC_LIBINTL=1 -DNODYNAMIC=1 -D_STATIC_LIBUNINAMESLIST=1  '-DSHAREDIR="$(sharedir)"' -DLIBDIR='"$(libdir)"' \
     '-DPREFIX="/usr/local"'
CFLAGS = -g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 -UHAVE_LIBINTL_H  -I/usr/include/freetype2/ -I/usr/include/libxml2/ $(_CFLAGS)

LIBS = -rpath $(libdir)  -L/usr/lib -lm

PLUGINS = gb12345.la

all: $(PLUGINS)

clean:
	-rm -rf .libs *.la

distclean: clean
	-rm -rf Makefile

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

%.la: %.lo
	$(LIBTOOL) --mode=link $(CC) -export-dynamic -module -o $@ $< $(LIBS)

install: all
	mkdir -p $(plugindir)
	-mv .libs/*.{so,dll,dylib,lai}* $(plugindir)
	mv *.la $(plugindir)

