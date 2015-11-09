# see License.txt for copyright and terms of use

CC := gcc
CFLAGS := -Wall
# CFLAGS += -g
FLEX := flex

.SUFFIXES:
.PHONY: all clean distbuild distclean

# **** for distribution client use

all: topformflat

topformflat: %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f topformflat

# **** for building the distribution

distbuild: topformflat.c

distclean: clean
	rm -f topformflat.c

topformflat.c: %.c: %.lex
	$(FLEX) -o$@ $<
