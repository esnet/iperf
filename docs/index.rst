.. iperf documentation master file, created by
   sphinx-quickstart on Fri Mar 28 14:58:40 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

iperf3
======

The iperf series of tools perform active measurements to determine the
maximum achievable bandwidth on IP networks.  It supports tuning of
various parameters related to timing, protocols, and buffers.  For
each test it reports the measured throughput, loss, and other
parameters.

This version, sometimes referred to as iperf3, is a redesign of an
original version developed at NLANR / DAST.  iperf3 is a new
implementation from scratch, with the goal of a smaller, simpler code
base, and a library version of the functionality that can be used in
other programs. iperf3 also incorporates a number of features found in
other tools such as nuttcp and netperf, but were missing from the
original iperf.  These include, for example, a zero-copy mode and
optional JSON output.  Note that iperf3 is *not* backwards compatible
with the original iperf.

Primary development for iperf3 takes place on CentOS Linux, FreeBSD,
and macOS.  At this time, these are the only officially
supported platforms, however there have been some reports of success
with OpenBSD, Android, and other Linux distributions.

iperf3 is principally developed by `ESnet <http://www.es.net/>`_ /
`Lawrence Berkeley National Laboratory <http://www.lbl.gov/>`_.  It
is released under a three-clause BSD license.

iperf2 is no longer being developed by its original maintainers.
However, beginning in 2014, another developer began fixing bugs and
enhancing functionality, and generating releases of iperf2.  Both
projects (as of late 2017) are currently being developed actively, but
independently.  More information can be found in the :ref:`faq`.

Links for the Impatient
-----------------------

Project homepage and documentation hosted on GitHub Pages:
https://software.es.net/iperf/

Project site (source code repository, issue tracker) hosted on GitHub:
https://github.com/esnet/iperf

Source code downloads:
https://downloads.es.net/pub/iperf/

Contents
--------

.. toctree::
   :maxdepth: 2

   news
   obtaining
   building
   invoking
   dev
   faq

Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
