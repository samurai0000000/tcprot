/*
 * tcprot2.c
 */

#include "tcprot.h"

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

int main(int argc, char **argv)
{
	struct pair pair;
	int i;
	char buf[1024];

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGTERM, sighandler);
	atexit(cleanup);

	FD_ZERO(&fdset);
	for (i = 0; i < MAX_TUNNELS; i++) {
		tunpairs[i].in_sock = -1;
		tunpairs[i].out_sock = -1;
	}
	
	serv_sock = tcprot_bind_listen(DEFAULT_INPORT);
	if (serv_sock < 0) {
		exit(serv_sock);
	}

	FD_SET(serv_sock, &fdset);

	for (;;) {
		select(FD_SETSIZE, &fdset, NULL, NULL, NULL);

		if (FD_ISSET(serv_sock, &fdset)) {
			if (tcprot_accept(serv_sock, &pair,
					  DEFAULT_OUTHOST, DEFAULT_OUTPORT) == 0) {
				i = tcprot_find_free_pair(tunpairs, MAX_TUNNELS);
				if (i < 0 || i >= MAX_TUNNELS) {
					fprintf(stderr, "no free tunnel left!\n");
					close(pair.in_sock);
					close(pair.out_sock);
				} else {
					tunpairs[i].in_sock = pair.in_sock;
					tunpairs[i].out_sock = pair.out_sock;
					FD_SET(pair.in_sock, &fdset);
					//FD_SET(pair.out_sock, &fdset);
				}
			}
		}

		for (i = 0; i < MAX_TUNNELS; i++) {
			int in_sock = tunpairs[i].in_sock;
			int out_sock = tunpairs[i].out_sock;
			int count;
			if (in_sock >= 0 && FD_ISSET(in_sock, &fdset)) {
				count = read(in_sock, buf, sizeof(buf));
				fprintf(stderr, "%d: %s\n", count, buf);
				if (count < 0) {
					close(in_sock);
					close(out_sock);
				}
			}
		}
	}

	return 0;
}
