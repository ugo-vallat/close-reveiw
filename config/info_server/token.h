#ifndef TOKEN_H
#define TOKEN_H

#define TOKEN_SIZE 16

/**
 * @brief Generate a secure random token.
 * @param[out] buffer The buffer to store the generated token. This buffer must be at least TOKEN_SIZE*2 + 1 characters long.
 * @return void
 * @note The generated token is a hexadecimal string representation of a byte array. Therefore, the length of the string is twice the size of the original byte array (TOKEN_SIZE*2).
 */
void generate_token(char *buffer);

#endif // TOKEN_H