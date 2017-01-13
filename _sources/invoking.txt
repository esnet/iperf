Invoking iperf3
===============

iperf3 includes a manual page listing all of the command-line options.
The manual page is the most up-to-date reference to the various flags and parameters.

For sample command line usage, see: 

http://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf-and-iperf3/

Using the default options, iperf3 is meant to show typical well
designed application performance.  "Typical well designed application"
means avoiding artificial enhancements that work only for testing
(such as ``splice()``-ing the data to ``/dev/null``).  iperf3 does
also have flags for "extreme best case" optimizations but they must be
explicitly activated.  These flags include the ``-Z`` (``--zerocopy``)
and ``-A`` (``--affinity``) options.

iperf3 Manual Page
------------------

This section contains a plaintext rendering of the iperf3 manual page.
It is presented here only for convenience; the authoritative iperf3
manual page is included in the source tree and installed along with
the executable.

::

   IPERF3(1)                        User Manuals                        IPERF3(1)
   
   
   
   NAME
          iperf3 - perform network throughput tests
   
   SYNOPSIS
          iperf3 -s [ options ]
          iperf3 -c server [ options ]
   
   
   DESCRIPTION
          iperf3  is  a  tool for performing network throughput measurements.  It
          can test either TCP or UDP throughput.  To perform an iperf3  test  the
          user must establish both a server and a client.
   
   
   GENERAL OPTIONS
          -p, --port n
                 set server port to listen on/connect to to n (default 5201)
   
          -f, --format
                 [kmKM]   format to report: Kbits, Mbits, KBytes, MBytes
   
          -i, --interval n
                 pause  n  seconds between periodic bandwidth reports; default is
                 1, use 0 to disable
   
          -F, --file name
                 client-side: read from  the  file  and  write  to  the  network,
                 instead of using random data; server-side: read from the network
                 and write to the file, instead of throwing the data away
   
          -A, --affinity n/n,m
                 Set the CPU affinity, if possible (Linux and FreeBSD only).   On
                 both  the  client  and  server you can set the local affinity by
                 using the n form of this argument (where n is a CPU number).  In
                 addition,  on  the  client  side  you  can override the server's
                 affinity for just that one test, using the n,m form of argument.
                 Note  that when using this feature, a process will only be bound
                 to a single CPU (as opposed to a set containing potentialy  mul-
                 tiple CPUs).
   
          -B, --bind host
                 bind to a specific interface
   
          -V, --verbose
                 give more detailed output
   
          -J, --json
                 output in JSON format
   
          --logfile file
                 send output to a log file.
   
          --forceflush
                 force  flushing output at every interval.  Used to avoid buffer-
                 ing when sending output to pipe.
   
          -d, --debug
                 emit debugging output.  Primarily (perhaps exclusively)  of  use
                 to developers.
   
          -v, --version
                 show version information and quit
   
          -h, --help
                 show a help synopsis
   
   
   SERVER SPECIFIC OPTIONS
          -s, --server
                 run in server mode
   
          -D, --daemon
                 run the server in background as a daemon
   
          -I, --pidfile file
                 write  a file with the process ID, most useful when running as a
                 daemon.
   
          -1, --one-off
                 handle one client connection, then exit.
   
   
   CLIENT SPECIFIC OPTIONS
          -c, --client host
                 run in client mode, connecting to the specified server
   
          --sctp use SCTP rather than TCP (FreeBSD and Linux)
   
          -u, --udp
                 use UDP rather than TCP
   
          -b, --bandwidth n[KM]
                 set target bandwidth to n bits/sec (default 1 Mbit/sec for  UDP,
                 unlimited  for  TCP).   If there are multiple streams (-P flag),
                 the bandwidth limit is applied separately to each  stream.   You
                 can  also  add  a  '/'  and a number to the bandwidth specifier.
                 This is called "burst mode".  It will send the given  number  of
                 packets  without  pausing,  even if that temporarily exceeds the
                 specified bandwidth limit.  Setting the target  bandwidth  to  0
                 will  disable  bandwidth  limits  (particularly  useful  for UDP
                 tests).  This bandwidth limit is implemented  internally  inside
                 iperf3,  and  is  available  on all platforms.  Compare with the
                 --fq-rate flag.
   
          --fq-rate n[KM]
                 Set a rate to be used with fair-queueing based socket-level pac-
                 ing,  in bits per second.  This pacing (if specified) will be in
                 addition to any pacing due to iperf3's internal bandwidth pacing
                 (-b  flag),  and  both can be specified for the same test.  Only
                 available on platforms supporting the SO_MAX_PACING_RATE  socket
                 option  (currently only Linux).  The default is no fair-queueing
                 based pacing.
   
          --no-fq-socket-pacing
                 This option is deprecated and will be removed.  It is equivalent
                 to specifying --fq-rate=0.
   
          -t, --time n
                 time in seconds to transmit for (default 10 secs)
   
          -n, --bytes n[KM]
                 number of bytes to transmit (instead of -t)
   
          -k, --blockcount n[KM]
                 number of blocks (packets) to transmit (instead of -t or -n)
   
          -l, --length n[KM]
                 length  of  buffer to read or write.  For TCP tests, the default
                 value is 128KB.  In the case of UDP, iperf3 tries to dynamically
                 determine  a  reasonable  sending size based on the path MTU; if
                 that cannot be determined it uses 1460 bytes as a sending  size.
                 For SCTP tests, the default size is 64KB.
   
          --cport port
                 bind  data  streams  to  a specific client port (for TCP and UDP
                 only, default is to use an ephemeral port)
   
          -P, --parallel n
                 number of parallel client streams to run
   
          -R, --reverse
                 run in reverse mode (server sends, client receives)
   
          -w, --window n[KM]
                 window size / socket buffer size (this gets sent to  the  server
                 and used on that side too)
   
          -M, --set-mss n
                 set TCP/SCTP maximum segment size (MTU - 40 bytes)
   
          -N, --no-delay
                 set TCP/SCTP no delay, disabling Nagle's Algorithm
   
          -4, --version4
                 only use IPv4
   
          -6, --version6
                 only use IPv6
   
          -S, --tos n
                 set the IP 'type of service'
   
          -L, --flowlabel n
                 set the IPv6 flow label (currently only supported on Linux)
   
          -X, --xbind name
                 Bind  SCTP  associations  to  a  specific  subset of links using
                 sctp_bindx(3).  The --B flag will be ignored  if  this  flag  is
                 specified.  Normally SCTP will include the protocol addresses of
                 all active links on the local host when setting up  an  associa-
                 tion.  Specifying at least one --X name will disable this behav-
                 iour.  This flag must be specified for each link to be  included
                 in  the association, and is supported for both iperf servers and
                 clients (the latter are supported by passing the first --X argu-
                 ment  to  bind(2)).  Hostnames are accepted as arguments and are
                 resolved using getaddrinfo(3).  If the  --4  or  --6  flags  are
                 specified,  names  which  do not resolve to addresses within the
                 specified protocol family will be ignored.
   
          --nstreams n
                 Set number of SCTP streams.
   
          -Z, --zerocopy
                 Use a "zero copy" method of sending data, such  as  sendfile(2),
                 instead of the usual write(2).
   
          -O, --omit n
                 Omit the first n seconds of the test, to skip past the TCP slow-
                 start period.
   
          -T, --title str
                 Prefix every output line with this string.
   
          -C, --congestion algo
                 Set the congestion control algorithm (Linux and  FreeBSD  only).
                 An  older  --linux-congestion  synonym for this flag is accepted
                 but is deprecated.
   
          --get-server-output
                 Get the output from the server.  The output format is determined
                 by the server (in particular, if the server was invoked with the
                 --json flag, the output will be in  JSON  format,  otherwise  it
                 will  be  in  human-readable format).  If the client is run with
                 --json, the server output is included in a JSON  object;  other-
                 wise  it is appended at the bottom of the human-readable output.
   
   
   AUTHORS
          A list of the contributors to iperf3 can be found within the documenta-
          tion located at http://software.es.net/iperf/dev.html#authors.
   
   
   SEE ALSO
          libiperf(3), http://software.es.net/iperf
   
   
   
   ESnet                            January 2017                        IPERF3(1)

The iperf3 manual page will typically be installed in manual
section 1.

