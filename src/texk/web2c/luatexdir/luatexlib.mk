## luatexlib.mk - Makefile fragment for libraries used by pdf[ex]tex.
# Public domain.

# The pdf*tex programs depend on a number of libraries.
# Include dependencies to get the built if we don't do make
# from the top-level directory.

Makefile: luatexdir/luatexlib.mk

# libz

ZLIBDIR=../../libs/zlib
ZLIBSRCDIR=$(srcdir)/$(ZLIBDIR)
ZLIBDEP = @ZLIBDEP@
LDZLIB = @LDZLIB@

$(ZLIBDIR)/libz.a: $(ZLIBSRCDIR)
	cd $(ZLIBDIR) && $(MAKE) $(common_makeargs) libz.a


# libpng

LIBPNGDIR=../../libs/libpng
LIBPNGSRCDIR=$(srcdir)/$(LIBPNGDIR)
LIBPNGDEP = @LIBPNGDEP@
LDLIBPNG = @LDLIBPNG@

$(LIBPNGDIR)/libpng.a: $(LIBPNGSRCDIR)/*.c
	cd $(LIBPNGDIR) && $(MAKE) $(common_makeargs) libpng.a


# libxpdf

LIBXPDFDIR=../../libs/xpdf
LIBXPDFSRCDIR=$(srcdir)/$(LIBXPDFDIR)
LIBXPDFDEP = @LIBXPDFDEP@
LDLIBXPDF = @LDLIBXPDF@

$(LIBXPDFDIR)/fofi/libfofi.a: $(LIBXPDFSRCDIR)/fofi/*.cc \
	$(LIBXPDFSRCDIR)/fofi/*.h
	cd $(LIBXPDFDIR)/fofi; $(MAKE) $(common_makeargs) libfofi.a
$(LIBXPDFDIR)/goo/libGoo.a: $(LIBXPDFSRCDIR)/goo/*.cc \
	$(LIBXPDFSRCDIR)/goo/*.c $(LIBXPDFSRCDIR)/goo/*.h
	cd $(LIBXPDFDIR)/goo; $(MAKE) $(common_makeargs) libGoo.a
$(LIBXPDFDIR)/xpdf/libxpdf.a: $(LIBXPDFSRCDIR)/xpdf/*.cc \
	$(LIBXPDFSRCDIR)/xpdf/*.h
	cd $(LIBXPDFDIR)/xpdf; $(MAKE) $(common_makeargs) libxpdf.a


# md5

LIBMD5DIR=../../libs/md5
LIBMD5SRCDIR=$(srcdir)/$(LIBMD5DIR)
LIBMD5DEP=$(LIBMD5DIR)/md5.o

$(LIBMD5DEP): $(LIBMD5SRCDIR)/md5.c $(LIBMD5SRCDIR)/md5.h
clean:: md5lib-clean
md5lib-clean:
	rm -f $(LIBMD5DEP)


# libpdf itself
pdflib = luatexdir/libpdf.a
pdflib_sources = $(srcdir)/luatexdir/*.c $(srcdir)/luatexdir/*.cc \
	$(srcdir)/luatexdir/*.h

luatexdir/libpdf.a: $(pdflib_sources) luatexdir/luatexextra.h
	cd luatexdir && $(MAKE) $(common_makeargs) libpdf.a


# lua

LIBLUADIR=../../libs/lua51
LIBLUASRCDIR=$(srcdir)/$(LIBLUADIR)
LIBLUADEP=$(LIBLUADIR)/liblua.a
$(LIBLUADEP):
	mkdir -p $(LIBLUADIR) && cd $(LIBLUADIR) && cp -f $(LIBLUASRCDIR)/* . && $(MAKE) linux


# slnunicode
SLNUNICODEDIR=../../libs/slnunicode-0.9.1
SLNUNICODESRCDIR=$(srcdir)/$(SLNUNICODEDIR)
SLNUNICODEDEP=$(SLNUNICODEDIR)/slnunico.o
$(SLNUNICODEDEP): $(SLNUNICODEDIR)/slnunico.c $(SLNUNICODEDIR)/slnudata.c
	mkdir -p $(SLNUNICODEDIR) && cd $(SLNUNICODEDIR) && cp -f $(SLNUNICODESRCDIR)/* . && $(CC) -I$(LIBLUADIR) -o slnunico.o -c slnunico.c



# Convenience variables.

luatexlibs = $(pdflib) $(LDLIBPNG) $(LDZLIB) $(LDLIBXPDF) $(LIBMD5DEP) $(LIBLUADEP) $(SLNUNICODEDEP)
luatexlibsdep = $(pdflib) $(LIBPNGDEP) $(ZLIBDEP) $(LIBXPDFDEP) $(LIBMD5DEP) $(LIBLUADEP) $(SLNUNICODEDEP)

## end of luatexlib.mk - Makefile fragment for libraries used by pdf[ex]tex.

