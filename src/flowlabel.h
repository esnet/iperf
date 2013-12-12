/*      
 * Copyright (c) 2009-2011, The Regents of the University of California, 
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.                                                                                 
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
