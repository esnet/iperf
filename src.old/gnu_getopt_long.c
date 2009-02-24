/* gnu_getopt_long and getopt_long_only entry points for GNU getopt.
   Copyright (C) 1987,88,89,90,91,92,93,94,96,97 Free Software Foundation, Inc.

   This file is part of the GNU C Library.  Its master source is NOT part of
   the C library, however.  The master source lives in /gd/gnu/lib.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/*
 * modified July 9, 1999 by mark gates <mgates@nlanr.net>
 *          Dec 17, 1999
 *
 * renamed all functions and variables by prepending "gnu_"
 * removed/redid a bunch of stuff under the assumption we're
 *   using a modern standard C compiler.
 * renamed file to gnu_getopt_long.c (from gnu_getopt1.c)
 *
 * $Id: gnu_getopt_long.c,v 1.1.1.1 2004/05/18 01:50:44 kgibbs Exp $
 */



#include "gnu_getopt.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int
gnu_getopt_long( int argc,
                 char *const *argv,
                 const char *options,
                 const struct option *long_options,
                 int *opt_index ) {
    return _gnu_getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

/* Like gnu_getopt_long, but '-' as well as '--' can indicate a long option.
   If an option that starts with '-' (not '--') doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead.  */

int
gnu_getopt_long_only( int argc,
                      char *const *argv,
                      const char *options,
                      const struct option *long_options,
                      int *opt_index ) {
    return _gnu_getopt_internal (argc, argv, options, long_options, opt_index, 1);
}

#ifdef __cplusplus
} /* end extern "C" */
#endif


#ifdef TEST

    #include <stdio.h>

int
main (argc, argv)
int argc;
char **argv;
{
int c;
int digit_optind = 0;

while ( 1 ) {
    int this_option_optind = gnu_optind ? gnu_optind : 1;
    int option_index = 0;
    static struct option long_options[] =
    {
        {"add", 1, 0, 0},
        {"append", 0, 0, 0},
        {"delete", 1, 0, 0},
        {"verbose", 0, 0, 0},
        {"create", 0, 0, 0},
        {"file", 1, 0, 0},
        {0, 0, 0, 0}
    };

    c = gnu_getopt_long (argc, argv, "abc:d:0123456789",
                         long_options, &option_index);
    if ( c == -1 )
        break;

    switch ( c ) {
        case 0:
            fprintf ( stderr, "option %s", long_options[option_index].name);
            if ( gnu_optarg )
                fprintf ( stderr, " with arg %s", gnu_optarg);
            fprintf ( stderr, "\n");
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if ( digit_optind != 0 && digit_optind != this_option_optind )
                fprintf ( stderr, "digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            fprintf ( stderr, "option %c\n", c);
            break;

        case 'a':
            fprintf ( stderr, "option a\n");
            break;

        case 'b':
            fprintf ( stderr, "option b\n");
            break;

        case 'c':
            fprintf ( stderr, "option c with value `%s'\n", gnu_optarg);
            break;

        case 'd':
            fprintf ( stderr, "option d with value `%s'\n", gnu_optarg);
            break;

        case '?':
            break;

        default:
            fprintf ( stderr, "?? gnu_getopt returned character code 0%o ??\n", c);
    }
}

if ( gnu_optind < argc ) {
    fprintf ( stderr, "non-option ARGV-elements: ");
    while ( gnu_optind < argc )
        fprintf ( stderr, "%s ", argv[gnu_optind++]);
    fprintf ( stderr, "\n");
}

exit (0);
}

#endif /* TEST */
