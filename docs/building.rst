Building iperf3
===============

Building iperf3, as with most tools of this type, is a fairly
straightforward operation.  The instructions in this section assume
that the source distribution has already been unpacked.

Prerequisites
-------------

iperf3 requires few (if any) dependencies on common operating
systems.  The only known prerequesites are listed here.

* ``libuuid``:  ``libuuid`` is not installed by default on Debian /
  Ubuntu Linux systems.  To install this, use this command before
  building iperf3:

  ``apt-get install uuid-dev``

Building
--------

In many cases, iperf3 can be built and installed as follows:

``./configure && make && make install``

In some cases, configuration might fail.  If this happens, it might
help to run ``./bootstrap.sh`` first from the top-level directory.

By default, the ``libiperf`` library is built in both shared and
static forms.  Either can be suppressed by using the
``--disabled-shared`` or ``--disable-static`` configure-time options.

