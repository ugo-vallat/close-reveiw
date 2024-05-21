#include <mysql.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types/genericlist.h>

#include <server/database-manager.h>
#include <server/weak_password.h>
#include <utils/logger.h>

#define SIZE_HASH 256
#define SIZE_QUERY 512

void mysqlQuery(MYSQL *conn, const char *query, char *fun_name, int exit_status) {
    if (mysql_query(conn, query)) {
        exitl("database-manager", fun_name, exit_status, mysql_error(conn));
    }
}

void createUser(MYSQL *conn, char *username, char *password) {
    char fun_name[16] = "createUser";
    char query[SIZE_QUERY];
    char hash[SIZE_HASH];

    /* Hashage du mot de passe */
    password_to_md5_hash(password, hash);

    /* Ajout de l'utilisateur à la table user */
    sprintf(query, "INSERT INTO user (username) VALUES ('%s')", username);
    mysqlQuery(conn, query, fun_name, 1);

    /* Récupération de l'ID de l'utilisateur nouvellement créé */
    int user_id = mysql_insert_id(conn);

    /* Ajout du mot de passe à la table password */
    sprintf(query, "INSERT INTO password (user_id, password) VALUES (%d, '%s')", user_id, hash);
    mysqlQuery(conn, query, fun_name, 1);
}

void setup(MYSQL *conn) {
    char fun_name[16] = "setup";
    /* Suppression de la base de données */
    mysqlQuery(conn, "DROP DATABASE IF EXISTS close-review", fun_name, 1);

    /* Création de la base de données */
    mysqlQuery(conn, "CREATE DATABASE IF NOT EXISTS close-review", fun_name, 1);

    /* Utilisation de la base de données */
    mysqlQuery(conn, "USE close-review", fun_name, 1);

    /* Création de la table user */
    mysqlQuery(conn,
               "CREATE TABLE IF NOT EXISTS user ("
               "id INT PRIMARY KEY AUTO_INCREMENT,"
               "username VARCHAR(30),"
               "user_nb INT,"
               "request_by INT,"
               "FOREIGN KEY (request_by) REFERENCES user(id))",
               fun_name, 1);

    /* Création de la table password */
    mysqlQuery(conn,
               "CREATE TABLE IF NOT EXISTS password(user_id INT, password VARCHAR(32), "
               "FOREIGN KEY(user_id) REFERENCES user(id))",
               fun_name, 1);
}

bool login(MYSQL *conn, char *username, char *password, int user_nb) {
    char query[256];
    char hash[256];

    /* Hashage du mot de passe */
    password_to_md5_hash(password, hash);

    /* Recherche de l'utilisateur dans la table user */
    sprintf(query, "SELECT id FROM user WHERE username = '%s'", username);

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == NULL) {
        return false;
    }

    int user_id = atoi(row[0]);

    /* Recherche du mot de passe dans la table password */
    sprintf(query, "SELECT password FROM password WHERE user_id = %d", user_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    row = mysql_fetch_row(res);
    if (row == NULL) {
        return false;
    }

    if (strcmp(row[0], hash) == 0) {
        sprintf(query, "UPDATE user SET request_by=NULL, user_nb = %d  WHERE user_id = %d", user_nb,
                user_id);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            exit(1);
        }

        return true;
    }

    return strcmp(row[0], hash) == 0;
}

void disconnect(MYSQL *conn, int user_nb) {
    char fun_name[16] = "disconnect";
    char query[256];

    sprintf(query, "UPDATE user SET request_by=NULL, user_nb = NULL  WHERE user_nb = %d", user_nb);
    mysqlQuery(conn, query, fun_name, 1);
}

bool usernameExists(MYSQL *conn, char *username) {
    char query[256];
    char fun_name[16] = "usernameExits";

    /* Recherche de l'utilisateur dans la table user */
    sprintf(query, "SELECT id FROM user WHERE username = '%s'", username);
    mysqlQuery(conn, query, fun_name, 1);

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    return row != NULL;
}

void logginDatabase(MYSQL *conn, char *server, char *sql_user, char *sql_password, char *database) {
    /* Initialisation de la connexion à la base de données */
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Connexion à la base de données */
    if (mysql_real_connect(conn, server, sql_user, sql_password, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
}

GenList *listUserAvalaible(MYSQL *conn) {
    char fun_name[32] = "listUserAvalaible";
    char query[SIZE_QUERY];

    sprintf(query, "SELECT username FROM user WHERE user_nb IS NOT NULL AND request_by IS NULL");
    mysqlQuery(conn, query, fun_name, 1);

    MYSQL_RES *res = mysql_store_result(conn);
    assertl(res == NULL, "database-manager.c", fun_name, 1, mysql_error(conn));

    int num_rows = mysql_num_rows(res);

    MYSQL_ROW row;
    GenList *results = initGenList(num_rows);
    int i = 0;
    while ((row = mysql_fetch_row(res))) {
        genListAdd(results, row);
    }

    return results;
}
