#define _POSIX_C_SOURCE 200890L
#include "../../include/auth.h"
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

char *hash_password(const char *password)
{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256((const unsigned char *)password, strlen(password), hash);
  char *output = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(output + (i * 2), "%02x", hash[i]);
  }
  output[SHA256_DIGEST_LENGTH * 2] = '\0';
  return output;
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
