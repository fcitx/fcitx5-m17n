CFLAGS := $(shell m17n-config --cflags) $(shell pkg-config --cflags fcitx)
CFLAGS := $(CFLAGS) -fPIC -g -DLOCALEDIR=\"$(PREFIX)/share/locale\" -D_GNU_SOURCE
LDFLAGS := -Wl,--as-needed
M17NLIBS := $(shell m17n-config --libs)
FCITXLIBS := $(shell pkg-config --libs fcitx)

INTERFILES := fcitx-m17n.conf.in.h
OUTFILES := fcitx-m17n.so fcitx-m17n.pot fcitx-m17n.conf

include ../common.mk
include ../config.mk

fcitx-m17n.o: fcitx-m17n.h

fcitx-m17n.so: fcitx-m17n.o
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(M17NLIBS) $(FCITXLIBS)

fcitx-m17n.conf.in.h: fcitx-m17n.conf.in
	intltool-extract --update --type=gettext/ini $^

fcitx-m17n.conf: fcitx-m17n.conf.in ../po/*.po
	intltool-merge --desktop --utf8 ../po $< $@

fcitx-m17n.pot: *.c $(CONFINH)
	xgettext -k_ -kN_ -o $@ $^

all: fcitx-m17n.so fcitx-m17n.pot fcitx-m17n.conf

install: all
	cp fcitx-m17n.so $(PREFIX)/lib/fcitx/
	cp fcitx-m17n.conf $(PREFIX)/share/fcitx/addon/

