#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iperf_util.h"

int
main(int argc, char **argv)
{
    char     uuid[37];
    get_uuid(uuid);
    if (strlen(uuid) != 36)
    {
	printf("uuid is not 37 characters long %s.\n", uuid);
	exit(-1);
    }
    exit(0);
}
