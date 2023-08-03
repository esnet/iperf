iperf3:  A TCP, UDP, and SCTP network bandwidth measurement tool
================================================================

Summary
-------

iperf is a tool for active measurements of the maximum achievable
bandwidth on IP networks.  It supports tuning of various parameters
related to timing, protocols, and buffers.  For each test it reports
the measured throughput / bitrate, loss, and other parameters.

This version, sometimes referred to as iperf3, is a redesign of an
original version developed at NLANR/DAST.  iperf3 is a new
implementation from scratch, with the goal of a smaller, simpler code
base, and a library version of the functionality that can be used in
other programs. iperf3 also has a number of features found in other tools
such as nuttcp and netperf, but were missing from the original iperf.
These include, for example, a zero-copy mode and optional JSON output.
Note that iperf3 is *not* backwards compatible with the original iperf.

Primary development for iperf3 takes place on Ubuntu Linux, FreeBSD,
and macOS.  At this time, these are the only officially supported
platforms, however there have been some reports of success with
OpenBSD, NetBSD, Android, Solaris, and other Linux distributions.

iperf3 is principally developed by ESnet / Lawrence Berkeley National
Laboratory.  It is released under a three-clause BSD license.

For more information see: https://software.es.net/iperf

Source code and issue tracker: https://github.com/esnet/iperf

Discussion forums: https://github.com/esnet/iperf/discussions

Obtaining iperf3
----------------

Downloads of iperf3 are available at:

    https://downloads.es.net/pub/iperf/

To check out the most recent code, clone the git repository at:

    https://github.com/esnet/iperf.git

Building iperf3
---------------

### Prerequisites: ###

None.

### Building ###

    ./configure; make; make install

(Note: If configure fails, try running `./bootstrap.sh` first)

Invoking iperf3
---------------

iperf3 includes a manual page listing all of the command-line options.
The manual page is the most up-to-date reference to the various flags and parameters.

For sample command line usage, see:

https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf/

Using the default options, iperf is meant to show typical well
designed application performance.  "Typical well designed application"
means avoiding artificial enhancements that work only for testing
(such as splice()'ing the data to /dev/null).  iperf does also have
flags for "extreme best case" optimizations, but they must be
explicitly activated.

These flags include:

    -Z, --zerocopy            use a 'zero copy' sendfile() method of sending data
    -A, --affinity n/n,m      set CPU affinity

Bug and Security Reports
------------------------

Before submitting a bug report, please make sure you're running the
latest version of the code, and confirm that your issue has not
already been fixed.  Then submit to the iperf3 issue tracker on
GitHub:

https://github.com/esnet/iperf/issues

In your issue submission, please indicate the version of iperf3 and
what platform you're trying to run on (provide the platform
information even if you're not using a supported platform, we
*might* be able to help anyway).  Exact command-line arguments will
help us recreate your problem.  If you're getting error messages,
please include them verbatim if possible, but remember to sanitize any
sensitive information.

If you have a question about usage or about the code, please do *not*
submit an issue.  Please use one of the mailing lists for that.

If you suspect there is a potential security issue, please contact the
developers at:

iperf@es.net

Relation to iperf 2.x
---------------------

Although iperf2 and iperf3 both measure network performance,
they are not compatible with each other.
The projects (as of mid-2021) are in active, but separate, development.
The continuing iperf2 development
project can be found at https://sourceforge.net/projects/iperf2/.

iperf3 contains a number of options and functions not present in
iperf2.  In addition, some flags are changed from their iperf2
counterparts:

    -C, --linux-congestion    set congestion control algorithm (Linux only)
                              (-Z in iperf2)
    --bidir                   bidirectional testing mode
                              (-d in iperf2)

Some iperf2 options are not available in iperf3:

    -r, --tradeoff           Do a bidirectional test individually
    -T, --ttl                time-to-live, for multicast (default 1)
    -x, --reportexclude [CDMSV]   exclude C(connection) D(data) M(multicast)
                                  S(settings) V(server) reports
    -y, --reportstyle C      report as a Comma-Separated Values

Also removed is the ability to set the options via environment
variables.

Known Issues
------------

A set of known issues is maintained on the iperf3 Web pages:

https://software.es.net/iperf/dev.html#known-issues

Links
-----

This section lists links to user-contributed Web pages regarding
iperf3.  ESnet and Lawrence Berkeley National Laboratory bear no
responsibility for the content of these pages.

* Installation instructions for Debian Linux (by Cameron Camp
  <cameron@ivdatacenter.com>):

  http://cheatsheet.logicalwebhost.com/iperf-network-testing/

Copyright
---------

iperf, Copyright (c) 2014-2023, The Regents of the University of
California, through Lawrence Berkeley National Laboratory (subject
to receipt of any required approvals from the U.S. Dept. of
Energy).  All rights reserved.

If you have questions about your rights to use or distribute this
software, please contact Berkeley Lab's Technology Transfer
Department at TTD@lbl.gov.

NOTICE.  This software is owned by the U.S. Department of Energy.
As such, the U.S. Government has been granted for itself and others
acting on its behalf a paid-up, nonexclusive, irrevocable,
worldwide license in the Software to reproduce, prepare derivative
works, and perform publicly and display publicly.  Beginning five
(5) years after the date permission to assert copyright is obtained
from the U.S. Department of Energy, and subject to any subsequent
five (5) year renewals, the U.S. Government is granted for itself
and others acting on its behalf a paid-up, nonexclusive,
irrevocable, worldwide license in the Software to reproduce,
prepare derivative works, distribute copies to the public, perform
publicly and display publicly, and to permit others to do so.

This code is distributed under a BSD style license, see the LICENSE
file for complete information.
