
CC =		gcc
CFLAGS =	-Wall -O3 -g


.PHONY: default clean distclean

TARGETS =	tcprot1 tcprot2

default: $(TARGETS)

tcprot1: tcprot1.c
	$(CC) $(CFLAGS) -o $@ $<

tcprot2: tcprot2.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) *~ *.o $(TARGETS)

distclean: clean
