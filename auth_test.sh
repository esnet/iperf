#!/bin/bash
#
# This is a set of commands to run and verify they work before doing a new release.
# Eventually they should also use the -J flag to generate JSON output, and a program should
# be written to check the output.
# Be sure to test both client and server on Linux, BSD, and OSX
#

if [ $# -ne 2 ]
then
  echo "Usage: `basename "$0"` binary output"
  exit $E_BADARGS
fi
#
set -x

binary=$1
output=$2

#sudo apt-get install libssl-dev
openssl genrsa -out private.pem 2048
openssl rsa -in private.pem -outform PEM -pubout -out public.pem

#user,sha256(pwd)
S_USER=mario S_PASSWD=luigi
echo $U_USER
SHA=$(echo -n "{$S_USER}$S_PASSWD" | sha256sum | awk '{ print $1 }')
echo ${S_USER},${SHA} > authorized-user.txt
cat authorized-user.txt

./src/${binary} -s --rsa-private-key-path=./private.pem  --authorized-user=./authorized-user.txt &> ${output} &

IPERF3_PASSWORD=${S_PASSWD} ./src/${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD="very incorrect" ./src/${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD="very,very,very, incorrect, and really, really, really, really, really long. Just to check." ./src/${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD=${S_PASSWD} ./src/${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}

pkill ${binary}

cat ${output}
rm ${output}
