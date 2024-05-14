#include <openssl/evp.h>
#include <omp.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void passlist_to_hashlist(char *passlist, char *hashlist, int num_passwords) {
    char **passwords = malloc(num_passwords * sizeof(char *));
    unsigned char **hashes = malloc(num_passwords * sizeof(unsigned char *));
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *passlist_file = fopen(passlist, "r");
    FILE *hashlist_file = fopen(hashlist, "w");
    if (passlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", passlist);
        exit(1);
    }
    if (hashlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", hashlist);
        exit(1);
    }
    int i = 0;
    while ((read = getline(&line, &len, passlist_file)) != -1) {
        line[strcspn(line, "\n")] = 0;
        passwords[i] = malloc(strlen(line) + 1);
        strcpy(passwords[i], line);
        hashes[i] = malloc(EVP_MAX_MD_SIZE);
        i++;
    }
    fclose(passlist_file);

    for (int j = 0; j < i; j++) {
        unsigned int md_len;
        EVP_Digest(passwords[j], strlen(passwords[j]), hashes[j], &md_len, EVP_md5(), NULL);
    }


    for (int j = 0; j < i; j++) {
    char hash_string[EVP_MAX_MD_SIZE * 2 + 1];
    for (unsigned int k = 0; k < EVP_MD_size(EVP_md5()); k++) {
        sprintf(&hash_string[k * 2], "%02x", hashes[j][k]);
    }
    fprintf(hashlist_file, "%s\n", hash_string);
}

    fclose(hashlist_file);
    if (line) {
        free(line);
    }
    for (int j = 0; j < i; j++) {
        free(passwords[j]);
        free(hashes[j]);
    }
    free(passwords);
    free(hashes);

}
/*
#define MAX_PASSWORD_LENGTH 256

void passlist_to_hashlist(char *passlist, char *hashlist, int num_passwords) {
    char *password = malloc(MAX_PASSWORD_LENGTH);
    unsigned char *hash = malloc(EVP_MAX_MD_SIZE*8);
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *passlist_file = fopen(passlist, "r");
    FILE *hashlist_file = fopen(hashlist, "w");
    if (passlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", passlist);
        exit(1);
    }
    if (hashlist_file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", hashlist);
        exit(1);
    }

    while ((read = getline(&line, &len, passlist_file)) != -1) {
        line[strcspn(line, "\n")] = 0;
        strcpy(password, line);
        unsigned int md_len;
        EVP_Digest(password, strlen(password), hash, &md_len, EVP_md5(), NULL);
        for (unsigned int k = 0; k < EVP_MD_size(EVP_md5()); k++) {
            fprintf(hashlist_file, "%02x", hash[k]);
        }
        fprintf(hashlist_file, "\n");
    }
    printf("miaou\n");
    fclose(passlist_file);
    fclose(hashlist_file);
    printf("aled\n");
    // Don't forget to free the allocated memory
    free(password);
    printf("test\n");
    free(hash);
    printf("oskour\n");
}*/

int get_num_password(char *passlist){
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

int main(int argc, char *argv[]){
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <passlist> <hashlist>\n", argv[0]);
        exit(1);
    }
    double start = omp_get_wtime();
    passlist_to_hashlist(argv[1], argv[2], get_num_password(argv[1]));
    double end = omp_get_wtime();
    double exec = end-start;
    printf("Temps d'éxécution : %f\n",exec);
    return 0;

}