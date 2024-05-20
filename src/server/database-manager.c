#include <mysql.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <server/weak_password.h>

#define SIZE_HASH 256
#define SIZE_QUERY 512

void create_user(MYSQL *conn, char *username, char *password) {
    char query[SIZE_QUERY];
    char hash[SIZE_HASH];

    /* Hashage du mot de passe */
    password_to_md5_hash(password, hash);

    /* Ajout de l'utilisateur à la table user */
    sprintf(query, "INSERT INTO user (username) VALUES ('%s')", username);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Récupération de l'ID de l'utilisateur nouvellement créé */
    int user_id = mysql_insert_id(conn);

    /* Ajout du mot de passe à la table password */
    sprintf(query, "INSERT INTO password (user_id, password) VALUES (%d, '%s')", user_id, hash);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
}

void setup(MYSQL *conn) {
    /* Suppression de la base de données */
    if (mysql_query(conn, "DROP DATABASE IF EXISTS testdb")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Création de la base de données */
    if (mysql_query(conn, "CREATE DATABASE IF NOT EXISTS testdb")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Utilisation de la base de données */
    if (mysql_query(conn, "USE testdb")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Création de la table user */
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS user(id INT PRIMARY KEY AUTO_INCREMENT, "
                          "username VARCHAR(30))")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    /* Création de la table password */
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS password(user_id INT, password VARCHAR(32), "
                          "FOREIGN KEY(user_id) REFERENCES user(id))")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
}

bool login(MYSQL *conn, char *username, char *password) {
    char query[256];
    char hash[256];

    /* Hashage du mot de passe */
    password_to_md5_hash(password, hash);

    /* Recherche de l'utilisateur dans la table user */
    sprintf(query, "SELECT id FROM user WHERE username = '%s'", username);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

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

    return strcmp(row[0], hash) == 0;
}

bool username_exists(MYSQL *conn, char *username) {
    char query[256];

    /* Recherche de l'utilisateur dans la table user */
    sprintf(query, "SELECT id FROM user WHERE username = '%s'", username);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    return row != NULL;
}

void se_connecter_a_la_base_de_donnees(MYSQL *conn, char *server, char *sql_user,
                                       char *sql_password, char *database) {
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