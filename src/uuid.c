#include <stdio.h>
#include <stdlib.h>

#if defined(__FreeBSD__)
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif

char *
get_uuid()
{
    char *s;
    uuid_t uu;

#if defined(__FreeBSD__)
    uuid_create(&uu, NULL);
    uuid_to_string(&uu, &s, 0);
#else
    s = (char *) malloc(37); /* UUID is 36 chars + \0 */
    uuid_generate(uu);
    uuid_unparse(uu, s);
#endif

    return s;
}
