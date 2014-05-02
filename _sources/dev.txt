iperf3 Development
==================

The iperf3 project is hosted on GitHub at:

http://github.com/esnet/iperf

This site includes the source code repository, issue tracker, and
wiki.

Mailing Lists
-------------

The developer list for iperf3 is:  iperf-dev@googlegroups.com.
Information on joining the mailing list can be found at:

http://groups.google.com/group/iperf-dev

There is, at the moment, no mailing list for user questions, although
a low volume of inquiries on the developer list is probably
acceptable.  If necessary, a user-oriented mailing list might be
created in the future.

Bug Reports
-----------

Before submitting a bug report, try checking out the latest version of
the code, and confirm that it's not already fixed.  Then submit to the
iperf3 issue tracker on GitHub:

https://github.com/esnet/iperf/issues

**Note:** Issues submitted to the old iperf3 issue tracker on Google
Code (or comments to existing issues on the Google Code issue tracker)
will be ignored.

Changes from iperf 2.x
----------------------

New options (not necessarily complete, please refer to the manual page
for a complete list of iperf3 options)::

    -V, --verbose             more detailed output than before
    -J, --json                output in JSON format
    -Z, --zerocopy            use a 'zero copy' sendfile() method of sending data
    -O, --omit N              omit the first n seconds (to ignore slowstart)
    -T, --title str           prefix every output line with this string
    -F, --file name           xmit/recv the specified file
    -A, --affinity n/n,m      set CPU affinity (Linux and FreeBSD only)
    -k, --blockcount #[KMG]   number of blocks (packets) to transmit (instead 
                              of -t or -n)
    -L, --flowlabel           set IPv6 flow label (Linux only)

Changed flags::

    -C, --linux-congestion    set congestion control algorithm (Linux only)
                              (-Z in iperf2)


Deprecated flags (currently no plans to support)::

    -d, --dualtest           Do a bidirectional test simultaneously
    -r, --tradeoff           Do a bidirectional test individually
    -T, --ttl                time-to-live, for multicast (default 1)
    -x, --reportexclude [CDMSV]   exclude C(connection) D(data) M(multicast) 
                                  S(settings) V(server) reports
    -y, --reportstyle C      report as a Comma-Separated Values

Also deprecated is the ability to set the options via environment
variables.

Known Issues
------------

The following problems are notable known issues, which are probably of
interest to a large fraction of users or have high impact for some
users, and for which issues have already been filed in the issue
tracker.  These issues are either open (indicating no solution
currently exists) or closed with the notation that no further attempts
to solve the problem are currently being made:

* UDP performance: Some problems have been noticed with iperf3 on the
  ESnet 100G testbed at high UDP rates (above 10Gbps).  The symptom is
  that on any particular run of iperf3 the receiver reports a loss
  rate of about 20%, regardless of the ``-b`` option used on the client
  side.  This problem appears not to be iperf3-specific, and may be
  due to the placement of the iperf3 process on a CPU and its relation
  to the inbound NIC.  In some cases this problem can be mitigated by
  an appropriate use of the CPU affinity (``-A``) option.  (Issue #55)

* Interval reports on high-loss networks: The way iperf3 is currently
  implemented, the sender write command will block until the entire
  block has been written. This means that it might take several
  seconds to send a full block if the network has high loss, and the
  interval reports will have widely varying interval times.  A
  solution is being discussed, but in the meantime a work around is to
  try using a small block size, for example ``-l 4K``.  (Issue #125,
  a fix will be released in iperf 3.1)

* The ``-Z`` flag sometimes causes the iperf3 client to hang on OSX.
  (Issue #129)

* On OpenBSD, the server seems to require a ``-4`` argument, implying
  that it can only be used with IPv4.  (Issue #108)

* When specifying the TCP buffer size using the ``-w`` flag on Linux,
  the Linux kernel automatically doubles the value passed in to
  compensate for overheads.  (This can be observed by using
  iperf3's ``--debug`` flag.)  However, CWND does not actually ramp up
  to the doubled value, but only to about 75% of the doubled
  value.  Some part of this behavior is documented in the tcp(7)
  manual page.  (Issue #145)

* On some platforms (observed on at least one version of Ubuntu
  Linux), it might be necessary to invoke ``ldconfig`` manually after
  doing a ``make install`` before the ``iperf3`` executable can find
  its shared library.  (Issue #153)

There are, of course, many other open and closed issues in the issue
tracker.

Versioning
----------

iperf3 version numbers use (roughly) a `Semantic Versioning
<http://semver.org/>`_ scheme, in which version numbers consist of
three parts:  *MAJOR.MINOR.PATCH*

The developers increment the:

* *MAJOR* version when making incompatible API changes,

* *MINOR* version when adding functionality in a backwards-compatible manner, and

* *PATCH* version when making backwards-compatible bug fixes.

Release Engineering Checklist
-----------------------------

1. Update the ``README`` and ``RELEASE_NOTES`` files to be accurate. Make sure
   that the "Known Issues" section of the ``README`` file is up to date.

2. Compose a release announcement.  Most of the release announcement
   can be written before tagging.

3. Starting from a clean source tree (be sure that ``git status`` emits
   no output)::

    vi src/version.h   # update the strings IPERF_VERSION and IPERF_VERSION_DATE
    vi configure.ac    # update version parameter in AC_INIT
    vi src/iperf3.1    # update manpage revision date if needed
    vi src/libiperf.3  # update manpage revision date if needed
    git commit -a
    ./bootstrap.sh     # regenerate configure script, etc.
    git commit -a
    git push

    ./make_release tag  # this creates a tag in the local repo that matches the version.h version
    git push --tags     # Push the new tag to the GitHub repo
    ./make_release tar  # create tarball and compute SHA256 hash

   These steps should be done on a platform with a relatively recent
   version of autotools / libtools.  Examples are MacOS / MacPorts or
   FreeBSD.  The versions of these tools in CentOS 6 are somewhat
   older and probably should be avoided.

4. Stage the tarball (and a file containing the SHA256 hash) to the
   download site.  Currently this is located on ``stats.es.net``.

5. From another host, test the link in the release announcement by
   downloading a fresh copy of the file and verifying the SHA256
   checksum.

6. Also verify (with file(1)) that the tarball is actually a gzipped
   tarball.

7. For extra points, actually try downloading, compiling, and
   smoke-testing the results of the tarball on all supported
   platforms.
   
8. Plug the SHA256 checksum into the release announcement.

9. Send the release announcement (PGP-signed) to these addresses:

   * iperf-dev@googlegroups.com

   * iperf-users@lists.sourceforge.net

   * perfsonar-user@internet2.edu

   * perfsonar-dev@internet2.edu

10.  Update the iperf3 Project News section of the documentation site
     to announce the new release (see ``docs/news.rst`` in the source
     tree) and deploy a new build of the documentation to GitHub
     Pages.

