/*
 * Copyright (c) 2009-2011, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <assert.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>

#include "iperf.h"
#include "units.h"

int 
main(int argc, char **argv)
{
    iperf_size_t llu;
    double    d;
    char      s[11];

    assert(1024.0 * 0.5 == unit_atof("0.5K"));
    assert(1024.0 == unit_atof("1K"));
    assert(1024.0 * 1024.0 == unit_atof("1M"));
    assert(4.0 * 1024.0 * 1024.0 * 1024.0 == unit_atof("4G"));

#ifdef notdef
    /* Obsolete - we no longer make a distinction between upper and lower
    ** case.
    */
    assert(1000.0 * 0.5 == unit_atof("0.5k"));
    assert(1000.0 == unit_atof("1k"));
    assert(1000.0 * 1000.0 == unit_atof("1m"));
    assert(4.0 * 1000.0 * 1000.0 * 1000.0 == unit_atof("4g"));
#endif
    assert(1024.0 * 0.5 == unit_atof("0.5k"));
    assert(1024.0 == unit_atof("1k"));
    assert(1024.0 * 1024.0 == unit_atof("1m"));
    assert(4.0 * 1024.0 * 1024.0 * 1024.0 == unit_atof("4g"));

    assert(1024 * 0.5 == unit_atoi("0.5K"));
    assert(1024 == unit_atoi("1K"));
    assert(1024 * 1024 == unit_atoi("1M"));
    d = 4.0 * 1024 * 1024 * 1024;
    llu = (iperf_size_t) d;
    assert(llu == unit_atoi("4G"));

#ifdef notdef
    /* Also obsolete. */
    assert(1000 * 0.5 == unit_atoi("0.5k"));
    assert(1000 == unit_atoi("1k"));
    assert(1000 * 1000 == unit_atoi("1m"));
    d = 4.0 * 1000 * 1000 * 1000;
    llu = (iperf_size_t) d;
    assert(llu == unit_atoi("4g"));
#endif
    assert(1024 * 0.5 == unit_atoi("0.5k"));
    assert(1024 == unit_atoi("1k"));
    assert(1024 * 1024 == unit_atoi("1m"));
    d = 4.0 * 1024 * 1024 * 1024;
    llu = (iperf_size_t) d;
    assert(llu == unit_atoi("4g"));

    unit_snprintf(s, 11, 1024.0, 'A');
    assert(strncmp(s, "1.00 KByte", 11) == 0);

    unit_snprintf(s, 11, 1024.0 * 1024.0, 'A');
    assert(strncmp(s, "1.00 MByte", 11) == 0);

    unit_snprintf(s, 11, 1000.0, 'k');
    assert(strncmp(s, "8.00 Kbit", 11) == 0);

    unit_snprintf(s, 11, 1000.0 * 1000.0, 'a');
    assert(strncmp(s, "8.00 Mbit", 11) == 0);

    d = 4.0 * 1024 * 1024 * 1024;
    unit_snprintf(s, 11, d, 'A');
    assert(strncmp(s, "4.00 GByte", 11) == 0);

    unit_snprintf(s, 11, d, 'a');
    assert(strncmp(s, "34.4 Gbit", 11) == 0);

    return 0;
}
