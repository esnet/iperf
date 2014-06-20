#!/bin/sh
#
#########################################################################
#									#
#			   Copyright (C)  2003				#
#	     			Internet2				#
#                                                                       #
#  Licensed under the Apache License, Version 2.0 (the "License");      #
#  you may not use this file except in compliance with the License.     #
#  You may obtain a copy of the License at                              #
#                                                                       #
#  http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                       #
#  Unless required by applicable law or agreed to in writing, software  #
#  distributed under the License is distributed on an "AS IS" BASIS,    #
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      #
#  implied. See the License for the specific language governing         #
#  permissions and limitations under the License.                       #
#									#
#########################################################################
#
#	File:		bootstrap
#
#	Author:		Jeff Boote
#			Internet2
#
#	Date:		Tue Sep 16 14:21:57 MDT 2003
#
#	Description:	
#			This script is used to bootstrap the autobuild
#			process.
#
#			RUNNING THIS SCRIPT IS NOT RECOMMENDED
#			(You should just use the "configure" script
#			that was bundled with the distribution if
#			at all possible.)
#
case "$1" in
	ac257)
		alias autoconf=autoconf257
		alias autoheader=autoheader257
		alias automake=automake17
		alias aclocal=aclocal17
		export AUTOCONF=autoconf257
		;;
	*)
		;;
esac

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

set -x
$libtoolize --copy --force --automake
aclocal -I config
autoheader
automake --foreign --add-missing --copy
autoconf
rm -rf config.cache
