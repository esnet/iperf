#!/bin/bash
#
# This is a set of commands to run and verify they work before doing a new release.
# Eventually they should also use the -J flag to generate JSON output, and a program should
# be written to check the output.
# Be sure to test both client and server on Linux, BSD, and OSX
#

#
set -x

binary=./src/iperf3
output=/tmp/iperf3_auth_tmp.txt

# Generate keys and authorized user file

#sudo apt-get install libssl-dev
openssl genrsa -out private.pem 2048
openssl rsa -in private.pem -outform PEM -pubout -out public.pem

#user,sha256(pwd)
S_USER=mario S_PASSWD=luigi
echo $U_USER
SHA=$(echo -n "{$S_USER}$S_PASSWD" | sha256sum | awk '{ print $1 }')
echo ${S_USER},${SHA} > authorized-user.txt
cat authorized-user.txt

# Run Server

${binary} -s --rsa-private-key-path=./private.pem  --authorized-user=./authorized-user.txt &> ${output} &

# Run Client

IPERF3_PASSWORD=${S_PASSWD} ${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD="very incorrect" ${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD="very,very,very, incorrect, and really, really, really, really, really long. Just to check." ${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}
IPERF3_PASSWORD=${S_PASSWD} ${binary} -c localhost --rsa-public-key-path=./public.pem --username=${S_USER}

pkill ${binary}

cat ${output}
rm ${output}
