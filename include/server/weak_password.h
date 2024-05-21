#ifndef WEAK_PASSWORD_H
#define WEAK_PASSWORD_H

#include <stdbool.h>
#include <utils/project_constants.h>

#define MD5_WORDLIST_PATH "hash_test"

/**
 * @brief Get the number of passwords in the password list.
 * @param[in] passlist The list of passwords.
 * @return The number of passwords in the list.
 * @note
 */
int get_num_password(char *passlist);

/**
 * @brief Check if the hash of the password is in the wordlist.
 * @param[in] password The hash of the password to check.
 * @return true if the hash of the password is in the wordlist, false otherwise.
 * @note
 */
bool wordlist_check(char *password);

/**
 * @brief Convert a password to an MD5 hash.
 * @param[in] password The password to convert.
 * @param[out] hash The resulting MD5 hash.
 * @note
 */
void password_to_md5_hash(char *password, char *hash);

/**
 * @brief Check if the password meets certain character requirements.
 * @param[in] password The password to check.
 * @return true if the password meets the requirements, false otherwise.
 * @note
 */
bool check_chars(char *password);

/**
 * @brief Check if the password meets certain requirements.
 * @param[in] password The password to check.
 * @return true if the password meets the requirements, false otherwise.
 * @note
 */
bool check_password(char *password);

#endif /* WEAK_PASSWORD_H */
