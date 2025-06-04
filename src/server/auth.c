#define _POSIX_C_SOURCE 200890L
#include "../../include/auth.h"
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Gera o hash SHA256 de uma senha.
 * @param password A senha em texto claro a ser hashada.
 * @return Uma string recém-alocada contendo o hash hexadecimal da senha.
 * O chamador é responsável por liberar esta memória.
 */
char *hash_password(const char *password)
{
  unsigned char hash[SHA256_DIGEST_LENGTH];

  SHA256((const unsigned char *)password, strlen(password), hash);

  char *output = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
  if (output == NULL) {
    perror("Failed to allocate memory for hashed password");
    return NULL;
  }

  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(output + (i * 2), "%02x", hash[i]);
  }
  output[SHA256_DIGEST_LENGTH * 2] = '\0';
  return output;
}

/**
 * @brief Tenta registrar um novo usuário no banco de dados.
 * Valida o comprimento do nome de usuário e da senha antes de fazer o hash.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser registrado.
 * @param password A senha em texto claro do usuário.
 * @return true se o registro for bem-sucedido, false caso contrário (ex:
 * usuário já existe, credenciais inválidas).
 */
bool register_user(Database *db, const char *username, const char *password)
{
  if (strlen(username) < 3 || strlen(password) < 4) return false;

  char *hashed = hash_password(password);
  if (hashed == NULL) return false;

  bool result = add_user(db, username, hashed);
  free(hashed);
  return result;
}

/**
 * @brief Tenta autenticar um usuário.
 * Faz o hash da senha fornecida e compara com o hash armazenado no banco de
 * dados.
 *
 * @param db Ponteiro para a estrutura Database.
 * @param username O nome de usuário a ser autenticado.
 * @param password A senha em texto claro fornecida pelo usuário.
 * @return true se as credenciais forem válidas, false caso contrário.
 */
bool authenticate_user(Database *db, const char *username,
                       const char *password)
{
  char *hashed = hash_password(password);
  if (hashed == NULL) return false;

  bool result = verify_user(db, username, hashed);
  free(hashed);
  return result;
}
