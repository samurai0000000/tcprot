#
# Makefile
#
# Copyright (C) 2019, Charles Chiou

CC =		gcc
CFLAGS =	-Wall -O3 -g
LDFLAGS =	-lncurses

.PHONY: default clean distclean

TARGETS =	tcptun

default: $(TARGETS)

tcptun: tcptun.o net.o nc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *~ *.o $(TARGETS)

distclean: clean
