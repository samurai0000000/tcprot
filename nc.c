/*
 * nc.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include "tcptun.h"

static int G_ncinit = 0;
static char G_title[256] = "*** TCPTUN ***";
static char G_status[256] = "";
static time_t G_last_sec = 0;

void nc_init(void)
{
    initscr();
    G_ncinit = 1;
}

void nc_set_title(const char *title)
{
    if (title == NULL)
        return;

    strncpy(G_title, title, sizeof(G_title) - 1);
}

void nc_log(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    if (G_ncinit == 0) {
        vfprintf(stderr, format, ap);
    } else {
        vsnprintf(G_status, sizeof(G_status) - 1, format, ap);
    }
    va_end(ap);
}

void nc_refresh(const struct pair pairs[], unsigned int npairs)
{
    unsigned int i, instance = 0;
    char timestr[64];
    char instr[64];
    struct timeval timeval;
    time_t conntime, secs, mins, hours, days;

    if (G_ncinit == 0)
        return;

    gettimeofday(&timeval, NULL);

    clear();
    mvprintw(0, 0, G_title);
    mvhline(1, 0, '-', COLS);
    mvprintw(LINES - 1, 0, G_status);

    if (pairs == NULL)
        goto done;

    for (i = 0; i < npairs; i++) {
        if (pairs[i].in_sock <= 0)
            continue;

        conntime = timeval.tv_sec - pairs[i].tod_sec;
        secs = conntime % 60;
        mins = conntime / 60;
        mins %= 60;
        hours = conntime / 3600;
        hours %= 60;
        days = conntime / 86400;

        if (days > 0) {
            sprintf(timestr, "%lud:%.2lu:%.2lu:%.2lu",
                    days, hours, mins, secs);
        } else {
            sprintf(timestr, "%.2lu:%.2lu:%.2lu", hours, mins, secs);
        }
        strcpy(instr, inet_ntoa(pairs[i].in_addr.sin_addr));
        mvprintw(2 + instance, 0,
                 "%2d %s %s %llu/%llu",
                 instance,
                 timestr,
                 instr,
                 pairs[i].inbytes,
                 pairs[i].outbytes);
        instance++;
    }

done:

    if (timeval.tv_sec != G_last_sec) {
        refresh();
        G_last_sec = timeval.tv_sec;
    }
}

void nc_cleanup(void)
{
    if (G_ncinit) {
        endwin();
        G_ncinit = 0;
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
