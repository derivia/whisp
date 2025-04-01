#include "../../include/db.h"

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
  rc = sqlite3_exec(db->db, sql, 0, 0, &err_msg);
  pthread_mutex_unlock(&db->mutex);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

void close_database(Database *db)
{
  pthread_mutex_lock(&db->mutex);
  sqlite3_close(db->db);
  pthread_mutex_unlock(&db->mutex);
  pthread_mutex_destroy(&db->mutex);
}

bool user_exists(Database *db, const char *username)
{
  char sql[512];
  snprintf(sql, sizeof(sql), "SELECT 1 FROM users WHERE username = '%s';",
           username);

  sqlite3_stmt *stmt;
  bool exists = false;

  pthread_mutex_lock(&db->mutex);
  if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
  }
  pthread_mutex_unlock(&db->mutex);

  return exists;
}

bool add_user(Database *db, const char *username, const char *hashed_password)
{
  if (user_exists(db, username)) return false;

  char sql[1024];
  snprintf(sql, sizeof(sql),
           "INSERT INTO users (username, password) VALUES ('%s', '%s');",
           username, hashed_password);

  char *err_msg = NULL;
  bool success = false;

  pthread_mutex_lock(&db->mutex);
  int rc = sqlite3_exec(db->db, sql, 0, 0, &err_msg);
  if (rc == SQLITE_OK) {
    success = true;
  } else {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
  }
  pthread_mutex_unlock(&db->mutex);

  return success;
}

bool verify_user(Database *db, const char *username, const char *password)
{
  char sql[512];
  snprintf(sql, sizeof(sql),
           "SELECT password FROM users WHERE username = '%s';", username);

  sqlite3_stmt *stmt;
  bool success = false;

  pthread_mutex_lock(&db->mutex);
  if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *stored_hash = (const char *)sqlite3_column_text(stmt, 0);
      success = (strcmp(stored_hash, password) == 0);
    }
    sqlite3_finalize(stmt);
  }
  pthread_mutex_unlock(&db->mutex);

  return success;
}
