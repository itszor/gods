# Static recompilation of Gods: the game (gen/: Gods as its own squeeze
# decompressor unpacks it, ../tools/unpack.py) and the intro (genintro/:
# Intro, likewise), from SuperSpider's analyse and gen, and game.c, linked
# with libroger (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/code*.c) gen/entries.c
GENINTRO:= $(wildcard genintro/code*.c) genintro/entries.c
OBJ     := $(GEN:.c=.o) $(GENINTRO:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread
INC     := -I$(ROGER)/include

gods: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

%.o: %.c $(ROGER)/include/roger.h $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Code/data analysis: SuperSpider, into ../analysis/<name>.json, then
# superspider gen. Each image's ../analysis/<name>.roots adds entry points
# to the analysis: "make roots" merges the gap entries a --trace-gaps run
# logged (untranslated.log, or LOG=...) into them, so the next "make regen"
# translates them. Gods' object handlers (../unpacked/gods.roots, from
# ../tools/unpack.py) are roots too. The two images share &8000 (one runs
# after the other): the log tags each entry with the image that was running.
SS      ?= $(HOME)/stuff/superspider
SSBIN   := $(SS)/_build/default/bin/main.exe
AN      := ../analysis
GAME    := ../unpacked/gods
INTRO   := ../unpacked/intro
LOG     ?= untranslated.log

# --roots for ../analysis/<name>.roots, if there is one
roots = $(if $(wildcard $(AN)/$(1).roots),--roots $(AN)/$(1).roots)

# $(call analyse,name,image,base[,more options])
define analyse
	$(SSBIN) analyse "$(2)" $(3) $(4) --full $(call roots,$(1)) -o $(AN)/$(1).json
endef

regen:
	cd $(SS) && dune build
	python3 -I ../tools/unpack.py
	mkdir -p $(AN)
	$(call analyse,gods,$(GAME),8000,--roots ../unpacked/gods.roots)
	$(call analyse,intro,$(INTRO),8000)
	rm -rf gen genintro
	$(SSBIN) gen "$(GAME)" 8000 $(AN)/gods.json --c gen --leaders=dispatch
	$(SSBIN) gen "$(INTRO)" 8000 $(AN)/intro.json --c genintro --leaders=dispatch \
	    --prefix=I --tabprefix=recompI --files=1 --covtag=I

roots:
	python3 -I $(SS)/tools/gap_roots.py "$(GAME)" 8000 $(AN)/gods.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py "$(INTRO)" 8000 $(AN)/intro.roots $(LOG)

clean:
	rm -f gen/*.o genintro/*.o game.o gods

.PHONY: FORCE regen roots clean
