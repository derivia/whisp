#ifndef WHISP_AUTH_H
#define WHISP_AUTH_H

#include "common.h"
#include "db.h"

typedef struct {
  char username[MAX_USERNAME];
  int sockfd;
  bool authenticated;
  char current_group[MAX_GROUPNAME];
} User;

bool register_user(Database *db, const char *username, const char *password);
bool authenticate_user(Database *db, const char *username,
                       const char *password);
char *hash_password(const char *password);

#endif
