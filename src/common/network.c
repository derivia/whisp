#include "../../include/network.h"
#include "../../include/common.h"

/**
 * @brief Cria um socket TCP reutilizável para comunicação.
 *
 * @return O descritor de arquivo do socket criado.
 */
int create_socket(void)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error_exit("socket creation failed");

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    error_exit("setsockopt failed");

  return sockfd;
}

/**
 * @brief Conecta-se a um servidor remoto usando um endereço IP e porta.
 *
 * @param address O endereço IP do servidor (string).
 * @param port A porta do servidor.
 * @return O descritor de arquivo do socket conectado.
 */
int connect_to_server(const char *address, int port)
{
  int sockfd = create_socket();

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0)
    error_exit("invalid address");

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0)
    error_exit("connection failed");

  return sockfd;
}

/**
 * @brief Configura um socket TCP para escuta em todas as interfaces na porta
 * especificada.
 *
 * @param port A porta em que o servidor irá escutar.
 * @return O descritor de arquivo para o socket do servidor em escuta.
 */
int setup_server(int port)
{
  int sockfd = create_socket();

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    error_exit("bind failed");

  if (listen(sockfd, 10) < 0) error_exit("listen failed");

  return sockfd;
}

/**
 * @brief Envia uma struct Message pelo socket usando envio binário direto.
 *
 * @param sockfd O descritor de arquivo do socket para enviar.
 * @param msg Um ponteiro para a estrutura Message a ser enviada.
 */
void send_message(int sockfd, const Message *msg)
{
  if (send(sockfd, msg, sizeof(Message), 0) < 0) error_exit("send failed");
}

/**
 * @brief Recebe dados do socket e preenche uma struct Message; trata modo
 * não-bloqueio e erros.
 *
 * @param sockfd O descritor de arquivo do socket para receber.
 * @param msg Um ponteiro para a estrutura Message a ser preenchida.
 * @return O número de bytes recebidos, 0 se não há dados (modo
 * não-bloqueante), ou -1 em caso de erro.
 */
int receive_message(int sockfd, Message *msg)
{
  int n = recv(sockfd, msg, sizeof(Message), 0);
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
    perror("recv failed");
    return -1;
  }
  return n;
}
