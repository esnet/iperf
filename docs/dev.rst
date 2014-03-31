iperf3 Development
==================

The iperf3 project is hosted on GitHub at:

http://github.com/esnet/iperf

This site includes the source code repository, issue tracker, and
wiki.

Mailing Lists
-------------

The developer list for iperf3 is:  iperf3-dev@googlegroups.com.
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

**Note:**  Issues submitted to the old iperf3 issue tracker on Google
Code will be ignored.

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

* UDP performance: iperf2/iperf3 both only are only about 50% as fast
  as nuttcp in UDP mode.  We are looking into this, but in the
  meantime, if you want to get UDP above 5Gbps, we recommend using
  nuttcp instead (http://www.nuttcp.net/).  (Issue #55)

* Interval reports on high-loss networks: The way iperf3 is currently
  implemented, the sender write command will block until the entire
  block has been written. This means that it might take several
  seconds to send a full block if the network has high loss, and the
  interval reports will have widely varying interval times. We are
  trying to determine the best solution to this, but in the meantime,
  try using a smaller block size if you get strange interval reports.
  For example, try ``-l 4K``.  (Issue #125)

* The ``-Z`` flag sometimes hangs on OSX.  (Issue #129)

* On OpenBSD, the server seems to require a ``-4`` argument, implying
  that it can only be used with IPv4.  (Issue #108)

* When specifying the TCP buffer size using the ``-w`` flag on Linux,
  Linux doubles the value you pass in. (You can see this using
  iperf3's debug flag.)  But then the CWND does not actually ramp up
  to the doubled value, but only to about 75% of the doubled
  value. This appears to be by design.  (Issue #145)

There are, of course, many other open and closed issues in the issue
tracker.

Versioning
----------

iperf version numbers use three-part release numbers:  *MAJOR.MINOR.PATCH*

The developers increment the:

* *MAJOR* version when making incompatible API changes,

* *MINOR* version when adding functionality in a backwards-compatible manner, and

* *PATCH* version when making backwards-compatible bug fixes.

This is roughly along the line of `Semantic Versioning <http://semver.org/>`_.

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

   Doing the above steps on CentOS 6 (with its somewhat older
   autotools / libtools suite) is preferred; newer systems generate
   ``configure`` and ``Makefile`` scripts that tend to rebuild
   themselves rather frequently.  We might be able to address this
   problem (and graduate to newer autotools) by using
   ``AC_MAINTAINER_MODE`` but there's a fair amount of religion
   associated with this.

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

