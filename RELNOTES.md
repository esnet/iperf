iperf3 Release Notes
====================

iperf-3.13 2023-02-16
---------------------

* Notable user-visible changes

  * fq-rate (PR #1461, Issue #1366), and bidirectional flag (Issue #1428,
    PR #1429) were added to the JSON output.

  * Added support for OpenBSD including cleaning up endian handling (PR #1396)
    and support for TCP_INFO on systems where it was implemented (PR #1397).

  * Fixed bug in how TOS is set in mapped v4 (PR #1427).

  * Corrected documentation, such as updating binary download links and text
    (Issue #1459), updating version on iperf3 websites, and fixing an
    incorrect error message (Issue #1441).

  * Fixed crash on rcv-timeout with JSON logfile (#1463, #1460, issue #1360,
    PR #1369).

  * Fixed a bug that prevented TOS/DSCP from getting set correctly for reverse
    tests (PR #1427, Issue #638).

* Developer-visible changes

  * Getter and setter are now available for bind_dev (PR #1419).

  * Added missing getter for bidirectional tests (PR #1453).

  * Added minor changes to clean up .gitignore and error messages (#1408).

  * Made sure configure scripts are runnable with /bin/sh (PR #1398).

  * Cleaned up RPM spec, such as adding missing RPM build dependencies, dropping
    EL5 and removing outdated %changelog (PR #1401) to make.

  * Added a fix for a resource leak bug in function iperf_create_pidfile(#1443).


iperf-3.12 2022-09-30
---------------------

* Notable user-visible changes

  * cJSON has been updated to version 1.7.15 (#1383).

  * The --bind <host>%<dev> option syntax now works properly (#1360 /
    #1371).

  * A server-side file descriptor leak with the --logfile option has
    been fixed (#1369 / #1360 / #1369 / #1389 / #1393).

  * A bug that caused some large values from TCP_INFO to be misprinted
    as negative numbers has been fixed (#1372).

  * Using the -k or -n flags with --reverse no longer leak into future
    tests (#1363 / #1364).

  * There are now various debug level options available with the
    --debug option. These can be used to adjust the amount of
    debugging output (#1327).

  * A new --snd-timeout option has been added to set a termination
    timeout for idle TCP connections (#1215 / #1282).

  * iperf3 is slightly more robust to out-of-order packets during UDP
    connection setup in --reverse mode (#914 / #1123 / #1182 / #1212 /
    #1260).

  * iperf3 will now use different ports for each direction when the
    --cport and --bdir options are set (#1249 / #1259).

  * The iperf3 server will now exit if it can't open its log file
    (#1225 / #1251).

  * Various help message and output fixes have been made (#1299 /
    #1330 / #1345 / #1350).

  * Various compiler warnings have been fixed (#1211 / #1316).

* Developer-visible changes

  * Operation of bootstrap.sh has been fixed and simplified (#1335 /
    #1325).
    
  * Flow label support / compatibility under Linux has been improved
    (#1310).
    
  * Various minor memory leaks have been fixed (#1332 / #1333).

  * A getter/setter has been added for the bind_port parameter
    (--cport option). (#1303, #1305)

  * Various internal documentation improvements (#1265 / #1285 / #1304).

iperf-3.11 2022-01-31
---------------------

* Notable user-visible changes

  * Update links to Discussions in documentation

  * Fix DSCP so that TOS = DSCP * 4 (#1162)

  * Fix --bind-dev for TCP streams (#1153)

  * Fix interface specification so doesn't overlap with IPv6 link-local addresses for -c and -B (#1157, #1180)

  * Add get/set test_unit_format function declaration to iperf_api.h

  * Auto adjustment of test-end condition for file transfers (-F), if no end condition is set, it will automatically adjust it to file size in bytes

  * Exit if idle time expires waiting for a connection in one-off mode (#1187, #1197)

  * Support zerocopy by reverse mode (#1204)

  * Update help and manpage text for #1157, support bind device

  * Consistently print target_bandwidth in JSON start section (#1177)

  * Test bitrate added to JSON output (#1168)

  * Remove fsync call after every write to receiving --file (#1176, #1159)

  * Update documentation for -w (#1175)

  * Fix for #952, different JSON object names for bidir reverse channel

iperf-3.10.1 2021-06-03
-----------------------

* Notable user-visible changes

  * Fixed a problem with autoconf scripts that made builds fail in
    some environments (#1154 / #1155).

* Developer-visible changes

  * GNU autoconf 2.71 or newer is now required to regenerate iperf3's
    configure scripts.

iperf 3.10 2021-05-26
---------------------

* Notable user-visible changes

  * Fix a bug where some --reverse tests didn't terminate (#982 /
    #1054).

  * Responsiveness of control connections is slightly improved (#1045
    / #1046 / #1063).

  * The allowable clock skew when doing authentication between client
    and server is now configurable with the new --time-skew-threshold
    (#1065 / #1070).

  * Bitrate throttling using the -b option now works when a burst size
    is specified (#1090).

  * A bug with calculating CPU utilization has been fixed (#1076 /
    #1077).

  * A --bind-dev option to support binding sockets to a given network
    interface has been added to make iperf3 work better with
    multi-homed machines and/or VRFs (#817 / #1089 / #1097).

  * --pidfile now works with --client mode (#1110).

  * The server is now less likely to get stuck due to network errors
    (#1101, #1125), controlled by the new --rcv-timeout option.

  * Fixed a few bugs in termination conditions for byte or
    block-limited tests (#1113, #1114, #1115).

  * Added tcp_info.snd_wnd to JSON output (#1148).

  * Some bugs with garbled JSON output have been fixed (#1086, #1118,
    #1143 / #1146).

  * Support for setting the IPv4 don't-fragment (DF) bit has been
    added with the new --dont-fragment option (#1119).

  * A failure with not being able to read the congestion control
    algorithm under WSL1 has been fixed (#1061 / #1126).

  * Error handling and error messages now make more sense in cases
    where sockets were not successfully opened (#1129 / #1132 /
    #1136, #1135 / #1138, #1128 / #1139).

  * Some buffer overflow hazards were fixed (#1134).

* Notable developer-visible changes

  * It is now possible to use the API to set/get the congestion
    control algorithm (#1036 / #1112).


iperf 3.9 2020-08-17
--------------------

* Notable user-visible changes

  * A --timestamps flag has been added, which prepends a timestamp to
    each output line.  An optional argument to this flag, which is a
    format specification to strftime(3), allows for custom timestamp
    formats (#909, #1028).

  * A --server-bitrate-limit flag has been added as a server-side
    command-line argument.  It allows a server to enforce a maximum
    throughput rate; client connections that specify a higher bitrate
    or exceed this bitrate during a test will be terminated.  The
    bitrate is expressed in bits per second, with an optional trailing
    slash and integer count that specifies an averaging interval over
    which to enforce the limit (#999).

  * A bug that caused increased CPU usage with the --bidir option has
    been fixed (#1011).

* Notable developer-visible changes

  * Fixed various minor memory leaks (#1023).

iperf 3.8.1 2020-06-10
----------------------

* Notable user-visible changes

  * A regression with "make install", where the libiperf shared
    library files were not getting installed, has been fixed (#1013 /
    #1014).

iperf 3.8 2020-06-08
--------------------

* Notable user-visible changes

  * Profiled libraries and binaries are no longer built by default
    (#950).

  * A minimal Dockerfile has been added (#824).

  * A bug with burst mode and unlimited rate has been fixed (#898).

  * Configuring with the --enable-static-bin flag will now cause
    a statically-linked iperf3 binary to be built (#989).

  * Configuring with the --without-sctp flag will now prevent SCTP
    from being auto-detected (#1008).  This flag allows building a
    static binary (see above item) on a CentOS system with SCTP
    installed, because no static SCTP libraries are available.

  * Clock skew between the iperf3 client and server will no longer
    skew the computation of jitter during UDP tests (#842 / #990).

  * A possible buffer overflow in the authentication feature has been
    fixed.  This was only relevant when configuration authentication
    using the libiperf3 API, and did not affect command-line usage.
    Various other improvements and fixes in this area were also made
    (#996).

* Notable developer-visible changes

  * The embedded version of cJSON has been updated to 1.7.13 (#978).

  * Some server authentication functions have been added to the API
    (#911).

  * API access has been added to the connection timeout parameter
    (#1001).

  * Tests for some authentication functions have been added.

  * Various compiler errors and warnings have been fixed.

iperf 3.7 2019-06-21
--------------------

* Notable user-visible changes

  * Support for simultaneous bidirectional tests with the --bidir flag
    (#201/#780).

  * Use POSIX standard clock_gettime(3) interface for timekeeping where
    available (#253/#738).

  * Passwords for authentication can be provided via environment
    variable (#815).

  * Specifying --repeating-payload and --reverse now works (#867).

  * Failed authentication doesn't count for --one-off (#864/#877).

  * Several memory leaks related to authenticated use were fixed
    (#881/#888).

  * The delay for tearing down the control connection for the default
    timed tests has been increased, to more gracefully handle
    high-delay paths (#751/#859).

* Notable developer-visible changes

  * Various improvements to the libiperf APIs (#767, #775, #869, #870,
    #871)

  * Fixed build behavior when OpenSSL is absent (#854).

  * Portability fixes (#821/#874).

iperf 3.6 2018-06-25
--------------------

* Notable user-visible changes

  * A new --extra-data option can be used to fill in a user-defined
    string field that appears in JSON output.  (#600 / #729)

  * A new --repeating-payload option makes iperf3 use a payload pattern
    similar to that used by iperf2, which could help in recreating
    results that might be affected by payload entropy (for example,
    compression).  (#726)

  * -B now works properly with SCTP tests.  (#678 / #715)

  * A compile fix for Solaris 10 was added.  (#711)

  * Some minor bug fixes for JSON output.  In particular, warnings for
    debug and/or verbose modes with --json output (#737) and a fix for
    JSON output on CentOS 6 (#727 / #744).

  * software.es.net and downloads.es.net now support HTTPS, so URLs in
    documentation that refer to those two hosts now use https://
    instead of http:// URLs. (#759)

* Notable developer-visible changes

  * Functions related to authenticated iperf3 connections have been
    exposed via libiperf.  (#712 / #713)

  * The ToS byte is now exposed in the libiperf API. (#719)

iperf 3.5 2018-03-02
--------------------

* Notable user-visible changes

  * iperf3 no longer counts data received after the end of a test in
    the bytecounts.  This fixes a bug that could, under some
    conditions, artificially inflate the transfer size and measured
    bitrate.  This bug was most noticeable on reverse direction
    transfers on short tests over high-latency or buffer-bloated
    paths.  Many thanks to @FuzzyStatic for providing access to a test
    environment for diagnosing this issue (#692).

iperf 3.4 2018-02-14
--------------------

* Notable user-visible changes

  * The -A (set processor affinity) command-line flag is now supported
    on Windows (#665).

  * iperf3 now builds on systems lacking a daemon(3) library call
    (#369).

  * A bug in time skew checking under authentication was fixed (#674).

  * IPv6 flow labels now work correctly with multiple parallel streams
    (#694).

  * The client no longer closes its control connection before sending
    end-of-test statistics to the server (#677). This fixes a
    regression introduced in iperf-3.2.

  * Sending output to stdout now makes errors go to stderr, as per
    UNIX convention (#695).

  * A server side crash in verbose output with a client running
    multiple parallel connections has been fixed (#686).

  * The --cport option can now be specified without the --bind option.
    Using the --cport option on Linux can eliminate a problem with
    ephemeral port number allocation that can make multi-stream iperf3
    tests perform very poorly on LAGG links.  Also, the --cport option
    now works on SCTP tests (#697).

* Notable developer-visible changes

  * iperf3 now builds on (some) macOS systems older than 10.7 (#607).

  * Some unused code and header inclusions were eliminated (#667,
    #668).  Also some code was cleaned up to eliminate (or at least
    reduce) compiler warnings (#664, #671).

iperf 3.3 2017-10-31
--------------------

* Notable user-visible changes

  * iperf3 can now be built --without-openssl on systems where OpenSSL
    is present (#624, #633).

  * A bug with printing very large numbers has been fixed (#642).

  * A bug where the server would, under certain circumstances, halt a
    test after exactly fifteen seconds has been fixed (#645).

  * The --tos parameter is no longer "sticky" between tests when doing
    --reverse tests (#639).

  * The authentication token on the server is properly reset between
    tests (#650).

  * A bug that could cause iperf3 to overwrite the PID file of an
    already-existing iperf3 process has been fixed (#623).

  * iperf3 will now ignore nonsensical TCP MSS values (from the TCP
    control connection) when trying to determine a reasonable block
    size for UDP tests.  This condition primarily affected users on
    Windows, but potentially improves robustness for all
    platforms. (#659)

* Notable developer-visible changes

iperf 3.2 2017-06-26
--------------------

* User-visible changes

  * Authentication via a username/password mechanism, coupled with a
    public-key pair, is now an optional way of limiting access to an
    iperf3 server (#517).

  * Ending statistics are less ambiguous for UDP and also now use
    correct test durations for all protocols (#560, #238).  Many fixes
    have been made in statistics printing code, generally for
    human-readable output (#562, #575, #252, #443, #236).

  * Several problems with the -F/--file options have been fixed.
    Documentation has been improved to note some ways in which this
    feature might not behave as expected (#588).

  * iperf3 now uses the correct "bitrate" phraseology rather than
    "bandwidth" when describing measurement results.  The --bandwidth
    option has been renamed --bitrate, although --bandwidth is still
    accepted for backwards compatibility (#583).

  * Application-level bandwidth pacing (--bitrate option) is now
    checked every millisecond by default, instead of of every tenth of
    a second, to provide smoother traffic behavior when using
    application pacing (#460).  The pacing can be tuned via the use of
    the --pacing-timer option (#563).

  * A new --dscp option allows specifying the DSCP value to be used
    for outgoing packets (#508).  The TOS byte value is now printed in
    the JSON output (#226).

  * Congestion window data on FreeBSD is now computed correctly (#465,
    #475, #338).

  * The T/t suffixes for terabytes/terabits are now accepted for
    quantities where suffixes are supported, such as --bandwidth
    (#402).

  * Sanity checks for UDP send sizes have been added (#390), and
    existing checks on the --window option have been improved (#557).

  * The TCP rttvar value is now available in the JSON output (#534), as are
    the socket buffer sizes (#558).

  * Error handling and documentation have been improved for the
    -f/--format options (#568).

  * A new --connect-timeout option on the client allows specifying a
    length of time that the client will attempt to connect to the
    server, in milliseconds (#216).

  * The hostname and current timestamp are no longer used in the
    cookie used to associate the client and server.  Instead, random
    data is used.  Note that iperf3 now requires the /dev/urandom
    device (#582).

  * Prior versions of iperf3 doing UDP tests used to overcount packet
    losses in the presence of packet reordering.  This has been
    (partially) fixed by try to not count the sequence number gaps
    resulting from out-of-order packets as actual losses (#457).

  * iperf3 no longer prints results from very small intervals (10% of
    the statistics reporting interval) at the end of the test run if
    they contain no data.  This can happen due to timing difference or
    network queueing on the path between the client and server.  This
    is primarily a cosmetic change to prevent these fairly meaningless
    intervals from showing up in the output (#278).

  * Compatibility note: Users running iperf3 3.2 or newer from the
    bwctl utility will need to obtain version 1.6.3 or newer of bwctl.
    Note that bwctl, a component of the perfSONAR toolkit, has been
    deprecated in favor of pScheduler since the release of perfSONAR
    4.0.

* Developer-visible changes

  * Various warnings and build fixes (#551, #564, #518, #597).

  * Some improvements have been made for increased compatibility on
    IRIX (#368) and with C++ (#587).

  * cJSON has been updated to 1.5.2 (#573), bringing in a number of
    bugfixes.

  * Some dead code has been removed.

iperf 3.1.7 2017-03-06
----------------------

iperf 3.1.7 is functionally identical to iperf 3.1.6.  Its only
changes consist of updated documentation files and text in the RPM
spec file.

iperf 3.1.6 2017-02-02
----------------------

The release notes for iperf 3.1.6 describe changes, including bug
fixes and new functionality, made since iperf 3.1.5.

* User-visible changes

  * Specifying --fq-rate or --no-fq-socket-pacing on a system where
    these options are not supported now generate an error instead of a
    warning.  This change makes diagnosing issues related to pacing
    more apparent.

  * Fixed a bug where two recently-added diagnostic messages spammed
    the JSON output on UDP tests.

iperf 3.1.5 2017-01-12
----------------------

The release notes for iperf 3.1.5 describe changes, including bug
fixes and new functionality, made since iperf 3.1.4.

Compatibility note: Fair-queueing is now specified differently in
iperf 3.1.5 than in prior versions (which include 3.1.3 and 3.1.4).

Compatibility note: UDP tests may yield different results from all
prior versions of iperf3 (through 3.1.4) due to the new default UDP
sending size.

* User-visible changes

  * The fair-queueing per-socket based pacing available on recent
    Linux systems has been reimplemented with a different user
    interface (#325, #467, #488).  The --bandwidth command-line flag
    now controls only the application-level pacing, as was the case in
    iperf 3.1.2 and all earlier versions.  Fair-queueing per-socket
    based pacing is now controlled via a new --fq-rate command-line
    flag.  Note that TCP and UDP tests may use --bandwidth, --fq-rate,
    both flags, or neither flag.  SCTP tests currently support
    --bandwidth only.  The --no-fq-socket-pacing flag, which was
    introduced in iperf 3.1.3, has now been deprecated, and is
    equivalent to --fq-rate=0.  iperf3 now reacts more gracefully if
    --no-fq-socket-pacing or --fq-rate are specified on platforms that
    don't support these options.

    For UDP tests, note that the default --bandwidth is 1 Mbps.  Using
    the fair-queueing-based pacing will probably require explicitly
    setting both --bandwidth and --fq-rate, preferably to the same
    value.  (While setting different values for --bandwidth and
    --fq-rate can certainly be done, the results can range from
    entertaining to perplexing.)

  * iperf3 now chooses a more sane default UDP send size (#496, #498).
    The former default (8KB) caused IP packet fragmentation on paths
    having smaller MTUs (including any Ethernet network not configured
    for jumbo frames).  This could have effects ranging from increased
    burstiness, to packet loss, to complete failure of the test.
    iperf3 now attempts to use the MSS of the control connection to
    determine a default UDP send size if no sending length was
    explicitly specified with --length.

  * Several checks are now made when setting the socket buffer sizes
    with the -w option, to verify that the settings have been applied
    correctly.  The results of these checks can been seen when the
    --debug flag is specified.  (#356)

  * A --forceflush flag has been added to flush the output stream
    after every statistics reporting interval.

* Developer-visible changes

  * A systemd service file has been added (#340, #430).

iperf 3.1.4 2016-10-31
----------------------

The release notes for iperf 3.1.4 describe changes, including bug
fixes and new functionality, made since iperf 3.1.3.

* User-visible changes

  * On systems that support setting the congestion control algorithm,
    iperf3 now keeps track of the congestion control algorithm and
    print it in the JSON output in the members sender_tcp_congestion
    and receiver_tcp_congestion (issue #461).  A few bugs (probably
    not user-visible) with setting the congestion control algorithm
    were also fixed.

* Developer-visible changes

  * Fixed a buffer overflow in the cJSON library (issue #466).  It is
    not believed that this bug created any security vulnerabilities in
    the context of iperf3.

  * Travis CI builds are now enabled on this codeline (pull request #424).

  * Various bug fixes (issue #459, pull request #429, issue #388).

iperf 3.1.3 2016-06-08
----------------------

The release notes for iperf 3.1.3 describe changes, including bug
fixes and new functionality, made since iperf 3.1.2.

* Security

  * Fixed a buffer overflow / heap corruption issue that could occur
    if a malformed JSON string was passed on the control channel.  In
    theory, this vulnerability could be leveraged to create a heap
    exploit.  This issue, present in the cJSON library, was already
    fixed upstream, so was addressed in iperf3 by importing a newer
    version of cJSON (plus local ESnet modifications).  Discovered and
    reported by Dave McDaniel, Cisco Talos.  Cross-references:
    TALOS-CAN-0164, ESNET-SECADV-2016-0001, CVE-2016-4303.

* User-visible changes

  * On supported platforms (recent Linux), iperf3 can use
    fair-queueing-based per-socket pacing instead of its own
    application-level pacing for the --bandwidth option.
    Application-level pacing can be forced with the
    -no-fq-socket-pacing flag.

  * A bug that could show negative loss counters with --udp and --omit
    has been fixed (issue #412, pull request #414).

  * Error handling has been made slightly more robust.  Also, the
    iperf3 server will no longer exit after five consecutive errors,
    but will only exit for certain types of errors that prevent it
    from participating in any tests at all.

* Developer-visible changes

  * Fixed the build on FreeBSD 11-CURRENT (issue #413).

  * Fixed various coding errors (issue #423, issue #425).

iperf 3.1.2 2016-02-01
----------------------

The release notes for iperf 3.1.2 describe changes, including bug
fixes and new functionality, made since iperf 3.1.1.

* User-visible changes

  * Fixed a bug that caused nan values to be emitted (incorrectly)
    into JSON, particularly at the end of UDP tests (issue #278).

  * Fixed a bug that caused the wrong value to be printed for
    out-of-order UDP packets (issue #329).

  * Added a contrib/ directory containing a few submitted graphing
    scripts.

* Developer-visible changes

iperf 3.1.1 2015-11-19
----------------------

The release notes for iperf 3.1.1 describe changes and new
functionality in iperf 3.1.1, but not present in 3.1.

* User-visible changes

  * Some markup fixes have been made in the manpages for Debian
    compatibility (issue #291).

  * A bug where the -T title option was not being output correctly
    in JSON output has been fixed (issue #292).

  * Argument handling for some command-line options has been improved
    (issue #316).

* Developer-visible changes

  * A regression with C++ compatibility in one of the iperf header
    files has been fixed (issue #323).

iperf 3.1 2015-10-16
--------------------

The release notes for iperf 3.1 describe changes and new
functionality in iperf 3.1, but not present in 3.0.11 or any earlier
3.0.x release.

* Selected user-visible changes

  * SCTP support has been added (with the --sctp flag), on Linux,
    FreeBSD, and Solaris (issue #131).

  * Setting CPU affinity now works on FreeBSD.

  * Selection of TCP congestion now works on FreeBSD, and is now
    called --congestion (the old --linux-congestion option works
    but is now deprecated).

  * A new -I option for the server causes it to write a PID file,
    mostly useful for daemon mode (issue #120).

  * A --logfile argument can now force all output to go to a file,
    rather than to a file.  This is especially useful when running an
    iperf3 server in daemon mode (issue #119).

  * Various compatibility fixes for Android (issue #184, issue #185),
    iOS (issue #288), NetBSD (issue #248), Solaris (issue #175, issue
    #178, issue #180, issue #211), vxWorks (issue #268).

  * A --udp-counters-64bit flag has been added to support very
    long-running UDP tests, which could cause a counter to overflow
    (issue #191).

  * A --cport option to specify the client-side port has been added
    (issue #207, issue #209, issue #239).

  * Some calculation errors with the -O feature have been fixed (issue
    #236).

  * A potential crash in the iperf3 server has been fixed (issue #257,
    issue #258).

  * Many miscellaneous bug fixes.

* Selected developer-visible changes

  * Consumers of libiperf can now get the JSON output for a
    just-completed test (issue #147).

  * Detection of various optional features has been improved to check
    for the presence or absence of platform functionality, not the name
    of platforms.

  * Out-of-tree builds now work (issue #265).

iperf 3.0.11 2015-01-09
-----------------------

* User-visible changes

  * Added -1 / --one-off flag, which causes the iperf3 server to
    process one client connection and then exit.  Intended primarily
    for bwctl integration (issue #230).

  * Added various minor bug fixes (issues #231, #232, #233).

  * Added 30-second timeout for UDP tests if unable to establish UDP
    connectivity between sender and receiver (issue #222).

iperf 3.0.10 2014-12-16
-----------------------

* User-visible changes

  * Fixed the build on MacOS X Yosemite (issue #213).

  * UDP tests now honor the -w option for setting the socket buffer
    sizes (issue #219).

* Developer-visible changes

  * Added an RPM spec file plus functionality to fill in the version
    number.

  * Fixed potential filename collision with a system header (issue
    #203).

iperf 3.0.9 2014-10-14
----------------------

* User-visible changes

  * Fixed a series of problems that came from attempting a UDP test
    with a pathologically large block size.  This put the server into
    an odd state where it could not accept new client connections.
    This in turn caused subsequent client connections to crash when
    interrupted (issue #212).

* Developer-visible changes

  * None.

iperf 3.0.8 2014-09-30
----------------------

* User-visible changes

  * Updated license and copyright verbage to confirm to LBNL Tech
    Transfer requirements.  No substantive changes; license remains
    the 3-clause BSD license.

* Developer-visible changes

  * None.

iperf 3.0.7 2014-08-28
----------------------

* User-visible changes

  * A server bug where new connections from clients could disrupt
    running tests has been fixed (issue #202).

  * Rates now consistently use 1000-based prefixes (K, M, G), where
    sizes of objects now consistently use 1024-based prefixes (issue #173).

  * UDP tests with unlimited bandwidth are now supported (issue #170).

  * An interaction between the -w and -B options, which kept them from
    working when used together, has been fixed (issue #193).

* Developer-visible changes

iperf 3.0.6 2014-07-28
----------------------

* User-visible changes

  * Several bugs that kept the -B option from working in various
    circumstances have been fixed (issue #193).

  * Various compatibility fixes for OpenBSD (issue #196) and
    Solaris (issue #177).

* Developer-visible changes

  * The {get,set}_test_bind_address API calls have been added to
    expose the -B functionality to API consumers (issue #197).

iperf 3.0.5 2014-06-16
----------------------

* User-visible changes

  * Erroneous output when doing --json output has been fixed (this
    problem was caused by an attempt to fix issue #158).

  * The maximum test running time has been increased from one hour to
    one day (issue #166).

  * Project documentation has been moved to GitHub Pages at this URL:
    http://software.es.net/iperf/.

  * A bug that caused CPU time to be computed incorrectly on FreeBSD
    has been fixed.

  * A timing issue which caused measurement intervals to be wrong
    with TCP tests on lossy networks has been fixed (issue #125).

  * Newer versions of autoconf / automake / libtool are now used by
    default (issue #161).

  * JSON output now indicates whether the test was run in --reverse
    mode (issue #167).

  * It is now possible to get (most of) the server-side output at
    the client by using the --get-server-output flag (issue #160).

* Developer-visible changes

  * automake/autoconf/libtool have been updated to more recent
    versions.  AM_MAINTAINER_MODE is now used to avoid requiring these
    tools at build-time.

iperf 3.0.4 was not released
----------------------------

iperf 3.0.3 2014-03-26
----------------------

* User-visible changes

  * Due to several oversights, the source code archive for iperf 3.0.2
    was distributed as an uncompressed tarball, despite having an
    extension (".tar.gz") that indicated it was compressed.  The
    release generation procedure has been changed to avoid this
    problem going forward.

  * Summary structures in the JSON output are now included, even if
    there is only one stream.  This change makes consuming the JSON
    output easier and more consistent (issue #151).

  * A possible buffer overflow in iperf_error.c has been fixed (issue
    #155).

* Developer-visible changes

  * Example programs now build correctly, after having been broken in
    the 3.0.2 release (issue #152).

iperf 3.0.2 2014-03-10
----------------------

* User-visible changes

  * The iperf3 project has been moved to GitHub, and various URLs in
    documentation files have been changed to point there.

  * iperf3 now builds on Linux systems that do not support
    TCP_CONGESTION.  Most notably this allows iperf3 to work on CentOS
    5.

  * An abort on MacOS 10.9 has been fixed (issue #135).

  * Added -I flag for the server to write a PID file, mostly useful for
    daemon mode (issue #120).

  * A bug that could break some TCP tests on FreeBSD has been fixed.

  * TCP snd_cwnd output is now printed by default on Linux (issue #99).

  * In JSON output, the --title string no longer has a colon and two
    spaces appended (issue #139).

  * A buffer for holding formatted numeric values is now
    properly-sized so that output is not truncated (issue #142).

* Developer-visible changes

  * Some memory leaks have been fixed.

  * A -d flag enables debugging output.

  * A .gitignore file has been added.

  * libtoolize is now invoked correctly from the bootstrap.sh script.

  * The test unit format can now be set from the API (issue #144).

  * libiperf is now built as both shared and static libraries.

  * In the JSON output, the "connection" structures are now stored as
    an array in the "start" block, instead of overwriting each other.
    While technically an incompatible API change, the former behavior
    generated unusable JSON.

iperf 3.0.1 2014-01-10
----------------------

  * Added the following new flags
     -D, --daemon	       run server as a daemon
     -L, --flowlabel           set IPv6 flow label (Linux only)
     -C, --linux-congestion    set congestion control algorithm (Linux only)
     -k, --blockcount #[KMG]   number of blocks (packets) to transmit
     	 	      	       (instead of -t or -n)
  * Bug fixes

iperf 3.0-RC5 2013-11-15
------------------------

  * Added the following new flags
     -F, --file name           xmit/recv the specified file
     -A, --affinity n/n,m      set CPU affinity (Linux only)
     -J, --json                output in JSON format
     -Z, --zerocopy            use a 'zero copy' method of sending data
     -O, --omit N              omit the first n seconds
     -T, --title str           prefix every output line with this string
  * more useful information in 'verbose' mode
  * Many bug fixes


iperf 3.0b4 2010-08-02
----------------------

  * Added support for binding to a specific interface (-B)
  * Added support for IPv6 mode (-6)
  * Setting TCP window size (-w) is now supported
  * Updates to iperf_error
      * Added new errors
      * Should generate more relevant messages
  * Stream list now managed by queue.h macros
  * Test structures are now kept intact after a test is run (for API users)
  * Improved interval timer granularity
      * Support for decimal values
  * Many bug fixes

iperf 3.0b3 2010-07-23
----------------------

  * Better error handling
      * All errors now handled with iperf_error()
      * All functions that can return errors return NULL or -1 on error and set i_errno appropriately
  * Iperf API introduced
      * Support for adding new protocols
      * Added support for callback functions
          * on_connect - executes after a connection is made to the server
          * on_new_stream - executes after a new stream is created
          * on_test_start - executes right before the test begins
          * on_test_finish - executes after the test is finished
  * Added early support for verbose mode (-V)

iperf 3.0b2 2010-07-15
----------------------

  * UDP mode now supported
      * Support for setting bandwidth (-b)
      * Parallel UDP stream support
      * Reverse mode UDP support
  * Support for setting TCP_NODELAY (-N), disabling Nagle's Algorithm
  * Support for setting TCP MSS (-M)
      * Note: This feature is still in development. It is still very buggy.

iperf 3.0b1 2010-07-08
----------------------

  * TCP control socket now manages messages between client and server
  * Dynamic server (gets test parameters from client)
      * Server can now set test options dynamically without having to restart.
          * Currently supported options: -l, -t, -n, -P, -R
          * Future options: -u, -b, -w, -M, -N, -I, -T, -Z, -6
  * Results exchange
      * Client can now see server results (and vice versa)
  * Reverse mode (-R)
      * Server sends, client receives
