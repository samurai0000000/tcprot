/*
 * tcprot.h
 */

#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#ifndef __TCPROT_H__
#define __TCPROT_H__

struct pair {
	int in_sock;
	struct sockaddr_in in_addr;
	int out_sock;
	struct sockaddr_in out_addr;
};

extern int tcprot_find_free_pair(struct pair *pair_set, unsigned int pair_count);
extern void tcprot_terminate_pair(struct pair *pair);

extern int tcprot_bind_listen(uint16_t port);
extern int tcprot_accept(int sock, struct pair *pair,
			 const char *outhost, uint16_t outport);

#endif
