iperf3 Project News
===================

2015-01-09:  iperf-3.0.11 released
----------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.11.tar.gz
| SHA256:  ``e01db5be6f47f67c987463095fe4f5b8b9ff891fb92c39104d042ad8fde97f6e  iperf-3.0.11.tar.gz``

This maintenance release adds a -1 flag to make the iperf3 execute a
single test and exit, needed for an upcoming bwctl release.  A few
other bugs are also fixed.

2014-12-16:  iperf-3.0.10 released
----------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.10.tar.gz
| SHA256:  ``a113442967cf0981b0b2d538be7c88903b2fb0f87b0d281384e41b462e33059d  iperf-3.0.10.tar.gz``

This maintenance release fixes building on MacOS X Yosemite, as well
as making the -w option work correctly with UDP tests.

2014-10-14:  iperf-3.0.9 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.9.tar.gz
| SHA256:  ``40249a2b30d26b937350b969bcb19f88e1beb356f886ed31422b554bac692459  iperf-3.0.9.tar.gz``

This maintenance release fixes an issue for a situation in which
attempting a UDP test with pathologically large (and illegal) packet
sizes could put the iperf3 server in a state where it would stop
accepting connections from clients, thus causing the clients to crash
when interrupted.


2014-09-30:  iperf-3.0.8 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.8.tar.gz
| SHA256:  ``81b8d91159862896c57f9b90a006e8b5dc22bd94175d97bd0db50b0ae2c1a78e  iperf-3.0.8.tar.gz``

This maintenance release is functionally identical to 3.0.7.  It
incorporates updated license verbage and a minor compilation fix.


2014-08-28:  iperf-3.0.7 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.7.tar.gz
| SHA256:  ``49510e886f9e876cd73dcd80414bfb8c49b147c82125585e09c2a6e92369d3f2  iperf-3.0.7.tar.gz``

This maintenance release fixes several minor bugs.  Of particular
note:

* A bug that caused some problems with bwctl / perfSONAR has been
  fixed.

* A bug that made it possible to disrupt existing, running tests has
  been fixed.

2014-07-28:  iperf-3.0.6 released
---------------------------------

| URL:  http://downloads.es.net/pub/iperf/iperf-3.0.6.tar.gz
| SHA256:  ``3c5909c9b286b6503ffa141a94cfc588915d6e67f2aa732b08df0af73e21938  iperf-3.0.6.tar.gz``

This maintenance release includes the following bug fixes:

* Several problems with the -B option have been fixed.  Also, API
  calls have been added to libiperf to extend this functionality to
  API clients.

* Some portability fixes for OpenBSD and Solaris have been merged from
  the mainline.

As always, more details can be found in the ``RELEASE_NOTES`` file in
the source distribution.

Older News
----------

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
``RELEASE_NOTES`` file in the source distribution.

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

More information on changes can be found in the ``RELEASE_NOTES``
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

More information on changes can be found in the ``RELEASE_NOTES``
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
of the possiblity for confusion, this first public release of iperf3
was numbered 3.0.1.

