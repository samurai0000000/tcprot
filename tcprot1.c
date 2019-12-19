/*
 * tcprot1.c
 */

#include "tcprot.h"

#define DEFAULT_INPORT		42022
#define DEFAULT_OUTPORT		42021
#define DEFAULT_OUTHOST		"localhost"

static int serv_sock = -1;

static void sighandler(int signal)
{
	fprintf(stderr, "caught signal %d, exiting\n", signal);
	exit(0);
}

static void cleanup(void)
{
	close(serv_sock);
	fprintf(stderr, "goodbye!\n");
}

int main(int argc, char **argv)
{
	struct pair pair;

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGTERM, sighandler);
	atexit(cleanup);

	serv_sock = tcprot_bind_listen(DEFAULT_INPORT);
	if (serv_sock < 0) {
		exit(serv_sock);
	}

	for (;;) {
		tcprot_accept(serv_sock, &pair, DEFAULT_OUTHOST, DEFAULT_OUTPORT);
	}

	return 0;
}
