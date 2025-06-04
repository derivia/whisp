#ifndef WHISP_COMMON_H
#define WHISP_COMMON_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT     6969
#define MAX_BUFFER       4096
#define MAX_CONTENT_SIZE 4096
#define MAX_MESSAGE      4096
#define MAX_USERNAME     32
#define MAX_PASSWORD     64
#define MAX_GROUPNAME    32
#define MAX_CLIENTS      100
#define MAX_GROUPS       50

typedef enum {
  CMD_REGISTER,
  CMD_LOGIN,
  CMD_LOGOUT,
  CMD_CREATE,
  CMD_ENTER,
  CMD_LEAVE,
  CMD_DELETE,
  CMD_MESSAGE,
  CMD_DIRECT_MESSAGE,
  CMD_LIST_GROUPS,
  CMD_LIST_MEMBERS,
  CMD_SUCCESS,
  CMD_ERROR,
  CMD_NOTIFICATION
} CommandType;

typedef struct {
  CommandType type;
  char username[MAX_USERNAME];
  char password[MAX_PASSWORD];
  char groupname[MAX_GROUPNAME];
  char message[MAX_BUFFER];
  time_t timestamp;
} Message;

void error_exit(const char *message);

int set_nonblocking(int sockfd);

#endif
