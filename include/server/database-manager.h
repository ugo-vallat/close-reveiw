#ifndef TEST_MYSQL_H
#define TEST_MYSQL_H

#include <mysql.h>
#include <stdbool.h>
#include <types/genericlist.h>
#include <types/p2p-msg.h>

/**
 * @brief Set up the database, creating necessary tables.
 * @param[in] conn The MySQL connection handle.
 */
void setup(MYSQL *conn);

/**
 * @brief Create a new user in the database.
 * @param[in] conn The MySQL connection handle.
 * @param[in] username The username of the new user.
 * @param[in] password The password of the new user.
 */
void createUser(MYSQL *conn, char *username, char *password);

/**
 * @brief Attempt to log in with a given username and password.
 * @param[in] conn The MySQL connection handle.
 * @param[in] username The username to log in with.
 * @param[in] password The password to log in with.
 * @return true if login was successful, false otherwise.
 */
bool login(MYSQL *conn, char *username, char *password);


/**
 * @brief Check if a username exists in the database.
 * @param[in] conn The MySQL connection handle.
 * @param[in] username The username to check.
 * @return true if the username exists, false otherwise.
 */
bool usernameExists(MYSQL *conn, char *username);

/**
 * @brief Connect to the MySQL database.
 * @param[in] conn The MySQL connection handle.
 * @param[in] server The server to connect to.
 * @param[in] sql_user The username to connect with.
 * @param[in] sql_password The password to connect with.
 * @param[in] database The database to connect to.
 */
void logginDatabase(MYSQL *conn, char *server, char *sql_user, char *sql_password, char *database);

int getId(MYSQL *conn, char *username);


#endif // TEST_MYSQL_H
