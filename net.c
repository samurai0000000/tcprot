/*
 * net.c
 */

#include "tcprot.h"

int tcprot_find_free_pair(struct pair *pair_set, unsigned int pair_count)
{
	int i = 0;

	for (i = 0; i < pair_count; i++) {
		if (pair_set[i].in_sock < 0 &&
		    pair_set[i].out_sock < 0)
			return i;
	}

	return -1;
}

int tcprot_bind_listen(uint16_t port)
{
	int sock;
	int val, rval;
	struct sockaddr_in serveraddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(stderr, "failed to create socket\n");
		goto done;
	}

	val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &val, sizeof(val));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);

	rval = bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if (rval < 0) {
		fprintf(stderr, "failed in bind(%d)\n", port);
		close(sock);
		sock = -1;
		goto done;
	}

	rval = listen(sock, 5);
	if (rval < 0) {
		fprintf(stderr, "failed in listen()\n");
		close(sock);
		sock = -1;
		goto done;
	}

	fprintf(stderr, "listening to %d\n", port);

done:

	return sock;
}

int tcprot_accept(int sock, struct pair *pair, const char *outhost, uint16_t outport)
{
	char *hostaddrp;
	socklen_t len;

	pair->in_sock = -1;
	pair->out_sock = -1;

	/* Accept an incoming socket connection */
	len = sizeof(pair->in_addr);
	pair->in_sock = accept(sock, (struct sockaddr *) &pair->in_addr, &len);
	if (pair->in_sock < 0) {
		fprintf(stderr, "failed in accept()!\n");
		goto done;
	}

	hostaddrp = inet_ntoa(pair->in_addr.sin_addr);
	if (hostaddrp == NULL) {
		fprintf(stderr, "failed in inet_ntoa()!\n");
		close(pair->in_sock);
		pair->in_sock = -1;
		goto done;
	}

	fprintf(stderr, "incoming from %s\n", hostaddrp);

	memset(&pair->out_addr, 0x0, sizeof(pair->out_addr));
	pair->out_addr.sin_family = AF_INET;
	if (inet_aton(outhost, &pair->out_addr.sin_addr) == 0) {
		fprintf(stderr, "invalid outgoing host '%s'\n", outhost);
		goto done;
	}
	
	/* Make an outgoing socket connection */
	pair->out_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (pair->out_sock < 0) {
		fprintf(stderr, "failed in socket()!\n");
		goto done;
	}

	pair->out_addr.sin_addr.s_addr = inet_addr(outhost);
	pair->out_addr.sin_port = htons(outport);

	if (connect(pair->out_sock, (const struct sockaddr *) &pair->out_addr,
		    sizeof(pair->out_addr)) != 0) {
		perror("connect");
		fprintf(stderr, "outgoing to %s:%d failed!\n", outhost, outport);
		close(pair->out_sock);
		pair->out_sock = -1;
		goto done;
	}

	fprintf(stderr, "outgoing to %s:%d\n", outhost, outport);

done:

	if (pair->in_sock >= 0 && pair->out_sock >= 0) {
		return 0;
	}

	if (pair->in_sock >= 0) {
		close(pair->in_sock);
		pair->in_sock = -1;
	}

	if (pair->out_sock >= 0) {
		close(pair->out_sock);
		pair->out_sock = -1;
	}

	return -1;
}
