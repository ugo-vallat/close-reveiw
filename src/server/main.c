#include <server/database-manager.h>
#include <utils/logger.h>
#include <network/tls-com.h>


char FILE_NAME[16] = "MAIN";


int main(){
    char *FUN_NAME = "MAIN";
    init_logger(NULL);

    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    char *server = "localhost"; //TODO les faire passer en argument
    char *sql_user = "newuser";
    char *sql_password = "password"; 
    char *database = "";



    //TODO modifier pour que se soit dans u scripte appart
    /* Initialisation de la connexion à la base de données */ 
    conn = mysql_init(NULL);
    if (conn == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }
    /* Connexion à la base de données */
    if (mysql_real_connect(conn, server, sql_user, sql_password, database, 0, NULL, 0) == NULL) {
        exitl(FILE_NAME, FUN_NAME, -1, "%s\n", mysql_error(conn));
    }

    // TLS_infos tls = initTLSInfos(ip_server, port_server, TLS_MAIN_SERVER, *path_cert, *path_key);
}