#include "../../include/common.h"

void error_exit(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

int set_nonblocking(int sockfd)
{
  int flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}
