#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include<string.h>

#if defined(HAVE_UUID_H)
#warning DOING SOMETHING
#include <uuid.h>
#elif defined(HAVE_UUID_UUID_H)
#include <uuid/uuid.h>
#else
#error No uuid header file specified
#endif


/* XXX: this code is not portable: not all versions of linux install libuuidgen
	by default
 * if not installed, may need to do something like this:
 *   yum install libuuid-devel
 *   apt-get install apt-get install
*/


void
get_uuid(char *temp)
{
    char     *s;
    uuid_t    uu;

#if defined(HAVE_UUID_CREATE)
    uuid_create(&uu, NULL);
    uuid_to_string(&uu, &s, 0);
#elif defined(HAVE_UUID_GENERATE)
    s = (char *) malloc(37);
    uuid_generate(uu);
    uuid_unparse(uu, s);
#else
#error No uuid function specified
#endif
    memcpy(temp, s, 37);
}
