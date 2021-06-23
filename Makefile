#
# Makefile
#
# Copyright (C) 2019, Charles Chiou

CC =		gcc
CFLAGS =	-Wall -O3 -g
LDFLAGS =	-lncurses

.PHONY: default clean distclean

TARGETS =	tcptun1 tcptun2

default: $(TARGETS)

tcptun1: tcptun1.o net.o nc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tcptun2: tcptun2.o net.o nc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *~ *.o $(TARGETS)

distclean: clean
