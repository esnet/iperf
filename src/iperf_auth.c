# include <openssl/bio.h>
# include <openssl/pem.h>
# include <string.h>
# include <openssl/buffer.h>
# include <assert.h>
# include <stdio.h>
# include <time.h>



int Base64Encode(const unsigned char* buffer, size_t length, char** b64text) { //Encodes a binary safe base 64 string
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

int encode_auth_setting(char *username, char *password, char *public_keyfile, char **authtoken){
    time_t t = time(NULL);
    time_t utc_seconds = mktime(localtime(&t));
    char text[150];
    sprintf (text, "user: %-20s\npwd:  %-20s\nts:   %ld", username, password, utc_seconds);
    unsigned char *encrypted = NULL;
    int encrypted_len;
    encrypted_len = encrypt_rsa_message(text, public_keyfile, &encrypted);
    Base64Encode(encrypted, encrypted_len, authtoken);
    return (0); //success
}

/*
int main(int argc, char *argv[]) {

    time_t t = time(NULL);
    time_t utc_seconds = mktime(localtime(&t));
    char text[150];
    sprintf (text, "user: %-20s\npwd:  %-20s\nts:   %ld", "mario", "pippo", utc_seconds);
    char *public_keyfile = "public.pem";
    
    unsigned char *encrypted = NULL;
    char *b64message = NULL;
    int encrypted_len;
    encrypted_len = encrypt_rsa_message(text, public_keyfile, &encrypted);
    printf("--> %d\n",encrypted_len);            
    
    Base64Encode(encrypted, encrypted_len, &b64message);
    printf("%s\n",b64message);



    unsigned char *encrypted_b64 = NULL;
    size_t encrypted_len_b64;
    Base64Decode(b64message, &encrypted_b64, &encrypted_len_b64);        

    char *private_keyfile = "private_unencrypted.pem";
    unsigned char *rsa_out_text = NULL;
    int rsa_outlen_text;
    rsa_outlen_text = decrypt_rsa_message(encrypted_b64, encrypted_len_b64, private_keyfile, &rsa_out_text);
    rsa_out_text[rsa_outlen_text] = '\0';

    printf("%s\n",rsa_out_text);
    printf("%d\n",rsa_outlen_text);
    char user[20];
    char pwd[20];
    time_t ts;
    sscanf (rsa_out_text,"user: %20s\npwd:  %20s\nts:   %ld",user,pwd,&ts);
    printf("%ld\n",ts);
    
}
*/

    
