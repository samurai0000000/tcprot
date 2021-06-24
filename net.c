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

static int tcptun_setsockopt(int sock)
{
    int rv = 0;
    int val;

    /* Set TCP_NODELAY */
    val = 1;
    rv = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if (rv != 0) {
        nc_log("TCP_NODELAY failed!\n");
        goto done;
    }

    /* Set SO_KEEP_ALIVE */
    val = 1;
    rv = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
    if (rv != 0) {
        nc_log("SO_KEEPALIVE failed!\n");
        goto done;
    }

    /* Set TCP_KEEPIDLE */
    val = 10;
    rv = setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val));
    if (rv != 0) {
        nc_log("SO_KEEPIDLE failed!\n");
        goto done;
    }

    /* Set TCP_KEEPCNT */
    val = 5;
    rv = setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val));
    if (rv != 0) {
        nc_log("SO_KEEPCNT failed!\n");
        goto done;
    }

    /* Set TCP_KEEPINTV */
    val = 5;
    rv = setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val));
    if (rv != 0) {
        nc_log("SO_KEEPINTVL failed!\n");
        goto done;
    }

done:

    return rv;
}

int tcptun_accept(int sock, struct pair *pair,
                  const char *outhost, uint16_t outport)
{
    char hostname[128];
    char *hostaddrp;
    char instr[64];
    socklen_t len;
    int rv, flags;
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

#if (TCP_NONBLOCKING != 0)
    /* Set socket to non-blocking */
    rv = fcntl(pair->in_sock, F_SETFL, flags | O_NONBLOCK);
    if (rv != 0) {
        nc_log("F_SETFL failed %d!\n", rv, errno);
        close(pair->in_sock);
        pair->in_sock = -1;
        goto done;
    }
#endif

    /* Set desired socket properties */
    rv = tcptun_setsockopt(pair->in_sock);
    if (rv != 0) {
        close(pair->in_sock);
        pair->in_sock = -1;
        goto done;
    }

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

    /* Record outgoing ip/port */
    pair->out_addr.sin_addr.s_addr = inet_addr(hostname);
    pair->out_addr.sin_port = htons(outport);

    /* Connect to destination peer */
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

#if (TCP_NONBLOCKING != 0)
    /* Set socket to non-blocking */
    rv = fcntl(pair->out_sock, F_SETFL, flags | O_NONBLOCK);
    if (rv != 0) {
        nc_log("F_SETFL failed %d!\n", rv, errno);
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }
#endif

    /* Set desired socket properties */
    rv = tcptun_setsockopt(pair->out_sock);
    if (rv != 0) {
        close(pair->out_sock);
        pair->out_sock = -1;
        goto done;
    }

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

void tcptun_incoming_process(struct pair *pair)
{
    char buf[TCP_BUFFER_SIZE];
    ssize_t size;
    ssize_t wsize;
    int i;
    int rv, pending = 0;

    /* Get output buffer queue size */
    rv = ioctl(pair->in_sock, SIOCOUTQ, &pending);
    if (rv < 0) {
        nc_log("SIOCOUTQ failed %d!\n", rv);
        tcptun_terminate_pair(pair);
        goto done;
    }

    /* Back off if it's too high */
    if (pending > TCP_TQ_BACKOFF) {
        return;
    }

    /* Read data from socket */
    size = read(pair->in_sock, buf, sizeof(buf));
    if (size <= 0) {
        nc_log("broken incoming read %d\n", size);
        tcptun_terminate_pair(pair);
        goto done;
    }

    pair->inbytes += size;

    /* Transform */
    for (i = 0; i < size; i++) {
        buf[i] ^= 0x55;
    }

    /* Write data to socket */
    wsize = write(pair->out_sock, buf, size);
    if (wsize != size) {
        nc_log("broken incoming write %d != %d\n", wsize, size);
        tcptun_terminate_pair(pair);
        goto done;
    }

done:

    return;
}

void tcptun_outgoing_process(struct pair *pair)
{
    char buf[TCP_BUFFER_SIZE];
    ssize_t size;
    ssize_t wsize;
    int i;
    int rv, pending = 0;

    /* Get output buffer queue size */
    rv = ioctl(pair->in_sock, SIOCOUTQ, &pending);
    if (rv < 0) {
        nc_log("SIOCOUTQ failed %d!\n", rv);
        tcptun_terminate_pair(pair);
        goto done;
    }

    /* Back off if it's too high */
    if (pending > TCP_TQ_BACKOFF) {
        return;
    }

    /* Read data from socket */
    size = read(pair->out_sock, buf, sizeof(buf));
    if (size <= 0) {
        nc_log("broken outgoing read %d\n", size);
        tcptun_terminate_pair(pair);
        goto done;
    }

    /* Transform */
    for (i = 0; i < size; i++) {
        buf[i] ^= 0x55;
    }

    /* Write data to socket */
    wsize = write(pair->in_sock, buf, size);
    if (wsize != size) {
        nc_log("broken outgoing write %d != %d\n", wsize, size);
        tcptun_terminate_pair(pair);
        goto done;
    }

    pair->outbytes += size;

done:

    return;
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
