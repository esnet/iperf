Invoking iperf3
===============

iperf3 includes a manual page listing all of the command-line options.
The manual page is the most up-to-date reference to the various flags and parameters.

For sample command line usage, see:

https://fasterdata.es.net/performance-testing/network-troubleshooting-tools/iperf/

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
It is presented here only for convenience; the text here might not
correspond to the current version of iperf3.  The authoritative iperf3
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
          can test TCP, UDP, or SCTP throughput.  To perform an iperf3  test  the
          user must establish both a server and a client.
   
          The  iperf3  executable  contains both client and server functionality.
          An iperf3 server can be started using either of the -s or --server com-
          mand-line parameters, for example:
   
                 iperf3 -s
   
                 iperf3 --server
   
          Note that  many  iperf3  parameters  have  both  short  (-s)  and  long
          (--server) forms.  In this section we will generally use the short form
          of  command-line  flags,  unless only the long form of a flag is avail-
          able.
   
          By default, the iperf3 server listens on TCP port 5201 for  connections
          from  an iperf3 client.  A custom port can be specified by using the -p
          flag, for example:
   
                 iperf3 -s -p 5002
   
          After the server is started, it will listen for connections from iperf3
          clients (in other words, the iperf3 program run in client  mode).   The
          client mode can be started using the -c command-line option, which also
          requires a host to which iperf3 should connect.  The host can be speci-
          fied by hostname, IPv4 literal, or IPv6 literal:
   
                 iperf3 -c iperf3.example.com
   
                 iperf3 -c 192.0.2.1
   
                 iperf3 -c 2001:db8::1
   
          If  the  iperf3  server is running on a non-default TCP port, that port
          number needs to be specified on the client as well:
   
                 iperf3 -c iperf3.example.com -p 5002
   
          The initial TCP connection is used to exchange test parameters, control
          the start and end of the test, and to exchange test results.   This  is
          sometimes  referred  to  as  the "control connection".  The actual test
          data is sent over a separate TCP connection, as a separate flow of  UDP
          packets, or as an independent SCTP connection, depending on what proto-
          col was specified by the client.
   
          Normally, the test data is sent from the client to the server, and mea-
          sures  the  upload  speed  of the client.  Measuring the download speed
          from the server can be done by specifying the -R flag  on  the  client.
          This causes data to be sent from the server to the client.
   
                 iperf3 -c iperf3.example.com -p 5202 -R
   
          Results  are displayed on both the client and server.  There will be at
          least one line of output per measurement interval (by  default  a  mea-
          surement  interval lasts for one second, but this can be changed by the
          -i option).  Each line of output includes (at least) the time since the
          start of the test, amount of data transferred during the interval,  and
          the  average bitrate over that interval.  Note that the values for each
          measurement interval are taken from the point of view of  the  endpoint
          process  emitting that output (in other words, the output on the client
          shows the measurement interval data for the client.
   
          At the end of the test is a set of statistics that shows (at  least  as
          much  as possible) a summary of the test as seen by both the sender and
          the receiver, with lines tagged accordingly.  Recall  that  by  default
          the  client  is  the sender and the server is the receiver, although as
          indicated above, use of the -R flag will reverse these roles.
   
          The client can be made to retrieve the server-side output for  a  given
          test by specifying the --get-server-output flag.
   
          Either the client or the server can produce its output in a JSON struc-
          ture,  useful for integration with other programs, by passing it the -J
          flag.  Normally the contents of the JSON structure are only  completely
          known after the test has finished, no JSON output will be emitted until
          the  end of the test.  By enabling line-delimited JSON multiple objects
          will be emitted to provide a real-time parsable JSON output.
   
          iperf3 has a (overly) large set of command-line  options  that  can  be
          used  to  set the parameters of a test.  They are given in the "GENERAL
          OPTIONS" section of the manual page below, as  well  as  summarized  in
          iperf3's help output, which can be viewed by running iperf3 with the -h
          flag.
   
   GENERAL OPTIONS
          -p, --port n
                 Set server port to listen on/connect to to n (default 5201)
   
          -f, --format [kmgtKMGT]
                 Set format to report: Kbits/Mbits/Gbits/Tbits
   
          -i, --interval n
                 Pause  n seconds between periodic throughput reports; default is
                 1, use 0 to disable.
   
          -I, --pidfile file
                 Write a file with the process ID.  This option  is  most  useful
                 when running as a daemon.
   
          -F, --file name
                 Use  a  file  as  the source (on the sender) or sink (on the re-
                 ceiver) of data, rather than  just  generating  random  data  or
                 throwing  it  away.  This feature is used for finding whether or
                 not the storage subsystem is the bottleneck for file  transfers.
                 It  does not turn iperf3 into a file transfer tool.  The length,
                 attributes, and in some cases contents of the received file  may
                 not match those of the original file. This option is unavailable
                 when doing --udp tests.
   
          -A, --affinity n/n,m
                 Set  the  CPU affinity, if possible (Linux, FreeBSD, and Windows
                 only).  On both the client and server  you  can  set  the  local
                 affinity  by using the n form of this argument (where n is a CPU
                 number).  In addition, on the client side you can  override  the
                 server's  affinity for just that one test, using the n,m form of
                 argument.  Note that when using this  feature,  a  process  will
                 only  be  bound  to a single CPU (as opposed to a set containing
                 potentially multiple CPUs).
   
          -B, --bind host[%dev]
                 Bind to the specific interface associated with address host.  If
                 an optional interface is specified, it is treated as a  shortcut
                 for  --bind-dev dev.  Note that a percent sign and interface de-
                 vice name are required for IPv6 link-local address literals,  in
                 order to set the link-local scope.
   
          --bind-dev dev
                 Bind  to  the  specified  network  interface.   This option uses
                 SO_BINDTODEVICE, and may require root  permissions.   (Available
                 on Linux and possibly other systems.)
   
          -V, --verbose
                 Produce more detailed output.
   
          -J, --json
                 Output in JSON format instead of the default human-readable out-
                 put.
   
          --json-stream
                 Output  in line-delimited JSON format instead of the default hu-
                 man-readable output. This option overrides the --json option, if
                 that option was also specified.
   
          --json-stream-full-output
                 Output in JSON format with JSON streams enabled. This flag  only
                 takes effect if the --json-stream option was also specified.
   
          --logfile file
                 Send output to a log file.
   
          --forceflush
                 Force  flushing output at every interval.  Used to avoid buffer-
                 ing when sending output to pipe.
   
          --timestamps[=format]
                 Prepend a timestamp at the start of each output  line.   By  de-
                 fault,  timestamps have the format emitted by ctime(1).  Option-
                 ally, = followed by a format specification can be passed to cus-
                 tomize the timestamps, see strftime(3).  If this optional format
                 is given, the = must immediately follow the --timestamps  option
                 with no whitespace intervening.
   
          --rcv-timeout #
                 Set idle timeout for receiving data during active tests. The re-
                 ceiver  will  halt a test if no data is received from the sender
                 for this number of ms (default to 120000 ms, or 2 minutes).
   
          --snd-timeout #
                 Set timeout for unacknowledged TCP data (on both test  and  con-
                 trol connections) This option can be used to force a faster test
                 timeout  in  case  of a network partition during a test. The re-
                 quired parameter is specified in ms, and defaults to the  system
                 settings.   This  functionality  depends on the TCP_USER_TIMEOUT
                 socket option, and will not work on systems that do not  support
                 it.
   
          --use-pkcs1-padding
                 This  option  is only meaningful when using iperf3's authentica-
                 tion features. Versions of  iperf3  prior  to  3.17  used  PCKS1
                 padding  in  the RSA-encrypted credentials, which was vulnerable
                 to a side-channel attack that could reveal  a  server's  private
                 key.   Beginning  with iperf-3.17, OAEP padding is used, however
                 this is a breaking change that  is  not  compatible  with  older
                 iperf3  versions.   Use this option to preserve the less secure,
                 but more compatible, behavior.
   
          -m, --mptcp
                 Use the MPTCP variant for the current protocol.  This  only  ap-
                 plies to TCP and enables MPTCP usage.
   
          -d, --debug
                 Emit  debugging  output.  Primarily (perhaps exclusively) of use
                 to developers.
   
          -v, --version
                 Show version information and quit.
   
          -h, --help
                 Show a help synopsis.
   
   
   SERVER SPECIFIC OPTIONS
          -s, --server
                 Run in server mode.
   
          -D, --daemon
                 Run the server in background as a daemon.
   
          -1, --one-off
                 Handle (at most) one client connection, then exit.  If  an  idle
                 time is set, the server will exit after that amount of time with
                 no connection.
   
          --idle-timeout n
                 Restart  the  server  after n seconds in case it gets stuck.  In
                 one-off mode, this is the number of seconds the server will wait
                 before exiting.
   
          --server-max-duration
                 The maximum time, in seconds,  that  an  iperf  client  can  run
                 against  the server.  When the sum of the client's time and omit
                 values exceeds the  max  duration  set  by  the  server  or  the
                 client's time value is 0, the measurement is rejected.
   
          --server-bitrate-limit n[KMGT][/n]
                 Set a limit on the server side, which will cause a test to abort
                 if  the  client specifies a test of more than n bits per second,
                 or if the average data sent or received by the client (including
                 all data streams) is greater than n bits per  second.   The  de-
                 fault  limit  is  0,  which implies no limit.  The interval over
                 which to average the data rate is 5 seconds by default, but  can
                 be specified by adding a / character and a number to the bitrate
                 specifier.
   
          --rsa-private-key-path file
                 Path to the RSA private key (not password-protected) used to de-
                 crypt  authentication credentials from the client (if built with
                 OpenSSL support).
   
          --authorized-users-path file
                 Path to the configuration file containing authorized users  cre-
                 dentials  to  run  iperf  tests (if built with OpenSSL support).
                 The file is a comma separated list  of  usernames  and  password
                 hashes;  more  information  on  the structure of the file can be
                 found in the EXAMPLES section.
   
          --time-skew-threshold seconds
                 Specify the allowable time skew threshold (in  seconds)  between
                 the server and client during the authentication process.
   
   CLIENT SPECIFIC OPTIONS
          -c, --client host[%dev]
                 Run  in client mode, connecting to the specified server.  By de-
                 fault, a test consists of sending data from the  client  to  the
                 server,  unless the -R flag is specified.  If an optional inter-
                 face is specified, it is treated as a  shortcut  for  --bind-dev
                 dev.  Note that a percent sign and interface device name are re-
                 quired for IPv6 link-local address literals.
   
          --sctp Use  SCTP  for  tests rather than TCP (FreeBSD and Linux).  Note
                 that TCP communication is still used for the control  connection
                 between client and server.
   
          -u, --udp
                 Use  UDP for tests rather than TCP.  Note that TCP communication
                 is still used for the  control  connection  between  client  and
                 server.
   
          --connect-timeout n
                 Set  timeout  for establishing the initial control connection to
                 the server, in milliseconds.  The default behavior is the  oper-
                 ating  system's  timeout for TCP connection establishment.  Pro-
                 viding a shorter value may speed up detection of a  down  iperf3
                 server.
   
          -b, --bitrate n[KMGT]
                 Set  target  bitrate  to n bits/sec (default 1 Mbit/sec for UDP,
                 unlimited for TCP/SCTP).  If  there  are  multiple  streams  (-P
                 flag),  the  throughput  limit  is  applied  separately  to each
                 stream.  You can also add a '/' and  a  number  to  the  bitrate
                 specifier.   This  is  called "burst mode".  It will perform the
                 given number of sends without pausing, even if that  temporarily
                 exceeds  the specified throughput limit.  Setting the target bi-
                 trate to 0 will disable bitrate limits (particularly useful  for
                 UDP tests).  This throughput limit is implemented internally in-
                 side  iperf3,  and  is available on all platforms.  Compare with
                 the --fq-rate flag.  This option replaces the --bandwidth  flag,
                 which is now deprecated but (at least for now) still accepted.
   
          --pacing-timer n[KMGT]
                 Set  pacing  timer  interval  in  microseconds (default 1000 mi-
                 croseconds, or 1 ms).  This controls  iperf3's  internal  pacing
                 timer  for  the -b/--bitrate option.  The timer fires at the in-
                 terval set by this parameter.   Smaller  values  of  the  pacing
                 timer  parameter  smooth  out the traffic emitted by iperf3, but
                 potentially at the cost of  performance  due  to  more  frequent
                 timer processing.
   
          --fq-rate n[KMGT]
                 Set a rate to be used with fair-queueing based socket-level pac-
                 ing,  in bits per second.  This pacing (if specified) will be in
                 addition to any pacing due to iperf3's internal throughput  pac-
                 ing  (-b/--bitrate flag), and both can be specified for the same
                 test.  Only available on platforms  supporting  the  SO_MAX_PAC-
                 ING_RATE  socket  option (currently only Linux).  The default is
                 no fair-queueing based pacing.
   
          --no-fq-socket-pacing
                 This option is deprecated and will be removed.  It is equivalent
                 to specifying --fq-rate=0 .
   
          -t, --time n
                 Set the test duration in seconds (default 10 secs).  The -t , -n
                 ", and" -k options are mutually exclusive.
   
          -n, --bytes n[KMGT]
                 Set the number of bytes to transmit.  The -t , -n ", and" -k op-
                 tions are mutually exclusive.
   
          -k, --blockcount n[KMGT]
                 Set the number of blocks (packets) to transmit.  The -t , -n  ",
                 and" -k options are mutually exclusive.
   
          -l, --length n[KMGT]
                 Set  the  length of the buffer to read or write.  For TCP tests,
                 the default value is 128KB.  In the case of UDP, iperf3 tries to
                 dynamically determine a reasonable sending  size  based  on  the
                 path  MTU;  if that cannot be determined it uses 1460 bytes as a
                 sending size.  For SCTP tests, the default size is 64KB.
   
          --cport port
                 Bind data streams to a specific TCP or UDP client port (for  TCP
                 and UDP only, default is to use an ephemeral port).
   
          -P, --parallel n
                 Set the number of parallel client streams to run. Beginning with
                 iperf-3.16,  iperf3  will  spawn  off a separate thread for each
                 test stream.   Using  multiple  streams  may  result  in  higher
                 throughput than a single stream, in cases where network through-
                 put is CPU-limited.
   
          -R, --reverse
                 Reverse  the  direction of a test, so that the server sends data
                 to the client.
   
          --bidir
                 Test in both directions (normal  and  reverse),  with  both  the
                 client and server sending and receiving data simultaneously
   
          -w, --window n[KMGT]
                 Set t he socket buffer size / window size.  This value gets sent
                 to  the server and used on that side too; on both sides this op-
                 tion sets both the sending and receiving  socket  buffer  sizes.
                 This option can be used to set (indirectly) the maximum TCP win-
                 dow  size.   Note  that  on Linux systems, the effective maximum
                 window size is approximately double what is  specified  by  this
                 option.   This  behavior is not a bug in iperf3 but a feature of
                 the Linux kernel, as documented by tcp(7) and socket(7)).
   
          -M, --set-mss n
                 Set the TCP/SCTP maximum segment size (MTU - 40 bytes).
   
          -N, --no-delay
                 Set the TCP/SCTP no delay option, disabling Nagle's Algorithm.
   
          -4, --version4
                 Force the use of IPv4.
   
          -6, --version6
                 Force the use of IPv6.
   
          -S, --tos n
                 Set the IP type of service bits.  The usual prefixes  for  octal
                 and  hex can be used, i.e. 52, 064 and 0x34 all specify the same
                 value.
   
          --dscp dscp
                 Set the IP DSCP bits.  Both numeric and symbolic values are  ac-
                 cepted.  Numeric  values  can be specified in decimal, octal and
                 hex (see --tos above).
   
          -L, --flowlabel n
                 Set the IPv6 flow label (currently only supported on Linux).
   
          -X, --xbind name
                 Bind SCTP associations to  a  specific  subset  of  links  using
                 sctp_bindx(3).   The  --B  flag  will be ignored if this flag is
                 specified.  Normally SCTP will include the protocol addresses of
                 all active links on the local host when setting up  an  associa-
                 tion.  Specifying at least one --X name will disable this behav-
                 iour.  This flag must be specified for each link to be  included
                 in  the association, and is supported for both iperf servers and
                 clients (the latter are supported by passing the first --X argu-
                 ment to bind(2)).  Hostnames are accepted as arguments  and  are
                 resolved using getaddrinfo(3).  If the --4 or --6 flags are also
                 specified,  names  which  do not resolve to addresses within the
                 specified protocol family will be ignored.
   
          --nstreams n
                 Set number of SCTP streams.
   
          -Z, --zerocopy
                 Use a "zero copy" method of sending data, such  as  sendfile(2),
                 instead of the usual write(2).
   
          --skip-rx-copy
                 Ignored  received  packet  data, using the MSG_TRUNC flag to the
                 recv(2) system call.
   
          -O, --omit n
                 Perform pre-test for n seconds and omit the pre-test statistics,
                 to skip past the TCP slow-start period.
   
          -T, --title str
                 Prefix every output line with the string str.
   
          --extra-data str
                 Specify an extra data string field to be included in  JSON  out-
                 put.
   
          -C, --congestion algo
                 Set  the  congestion control algorithm (Linux and FreeBSD only).
                 An older --linux-congestion synonym for this  flag  is  accepted
                 but is deprecated.
   
          --get-server-output
                 Get the output from the server.  The output format is determined
                 by the server (in particular, if the server was invoked with the
                 --json  flag,  the  output  will be in JSON format, otherwise it
                 will be in human-readable format).  If the client  is  run  with
                 --json,  the  server output is included in a JSON object; other-
                 wise it is appended at the bottom of the human-readable  output.
                 Note  that  the server output is available only if the test com-
                 pletes, not if it is interrupted.
   
          --udp-counters-64bit
                 Use 64-bit counters in UDP test packets.  The use of this option
                 can help prevent counter overflows during long  or  high-bitrate
                 UDP  tests.   Both client and server need to be running at least
                 version 3.1 for this option to work.  It may become the  default
                 behavior at some point in the future.
   
          --repeating-payload
                 Use  repeating pattern in payload, instead of random bytes.  The
                 same payload is used in iperf2  (ASCII  '0..9'  repeating).   It
                 might  help  to test and reveal problems in networking gear with
                 hardware compression (including some WiFi access points),  where
                 iperf2 and iperf3 perform differently, just based on payload en-
                 tropy.
   
          --dont-fragment
                 Set  the IPv4 Don't Fragment (DF) bit on outgoing packets.  Only
                 applicable to tests doing UDP over IPv4.
   
          --username username
                 Specify username to use for authentication to the  iperf  server
                 (if  built with OpenSSL support).  The password will be prompted
                 for interactively when the test is run.  Note the  password  can
                 also  be specified via the IPERF3_PASSWORD environment variable.
                 If this  variable  is  present,  the  password  prompt  will  be
                 skipped.
   
          --rsa-public-key-path file
                 Set  path  to  the RSA public key used to encrypt authentication
                 credentials (if built with OpenSSL support).
   
   
   EXAMPLES
      Authentication - RSA Keypair
          The authentication feature of iperf3 requires an  RSA  public  keypair.
          The  public  key is used to encrypt the authentication token containing
          the user credentials, while the private key is used to decrypt the  au-
          thentication  token.   The  private key must be in PEM format and addi-
          tionally must not have a password set.  The public key must be  in  PEM
          format  and  use SubjectPrefixKeyInfo encoding.  An example of a set of
          UNIX/Linux commands using OpenSSL to generate a  correctly-formed  key-
          pair follows:
   
                 > openssl genrsa -des3 -out private.pem 2048
                 >  openssl  rsa  -in  private.pem -outform PEM -pubout -out pub-
                 lic.pem
                 > openssl rsa -in private.pem -out private_not_protected.pem \
                   -outform PEM
   
          After these commands, the public key will be contained in the file pub-
          lic.pem and the  private  key  will  be  contained  in  the  file  pri-
          vate_not_protected.pem.
   
      Authentication - Authorized users configuration file
          A  simple plaintext file must be provided to the iperf3 server in order
          to specify the authorized user credentials.  The file is a simple  list
          of  comma-separated  pairs  of  a username and a corresponding password
          hash.  The password hash is a SHA256 hash of the string  "{$user}$pass-
          word".   The file can also contain commented lines (starting with the #
          character).  An example of commands to generate the password hash on  a
          UNIX/Linux system is given below:
   
                 > S_USER=mario S_PASSWD=rossi
                 > echo -n "{$S_USER}$S_PASSWD" | sha256sum | awk '{ print $1 }'
   
          An example of a password file (with an entry corresponding to the above
          username and password) is given below:
   
                 > cat credentials.csv
            # file format: username,sha256
            mario,bf7a49a846d44b454a5d11e7ac-
            faf13d138bbe0b7483aa3e050879700572709b
   
   AUTHORS
          A list of the contributors to iperf3 can be found within the documenta-
          tion located at https://software.es.net/iperf/dev.html#authors.
   
   
   SEE ALSO
          libiperf(3), https://software.es.net/iperf
   
   ESnet                            November 2025                       IPERF3(1)

The iperf3 manual page will typically be installed in manual
section 1.
