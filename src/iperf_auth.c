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

#include "iperf_config.h"

#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <stdio.h>
#include <termios.h>

#if defined(HAVE_SSL)

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>

void sha256(const char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

int check_authentication(const char *username, const char *password, const time_t ts, const char *filename){
    time_t t = time(NULL);
    time_t utc_seconds = mktime(localtime(&t));
    if ( (utc_seconds - ts) < 10 && (utc_seconds - ts) > 0 ){
        return 1;
    }

    char passwordHash[65];
    char salted[strlen(username) + strlen(password) + 3];
    sprintf(salted, "{%s}%s", username, password);
    sha256(&salted[0], passwordHash);

    char *s_username, *s_password;
    int i;
    FILE *ptr_file;
    char buf[1024];

    ptr_file =fopen(filename,"r");
    if (!ptr_file)
        return 2;

    while (fgets(buf,1024, ptr_file)){
        //strip the \n or \r\n chars
        for (i = 0; buf[i] != '\0'; i++){
            if (buf[i] == '\n' || buf[i] == '\r'){
                buf[i] = '\0';
                break;
            }
        }
        //skip empty / not completed / comment lines
        if (strlen(buf) == 0 || strchr(buf, ',') == NULL || buf[0] == '#'){
            continue;
        }
        s_username = strtok(buf, ",");
        s_password = strtok(NULL, ",");
        if (strcmp( username, s_username ) == 0 && strcmp( passwordHash, s_password ) == 0){
            return 0;
        }
    }
    return 3;
}


int Base64Encode(unsigned char* buffer, const size_t length, char** b64text) { //Encodes a binary safe base 64 string
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    *b64text=(*bufferPtr).data;
    (*b64text)[(*bufferPtr).length] = '\0';
    return (0); //success
}

size_t calcDecodeLength(const char* b64input) { //Calculates the length of a decoded string
    size_t len = strlen(b64input), padding = 0;
    if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
        padding = 2;
    else if (b64input[len-1] == '=') //last char is =
        padding = 1;

    return (len*3)/4 - padding;
}

int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) { //Decodes a base64 encoded string
    BIO *bio, *b64;

    int decodeLen = calcDecodeLength(b64message);
    *buffer = (unsigned char*)malloc(decodeLen + 1);
    (*buffer)[decodeLen] = '\0';

    bio = BIO_new_mem_buf(b64message, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    *length = BIO_read(bio, *buffer, strlen(b64message));
    assert(*length == decodeLen); //length should equal decodeLen, else something went horribly wrong
    BIO_free_all(bio);

    return (0); //success
}


EVP_PKEY *load_pubkey(const char *file) {
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    key = BIO_new_file(file, "r");
    pkey = PEM_read_bio_PUBKEY(key, NULL, NULL, NULL);

    BIO_free(key);
    return (pkey);
}   

EVP_PKEY *load_key(const char *file) {
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    key = BIO_new_file(file, "r");
    pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);

    BIO_free(key);
    return (pkey);
}


int test_load_pubkey(const char *file){
    EVP_PKEY *key = load_pubkey(file);
    if (key == NULL){
        return -1;
    }
    EVP_PKEY_free(key);
    return 0;
}

int test_load_private_key(const char *file){
    EVP_PKEY *key = load_key(file);
    if (key == NULL){
        return -1;
    }
    EVP_PKEY_free(key);
    return 0;
}

int encrypt_rsa_message(const char *plaintext, const char *public_keyfile, unsigned char **encryptedtext) {
    EVP_PKEY *public_key = NULL;
    RSA *rsa = NULL;
    unsigned char *rsa_buffer = NULL, pad = RSA_PKCS1_PADDING;
    int keysize, encryptedtext_len, rsa_buffer_len;

    public_key = load_pubkey(public_keyfile);
    rsa = EVP_PKEY_get1_RSA(public_key);
    EVP_PKEY_free(public_key);

    keysize = RSA_size(rsa);
    rsa_buffer  = OPENSSL_malloc(keysize * 2);
    *encryptedtext = (unsigned char*)OPENSSL_malloc(keysize);

    BIO *bioBuff   = BIO_new_mem_buf((void*)plaintext, (int)strlen(plaintext));
    rsa_buffer_len = BIO_read(bioBuff, rsa_buffer, keysize * 2);
    encryptedtext_len = RSA_public_encrypt(rsa_buffer_len, rsa_buffer, *encryptedtext, rsa, pad);

    RSA_free(rsa);
    OPENSSL_free(rsa_buffer);
    OPENSSL_free(bioBuff);  

    return encryptedtext_len;  
}

int decrypt_rsa_message(const unsigned char *encryptedtext, const int encryptedtext_len, const char *private_keyfile, unsigned char **plaintext) {
    EVP_PKEY *private_key = NULL;
    RSA *rsa = NULL;
    unsigned char *rsa_buffer = NULL, pad = RSA_PKCS1_PADDING;
    int plaintext_len, rsa_buffer_len, keysize;
    
    private_key = load_key(private_keyfile);
    rsa = EVP_PKEY_get1_RSA(private_key);
    EVP_PKEY_free(private_key);

    keysize = RSA_size(rsa);
    rsa_buffer  = OPENSSL_malloc(keysize * 2);
    *plaintext = (unsigned char*)OPENSSL_malloc(keysize);

    BIO *bioBuff   = BIO_new_mem_buf((void*)encryptedtext, encryptedtext_len);
    rsa_buffer_len = BIO_read(bioBuff, rsa_buffer, keysize * 2);
    plaintext_len = RSA_private_decrypt(rsa_buffer_len, rsa_buffer, *plaintext, rsa, pad);

    RSA_free(rsa);
    OPENSSL_free(rsa_buffer);
    OPENSSL_free(bioBuff);   

    return plaintext_len;
}

int encode_auth_setting(const char *username, const char *password, const char *public_keyfile, char **authtoken){
    time_t t = time(NULL);
    time_t utc_seconds = mktime(localtime(&t));
    char text[150];
    sprintf (text, "user: %s\npwd:  %s\nts:   %ld", username, password, utc_seconds);
    unsigned char *encrypted = NULL;
    int encrypted_len;
    encrypted_len = encrypt_rsa_message(text, public_keyfile, &encrypted);
    Base64Encode(encrypted, encrypted_len, authtoken);
    return (0); //success
}

int decode_auth_setting(int enable_debug, char *authtoken, const char *private_keyfile, char **username, char **password, time_t *ts){
    unsigned char *encrypted_b64 = NULL;
    size_t encrypted_len_b64;
    Base64Decode(authtoken, &encrypted_b64, &encrypted_len_b64);        

    unsigned char *plaintext = NULL;
    int plaintext_len;
    plaintext_len = decrypt_rsa_message(encrypted_b64, encrypted_len_b64, private_keyfile, &plaintext);
    plaintext[plaintext_len] = '\0';

    char s_username[20], s_password[20];
    sscanf ((char *)plaintext,"user: %s\npwd:  %s\nts:   %ld", s_username, s_password, ts);
    if (enable_debug) {
        printf("Auth Token Content:\n%s\n", plaintext);
        printf("Auth Token Credentials:\n--> %s %s\n", s_username, s_password);
    }
    *username = (char *) calloc(21, sizeof(char));
    *password = (char *) calloc(21, sizeof(char));
    strncpy(*username, s_username, 20);
    strncpy(*password, s_password, 20);
    return (0);
}

#endif //HAVE_SSL

ssize_t iperf_getpass (char **lineptr, size_t *n, FILE *stream) {
    struct termios old, new;
    ssize_t nread;

    /* Turn echoing off and fail if we can't. */
    if (tcgetattr (fileno (stream), &old) != 0)
        return -1;
    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr (fileno (stream), TCSAFLUSH, &new) != 0)
        return -1;

    /* Read the password. */
    printf("Password: ");
    nread = getline (lineptr, n, stream);

    /* Restore terminal. */
    (void) tcsetattr (fileno (stream), TCSAFLUSH, &old);

    //strip the \n or \r\n chars
    char *buf = *lineptr;
    int i;
    for (i = 0; buf[i] != '\0'; i++){
        if (buf[i] == '\n' || buf[i] == '\r'){
            buf[i] = '\0';
            break;
        }
    }

    return nread;
}


