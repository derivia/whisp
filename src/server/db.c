#include "../../include/db.h"

/**
 * @brief Callback padrão para execuções SQLite que não retornam dados.
 * @param NotUsed Não utilizado.
 * @param argc Não utilizado.
 * @param argv Não utilizado.
 * @param azColName Não utilizado.
 * @return Sempre 0.
 */
static int default_callback(void *NotUsed, int argc, char **argv,
                            char **azColName)
{
  (void)NotUsed;
  (void)argc;
  (void)argv;
  (void)azColName;
  return 0;
}

/**
 * @brief Inicializa o banco de dados SQLite, abre a conexão e cria a tabela
 * 'users' se não existir.
 *
 * @param db Ponteiro para a estrutura Database a ser inicializada.
 * @param filename Nome do arquivo do banco de dados.
 * @return true se a inicialização for bem-sucedida, false caso contrário.
 */
bool init_database(Database *db, const char *filename)
{
  pthread_mutex_init(&db->mutex, NULL);

  int rc = sqlite3_open(filename, &db->db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db->db));
    sqlite3_close(db->db);
    return false;
  }

  char *sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, "
              "password TEXT NOT NULL);";
  char *err_msg = NULL;

  pthread_mutex_lock(&db->mutex);
  rc = sqlite3_exec(db->db, sql, default_callback, 0, &err_msg);
  pthread_mutex_unlock(&db->mutex);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error creating table: %s\n", err_msg);
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

/**
 * @brief Fecha a conexão com o banco de dados e destrói o mutex.
 *
 * @param db Ponteiro para a estrutura Database a ser fechada.
 */
void close_database(Database *db)
{
  pthread_mutex_lock(&db->mutex);
  sqlite3_close(db->db);
  pthread_mutex_unlock(&db->mutex);
  pthread_mutex_destroy(&db->mutex);
}

/**
 * @brief Verifica se um usuário com o nome de usuário fornecido já existe no
 * banco de dados. Usa prepared statements para prevenir SQL Injection.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser verificado.
 * @return true se o usuário existir, false caso contrário.
 */
bool user_exists(Database *db, const char *username)
{
  sqlite3_stmt *stmt;
  bool exists = false;
  const char *sql = "SELECT 1 FROM users WHERE username = ?;";

  pthread_mutex_lock(&db->mutex);

  if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);

    exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
  } else {
    fprintf(stderr, "Failed to prepare statement for user_exists: %s\n",
            sqlite3_errmsg(db->db));
  }

  pthread_mutex_unlock(&db->mutex);

  return exists;
}

/**
 * @brief Adiciona um novo usuário ao banco de dados com seu nome de usuário e
 * senha hash. Usa prepared statements para prevenir SQL Injection.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser adicionado.
 * @param hashed_password A senha hash do usuário.
 * @return true se o usuário for adicionado com sucesso, false se já existir ou
 * houver erro.
 */
bool add_user(Database *db, const char *username, const char *hashed_password)
{
  if (user_exists(db, username)) return false;

  sqlite3_stmt *stmt;
  bool success = false;
  const char *sql = "INSERT INTO users (username, password) VALUES (?, ?);";

  pthread_mutex_lock(&db->mutex);

  if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
      success = true;
    } else {
      fprintf(stderr, "SQL error inserting user: %s\n",
              sqlite3_errmsg(db->db));
    }
    sqlite3_finalize(stmt);
  } else {
    fprintf(stderr, "Failed to prepare statement for add_user: %s\n",
            sqlite3_errmsg(db->db));
  }

  pthread_mutex_unlock(&db->mutex);

  return success;
}

/**
 * @brief Verifica as credenciais de um usuário comparando o nome de usuário e
 * a senha hash fornecidos com os armazenados no banco de dados. Usa prepared
 * statements para prevenir SQL Injection.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser autenticado.
 * @param password A senha (já hash) a ser verificada.
 * @return true se as credenciais forem válidas, false caso contrário.
 */
bool verify_user(Database *db, const char *username, const char *password)
{
  sqlite3_stmt *stmt;
  bool success = false;
  const char *sql = "SELECT password FROM users WHERE username = ?;";

  pthread_mutex_lock(&db->mutex);

  if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *stored_hash = (const char *)sqlite3_column_text(stmt, 0);

      success = (strcmp(stored_hash, password) == 0);
    }
    sqlite3_finalize(stmt);
  } else {
    fprintf(stderr, "Failed to prepare statement for verify_user: %s\n",
            sqlite3_errmsg(db->db));
  }

  pthread_mutex_unlock(&db->mutex);

  return success;
}
