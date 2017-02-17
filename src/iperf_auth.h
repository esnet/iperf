# include <openssl/bio.h>
# include <openssl/pem.h>
# include <string.h>
# include <openssl/buffer.h>
# include <assert.h>
# include <stdio.h>
# include <time.h>

int encode_auth_setting(char *username, char *password, char *public_keyfile, char **authtoken);
int decode_auth_setting(const char *authtoken, const char *private_keyfile, char **username, char **password, time_t *ts);
int check_authentication(const char *username, const char *password, const time_t ts, const char *filename);
    
