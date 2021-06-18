/*
 * tcptunstat.c
 *
 * Copyright (C) 2019, Charles Chiou
 */

#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include "tcptun.h"

int main(int argc, char **argv)
{
    int shmid;

    shmid = shmget((key_t) 0, sizeof(struct tcptunstat), 0644);
    if (shmid < 0) {
        fprintf(stdout, "inactive\n");
    } else {
    }

    return 0;
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
