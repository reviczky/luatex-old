prefix = /usr/local
localedir = $(prefix)/share/locale
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

MOFILES = de.mo el.mo en_GB.mo es.mo fr.mo it.mo ja.mo pl.mo ru.mo vi.mo zh_CN.mo zh_TW.mo

.SUFFIXES:
.SUFFIXES: .po .mo

all: $(MOFILES)

.po.mo:
	msgfmt -o $@ $<

install:
	mkdir -p $(localedir)/de/LC_MESSAGES
	$(INSTALL_DATA) de.mo $(localedir)/de/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/el/LC_MESSAGES
	$(INSTALL_DATA) el.mo $(localedir)/el/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/en_GB/LC_MESSAGES
	$(INSTALL_DATA) en_GB.mo $(localedir)/en_GB/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/es/LC_MESSAGES
	$(INSTALL_DATA) es.mo $(localedir)/es/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/fr/LC_MESSAGES
	$(INSTALL_DATA) fr.mo $(localedir)/fr/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/it/LC_MESSAGES
	$(INSTALL_DATA) it.mo $(localedir)/it/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/ja/LC_MESSAGES
	$(INSTALL_DATA) ja.mo $(localedir)/ja/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/pl/LC_MESSAGES
	$(INSTALL_DATA) pl.mo $(localedir)/pl/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/ru/LC_MESSAGES
	$(INSTALL_DATA) ru.mo $(localedir)/ru/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/vi/LC_MESSAGES
	$(INSTALL_DATA) vi.mo $(localedir)/vi/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/zh_CN/LC_MESSAGES
	$(INSTALL_DATA) zh_CN.mo $(localedir)/zh_CN/LC_MESSAGES/FontForge.mo
	mkdir -p $(localedir)/zh_TW/LC_MESSAGES
	$(INSTALL_DATA) zh_TW.mo $(localedir)/zh_TW/LC_MESSAGES/FontForge.mo
