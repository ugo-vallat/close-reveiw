#include <ctype.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PASSWORD_LENGTH 30
#define MIN_PASSWORD_LENGTH 8
#define MD5_WORDLIST_PATH "custom_hash"

int get_num_password(char *passlist) {
    FILE *passlist_file = fopen(passlist, "r");
    if (passlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", passlist);
        exit(1);
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int num_passwords = 0;
    while ((read = getline(&line, &len, passlist_file)) != -1) {
        num_passwords++;
    }
    fclose(passlist_file);
    if (line) {
        free(line);
    }
    return num_passwords;
}
void password_to_md5_hash(char *password, char *hash) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    char string[EVP_MAX_MD_SIZE * 2 + 1];
    int i;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, password, strlen(password));
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    for (i = 0; i < digest_len; i++)
        sprintf(&string[i * 2], "%02x", (unsigned int)digest[i]);
    strcpy(hash, string);
}

bool wordlist_check(char *password) {
    FILE *hashlist_file = fopen(MD5_WORDLIST_PATH, "r");
    unsigned char **hashes = malloc(get_num_password(MD5_WORDLIST_PATH) * sizeof(unsigned char *));
    if (hashlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", MD5_WORDLIST_PATH);
        exit(1);
    }

    char *hash = malloc(EVP_MAX_MD_SIZE * 2 + 1);
    password_to_md5_hash(password, hash);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;
    while ((read = getline(&line, &len, hashlist_file)) != -1) {
        line[strcspn(line, "\n")] = 0;
        hashes[i] = malloc(strlen(line) + 1);
        strcpy(hashes[i], line);
        i++;
    }
    fclose(hashlist_file);

    for (int j = 0; j < i; j++) {
        if (strcmp(hashes[j], hash) == 0) {
            return true;
        }
    }
    return false;
}

bool check_chars(char *password) {
    bool contains_digit = false;
    bool contains_upper = false;
    bool contains_special = false;
    int length = strlen(password);
    for (int i = 0; i < length; i++) {
        if (isdigit(password[i])) {
            contains_digit = true;
        }
        if (isupper(password[i])) {
            contains_upper = true;
        }
        if (!isalnum(password[i])) {
            contains_special = true;
        }
        if (contains_digit && contains_upper && contains_special) {
            break;
        }
    }
    return contains_digit && contains_upper && contains_special;
}

bool check_password(char *password) {
    char *hash = malloc(EVP_MAX_MD_SIZE * 2 + 1);
    password_to_md5_hash(password, hash);
    if (strlen(password) < MIN_PASSWORD_LENGTH) {
        fprintf(stderr, "Error: password must be at least %d characters long\n",
                MIN_PASSWORD_LENGTH);
        return false;
    }

    if (strlen(password) > MAX_PASSWORD_LENGTH) {
        fprintf(stderr, "Error: password must be at most %d characters long\n",
                MAX_PASSWORD_LENGTH);
        return false;
    }

    if (!check_chars(password)) {
        fprintf(stderr, "Error: password must contain at least one digit, one uppercase letter and "
                        "one special character\n");
        return false;
    }

    if (wordlist_check(password)) {
        // fprintf(stderr, "Error: password is too weak\n");
        return false;
    }
    return true;
}

/*
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <password>\n", argv[0]);
        exit(1);
    }
    double start = omp_get_wtime();
    char *password = argv[1];
    if (check_password(password)){
        printf("Password is strong\n");
    }
    double end = omp_get_wtime();
    printf("execution time: %f\n", end - start);
    return 0;
}*/
