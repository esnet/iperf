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
  iperf3. iperf2.0.8, released in 2015, made iperf2 a useful tool.
 
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
 
Why can’t I run a UDP client with no server?
  This is potentially dangerous, and an attacker could use this for a
  denial of service attack.  We don't want iperf3 to be an attack tool.

I'm trying to use iperf3 to test a 40G/100G link...What do I need to know?
  See the following pages on fasterdata.es.net:

   - https://fasterdata.es.net/host-tuning/100g-tuning/
   - https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf/multi-stream-iperf3/

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


