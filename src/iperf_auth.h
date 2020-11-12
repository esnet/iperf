/*
 * iperf, Copyright (c) 2014-2017, The Regents of the University of
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
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include <time.h>
#include <sys/types.h>
#include <openssl/bio.h>

int test_load_pubkey_from_file(const char *public_keyfile);
int test_load_private_key_from_file(const char *private_keyfile);
EVP_PKEY *load_pubkey_from_file(const char *file);
EVP_PKEY *load_pubkey_from_base64(const char *buffer);
EVP_PKEY *load_privkey_from_file(const char *file);
EVP_PKEY *load_privkey_from_base64(const char *buffer);
int encode_auth_setting(const char *username, const char *password, EVP_PKEY *public_key, char **authtoken);
int decode_auth_setting(int enable_debug, const char *authtoken, EVP_PKEY *private_key, char **username, char **password, time_t *ts);
int check_authentication(const char *username, const char *password, const time_t ts, const char *filename, int skew_threshold);
ssize_t iperf_getpass (char **lineptr, size_t *n, FILE *stream);
