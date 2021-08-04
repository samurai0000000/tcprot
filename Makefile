#
# Makefile
#
# Copyright (C) 2019, Charles Chiou

OS :=		$(shell uname -o)

CC ?=		gcc
CFLAGS =	-Wall -O3 -g
LDFLAGS =	-lcurses

ifeq ($(OS),illumos)
LDFLAGS +=	-lsocket -lnsl
endif

RM ?=		rm -f

.PHONY: default clean distclean

TARGETS =	tcptun

OBJS =		tcptun.o net.o nc.o

default: $(TARGETS)

tcptun: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *~ *.o $(TARGETS)

distclean: clean
