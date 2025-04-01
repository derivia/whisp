#define _POSIX_C_SOURCE 200890L
#include "../../include/auth.h"
#include <stdlib.h>
#include <string.h>

char *hash_password(const char *password)
{
  return strdup(password);
}

bool register_user(Database *db, const char *username, const char *password)
{
  if (strlen(username) < 3 || strlen(password) < 4) return false;
  char *hashed = hash_password(password);
  bool result = add_user(db, username, hashed);
  free(hashed);
  return result;
}

bool authenticate_user(Database *db, const char *username,
                       const char *password)
{
  char *hashed = hash_password(password);
  bool result = verify_user(db, username, hashed);
  free(hashed);
  return result;
}
