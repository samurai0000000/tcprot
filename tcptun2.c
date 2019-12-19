/*
 * tcptun2.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include "tcptun.h"

#define DEFAULT_INPORT		42021
#define DEFAULT_OUTPORT		22
#define DEFAULT_OUTHOST		"127.0.0.1"
#define MAX_TUNNELS		16

static int serv_sock = -1;
static struct pair tunpairs[MAX_TUNNELS];
static fd_set fdset;

static void sighandler(int signal)
{
	fprintf(stderr, "caught signal %d, exiting\n", signal);
	exit(0);
}

static void cleanup(void)
{
	close(serv_sock);
	serv_sock = -1;
	fprintf(stderr, "goodbye!\n");
}

static void tcptun2_incoming_process(struct pair *pair)
{
	char buf[1024];
	ssize_t size;
	ssize_t wsize;
	int i;

	size = read(pair->in_sock, buf, sizeof(buf));
	if (size <= 0) {
		tcptun_terminate_pair(pair);
		return;
	}

	for (i = 0; i < size; i++) {
		buf[i] ^= 0x55;
	}

	wsize = write(pair->out_sock, buf, size);
	if (wsize != size) {
		tcptun_terminate_pair(pair);
		return;
	}

	//fprintf(stderr, "-> %d bytes\n", (int) size);
}

static void tcptun2_outgoing_process(struct pair *pair)
{
	char buf[1024];
	ssize_t size;
	ssize_t wsize;
	int i;

	size = read(pair->out_sock, buf, sizeof(buf));
	if (size <= 0) {
		tcptun_terminate_pair(pair);
		return;
	}

	for (i = 0; i < size; i++) {
		buf[i] ^= 0x55;
	}

	wsize = write(pair->in_sock, buf, size);
	if (wsize != size) {
		tcptun_terminate_pair(pair);
		return;
	}

	//fprintf(stderr, "<- %d bytes\n", (int) size);
}

int main(int argc, char **argv)
{
	struct pair pair;
	int i, rval;
	struct timeval timeout;

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGTERM, sighandler);
	atexit(cleanup);

	for (i = 0; i < MAX_TUNNELS; i++) {
		tunpairs[i].in_sock = -1;
		tunpairs[i].out_sock = -1;
	}
	
	serv_sock = tcptun_bind_listen(DEFAULT_INPORT);
	if (serv_sock < 0) {
		exit(serv_sock);
	}


	while (serv_sock >= 0) {
		FD_ZERO(&fdset);
		FD_SET(serv_sock, &fdset);
		for (i = 0; i < MAX_TUNNELS; i++) {
			if (tunpairs[i].in_sock >= 0)
				FD_SET(tunpairs[i].in_sock, &fdset);
			if (tunpairs[i].out_sock >= 0)
				FD_SET(tunpairs[i].out_sock, &fdset);
		}

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		rval = select(FD_SETSIZE, &fdset, NULL, NULL, &timeout);
		if (rval <= 0)
			continue;

		if (FD_ISSET(serv_sock, &fdset)) {
			if (tcptun_accept(serv_sock, &pair,
					  DEFAULT_OUTHOST, DEFAULT_OUTPORT) == 0) {
				i = tcptun_find_free_pair(tunpairs, MAX_TUNNELS);
				if (i < 0 || i >= MAX_TUNNELS) {
					fprintf(stderr, "no free tunnel left!\n");
					close(pair.in_sock);
					close(pair.out_sock);
					continue;
				} else {
					memcpy(&tunpairs[i], &pair, sizeof(pair));
					FD_SET(pair.in_sock, &fdset);
					FD_SET(pair.out_sock, &fdset);
					continue;
				}
			}
		}

		for (i = 0; i < MAX_TUNNELS; i++) {
			if (tunpairs[i].in_sock >= 0 &&
			    FD_ISSET(tunpairs[i].in_sock, &fdset)) {
				tcptun2_incoming_process(&tunpairs[i]);
			}
			if (tunpairs[i].out_sock >= 0 &&
			    FD_ISSET(tunpairs[i].out_sock, &fdset)) {
				tcptun2_outgoing_process(&tunpairs[i]);
			}
		}
	}

	return 0;
}
