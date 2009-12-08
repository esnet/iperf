#!/bin/sh
#
#      $Id: bootstrap.sh 446 2008-04-21 04:07:15Z boote $
#
#########################################################################
#									#
#			   Copyright (C)  2003				#
#	     			Internet2				#
#			   All Rights Reserved				#
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

set -x
libtoolize --copy --force --automake
aclocal -I config
autoheader
automake --foreign --add-missing --copy
autoconf
rm -rf config.cache
