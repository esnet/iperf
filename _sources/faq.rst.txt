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
 
I'm seeing quite a bit of unexpected UDP loss. Why?
  First, confirm you are using iperf 3.1.5 or higher. There was an
  issue with the default UDP send size that was fixed in
  3.1.5. Second, try adding the flag ``-w2M`` to increase the socket
  buffer sizes. That seems to make a big difference on some hosts.
 
iperf3 UDP does not seem to work at bandwidths less than 100Kbps. Why?
  You'll need to reduce the default packet length to get UDP rates of less that 100Kbps. Try ``-l100``.
 
What congestion control algorithms are supported?
  On Linux, run this command to see the available congestion control
  algorithms (note that some algorithms are packaged as kernel
  modules, which must be loaded before they can be used)::
    
    /sbin/sysctl net.ipv4.tcp_available_congestion_control
 
I’m using the ``--logfile`` option. How do I see file output in real time?
  Use the ``--forceflush`` flag.

I'm using the --fq-rate flag, but it does not seem to be working. Why?
  You need to add 'net.core.default_qdisc = fq' to /etc/sysctl.conf for that option to work.

I'm having trouble getting iperf3 to work on Windows, Android, etc. Where can I get help?
  iperf3 only supports Linux, FreeBSD, and OSX. For other platforms we recommend using iperf2.

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


