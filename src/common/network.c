#include "../../include/network.h"

int create_socket(void)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) error_exit("socket creation failed");

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    error_exit("setsockopt failed");

  return sockfd;
}

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

void send_message(int sockfd, const Message *msg)
{
  if (send(sockfd, msg, sizeof(Message), 0) < 0) error_exit("send failed");
}

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
