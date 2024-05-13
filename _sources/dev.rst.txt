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
the code, and confirm that it's not already fixed. Also see the :doc:`faq`.
Then submit to the iperf3 issue tracker on GitHub:

https://github.com/esnet/iperf/issues

For reporting potential security issues, please contact the developers at
iperf@es.net.

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

* The ``-Z`` flag sometimes causes the iperf3 client to hang on OSX.
  (Issue #129)

* When specifying the TCP buffer size using the ``-w`` flag on Linux,
  the Linux kernel automatically doubles the value passed in to
  compensate for overheads.  (This can be observed by using
  iperf3's ``--debug`` flag.)  However, CWND does not actually ramp up
  to the doubled value, but only to about 75% of the doubled
  value.  Some part of this behavior is documented in the tcp(7)
  manual page.

* Although the ``-w`` flag is documented as setting the (TCP) window
  size, it is also used to set the socket buffer size.  This has been
  shown to be helpful with high-bitrate UDP tests.

* On some platforms (observed on at least one version of Ubuntu
  Linux), it might be necessary to invoke ``ldconfig`` manually after
  doing a ``make install`` before the ``iperf3`` executable can find
  its shared library.  (Issue #153)

* The results printed on the server side at the end of a test do not
  correctly reflect the client-side measurements.  This is due to the
  ordering of computing and transferring results between the client
  and server.  (Issue #293)

* The server could have a very short measurement reporting interval at
  the end of a test (particularly a UDP test), containing few or no
  packets.  This issue is due to an artifact of timing between the
  client and server.  (Issue #278)

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

1. Start from a clean source tree (be sure that ``git status`` emits
   no output). Also ensure up-to-date installs of ``autoconf`` and
   ``automake``.

2. Ensure that ``README.md`` and ``LICENSE`` have correct copyright
   dates.

3. Update the ``README.md`` and ``RELNOTES.md`` files to be accurate. Make sure
   that the "Known Issues" section of the ``README.md`` file and in this document
   are up to date.

4. Compose a release announcement.  Most of the release announcement
   can be written before tagging.  Usually the previous version's
   announcement can be used as a starting point.

5. Make the changes necessary to produce
   the new version, such as bumping version numbers::

    vi RELNOTES.md     # update version number and release date
    vi configure.ac    # update version parameter in AC_INIT
                       # (there should not be any "+" in artifacts)
    vi src/iperf3.1    # update manpage revision date (only if needed)
    vi src/libiperf.3  # update manpage revision date (only if needed)
    git commit -a      # commit changes to the local repository only
                       # (commit log should mention version number)
    ./bootstrap.sh     # regenerate configure script, etc.
    git commit -a      # commit changes to the local repository only
                       # (commit can be simply "Regen.")

    # Assuming that $VERSION is the version number to be released...
    ./make_release tag $VERSION # this creates a tag in the local repo
    ./make_release tar $VERSION # create tarball and compute SHA256 hash

   These steps should be done on a platform with a relatively recent
   version of autotools / libtools.  Examples are MacOS / MacPorts or
   FreeBSD.  The versions of these tools in CentOS and similar
   distributions are somewhat
   older and probably should be avoided.

   The result will be release artifacts that should be used for
   pre-testing. One will be a compressed tarball
   (e.g. ``iperf-3.17.1.tar.gz``) and the other will contain SHA256
   checksum (e.g. ``iperf-3.17.1.tar.gz.sha256``)

6. Stage the tarball (and a file containing the SHA256 hash) to the
   download site.  Currently this is located on ``downloads.es.net``
   in the directory ``/var/www/html/pub/iperf/``.

7. From another host, test the link in the release announcement by
   downloading a fresh copy of the file and verifying the SHA256
   checksum.  Checking all other links in the release announcement is
   strongly recommended as well.

   The link to the tarball will be something of the form
   ``https://downloads.es.net/pub/iperf/iperf-3.17.1.tar.gz``. If
   composing a release announcement using a HTML-aware editor, verify
   the link targets point to the correct artifacts.

8. Also verify (with file(1)) that the tarball is actually a gzipped
   tarball.

9. Try downloading, compiling, and smoke-testing the results of the
   tarball on all supported platforms.

10. Verify that the version string in ``iperf3 --version`` matches the
    version number of the artifacts.

11. Plug the SHA256 checksum into the release announcement.

12. (optional) PGP-sign the release announcement text using ``gpg
    --clearsign``.  The signed announcement will be sent out in a
    subsequent emails, but could also be archived.  Decoupling the
    signing from emailing allows a signed release announcement to be
    resent via email or sent by other, non-email means.

13. At this point, the release can and should be considered
    finalized.  To commit the release-engineering-related changes to
    GitHub and make them public, push them out thusly::

     git push            # Push version changes
     git push --tags     # Push the new tag to the GitHub repo

14. Update GitHub Releases with the current release notes. Start from:
    ``https://github.com/esnet/iperf/releases/new``. Remember to
    properly select the tag from the dropdown menu and drop
    the artifacts into the GitHub Release. Check "Set as the latest
    release" and (optionally) "Create a discussion for this release".

15. Send the release announcement to the following
    addresses.  Remember to turn off signing in the MUA, if
    applicable.  Remember to check the source address when posting to
    lists, as "closed" list will reject posting from all from
    registered email addresses.

    * iperf-dev@googlegroups.com

    * iperf-users@lists.sourceforge.net

    * perfsonar-user@internet2.edu

    * perfsonar-developer@internet2.edu

    Note: Thunderbird sometimes mangles the PGP-signed release
    announcement so that it does not verify correctly.  This could be
    due to Thunderbird trying to wrap the length of extremely long
    lines (such as the SHA256 hash).  Apple Mail and mutt seem to
    handle this situation correctly.  Testing the release announcement
    sending process by sending a copy to oneself first and attempting
    to verify the signature is highly encouraged.

16. Announce the new release in the #iperf3 channel in ESnet Slack.

17. Update the iperf3 Project News section of the documentation site
    to announce the new release (see ``docs/news.rst`` and
    ``docs/conf.py`` in the source tree) and deploy a new build of the
    documentation to GitHub Pages. Be sure to double-check version
    numbers and copyright dates.

18. If an update to the on-line manual page is needed, it can be
    generated with this sequence of commands (tested on CentOS 7) and
    import the result into ``invoking.rst``::

     TERM=
     export TERM
     nroff -Tascii -c -man src/iperf3.1 | ul | sed 's/^/   /' > iperf3.txt

19. Update the version number in ``configure.ac`` to some
    post-release number (with a "+") and regenerate::

      vi configure.ac         # update version in AC_INIT, add "+"
      git commit configure.ac # commit changes to local repository
                              # commit log should mention
                              # "post-release version bump"
      ./bootstrap.sh          # regenerate configure script, etc.
      git commit -a           # commit changes to local repository
                              # (commit can be simply "Regen.")
      # test
      git push

Code Authors
------------

The main authors of iperf3 are (in alphabetical order):  Jon Dugan,
Seth Elliott, Bruce A. Mah, Jeff Poskanzer, Kaustubh Prabhu.
Additional code contributions have come from (also in alphabetical
order):  Mark Ashley, Aaron Brown, Aeneas Jaißle, Susant Sahani,
Bruce Simpson, Brian Tierney.

iperf3 contains some original code from iperf2.  The authors of iperf2
are (in alphabetical order): Jon Dugan, John Estabrook, Jim Ferbuson,
Andrew Gallatin, Mark Gates, Kevin Gibbs, Stephen Hemminger, Nathan
Jones, Feng Qin, Gerrit Renker, Ajay Tirumala, Alex Warshavsky.
