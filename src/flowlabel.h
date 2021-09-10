/*
 * iperf, Copyright (c) 2014, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#ifndef        __FLOW_LABEL_H
#define        __FLOW_LABEL_H


#include <linux/types.h>

/*
   It is just a stripped copy of the Linux kernel header "linux/in6.h"
   "Flow label" things are still not defined in "netinet/in*.h" headers,
   but we cannot use "linux/in6.h" immediately because it currently
   conflicts with "netinet/in.h" .
*/

#ifndef __ANDROID__
struct in6_flowlabel_req
{
    struct in6_addr flr_dst;
    __u32   flr_label;
    __u8    flr_action;
    __u8    flr_share;
    __u16   flr_flags;
    __u16   flr_expires;
    __u16   flr_linger;
    __u32   __flr_pad;
    /* Options in format of IPV6_PKTOPTIONS */
};
#endif

#define IPV6_FL_A_GET           0
#define IPV6_FL_A_PUT           1
#define IPV6_FL_A_RENEW         2

#define IPV6_FL_F_CREATE        1
#define IPV6_FL_F_EXCL          2

#define IPV6_FL_S_NONE          0
#define IPV6_FL_S_EXCL          1
#define IPV6_FL_S_PROCESS       2
#define IPV6_FL_S_USER          3
#define IPV6_FL_S_ANY           255

#define IPV6_FLOWINFO_FLOWLABEL 0x000fffff
#define IPV6_FLOWINFO_PRIORITY  0x0ff00000

#define IPV6_FLOWLABEL_MGR      32
#define IPV6_FLOWINFO_SEND      33

#endif
