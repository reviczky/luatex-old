# Makefile fragment for pdfeTeX and web2c. --infovore@xs4all.nl. Public domain.
# This fragment contains the parts of the makefile that are most likely to
# differ between releases of pdfeTeX.

# We build luatex
luatex = @LTEX@ luatex
luatexdir = luatexdir

Makefile: $(srcdir)/$(luatexdir)/luatex.mk

# luatex_bin = luatex ttf2afm pdftosrc
luatex_bin = luatex
linux_build_dir = $(HOME)/luatex/build/linux/texk/web2c

# Extract luatex version
$(luatexdir)/luatex.version: $(srcdir)/$(luatexdir)/luatex.web
	test -d $(luatexdir) || mkdir $(luatexdir)
	grep '^@d luatex_version_string==' $(srcdir)/$(luatexdir)/luatex.web \
	  | sed "s/^.*'-//;s/'.*$$//" \
	  >$(luatexdir)/luatex.version
          
# The C sources.
luatex_c = luatexini.c luatex0.c luatex1.c luatex2.c luatex3.c
luatex_o = luatexini.o luatex0.o luatex1.o luatex2.o luatex3.o luatexextra.o loadpool.o

# Making luatex
luatex: luatexd.h $(luatex_o) $(luatexextra_o) $(luatexlibsdep)
	@CXXHACKLINK@ $(luatex_o) $(luatexextra_o) $(luatexlibs) $(socketlibs) @CXXHACKLDLIBS@ @CXXLDEXTRA@

# C file dependencies.
$(luatex_c) luatexcoerce.h luatexd.h: luatex.p $(web2c_texmf) $(srcdir)/$(luatexdir)/luatex.defines $(srcdir)/$(luatexdir)/luatex.h
	$(web2c) luatex
luatexextra.c: $(luatexdir)/luatexextra.h lib/texmfmp.c
	test -d $(luatexdir) || mkdir $(luatexdir)
	sed s/TEX-OR-MF-OR-MP/luatex/ $(srcdir)/lib/texmfmp.c >$@
$(luatexdir)/luatexextra.h: $(luatexdir)/luatexextra.in $(luatexdir)/luatex.version etexdir/etex.version
	test -d $(luatexdir) || mkdir $(luatexdir)
	sed -e s/LUATEX-VERSION/`cat $(luatexdir)/luatex.version`/ \
	    -e s/ETEX-VERSION/`cat etexdir/etex.version`/ \
	  $(srcdir)/$(luatexdir)/luatexextra.in >$@
loadpool.c: luatex.pool $(srcdir)/$(luatexdir)/makecpool
	perl $(srcdir)/$(luatexdir)/makecpool luatex.pool > loadpool.c


# Tangling
luatex.p luatex.pool: tangle $(srcdir)/$(luatexdir)/luatex.web luatex.ch
	$(TANGLE) $(srcdir)/$(luatexdir)/luatex.web luatex.ch

#   Sources for luatex.ch:
luatex_ch_srcs = $(srcdir)/$(luatexdir)/luatex.web \
  $(srcdir)/$(luatexdir)/tex.ch0 \
  $(srcdir)/$(luatexdir)/web2c.ch \
  $(srcdir)/$(luatexdir)/luatex.ch \
  $(srcdir)/$(luatexdir)/lua.ch

#   Rules:
luatex.ch: $(TIE) $(luatex_ch_srcs)
	$(TIE) -c luatex.ch $(luatex_ch_srcs)

# for developing only
luatex-org.web: $(TIE) $(luatex_ch_srcs_org)
	$(TIE) -m $@ $(luatex_ch_srcs_org)
luatex-all.web: $(TIE) $(srcdir)/$(luatexdir)/luatex.web luatex.ch
	$(TIE) -m $@ $(srcdir)/$(luatexdir)/luatex.web luatex.ch
luatex-all.tex: luatex-all.web
	$(WEAVE) luatex-all.web
luatex-all.pdf: luatex-all.tex
	$(luatex) luatex-all.tex

check: @PETEX@ luatex-check
luatex-check: luatex luatex.fmt

clean:: luatex-clean
luatex-clean:
	$(LIBTOOL) --mode=clean $(RM) luatex
	rm -f $(luatex_o) $(luatex_c) luatexextra.c luatexcoerce.h
	rm -f $(luatexdir)/luatexextra.h
	rm -f luatexd.h luatex.p luatex.pool luatex.ch strpool.c
	rm -f luatex.fmt luatex.log

# Dumps
all_luafmts = @FMU@ luatex.fmt $(luafmts)

dumps: @LTEX@ luafmts
luafmts: $(all_luafmts)

luafmtdir = $(web2cdir)/luatex
$(luafmtdir)::
	$(SHELL) $(top_srcdir)/../mkinstalldirs $(luafmtdir)

luatex.fmt: luatex
	$(dumpenv) $(MAKE) progname=luatex files="etex.src plain.tex cmr10.tfm" prereq-check
	$(dumpenv) ./luatex --progname=luatex --jobname=luatex --ini \*\\pdfoutput=1\\input etex.src \\dump </dev/null

pdflatex.fmt: luatex
	$(dumpenv) $(MAKE) progname=pdflatex files="latex.ltx" prereq-check
	$(dumpenv) ./luatex --progname=pdflatex --jobname=pdflatex --ini \*\\pdfoutput=1\\input latex.ltx </dev/null

# 
# Installation.
install-luatex: install-luatex-exec install-luatex-data
install-luatex-exec: install-luatex-links
install-luatex-data: install-luatex-pool @FMU@ install-luatex-dumps
install-luatex-dumps: install-luatex-fmts

# The actual binary executables and pool files.
install-programs: @PETEX@ install-luatex-programs
install-luatex-programs: $(luatex) $(bindir)
	for p in luatex; do $(INSTALL_LIBTOOL_PROG) $$p $(bindir); done

install-links: @PETEX@ install-luatex-links
install-luatex-links: install-luatex-programs
	#cd $(bindir) && (rm -f luainitex luavirtex; \
	#  $(LN) luatex luainitex; $(LN) luatex luavirtex)

install-fmts: @PETEX@ install-luatex-fmts
install-luatex-fmts: luafmts $(luafmtdir)
	luafmts="$(all_luafmts)"; \
	  for f in $$luafmts; do $(INSTALL_DATA) $$f $(luafmtdir)/$$f; done
	luafmts="$(luafmts)"; \
	  for f in $$luafmts; do base=`basename $$f .fmt`; \
	    (cd $(bindir) && (rm -f $$base; $(LN) luatex $$base)); done

# Auxiliary files.
install-data:: @PETEX@ install-luatex-data
install-luatex-pool: luatex.pool $(texpooldir)
	$(INSTALL_DATA) luatex.pool $(texpooldir)/luatex.pool

# 
# luatex binaries archive
luatexbin:
	$(MAKE) $(luatex_bin)

luatex-cross:
	$(MAKE) web2c-cross
	$(MAKE) luatexbin

web2c-cross: $(web2c_programs)
	@if test ! -x $(linux_build_dir)/tangle; then echo Error: linux_build_dir not ready; exit -1; fi
	rm -f web2c/fixwrites web2c/splitup web2c/web2c
	cp -f $(linux_build_dir)/web2c/fixwrites web2c
	cp -f $(linux_build_dir)/web2c/splitup web2c
	cp -f $(linux_build_dir)/web2c/web2c web2c
	touch web2c/fixwrites web2c/splitup web2c/web2c
	$(MAKE) tangleboot && rm -f tangleboot && \
	cp -f $(linux_build_dir)/tangleboot .  && touch tangleboot
	$(MAKE) ctangleboot && rm -f ctangleboot && \
	cp -f $(linux_build_dir)/ctangleboot .  && touch ctangleboot
	$(MAKE) ctangle && rm -f ctangle && \
	cp -f $(linux_build_dir)/ctangle .  && touch ctangle
	$(MAKE) tie && rm -f tie && \
	cp -f $(linux_build_dir)/tie .  && touch tie
	$(MAKE) tangle && rm -f tangle && \
	cp -f $(linux_build_dir)/tangle .  && touch tangle

# vim: set noexpandtab
# end of luatex.mk
