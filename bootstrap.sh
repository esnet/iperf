#! /bin/sh
#
# iperf, Copyright (c) 2014, The Regents of the University of
# California, through Lawrence Berkeley National Laboratory (subject
# to receipt of any required approvals from the U.S. Dept. of
# Energy).  All rights reserved.
#
# If you have questions about your rights to use or distribute this
# software, please contact Berkeley Lab's Technology Transfer
# Department at TTD@lbl.gov.
#
# NOTICE.  This software is owned by the U.S. Department of Energy.
# As such, the U.S. Government has been granted for itself and others
# acting on its behalf a paid-up, nonexclusive, irrevocable,
# worldwide license in the Software to reproduce, prepare derivative
# works, and perform publicly and display publicly.  Beginning five
# (5) years after the date permission to assert copyright is obtained
# from the U.S. Department of Energy, and subject to any subsequent
# five (5) year renewals, the U.S. Government is granted for itself
# and others acting on its behalf a paid-up, nonexclusive,
# irrevocable, worldwide license in the Software to reproduce,
# prepare derivative works, distribute copies to the public, perform
# publicly and display publicly, and to permit others to do so.
#
# This code is distributed under a BSD style license, see the LICENSE
# file for complete information.
#

# When changes are made to the build infrastructure, invoke this
# script to regenerate all of the autotools-built files.
# Normally, this is only of use to developers.

# Figure out how to invoke libtoolize.  On MacOS (with MacPorts)
# it's invoked as glibtoolize.
if libtoolize --version >/dev/null 2>&1; then
  libtoolize=libtoolize
elif glibtoolize --version >/dev/null 2>&1; then
  libtoolize=glibtoolize
else
  libtoolize=""
fi
if [ "x$libtoolize" = "x" ]; then
  echo "Can't find libtoolize, exiting."
  exit 1
fi

# Execute the various autotools commands in the correct order.
set -x
$libtoolize --copy --force --automake
aclocal -I config
autoheader
automake --add-missing --copy
autoconf
rm -rf config.cache
