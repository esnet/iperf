/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003
 * The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software (Iperf) and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the names of the University of Illinois, NCSA,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 * National Laboratory for Applied Network Research
 * National Center for Supercomputing Applications
 * University of Illinois at Urbana-Champaign
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________
 *
 * tcp_window_size.c
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * set/getsockopt
 * ------------------------------------------------------------------- */

/*
 * imported into iperfjd branch: 3 Feb 2009 jdugan
 *
 * made variable names more sane
 * removed some cruft
 */

#include <stdio.h>
#include <sys/socket.h>
#include <assert.h>

/* -------------------------------------------------------------------
 * If bufsize > 0, set the TCP window size (via the socket buffer
 * sizes) for sock. Otherwise leave it as the system default.
 *
 * This must be called prior to calling listen() or connect() on
 * the socket, for TCP window sizes > 64 KB to be effective.
 *
 * This now works on UNICOS also, by setting TCP_WINSHIFT.
 * This now works on AIX, by enabling RFC1323.
 * returns -1 on error, 0 on no error.
 * -------------------------------------------------------------------
 */

int 
set_tcp_windowsize(int sock, int bufsize, int dir)
{
    int       rc;
    int       newbufsize;

    assert(sock >= 0);

    if (bufsize > 0)
    {
	/*
         * note: results are verified after connect() or listen(), since
         * some OS's don't show the corrected value until then.
         */
//        printf("Setting TCP buffer to size: %d\n", bufsize);
	newbufsize = bufsize;
	rc = setsockopt(sock, SOL_SOCKET, dir, (char *) &newbufsize, sizeof(newbufsize));
	if (rc < 0)
	    return rc;
    } else {
//        printf("      Using default TCP buffer size and assuming OS will do autotuning\n");
    }

    return 0;
}

/* -------------------------------------------------------------------
 * returns the TCP window size (on the sending buffer, SO_SNDBUF),
 * or -1 on error.
 * ------------------------------------------------------------------- */

int
get_tcp_windowsize(int sock, int dir)
{
    int       bufsize = 0;

    int       rc;
    socklen_t len;

    /* send buffer -- query for buffer size */
    len = sizeof bufsize;
    rc = getsockopt(sock, SOL_SOCKET, dir, (char *) &bufsize, &len);

    if (rc < 0)
	return rc;

    return bufsize;
}
