POFILES := $(wildcard *.po)
MOFILES := $(patsubst %.po,%.mo,$(POFILES))
OUTFILES := $(MOFILES)

include ../common.mk

%.po: ../im/fcitx-m17n.pot
	./mkpo $@ $^ $*.UTF8

%.mo: %.po
	msgfmt -o$@ $^

all: $(MOUTFILES)
