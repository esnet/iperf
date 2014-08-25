#! /bin/sh
#
# Copyright (c) 2014, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of
# any required approvals from the U.S. Dept. of Energy).  All rights
# reserved.
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
automake --foreign --add-missing --copy
autoconf
rm -rf config.cache
