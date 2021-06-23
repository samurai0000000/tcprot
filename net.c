/*
 * net.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include "tcptun.h"

int tcptun_find_free_pair(struct pair *pair_set, unsigned int pair_count)
{
    int i = 0;

    for (i = 0; i < pair_count; i++) {
        if (pair_set[i].in_sock < 0 &&
            pair_set[i].out_sock < 0)
            return i;
    }

    return -1;
}

void tcptun_terminate_pair(struct pair *pair)
{
    char instr[64];
    char outstr[64];
    char *s;

    s = inet_ntoa(pair->in_addr.sin_addr);
    strcpy(instr, s);
    s = inet_ntoa(pair->out_addr.sin_addr);
    strcpy(outstr, s);

    nc_log("terminated  %s:%d -> %s:%d\n",
           instr, ntohs(pair->in_addr.sin_port),
           outstr, ntohs(pair->out_addr.sin_port));

    close(pair->in_sock);
    pair->in_sock = -1;

    close(pair->out_sock);
    pair->out_sock = -1;
}

static void tcptun_nslookup(char *newname, const char *hostname)
{
    struct hostent *hp;

    hp = gethostbyname(hostname);
    if (hp) {
        char *newhostname = NULL;
        if (hp->h_addr_list[0] != NULL)
            newhostname = inet_ntoa(*( struct in_addr *)
                                    hp->h_addr_list[0]);
        if (newhostname) {
            strcpy(newname, newhostname);
            return;
        }
    }

    strcpy(newname, hostname);
}

int tcptun_bind_listen(uint16_t port)
{
    int sock;
    int val, rval;
    struct sockaddr_in serveraddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        nc_log("failed to create socket\n");
        goto done;
    }

    val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    rval = bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if (rval < 0) {
        nc_log("failed in bind(%d)\n", port);
        close(sock);
        sock = -1;
        goto done;
    }

    rval = listen(sock, 5);
    if (rval < 0) {
        nc_log("failed in listen()\n");
        close(sock);
        sock = -1;
        goto done;
    }

    nc_log("listening to %d\n", port);

done:

    return sock;
}

int tcptun_accept(int sock, struct pair *pair,
                  const char *outhost, uint16_t outport)
{
    char hostname[128];
    char *hostaddrp;
    char instr[64];
    socklen_t len;
    int rv, flags, val;
    struct timeval timeval;

    gettimeofday(&timeval, NULL);

    memset(pair, 0x0, sizeof(*pair));
    pair->in_sock = -1;
    pair->out_sock = -1;
    pair->tod_sec = timeval.tv_sec;

	/* Accept an incoming socket connection */
    len = sizeof(pair->in_addr);
    pair->in_sock = accept(sock, (struct sockaddr *) &pair->in_addr, &len);
    if (pair->in_sock < 0) {
        nc_log("failed in accept()!\n");
        goto done;
    }

    /* Get ip address of incoming socket */
    hostaddrp = inet_ntoa(pair->in_addr.sin_addr);
    if (hostaddrp == NULL) {
        nc_log("failed in inet_ntoa()!\n");
        close(pair->in_sock);
        pair->in_sock = -1;
        goto done;
    } else {
        strcpy(instr, hostaddrp);
    }

    /* Get socket flags */
    flags = fcntl(pair->in_sock, F_GETFL);
    if (flags < 0) {
        nc_log("F_GETFL failed %d!\n", flags);
        close(pair->in_sock);
        pair->in_sock = -1;
        goto done;
    }

#if 0
    /* Set socket to non-blocking */
    rv = fcntl(pair->in_sock, F_SETFL, flags | O_NONBLOCK);
    if (rv != 0) {
        nc_log("F_SETFL failed %d!\n", rv, errno);
        close(pair->in_sock);
        pair->in_sock = -1;
        goto done;
    }
#endif

    /* Resolve name of outgoing host to ip address */
    tcptun_nslookup(hostname, outhost);
    memset(&pair->out_addr, 0x0, sizeof(pair->out_addr));
    pair->out_addr.sin_family = AF_INET;
    if (inet_aton(hostname, &pair->out_addr.sin_addr) == 0) {
        nc_log("invalid outgoing host '%s'\n", hostname);
        goto done;
    }

    /* Make an outgoing socket connection */
    pair->out_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (pair->out_sock < 0) {
        nc_log("failed in socket()!\n");
        goto done;
    }

    /* Set socket options */
    val = 1;
    rv = setsockopt(pair->out_sock, SOL_SOCKET, SO_KEEPALIVE,
                    &val, sizeof(val));
    if (rv < 0) {
        nc_log("SOL_SOCKET failed!\n");
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }

#if 0
    rv = setsockopt(pair->out_sock, IPPROTO_TCP, TCP_NODELAY,
                    &val, sizeof(val));
    if (rv < 0) {
        nc_log("SOL_SOCKET failed!\n");
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }
#endif

    pair->out_addr.sin_addr.s_addr = inet_addr(hostname);
    pair->out_addr.sin_port = htons(outport);

    if (connect(pair->out_sock, (const struct sockaddr *) &pair->out_addr,
                sizeof(pair->out_addr)) != 0) {
        perror("connect");
        nc_log("outgoing to %s:%d failed!\n", hostname, outport);
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }

    /* Get socket flags */
    flags = fcntl(pair->out_sock, F_GETFL);
    if (flags < 0) {
        nc_log("F_GETFL failed %d!\n", flags);
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }

#if 0
    /* Set socket to non-blocking */
    rv = fcntl(pair->out_sock, F_SETFL, flags | O_NONBLOCK);
    if (rv != 0) {
        nc_log("F_SETFL failed %d!\n", rv, errno);
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }
#endif

    nc_log("established %s:%d -> %s:%d\n",
           instr, ntohs(pair->in_addr.sin_port),
           hostname, ntohs(pair->out_addr.sin_port));

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

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
