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
static int daemonize = 0;
static int inport = DEFAULT_INPORT;
static char outhost[128] = DEFAULT_OUTHOST;
static int outport = DEFAULT_OUTPORT;

static const struct option long_options[] = {
	{ "daemonize", no_argument, 0, 'd', },
	{ "inport", required_argument, 0, 'I', },
	{ "outport", required_argument, 0, 'O', },
	{ "outhost", required_argument, 0, 'H', },
	{ 0, 0, 0, 0, },
};

static void print_help(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
	fprintf(stderr,
		"	--help		This message\n"
		"	--daemonize	run as daemon\n"
		"	--inport  -I	incoming port\n"
		"	--outport -O	outgoing port\n"
		"	--outhost -H	outgoing host\n");
}

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

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "dI:O:H:",
				    long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'd':
			daemonize = 1;
			break;
		case 'I':
			inport = atoi(optarg);
			break;
		case 'O':
			outport = atoi(optarg);
			break;
		case 'H':
			strncpy(outhost, optarg, sizeof(outhost));
			break;
		case '?':
		default:
			print_help(argc, argv);
			exit(-1);
			break;
		}
	}

	tcptun_fix_hostname(outhost);

	if (daemonize) {
		if (daemon(1, 1) != 0) {
			perror("daemon");
			exit(-1);
		}
	}

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGTERM, sighandler);
	atexit(cleanup);

	for (i = 0; i < MAX_TUNNELS; i++) {
		tunpairs[i].in_sock = -1;
		tunpairs[i].out_sock = -1;
	}

	serv_sock = tcptun_bind_listen(inport);
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
					  outhost, outport) == 0) {
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
