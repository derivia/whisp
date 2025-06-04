#include "../../include/common.h"

/**
 * @brief Imprime uma mensagem de erro no stderr e encerra o programa.
 *
 * @param message A string da mensagem de erro a ser exibida.
 */
void error_exit(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

/**
 * @brief Configura um socket para operar no modo não-bloqueante.
 *
 * @param sockfd O descritor de arquivo do socket.
 * @return 0 em caso de sucesso, -1 em caso de falha.
 */
int set_nonblocking(int sockfd)
{
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}
