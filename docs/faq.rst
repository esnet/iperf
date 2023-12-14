.. _faq:

iperf3 FAQ
==========

What is the history of iperf3, and what is the difference between iperf2 and iperf3?
  iperf2 was orphaned in the late 2000s at version 2.0.5, despite some
  known bugs and issues. After spending some time trying to fix
  iperf2's problems, ESnet decided by 2010 that a new, simpler tool
  was needed, and began development of iperf3. The goal was make the
  tool as simple as possible, so others could contribute to the code
  base. For this reason, it was decided to make the tool single
  threaded, and not worry about backwards compatibility with
  iperf2. Many of the feature requests for iperf3 came from the
  perfSONAR project (http://www.perfsonar.net).

  Then in 2014, Bob (Robert) McMahon from Broadcom restarted
  development of iperf2 (See
  https://sourceforge.net/projects/iperf2/). He fixed many of the
  problems with iperf2, and added a number of new features similar to
  iperf3. iperf2.0.8, released in 2015, made iperf2 a useful tool. iperf2's
  current development is focused is on using UDP for latency testing, as well
  as broad platform support.

  As of this writing (2017), both iperf2 and iperf3 are being actively
  (although independently) developed.  We recommend being familiar with
  both tools, and use whichever tool’s features best match your needs.

  A feature comparison of iperf2, iperf3, and nuttcp is available at:
  https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/throughput-tool-comparision/

iperf3 parallel stream performance is much less than iperf2. Why?
  iperf3 is single threaded, and iperf2 is multi-threaded. We
  recommend using iperf2 for parallel streams.
  If you want to use multiple iperf3 streams use the method described `here <https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf/multi-stream-iperf3/>`_.

I’m trying to use iperf3 on Windows, but having trouble. What should I do?
  iperf3 is not officially supported on Windows, but iperf2 is. We
  recommend you use iperf2.

  Some people are using Cygwin to run iperf3 in Windows, but not all
  options will work.  Some community-provided binaries of iperf3 for
  Windows exist.

How can I build a statically-linked executable of iperf3?
  There are a number of reasons for building an iperf3 executable with
  no dependencies on any shared libraries.  Unfortunately this isn't
  quite a straight-forward process.

  The steps below have nominally been tested on CentOS 7.4, but
  can probably be adapted for use with other Linux distributions:

  #.  If necessary, install the static C libraries; for CentOS this is
      the ``glibc-static`` package.

  #.  If OpenSSL is installed, be sure that its static libraries are
      also installed, from the ``openssl-static`` package.

  #.  Be sure that ``lksctp-*`` packages are not installed, because
      as of this writing, there do not appear to be any static
      libraries available for SCTP.

  #.  Configure iperf3 thusly: ``configure "LDFLAGS=--static"
      --disable-shared`` These options are necessary to disable the
      generation of shared libraries and link the executable
      statically.  For iperf-3.8 or later, configuring as ``configure
      --enable-static-bin`` is another, shorter way to accomplish
      this.  If SCTP is installed on the system it might also be
      necessary to pass the ``--without-sctp`` flag at configure
      time.

  #.  Compile as normal.

  It appears that for FreeBSD (tested on FreeBSD 11.1-RELEASE), only
  the last two steps are needed to produce a static executable.

How can I build on a system that doesn't support profiled executables?
  This problem has been noted by users attempting to build iperf3 for
  Android systems, as well as some recent versions of macOS.
  There are several workarounds. In order from least
  effort to most effort:

  #. Beginning with iperf-3.8, profiled executables are actually not
     built by default, so this question becomes somewhat moot.  Pass
     the ``--enable-profiling`` flag to ``configure`` to build
     profiled executables.

  #. In iperf-3.6 and iperf-3.7, the ``--disable-profiling`` flag can be
     passed to ``configure`` to disable the building of profiled
     object files and the profiled executable.

  #. At the time the linking of the iperf3 profiled executable fails,
     the "normal" iperf3 executable is probably already created. So if
     you are willing to accept the error exit from the make process
     (and a little bit of wasted work on the build host), you might
     not need to do anything.

  #. After the configure step, there will be a definition in
     ``src/Makefile`` that looks like this::

       noinst_PROGRAMS = t_timer$(EXEEXT) t_units$(EXEEXT) t_uuid$(EXEEXT) \
         iperf3_profile$(EXEEXT)

     If you edit it to look like this, it will disable the build of the profiled iperf3::

       noinst_PROGRAMS = t_timer$(EXEEXT) t_units$(EXEEXT) t_uuid$(EXEEXT)

  #. Similar to item 2 above, but more permanent...if you edit
     ``src/Makefile.am`` and change the line reading like this::

       noinst_PROGRAMS         = t_timer t_units t_uuid iperf3_profile

     To look like this::

       noinst_PROGRAMS         = t_timer t_units t_uuid

     And then run ``./bootstrap.sh``, that will regenerate the project
     Makefiles to make the exclusion of the profiled iperf3 executable
     permanent (within that source tree).

I'm seeing quite a bit of unexpected UDP loss. Why?
  First, confirm you are using iperf 3.1.5 or higher. There was an
  issue with the default UDP send size that was fixed in
  3.1.5. Second, try adding the flag ``-w2M`` to increase the socket
  buffer sizes. That seems to make a big difference on some hosts.

iperf3 UDP does not seem to work at bandwidths less than 100Kbps. Why?
  You'll need to reduce the default packet length to get UDP rates of less that 100Kbps. Try ``-l100``.

TCP throughput drops to (almost) zero during a test, what's going on?
  A drop in throughput to almost zero, except maybe for the first
  reported interval(s), may be related to problems in NIC TCP Offload,
  which is used to offload TCP functionality to the NIC (see
  https://en.wikipedia.org/wiki/TCP_offload_engine). The goal of TCP
  Offload is to save main CPU performance, mainly in the areas of
  segmentation and reassembly of large packets and checksum
  computation.

  When TCP packets are sent with the "Don't Fragment" flag set, which
  is the recommended setting, segmentation is done by the TCP stack
  based on the reported next hop MSS in the ICMP Fragmentation Needed
  message. With TCP Offload, active segmentation is done by the NIC on
  the sending side, which is known as TCP Segmentation offload (TSO)
  or in Windows as Large Send Offload (LSO). It seems that there are
  TSO/LSO implementations which for some reason ignore the reported
  MSS and therefore don’t perform segmentation. In these cases, when
  large packets are sent, e.g. the default iperf3 128KB (131,072
  bytes), iperf3 will show that data was sent in the first interval,
  but since the packets don’t get to the server, no ack is received
  and therefore no data is sent in the following intervals. It may
  happen that after certain timeout the main CPU will re-send the
  packet by re-segmenting it, and in these cases data will get to the
  server after a while. However, it seems that segmentation is not
  automatically continued with the next packet, so the data transfer
  rate be very low.

  The recommended solution in such a case is to disable TSO/LSO, at
  least on the relevant port. See for example:
  https://atomicit.ca/kb/articles/slow-network-speed-windows-10/. If
  that doesn’t help then "Don't Fragment" TCP flag may be
  disabled. See for example:
  https://support.microsoft.com/en-us/help/900926/recommended-tcp-ip-settings-for-wan-links-with-a-mtu-size-of-less-than. However,
  note that disabling the “Don’t Fragment” flag may cause other
  issues.

  To test whether TSO/LSO may be the problem, do the following:

  * If different machine configurations are used for the client and
    server, try the iperf3 reverse mode (``-R``). If TSO/LSO is only
    enabled on the client machine, this test should succeed.
  * Reduce the sending length to a small value that should not require
    segmentation, using the iperf3 ``-l`` option, e.g. ``-l 512``. It
    may also help to reduce the MTU by using the iperf3 ``-M`` option,
    e.g. ``-M 1460``.
  * Using tools like Wireshark, identify the required MSS in the ICMP
    Fragmentation Needed messages (if reported). Run tests with the
    ``-l`` value set to 2 times the MSS and then 4 times, 6 times,
    etc. With TSO/LSO issue in each test the throughput should be
    reduced more. It may help to increase the testing time beyond the
    default 10 seconds to better see the behavior (iperf3 ``-t``
    option).

What congestion control algorithms are supported?
  On Linux, run this command to see the available congestion control
  algorithms (note that some algorithms are packaged as kernel
  modules, which must be loaded before they can be used)::

    /sbin/sysctl net.ipv4.tcp_available_congestion_control

  On FreeBSD, the equivalent command is::

    /sbin/sysctl net.inet.tcp.cc.available

I’m using the ``--logfile`` option. How do I see file output in real time?
  Use the ``--forceflush`` flag.

I'm using the --fq-rate flag, but it does not seem to be working. Why?
  You need to add 'net.core.default_qdisc = fq' to /etc/sysctl.conf for that option to work.

I'm having trouble getting iperf3 to work on Windows, Android, etc. Where can I get help?
  iperf3 only supports Linux, FreeBSD, and OSX. For other platforms we recommend using iperf2.

I managed to get a Windows executable built, but why do I get a BSOD on Windows 7?
  There seems to be a bug in Windows 7 where running iperf3 from a
  network filesystem can cause a system crash (in other words Blue
  Screen of Death, or BSOD).  This is a Windows bug addressed in kb2839149:

  https://support.microsoft.com/en-us/help/2839149/stop-error-0x00000027-in-the-rdbss-sys-process-in-windows-7-or-windows

  A hotfix is available under kb2732673:

  https://support.microsoft.com/en-us/help/2732673/-delayed-write-failed-error-message-when--pst-files-are-stored-on-a-ne

Why can’t I run a UDP client with no server?
  This is potentially dangerous, and an attacker could use this for a
  denial of service attack.  We don't want iperf3 to be an attack tool.

I'm trying to use iperf3 to test a 40G/100G link...What do I need to know?
  See the following pages on fasterdata.es.net:

  - https://fasterdata.es.net/host-tuning/100g-tuning/
  - https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf/multi-stream-iperf3/

My receiver didn't get all the bytes that got sent but there was no loss.  Huh?
  iperf3 uses a control connection between the client and server to
  manage the start and end of each test.  Sometimes the commands on
  the control connection can be received and acted upon before all of
  the test data has been processed.  Thus the test ends with data
  still in flight.  This effect can be significant for short (a few
  seconds) tests, but is probably negligible for longer tests.

A file sent using the ``-F`` option got corrupted...what happened?
  The ``-F`` option to iperf3 is not a file transfer utility.  It's a
  way of testing the end-to-end performance of a file transfer,
  including filesystem and disk overheads.  So while the test will
  mimic an actual file transfer, the data stored to disk may not be
  the same as what was sent.  In particular, the file size will be
  rounded up to the next larger multiple of the transfer block size,
  and for UDP tests, iperf's metadata (containing timestamps and
  sequence numbers) will overwrite the start of every UDP packet
  payload.

I have a question regarding iperf3...what's the best way to get help?
  Searching on the Internet is a good first step.
  http://stackoverflow.com/ has a number of iperf3-related questions
  and answers, but a simple query into your favorite search engine can
  also yield some results.

  There is a mailing list nominally used for iperf3 development,
  iperf-dev@googlegroups.com.

  We discourage the use of the iperf3 issue tracker on GitHub for
  support questions.  Actual bug reports, enhancement requests, or
  pull requests are encouraged, however.
