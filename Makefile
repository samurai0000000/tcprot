
CC =		gcc
CFLAGS =	-Wall -O3 -g


.PHONY: default clean distclean

TARGETS =	tcprot1 tcprot2

default: $(TARGETS)

tcprot1: tcprot1.o net.o
	$(CC) $(CFLAGS) -o $@ $^

tcprot2: tcprot2.o net.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *~ *.o $(TARGETS)

distclean: clean
