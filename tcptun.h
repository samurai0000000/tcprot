/*
 * tcptun.h
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include <stdio.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <libgen.h>
#include <ncurses.h>

#ifndef __TCPTUN_H__
#define __TCPTUN_H__

/*
 * A tunneling TCP socket pair.
 */
struct pair {
    int in_sock;        /* Socket for incoming TCP connection */
    struct sockaddr_in in_addr;  /* Incoming connection address */
    int out_sock;       /* Socket for outgoing TCP connection */
    struct sockaddr_in out_addr; /* Outgoing connection address */
    time_t tod_sec;     /* gettimeofday() tv_sec stamped at accept() */
    uint64_t inbytes;	/* Tally of incoming bytes */
    uint64_t outbytes;	/* Tally of outgoing bytes */
    uint8_t instatus;   /* Incoming status */
    uint8_t outstatus;  /* Outgoing status */
};

extern int tcptun_find_free_pair(struct pair *pair_set,
                                 unsigned int pair_count);
extern void tcptun_terminate_pair(struct pair *pair);
extern int tcptun_bind_listen(uint16_t port);
extern int tcptun_accept(int sock, struct pair *pair,
                         const char *outhost, uint16_t outport);
extern void nc_init(void);
extern void nc_set_title(const char *title);
extern void nc_log(const char *format, ...);
extern void nc_refresh(const struct pair pairs[], unsigned int npairs);
extern void nc_cleanup(void);

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
