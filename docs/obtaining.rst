.. _obtaining:

Obtaining iperf3
================

Binary Distributions
--------------------

Note that ESnet does not distribute binary packages of iperf3.  All of
the packages listed in this section are provided by third parties, who
are solely responsible for their contents.  This is an incomplete list
of binary packages for various operating systems and distributions:

* FreeBSD: via the FreeBSD Ports Collection with ``sudo pkg install
  benchmarks/iperf3``.
* Fedora / RedHat Linux / CentOS / Rocky: `iperf3
  <https://packages.fedoraproject.org/pkgs/iperf3/iperf3/>`_ and
  `iperf3-devel
  <https://packages.fedoraproject.org/pkgs/iperf3/iperf3-devel/`_ in Fedora
  19 and 20 and in Fedora EPEL 5, 6, and 7.  iperf3 is included as a
  part of RedHat Enterprise Linux 7.4 and later (as well as CentOS 7.4
  and later, and all versions of Rocky Linux), and can generally be
  installed with ``yum install iperf3``.
* Ubuntu:  `iperf3 <https://launchpad.net/ubuntu/+source/iperf3>`_,
  is available in Trusty (backports), and as a part of the main
  release in Vivid and newer. It can generally be installed with
  ``sudo apt-get install iperf3``.
* macOS:  via HomeBrew with ``brew install iperf3`` or MacPorts with
  ``sudo port install iperf3``.
* Windows:  iperf3 binaries for Windows (built with `Cygwin <https://www.cygwin.com/>`_) can be found in a variety of
  locations, including `<https://files.budman.pw/>`_
  (`discussion thread
  <https://www.neowin.net/forum/topic/1234695-iperf/>`_).

Source Distributions
--------------------

Source distributions of iperf are available as compressed (gzip)
tarballs at:

https://downloads.es.net/pub/iperf/

**Note:**  Due to a software packaging error, the 3.0.2 release
tarball was not compressed, even though its filename had a ``.tar.gz``
suffix.

**Note:**  GitHub, which currently hosts the iperf3 project, supports
a "Releases" feature, which can automatically generate ``.zip`` or ``.tar.gz``
archives, on demand, from tags in the iperf3 source tree.  These tags are
created during the release engineering process to mark the exact
version of files making up a release.

In theory, the ``.tar.gz`` files produced by GitHub contain the same
contents as what are in the official tarballs, note that the tarballs
themselves will be different due to internal timestamps or other
metadata.  Therefore these files will *not* match the published SHA256
checksums and no guarantees can be made about the integrity of the
files.  The authors of iperf3 always recommend downloading source
distributions from the the directory above (or a mirror site), and
verifying the SHA256 checksums before using them for any purpose, to
ensure the files have not been tampered with.

Source Code Repository
----------------------

The iperf3 project is hosted on GitHub at:

https://github.com/esnet/iperf

The iperf3 source code repository can be checked out directly from
GitHub using:

``git clone https://github.com/esnet/iperf.git``

Primary development for iperf3 takes place on CentOS 7 Linux, FreeBSD 11,
and macOS 10.12. At this time, these are the only officially supported
platforms, however there have been some reports of success with
NetBSD, OpenBSD, Windows, Solaris, Android, and iOS.
