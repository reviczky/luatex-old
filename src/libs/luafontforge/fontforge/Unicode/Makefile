prefix = /usr/local
exec_prefix = ${prefix}

top_srcdir = ..
top_builddir = ..
libdir = ${exec_prefix}/lib

bindir = ${exec_prefix}/bin

GU_VERSION=3
GU_REVISION=0
GU_AGE=0

LIBTOOL = $(SHELL) $(top_builddir)/libtool
CC = gcc

libgunicode_OBJECTS =  ArabicForms.lo alphabet.lo backtrns.lo char.lo \
 cjk.lo memory.lo ucharmap.lo unialt.lo ustring.lo utype.lo \
 usprintf.lo gwwiconv.lo

Incs = -I$(top_srcdir)/inc -I/usr/pkg/include -I/usr/pkg/include/giflib
CFLAGS = -g -O2 -D_NO_LIBJPEG=1 -D_NO_FREETYPE=1 -D_NO_LIBXML=1 -D_NO_LIBPNG=1 -D_NO_LIBUNINAMESLIST=1 -DFONTFORGE_CONFIG_NO_WINDOWING_UI=1 -DLUA_FF_LIB=1 -UHAVE_LIBINTL_H  -I/usr/include/freetype2/ -I/usr/include/libxml2/  $(Incs) -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -D_NO_PYTHON=1 -D_NO_LIBUNGIF=1 -D_NO_LIBTIFF=1 -D_NO_LIBTIFF=1 -DHAVE_LANGINFO_H=1 -DHAVE_STDINT_H=1 -DX_DISPLAY_MISSING=1 -DHAVE_LONG_DOUBLE=1 -D_HAS_LONGLONG=1 -DHAVE_LIBINTL_H=1 -D_STATIC_LIBINTL=1 -DNODYNAMIC=1 -D_STATIC_LIBUNINAMESLIST=1 

../libgunicode.la: $(libgunicode_OBJECTS)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -no-undefined  -L/usr/local/lib -o ../libgunicode.la $(libgunicode_OBJECTS) \
	    -ldl  -lpthread \
	    -rpath $(libdir) -version-info $(GU_VERSION):$(GU_REVISION):$(GU_AGE)

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o *.lo ../libgunicode.la

distclean cleaner:
	-rm -f *.o *.lo ../libgunicode.la Makefile
