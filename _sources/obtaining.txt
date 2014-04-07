.. _obtaining:

Obtaining iperf3
================

Binary Distributions
--------------------

Binary packages are available for several supported operating systems:

* FreeBSD:  `benchmarks/iperf3
  <http://freshports.org/benchmarks/iperf3>`_ in the FreeBSD Ports Collection
* Fedora / CentOS: `iperf3
  <https://apps.fedoraproject.org/packages/iperf3/>`_ and
  `iperf3-devel
  <https://apps.fedoraproject.org/packages/iperf3-devel>`_ in Fedora
  19 and 20 and in Fedora EPEL 5, 6, and 7.

Source Distributions
--------------------

Source distributions of iperf are available as compressed (gzip)
tarballs at:

http://stats.es.net/software/

**Note:**  Due to a software packaging error, the 3.0.2 release
tarball was not compressed, even though its filename had a ``.tar.gz``
suffix.

Source Code Repository
----------------------

The iperf3 project is hosted on GitHub at:

https://github.com/esnet/iperf

The iperf3 source code repository can be checked out directly from
GitHub using:

``git clone https://github.com/esnet/iperf.git``

Primary development for iperf3 takes place on CentOS 6 Linux, FreeBSD 10,
and MacOS X 10.8. At this time, these are the only officially supported
platforms, however there have been some reports of success with
OpenBSD, Android, and other Linux distributions.
