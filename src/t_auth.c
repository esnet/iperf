/*
 * iperf, Copyright (c) 2020, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iperf.h"
#include "iperf_api.h"
#if defined(HAVE_SSL)
#include "iperf_auth.h"
#endif /* HAVE_SSL */

#include "version.h"

#include "units.h"

#if defined(HAVE_SSL)
int test_authtoken(const char *authUser, const char *authPassword, EVP_PKEY *pubkey, EVP_PKEY *privkey);

int
main(int argc, char **argv)
{
    /* sha256 */
    void sha256(const char *string, char outputBuffer[65]);
    const char sha256String[] = "This is a SHA256 test.";
    const char sha256Digest[] = "4816482f8b4149f687a1a33d61a0de6b611364ec0fb7adffa59ff2af672f7232"; /* echo -n "This is a SHA256 test." | shasum -a256 */
    char sha256Output[65];

    sha256(sha256String, sha256Output);
    assert(strcmp(sha256Output, sha256Digest) == 0);

    /* Base64{Encode,Decode} */
    int Base64Encode(const unsigned char* buffer, const size_t length, char** b64text);
    int Base64Decode(const char* b64message, unsigned char** buffer, size_t* length);
    const char base64String[] = "This is a Base64 test.";
    char *base64Text;
    char *base64Decode;
    size_t base64DecodeLength;
    const char base64EncodeCheck[] = "VGhpcyBpcyBhIEJhc2U2NCB0ZXN0Lg=="; /* echo -n "This is a Base64 test." | b64encode -r - */

    assert(Base64Encode((unsigned char *) base64String, strlen(base64String), &base64Text) == 0);
    assert(strcmp(base64Text, base64EncodeCheck) == 0);
    assert(Base64Decode(base64Text, (unsigned char **) &base64Decode, &base64DecodeLength) == 0);
    assert(strcmp(base64String, base64Decode) == 0);

    /* public/private key tests */
    const char *pubkeyfile = "public.pem";
    const char *privkeyfile = "private.pem";

    /* built-in tests */
    assert(test_load_pubkey_from_file(pubkeyfile) == 0);
    assert(test_load_private_key_from_file(privkeyfile) == 0);

    /* load public key pair for use in further tests */
    EVP_PKEY *pubkey, *privkey;
    pubkey = load_pubkey_from_file(pubkeyfile);
    assert(pubkey);
    privkey = load_privkey_from_file(privkeyfile);
    assert(privkey);

    /* authentication token tests */
    assert(test_authtoken("kilroy", "fubar", pubkey, privkey) == 0);

    /* This should fail because the data is way too long for the RSA key */
    /* assert(test_authtoken("kilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroykilroy", "fubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubarfubar", pubkey, privkey) < 0); */

    return 0;
}

int
test_authtoken(const char *authUser, const char *authPassword, EVP_PKEY *pubkey, EVP_PKEY *privkey) {
    char *authToken;
    char *decodeUser;
    char *decodePassword;
    time_t decodeTime;

    int use_pkcs1_padding = 1;
    assert(encode_auth_setting(authUser, authPassword, pubkey, &authToken, use_pkcs1_padding) == 0);
    assert(decode_auth_setting(0, authToken, privkey, &decodeUser, &decodePassword, &decodeTime, use_pkcs1_padding) == 0);

    assert(strcmp(decodeUser, authUser) == 0);
    assert(strcmp(decodePassword, authPassword) == 0);

    time_t now = time(NULL);

    assert(now - decodeTime >= 0); /* time has to go forwards */
    assert(now - decodeTime <= 1); /* shouldn't take more than a second to run */

    return 0;
}
#else
int
main(int argc, char **argv)
{
    return 0;
}
#endif /* HAVE_SSL */
