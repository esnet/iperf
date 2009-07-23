#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uuid.h"

int
main(int argc, char **argv)
{
    char *uuid;
    uuid = get_uuid();
    if(strlen(uuid) != 36)
    {
        printf("uuid is not 37 characters long %s.\n", uuid);
        exit(-1);
    }
    exit(0);
}
