/*
 * nc.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include "tcptun.h"

void nc_refresh(const struct pair pairs[], unsigned int npairs)
{
    unsigned int i;
    char instr[64];

    clear();

    if (pairs == NULL)
        goto done;

    for (i = 0; i < npairs; i++) {
        if (pairs[i].in_sock <= 0)
            continue;

        strcpy(instr, inet_ntoa(pairs[i].in_addr.sin_addr));
        printw("%s %llu/%llu\n",
               instr,
               pairs[i].inbytes,
               pairs[i].outbytes);
    }

done:

    refresh();
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
