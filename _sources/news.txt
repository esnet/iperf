iperf3 Project News
===================

2014-03-26:  iperf-3.0.3 released
---------------------------------

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
---------------------------------

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
-------------------------------------

The new project page can be found at:

https://github.com/esnet/iperf

2014-01-10:  iperf-3.0.1 released
---------------------------------

| URL:  http://stats.es.net/software/iperf-3.0.1.tar.gz
| SHA256:  ``32b419ef634dd7670328c3cecc158babf7d706bd4b3d248cf95965528a20e614 iperf-3.0.1.tar.gz``

During development, there were various distributions of the source
code unofficially released carrying a 3.0.0 version number.  Because
of the possiblity for confusion, this first public release of iperf3
was numbered 3.0.1.

