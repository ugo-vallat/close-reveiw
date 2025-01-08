#ifndef __HISTORY_MANAGER_H__
#define __HISTORY_MANAGER_H__

#include <stdbool.h>
#include <types/genericlist.h>
#include <types/message.h>

typedef struct s_history_info HistoryInfo;

/**
 * @brief Initialise la structure History Info et ouvre ou crée le fichier path_fichier_historique
 * @param path_fichier_historique fichier dans lequel l'historique sera sauvegardé
 * @param nom_conversation nom de la conversation a sauvegarder (id de la personne avec qui on a la
 * discution)
 * @return HistoryInfo*
 * @note Structure malloc, utiliser deinitHistory() pour supprimer
 */
HistoryInfo *initHistory(char *path_dossier_historique, char *nom_conversation);

/**
 * @brief Supprime la structure History Info et ferme le fichier dont le path est dans info
 * @param info stucture supprimer
 */
void deinitHistory(HistoryInfo **info);

/**
 * @brief Ecrit le message dans l'historique dont les information sont donner par info
 * @param[in] message struture message a historiser
 * @param[in] info structure qui contient les information pour l'historisation
 * @return 0 en cas de succee, un code d'erreur sinon
 * @note Structure malloc, utiliser deinitHistory() pour supprimer
 */
int writeHistory(HistoryInfo *info, Msg *message);

/**
 * @brief lit les dernier nb_messasse le message dans l'historique dont les information sont donner
 * par info
 * @param[in] info structure qui contient les information pour l'historisation
 * @param[in] nb_message nombre de message a lire
 * @param[out] l liste generique qui contiendra les message
 * @return nombre de message lut, -1 en cas d'erreur
 * @note Structure malloc, utiliser deinitHistory() pour supprimer
 */
int readHistory(HistoryInfo *info, int nb_message, GenList *l);

/**
 * @brief delete all history for that conversation
 * @param info struture which say wath conversation to delete
 * @return 0 en cas de succee, un code d'erreur sinon
 */
int deleteHistory(HistoryInfo *info);

/**
 * @brief delete all history
 * @param path_history_dir path to where the history is save
 * @return 0 en cas de succee, un code d'erreur sinon
 */
int deleteAllHistory(char *path_history_dir);

/**
 * @brief export the directory to a dest_file.zip
 * @param[in] path_history_dir path to where the history is save
 * @param[in] dest_file name of the file create, can contain a path
 * @return 0 en cas de succee, un code d'erreur sinon
 */
int exportHistory(char *path_history_dir, char *dest_file);

/**
 * @brief import a file in the history dir
 * @param[in] path_history_dir path to where the history is save
 * @param[in] path_archive name of the file to importe
 * @return 0 en cas de succee, un code d'erreur sinon
 */
int importHistory(char *path_history_dir, char *path_archive);
#endif