#!/bin/sh
#
# This is a set of commands to run and verify they work before doing a new release.
# Eventually they should also use the -J flag to generate JSON output, and a program should 
# be written to check the output.
# Be sure to test both client and server on Linux, BSD, and OSX
#

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` hostname"
  exit $E_BADARGS
fi

set -x

host=$1

# basic testing
./src/iperf3 -c $host -V -t 5 -T "test1"
./src/iperf3 -c $host -u -V -t 5
# omit mode
./src/iperf3 -c $host -i .3 -O 2 -t 5
# JSON mode
./src/iperf3 -c $host -i 1 -J -t 5
# force V4
./src/iperf3 -c $host -4 -t 5
./src/iperf3 -c $host -4 -u -t 5
# force V6
./src/iperf3 -c $host -6 -t 5
./src/iperf3 -c $host -6 -u -t 5
# parallel streams
./src/iperf3 -c $host -P 3 -t 5 
./src/iperf3 -c $host -u -P 3 -t 5
# reverse mode
./src/iperf3 -c $host -P 2 -t 5 -R
./src/iperf3 -c $host -u -P 2 -t 5 -R
# zero copy
./src/iperf3 -c $host -Z -t 5 
./src/iperf3 -c $host -Z -t 5 -R
# window size
./src/iperf3 -c $host -t 5 -w 8M 
# -n flag
./src/iperf3 -c $host -n 5M  
./src/iperf3 -c $host -n 5M -u -b1G
# conflicting -n -t flags
./src/iperf3 -c $host -n 5M -t 5
# -k mode
./src/iperf3 -c $host -k 1K  
./src/iperf3 -c $host -k 1K -u -b1G
# CPU affinity
./src/iperf3 -c $host -A 2/2
./src/iperf3 -c $host -A 2/2 -u -b1G
# Burst mode
./src/iperf3 -c $host -u -b1G/100
# change MSS
./src/iperf3 -c $host -M 1000 -V
# test congestion control option (linux only)
./src/iperf3 -c $host -C reno -V


