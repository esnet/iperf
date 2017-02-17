# include <openssl/bio.h>
# include <openssl/pem.h>
# include <string.h>
# include <openssl/buffer.h>
# include <assert.h>
# include <stdio.h>
# include <time.h>

int test_load_pubkey(const char *public_keyfile);
int test_load_private_key(const char *private_keyfile);
int encode_auth_setting(const char *username, const char *password, const char *public_keyfile, char **authtoken);
int decode_auth_setting(const char *authtoken, const char *private_keyfile, char **username, char **password, time_t *ts);
int check_authentication(const char *username, const char *password, const time_t ts, const char *filename);
    
