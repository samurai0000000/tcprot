/*
 * nc.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include "tcptun.h"

static char G_title[256] = "*** TCPTUN ***";
static time_t last_sec = 0;

void nc_set_title(const char *title)
{
    if (title == NULL)
        return;

    strncpy(G_title, title, sizeof(G_title) - 1);
}

void nc_refresh(const struct pair pairs[], unsigned int npairs)
{
    unsigned int i, instance = 0;
    char timestr[64];
    char instr[64];
    struct timeval timeval;
    time_t conntime = 0, secs = 0, mins = 0, hours = 0, days = 0;

    gettimeofday(&timeval, NULL);

    clear();
    mvprintw(0, 0, "%s\n", G_title);
    mvprintw(1, 0, "-------------------------------------------------------\n");

    if (pairs == NULL)
        goto done;

    for (i = 0; i < npairs; i++) {
        if (pairs[i].in_sock <= 0)
            continue;

        conntime = timeval.tv_sec - pairs[i].tod_sec;
        secs = conntime % 60;
        if (conntime > 60) {
            mins = conntime / 60;
            mins %= 60;
        }
        if (conntime > 3600) {
            hours = conntime / 3600;
            hours %= 60;
        }
        if (conntime > 86400) {
            days = conntime / 86400;
        }

        if (days > 0) {
            sprintf(timestr, "%lud:%.2lu:%.2lu:%.2lu", days, hours, mins, secs);
        } else {
            sprintf(timestr, "%.2lu:%.2lu:%.2lu", hours, mins, secs);
        }
        strcpy(instr, inet_ntoa(pairs[i].in_addr.sin_addr));
        mvprintw(2 + instance, 0,
                 "%2d %s %s %llu/%llu\n",
                 instance,
                 timestr,
                 instr,
                 pairs[i].inbytes,
                 pairs[i].outbytes);
        instance++;
    }

done:

    if (timeval.tv_sec != last_sec) {
        refresh();
        last_sec = timeval.tv_sec;
    }
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
