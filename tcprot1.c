#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DEFAULT_INPORT		42020
#define DEFAULT_OUTPPORT	42021

int main(int argc, char **argv)
{
	int sock;
	int val;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	uint16_t serverport = DEFAULT_INPORT;
	int rval;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		error(sock, errno, "Failed to open socket");

	val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (const void *) &val, sizeof(val));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(serverport);

	rval = bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if (rval < 0)
		error(rval, errno, "Failed in bind");

	rval = listen(sock, 5);
	if (rval < 0)
		error(rval, errno, "Failed to listen");

	return 0;
}
