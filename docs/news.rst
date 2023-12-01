iperf3 Project News
===================

2023-07-07:  iperf-3.14 released
--------------------------------
| URL:  https://downloads.es.net/pub/iperf/iperf-3.14.tar.gz
| SHA256:  ``723fcc430a027bc6952628fa2a3ac77584a1d0bd328275e573fc9b206c155004``

iperf 3.14 fixes a memory allocation hazard that allowed a remote user
to crash an iperf3 process (server or client).

More information on this specific fix can be found at:

https://downloads.es.net/pub/iperf/esnet-secadv-2023-0001.txt.asc

This version of iperf3 also includes a number of minor bug fixes,
which are summarized in the release notes.

2023-02-16:  iperf-3.13 released
----------------------------------
| URL:  https://downloads.es.net/pub/iperf/iperf-3.13.tar.gz
| SHA256:  ``bee427aeb13d6a2ee22073f23261f63712d82befaa83ac8cb4db5da4c2bdc865``

iperf 3.13 is primarily a bugfix release.


2022-09-30:  iperf-3.12 released
----------------------------------
| URL:  https://downloads.es.net/pub/iperf/iperf-3.12.tar.gz
| SHA256:  ``72034ecfb6a7d6d67e384e19fb6efff3236ca4f7ed4c518d7db649c447e1ffd6``

iperf 3.12 is principally a bugfix release, although it includes an
updated version of cJSON and adds a few new features.


2022-01-28:  iperf-3.11 released
----------------------------------
| URL:  https://downloads.es.net/pub/iperf/iperf-3.11.tar.gz
| SHA256:  ``de8cb409fad61a0574f4cb07eb19ce1159707403ac2dc01b5d175e91240b7e5f``

iperf 3.11 is principally a bugfix release. Also GitHub
Discussions are now supported.


2021-06-02:  iperf-3.10.1 released
----------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.10.1.tar.gz
| SHA256:  ``03bc9760cc54a245191d46bfc8edaf8a4750f0e87abca6764486972044d6715a  iperf-3.10.1.tar.gz``

iperf 3.10.1 fixes a problem with the configure script that made it
make not work correctly in some circumstances. It is functionally
identical to iperf 3.10.

2021-05-26:  iperf-3.10 released
--------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.10.tar.gz
| SHA256:  ``4390982928542256c17d6dd1f56eede9092649ebfd8a97c8cecfad12d238ad57  iperf-3.10.tar.gz``

iperf 3.10 is principally a bugfix release. A few new features have
been added (``--time-skew-threshold``, ``--bind-dev``,
``--rcv-timeout``, and ``--dont-fragment``).  More information on
these new features can be found in the release notes.

2020-08-17:  iperf-3.9 released
---------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.9.tar.gz
| SHA256:  ``24b63a26382325f759f11d421779a937b63ca1bc17c44587d2fcfedab60ac038  iperf-3.9.tar.gz``

iperf 3.9 adds a ``--timestamps`` flag, which prepends a timestamp to
each output line.  A new ``--server-bitrate-limit`` flag has been
added as a server command-line argument, and allows an iperf3 server
to enforce a maximum throughput rate.  More information on these new
features can be found in the release notes.

2020-06-10:  iperf-3.8.1 released
---------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.8.1.tar.gz
| SHA256:  ``e5b080f3273a8a715a4100f13826ac2ca31cc7b1315925631b2ecf64957ded96 iperf-3.8.1.tar.gz``

iperf 3.8.1 fixes a regression with ``make install`` in iperf 3.8.  It
is otherwise identical to iperf 3.8.

2020-06-08:  iperf-3.8 released
-------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.8.tar.gz
| SHA256:  ``edc1c317b0ae31925e5eb84f0295faefbaa1db3229f4693e11d954d114de4bcd  iperf-3.8.tar.gz``

iperf 3.8 contains minor bugfixes and enhancements.


2019-06-21:  iperf-3.7 released
-------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.7.tar.gz
| SHA256:  ``d846040224317caf2f75c843d309a950a7db23f9b44b94688ccbe557d6d1710c  iperf-3.7.tar.gz``

iperf 3.7 adds the ``--bidir`` flag for bidirectional tests, includes
some minor enhancements, and fixes a number of bugs.  More details can
be found in the release notes.

Note:  Documentation for the ``--bidir`` flag was inadvertently
omitted from the manual page.  This will be fixed in a future
release.

2018-06-25:  iperf-3.6 released
-------------------------------

| URL:  https://downloads.es.net/pub/iperf/iperf-3.6.tar.gz
| SHA256:  ``de5d51e46dc460cc590fb4d44f95e7cad54b74fea1eba7d6ebd6f8887d75946e  iperf-3.6.tar.gz``

iperf 3.6 adds the ``--extra-data`` and ``--repeating-payload``
options and fixes some minor bugs.

2018-03-02:  iperf-3.5 released
-------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.5.tar.gz
| SHA256:  ``539bd9ecdca1b8c1157ff85b70ed09b3c75242e69886fc16b54883b399f72cd5  iperf-3.5.tar.gz``

iperf 3.5 fixes a bug that could over-count data transfers (and hence
measured bitrate).

2018-02-14:  iperf-3.4 released
-------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.4.tar.gz
| SHA256:  ``71528332d751df85e046d1944d9a0269773cadd6e51840aecdeed30925f79111  iperf-3.4.tar.gz``

iperf 3.4 fixes a number of minor bugs and adds a few enhancements.

2017-10-31:  iperf-3.3 released
-------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.3.tar.gz
| SHA256:  ``6f596271251056bffc11bbb8f17d4244ad9a7d4a317c2459fdbb853ae51284d8  iperf-3.3.tar.gz``

New minor release of iperf 3.3, fixing a number of minor bugs.

2017-06-26:  iperf-3.2 released
-------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.2.tar.gz
| SHA256:  ``f207b36f861485845dbdf09f909c62f3d2222a3cf3d2682095aede8213cd9c1d  iperf-3.2.tar.gz``

New minor release of iperf 3.2, with new features, bugfixes, and enhancements.

2017-06-06:  iperf3 update, June 2017
--------------------------------------

https://raw.githubusercontent.com/esnet/iperf/master/docs/2017-06-06.txt


2017-04-27:  iperf3 update, April 2017
--------------------------------------

https://raw.githubusercontent.com/esnet/iperf/master/docs/2017-04-27.txt


2017-03-06:  iperf-3.1.7 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.7.tar.gz
| SHA256:  ``a4ef73406fe92250602b8da2ae89ec53211f805df97a1d1d629db5a14043734f  iperf-3.1.7.tar.gz``

This version of iperf3 contains two documentation fixes, but is
otherwise identical to the prior release.


2017-02-02:  iperf-3.1.6 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.6.tar.gz
| SHA256:  ``70f0c72d9e60c6ecb2c478ed17e4fd81d3b827d57896fee43bcd0c53abccb29d  iperf-3.1.6.tar.gz``

This version of iperf3 contains two minor fixes.  Notably, one of them
unbreaks JSON output with UDP tests.


2017-01-12:  iperf-3.1.5 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.5.tar.gz
| SHA256:  ``6e1a6200cd38baeab58ef0d7b8769e7aa6410c3a3168e65ea8277a4de79e5500  iperf-3.1.5.tar.gz``

This version of iperf3 makes some improvements to the fair-queue-based
pacing and improves the selection of the default UDP packet size.
Users who use either of these aspects of iperf3 are encourage to
review the release notes for this version.


2016-10-31:  iperf-3.1.4 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.4.tar.gz
| SHA256:  ``db61d70ac62003ebe0bf15496bd8c4b3c4b728578a44d0a1a88fcf8afc0e8f76  iperf-3.1.4.tar.gz``

This release fixes a few minor bugs, including a
(non-security-impacting) buffer overflow fix ported from upstream
cjson.


2016-06-08:  Security Issue:  iperf-3.1.3, iperf-3.0.12 released
----------------------------------------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.3.tar.gz
| SHA256:  ``60d8db69b1d74a64d78566c2317c373a85fef691b8d277737ee5d29f448595bf  iperf-3.1.3.tar.gz``

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.12.tar.gz
| SHA256:  ``9393d646e4e616f0cd7864bc8ceacc379f5d36b08003a3d8d65cd7c99d15daec  iperf-3.0.12.tar.gz``

These releases address a security issue that could cause a crash of an
iperf3 process (it could theoretically lead to a remote code
execution).  Although the risk for common use cases is believed to be
low, all users are encouraged to update to these versions or newer as
soon as possible.  More information on the security vulnerability can
be found in the following ESnet Software Security Advisory:

https://raw.githubusercontent.com/esnet/security/master/cve-2016-4303/esnet-secadv-2016-0001.txt.asc

iperf-3.1.3 also includes support for fair-queueing, per-socket based
pacing of tests on platforms that support it (currently recent Linux
distributions), as well as several other fixes.


2016-02-01:  iperf-3.1.2 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.2.tar.gz
| SHA256:  ``f9dbdb99f869c077d14bc1de78675f5e4b8d1bf78dc92381e96c3eb5b1fd7d86  iperf-3.1.2.tar.gz``

This release fixes a couple of minor bugs, including one that results
in invalid JSON being emitted for UDP tests.

Older News
----------

2015-11-19:  iperf-3.1.1 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.1.tar.gz
| SHA256:  ``62f7c64eafe19046ba974b3ef2d962a5597194d6fbbddde328a15a5e74110564  iperf-3.1.1.tar.gz``

This release fixes a few minor bugs.

2015-10-16:  iperf3 Development Status
......................................

Beginning with the release of iperf 3.1, ESnet plans to support iperf3
in "maintenance mode".  At this point, we have no definite plans for
further iperf3 releases, and ESnet will be providing a very limited
amount of resources for support and development, going forward.
However, ESnet could issue new iperf3 releases to deal with security
issues or high-impact bug fixes.

Requests for support, enhancements, and questions should best be
directed to the iperf-dev mailing list.  ESnet would be open to adding
project members/committers from the community, in case there are
developers who are interested in doing more active work with iperf3
and/or supporting the user base.


2015-10-16:  iperf-3.1 released
...............................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.1.tar.gz
| SHA256:  ``4385a32ece25cb09d4606b4c99316356b3d2cb03b318aa056b99cdb91c5ce656  iperf-3.1.tar.gz``

This release adds support for SCTP on supported platforms, better
feature detection on FreeBSD, better compatibility with various
platforms, and a number of bug fixes.


2015-01-09:  iperf-3.0.11 released
..................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.11.tar.gz
| SHA256:  ``e01db5be6f47f67c987463095fe4f5b8b9ff891fb92c39104d042ad8fde97f6e  iperf-3.0.11.tar.gz``

This maintenance release adds a -1 flag to make the iperf3 execute a
single test and exit, needed for an upcoming bwctl release.  A few
other bugs are also fixed.

2014-12-16:  iperf-3.0.10 released
..................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.10.tar.gz
| SHA256:  ``a113442967cf0981b0b2d538be7c88903b2fb0f87b0d281384e41b462e33059d  iperf-3.0.10.tar.gz``

This maintenance release fixes building on MacOS X Yosemite, as well
as making the -w option work correctly with UDP tests.

2014-10-14:  iperf-3.0.9 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.9.tar.gz
| SHA256:  ``40249a2b30d26b937350b969bcb19f88e1beb356f886ed31422b554bac692459  iperf-3.0.9.tar.gz``

This maintenance release fixes an issue for a situation in which
attempting a UDP test with pathologically large (and illegal) packet
sizes could put the iperf3 server in a state where it would stop
accepting connections from clients, thus causing the clients to crash
when interrupted.


2014-09-30:  iperf-3.0.8 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.8.tar.gz
| SHA256:  ``81b8d91159862896c57f9b90a006e8b5dc22bd94175d97bd0db50b0ae2c1a78e  iperf-3.0.8.tar.gz``

This maintenance release is functionally identical to 3.0.7.  It
incorporates updated license verbage and a minor compilation fix.


2014-08-28:  iperf-3.0.7 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.7.tar.gz
| SHA256:  ``49510e886f9e876cd73dcd80414bfb8c49b147c82125585e09c2a6e92369d3f2  iperf-3.0.7.tar.gz``

This maintenance release fixes several minor bugs.  Of particular
note:

* A bug that caused some problems with bwctl / perfSONAR has been
  fixed.

* A bug that made it possible to disrupt existing, running tests has
  been fixed.

2014-07-28:  iperf-3.0.6 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.6.tar.gz
| SHA256:  ``3c5909c9b286b6503ffa141a94cfc588915d6e67f2aa732b08df0af73e21938  iperf-3.0.6.tar.gz``

This maintenance release includes the following bug fixes:

* Several problems with the -B option have been fixed.  Also, API
  calls have been added to libiperf to extend this functionality to
  API clients.

* Some portability fixes for OpenBSD and Solaris have been merged from
  the mainline.

As always, more details can be found in the ``RELNOTES.md`` file in
the source distribution.

2014-06-16:  Project documentation on GitHub Pages
..................................................

iperf3 project documentation can now be found at:

| URL:  http://software.es.net/iperf/

This is a GitHub Pages site.  In an ongoing series of steps, content
will be migrated from the iperf3 wiki to GitHub Pages.

2014-06-16:  iperf-3.0.5 released
.................................

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.5.tar.gz
| SHA256:  ``e1e1989985b17a4c03b0fa207004ad164b137e37ab0682fecbf5e93bcaa920a6  iperf-3.0.5.tar.gz``

This is the third maintenance release of iperf 3, with few more
enhancements and bug fixes.  Highlights:

* A timing issue which caused measurement intervals to be wrong with
  TCP tests on lossy networks has been fixed.

* It is now possible to get (most of) the server-side output at
  the client by using the ``--get-server-output`` flag.

* A number of bugs with ``--json`` output have been fixed.

A more extensive list of changes can always be found in the
``RELNOTES.md`` file in the source distribution.

Note:  An iperf-3.0.4 release was planned and tagged, but not
officially released.

2014-06-10:  New iperf3 download site
.....................................

iperf3 downloads are now hosted on a new server at ESnet:

| URL:  http://downloads.es.net/pub/iperf/

Going forward, new releases will be made available in this directory.
Older releases will, at least for now, continue to also be available
at the previous location.

2014-03-26:  iperf-3.0.3 released
.................................

| URL:  http://stats.es.net/software/iperf-3.0.3.tar.gz
| SHA256:  ``79daf3e5e5c933b2fc4843d6d21c98d741fe39b33ac05bd7a11c50d321a2f59d  iperf-3.0.3.tar.gz``

This is the second maintenance release of iperf 3.0, containing a few bug fixes and enhancements, notably:

* The structure of the JSON output is more consistent between the
  cases of one stream and multiple streams.

* The example programs once again build correctly.

* A possible buffer overflow related to error output has been fixed.
  (This is not believed to be exploitable.)

More information on changes can be found in the ``RELNOTES.md``
file in the source distribution.

2014-03-10:  iperf-3.0.2 released
.................................

| URL:  http://stats.es.net/software/iperf-3.0.2.tar.gz
| SHA256:  ``3c379360bf40e6ac91dfc508cb43fefafb4739c651d9a8d905a30ec99095b282  iperf-3.0.2.tar.gz``

**Note:**  Due to a mistake in the release process, the distribution tarball referred to above is actually not compressed, despite its ``.tar.gz`` extension.  Instead it is an uncompressed tar archive.  The file checksum is correct, as are the file contents.

This version is a maintenance release that
fixes a number of bugs, many reported by users, adds a few minor
enhancements, and tracks the migration of the iperf3 project to
GitHub.  Of particular interest:

* Build / runtime fixes for CentOS 5, MacOS 10.9, and FreeBSD.

* TCP snd_cwnd output on Linux in the default output format.

* libiperf is now built as both a shared and static library; by
  default, the iperf3 binary links to the shared library.

More information on changes can be found in the ``RELNOTES.md``
file in the source distribution.

2014-02-28:  iperf migrated to GitHub
.....................................

The new project page can be found at:

https://github.com/esnet/iperf

2014-01-10:  iperf-3.0.1 released
.................................

| URL:  http://stats.es.net/software/iperf-3.0.1.tar.gz
| SHA256:  ``32b419ef634dd7670328c3cecc158babf7d706bd4b3d248cf95965528a20e614 iperf-3.0.1.tar.gz``

During development, there were various distributions of the source
code unofficially released carrying a 3.0.0 version number.  Because
of the possibility for confusion, this first public release of iperf3
was numbered 3.0.1.
