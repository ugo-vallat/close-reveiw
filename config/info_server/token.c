#include <stdio.h>
#include <openssl/rand.h>

#define TOKEN_SIZE 16 // 16 bytes = 32 hex characters

void generate_token(char *buffer) {
    unsigned char token[TOKEN_SIZE];
    if (RAND_bytes(token, sizeof(token)) != 1) {
        fprintf(stderr, "Error generating secure random token.\n");
        exit(-1);
    }

    for (int i = 0; i < TOKEN_SIZE; i++) {
        sprintf(&buffer[i*2], "%02x", token[i]);
    }
}

int main() {
    char token_hex[TOKEN_SIZE*2 + 1];
    generate_token(token_hex);
    printf("Generated token: %s\n", token_hex);
    return 0;
}