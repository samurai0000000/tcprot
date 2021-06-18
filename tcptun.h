/*
 * tcptun.h
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>

#ifndef __TCPTUN_H__
#define __TCPTUN_H__

struct pair {
    int in_sock;
    struct sockaddr_in in_addr;
    int out_sock;
    struct sockaddr_in out_addr;
    uint64_t inbytes;	// for statistics
    uint64_t outbytes;	// for statistics
};

extern int tcptun_find_free_pair(struct pair *pair_set, unsigned int pair_count);
extern void tcptun_terminate_pair(struct pair *pair);
extern int tcptun_bind_listen(uint16_t port);
extern int tcptun_accept(int sock, struct pair *pair,
                         const char *outhost, uint16_t outport);

struct tcptunstat {
};

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
