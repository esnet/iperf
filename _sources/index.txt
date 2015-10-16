.. iperf documentation master file, created by
   sphinx-quickstart on Fri Mar 28 14:58:40 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

iperf3
======

iperf is a tool for active measurements of the maximum achievable
bandwidth on IP networks.  It supports tuning of various parameters
related to timing, protocols, and buffers.  For each test it reports
the bandwidth, loss, and other parameters.

This version, sometimes referred to as iperf3, is a redesign of an
original version developed at NLANR / DAST.  iperf3 is a new
implementation from scratch, with the goal of a smaller, simpler code
base, and a library version of the functionality that can be used in
other programs. iperf3 also incorporates a number of features found in
other tools such as nuttcp and netperf, but were missing from the
original iperf.  These include, for example, a zero-copy mode and
optional JSON output.  Note that iperf3 is *not* backwards compatible
with the original iperf.

Primary development for iperf3 takes place on CentOS 6 Linux, FreeBSD
10, and MacOS 10.10.  At this time, these are the only officially
supported platforms, however there have been some reports of success
with OpenBSD, Android, and other Linux distributions.

iperf3 is principally developed by `ESnet <http://www.es.net/>`_ /
`Lawrence Berkeley National Laboratory <http://www.lbl.gov/>`_.  It
is released under a three-clause BSD license.

Links for the Impatient
-----------------------

Project homepage and documentation hosted on GitHub Pages:
http://software.es.net/iperf/

Project site (source code repository, issue tracker) hosted on GitHub:
https://github.com/esnet/iperf

Source code downloads:
http://downloads.es.net/pub/iperf/

Contents
--------

.. toctree::
   :maxdepth: 2

   news
   obtaining
   building
   invoking
   dev

Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

