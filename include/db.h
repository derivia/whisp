#ifndef WHISP_DB_H
#define WHISP_DB_H

#include "common.h"
#include <sqlite3.h>

typedef struct {
  sqlite3 *db;
  pthread_mutex_t mutex;
} Database;

/**
 * @brief Inicializa o banco de dados SQLite, abre a conexão e cria a tabela
 * 'users' se não existir.
 *
 * @param db Ponteiro para a estrutura Database a ser inicializada.
 * @param filename Nome do arquivo do banco de dados.
 * @return true se a inicialização for bem-sucedida, false caso contrário.
 */
bool init_database(Database *db, const char *filename);

/**
 * @brief Fecha a conexão com o banco de dados e destrói o mutex.
 *
 * @param db Ponteiro para a estrutura Database a ser fechada.
 */
void close_database(Database *db);

/**
 * @brief Verifica se um usuário com o nome de usuário fornecido já existe no
 * banco de dados.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser verificado.
 * @return true se o usuário existir, false caso contrário.
 */
bool user_exists(Database *db, const char *username);

/**
 * @brief Adiciona um novo usuário ao banco de dados com seu nome de usuário e
 * senha hash.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser adicionado.
 * @param hashed_password A senha hash do usuário.
 * @return true se o usuário for adicionado com sucesso, false se já existir ou
 * houver erro.
 */
bool add_user(Database *db, const char *username, const char *hashed_password);

/**
 * @brief Verifica as credenciais de um usuário comparando o nome de usuário e
 * a senha hash fornecidos com os armazenados no banco de dados.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser autenticado.
 * @param password A senha (já hash) a ser verificada.
 * @return true se as credenciais forem válidas, false caso contrário.
 */
bool verify_user(Database *db, const char *username, const char *password);

#endif
