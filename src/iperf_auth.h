# include <openssl/bio.h>
# include <openssl/pem.h>
# include <string.h>
# include <openssl/buffer.h>
# include <assert.h>
# include <stdio.h>
# include <time.h>

int encode_auth_setting(char *username, char *password, char *public_keyfile, char *authtoken);
    
