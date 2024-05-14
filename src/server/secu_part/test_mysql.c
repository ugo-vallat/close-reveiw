#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdbool.h>

#include "weak_password.h"

void create_user(MYSQL *conn, char *username, char *password) {
    char query[256];
    char hash[256];

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
if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS user(id INT PRIMARY KEY AUTO_INCREMENT, username VARCHAR(30))")) {
    fprintf(stderr, "%s\n", mysql_error(conn));
    exit(1);
}

    /* Création de la table password */
    if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS password(user_id INT, password VARCHAR(32), FOREIGN KEY(user_id) REFERENCES user(id))")) {
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

void se_connecter_a_la_base_de_donnees(MYSQL *conn, char *server, char *sql_user, char *sql_password, char *database) {
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

int main() {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = "localhost";
    char *sql_user = "newuser";
    char *sql_password = "password"; 
    char *database = "";
    char user[32];
    char password[32];
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
    //fgets à modifier car non sécurisé
    while (true){
        printf("\n\n1 : setup la base de données (attention supprime l'ancienne si elle existe)\n2 : créer un utilisateur\n3 : se connecter\n4 : quitter\n");
        int choice;
        scanf("%d", &choice);
        getchar(); //pour enlever le \n et éviter les problèmes de fgets
        if (choice == 1){
            setup(conn);
            printf("\nBase de données prête\n");
        }
        else if (choice == 2){
            // utilise testdb 
            mysql_query(conn, "USE testdb");
            printf("Entrez le nom d'utilisateur : ");
            fgets(user, 31, stdin);
            user[strlen(user)-1] = '\0';
            if (username_exists(conn, user)){
                printf("Nom d'utilisateur déjà utilisé\n");
                continue;
            }
            printf("Entrez le mot de passe : ");
            fgets(password, 31, stdin); 
            password[strlen(password)-1] = '\0'; 
            //partie supérieur à remplacer quand on aura communication serveur
            if (check_password(password)){
                create_user(conn, user, password);
                printf("\nUtilisateur créé\n");
            }
            else{
                printf("\nMot de passe faible ou disponible dans une wordlist sur internet\n");
            }

        }
        else if (choice == 3){
            mysql_query(conn, "USE testdb"); // utilise testdb 
            printf("Entrez le nom d'utilisateur : ");
            fgets(user, 31, stdin);
            user[strlen(user)-1] = '\0';
            printf("Entrez le mot de passe : ");
            fgets(password, 31, stdin);
            password[strlen(password)-1] = '\0'; 
            //partie supérieur à remplacer quand on aura communication serveur
            if (login(conn, user, password)){
                printf("\nConnexion réussie\n");
            }
            else{
                printf("\nConnexion échouée\n");
            }
        }
        else if (choice == 4){
            mysql_close(conn);
            return 0;
        }
        else{
            printf("\nChoix invalide\n");
        }
        

    }
    /* Récupération des informations de l'utilisateur */
    fgets(password, 31, stdin);
    password[strlen(password)-1] = '\0';
    fgets(user, 31, stdin);
    user[strlen(user)-1] = '\0';

    /* Création de l'utilisateur */
    create_user(conn, user, password);

    /* Setup à la base de données */
    setup(conn);



    /* fermeture connection */
    mysql_close(conn);
}