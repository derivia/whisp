#include "../../include/network.h"

extern volatile bool running;
extern int client_fd;

void *receive_handler(void *arg)
{
  int sockfd = *(int *)arg;

  while (running) {
    Message msg;
    int received = receive_message(sockfd, &msg);

    if (received <= 0) {
      usleep(10000);
      continue;
    }

    switch (msg.type) {
    case CMD_SUCCESS:
      printf("\033[32m[SUCCESS] %s\033[0m\n", msg.message);
      break;
    case CMD_ERROR:
      printf("\033[31m[ERROR] %s\033[0m\n", msg.message);
      break;
    case CMD_NOTIFICATION:
      printf("\033[33m[NOTIFICATION] %s\033[0m\n", msg.message);
      break;
    case CMD_MESSAGE:
      printf("\033[34m[%s] %s\033[0m\n", msg.username, msg.message);
      break;
    default:
      break;
    }
  }

  return NULL;
}

void send_register_command(const char *username, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_REGISTER;
  strncpy(msg.username, username, MAX_USERNAME - 1);
  strncpy(msg.password, password, MAX_PASSWORD - 1);

  send_message(client_fd, &msg);
}

void send_login_command(const char *username, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_LOGIN;
  strncpy(msg.username, username, MAX_USERNAME - 1);
  strncpy(msg.password, password, MAX_PASSWORD - 1);

  send_message(client_fd, &msg);
}

void send_logout_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LOGOUT;

  send_message(client_fd, &msg);
}

void send_create_group_command(const char *groupname)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_CREATE;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);

  send_message(client_fd, &msg);
}

void send_enter_group_command(const char *groupname)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_ENTER;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);

  send_message(client_fd, &msg);
}

void send_leave_group_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LEAVE;

  send_message(client_fd, &msg);
}

void send_delete_group_command(const char *groupname)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_DELETE;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);

  send_message(client_fd, &msg);
}

void send_chat_message(const char *message)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_MESSAGE;
  strncpy(msg.message, message, MAX_BUFFER - 1);

  send_message(client_fd, &msg);
}
