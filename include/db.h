#ifndef WHISP_DB_H
#define WHISP_DB_H

#include "common.h"
#include <sqlite3.h>

typedef struct {
  sqlite3 *db;
  pthread_mutex_t mutex;
} Database;

bool init_database(Database *db, const char *filename);
void close_database(Database *db);
bool user_exists(Database *db, const char *username);
bool add_user(Database *db, const char *username, const char *hashed_password);
bool verify_user(Database *db, const char *username, const char *password);

#endif
