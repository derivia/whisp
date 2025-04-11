#include "../../include/common.h"

typedef void (*RegisterLoginFunc)(const char *, const char *);
typedef void (*GroupNameFunc)(const char *);
typedef void (*SimpleFunc)(void);
typedef void (*MessageFunc)(const char *);
typedef void (*GroupCommandWithPassword)(const char *, const char *);

void print_help()
{
  printf("\n");
  printf("Available commands:\n");
  printf("  register <username> <password> - Create a new account\n");
  printf("  login <username> <password> - Log in to your account\n");
  printf("  create <groupname> <password> - Create a new chat group\n");
  printf("  enter <groupname> <password> - Join a chat group\n");
  printf("  leave - Leave current chat group\n");
  printf("  delete <groupname> - Delete a chat group (must be owner)\n");
  printf("  help - Show this help\n");
  printf("  exit - Quit the application\n");
  printf("\n");
  printf("When in a group, any other text will be sent as a message.\n");
  printf("\n");
}

typedef struct {
  RegisterLoginFunc register_cmd;
  RegisterLoginFunc login_cmd;
  SimpleFunc leave_group_cmd;
  GroupNameFunc delete_group_cmd;
  SimpleFunc exit_cmd;
  MessageFunc chat_message_cmd;
  GroupCommandWithPassword create_group_cmd;
  GroupCommandWithPassword enter_group_cmd;
} CommandHandlers;

/**
 * @brief Analisa a entrada do usuário e executa o comando apropriado através
 * dos handlers.
 * É um bagulho feio e bonito.
 *
 * @param input
 * @param handlers
 */
void parse_command(char *input, CommandHandlers *handlers)
{
  if (strncmp(input, "register ", 9) == 0) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    if (sscanf(input + 9, "%s %s", username, password) == 2) {
      handlers->register_cmd(username, password);
    } else {
      printf("Usage: register <username> <password>\n");
    }
  } else if (strncmp(input, "login ", 6) == 0) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    if (sscanf(input + 6, "%s %s", username, password) == 2) {
      handlers->login_cmd(username, password);
    } else {
      printf("Usage: login <username> <password>\n");
    }
  } else if (strncmp(input, "create ", 7) == 0) {
    char groupname[MAX_GROUPNAME], password[MAX_PASSWORD];
    if (sscanf(input + 7, "%s %s", groupname, password) == 2) {
      handlers->create_group_cmd(groupname, password);
    } else {
      printf("Usage: create <groupname> <password>\n");
    }
  } else if (strncmp(input, "enter ", 6) == 0) {
    char groupname[MAX_GROUPNAME], password[MAX_PASSWORD];
    if (sscanf(input + 6, "%s %s", groupname, password) == 2) {
      handlers->enter_group_cmd(groupname, password);
    } else {
      printf("Usage: enter <groupname> <password>\n");
    }
  } else if (strcmp(input, "leave") == 0) {
    handlers->leave_group_cmd();
  } else if (strncmp(input, "delete ", 7) == 0) {
    char groupname[MAX_GROUPNAME];

    if (sscanf(input + 7, "%s", groupname) == 1) {
      handlers->delete_group_cmd(groupname);
    } else {
      printf("Usage: delete <groupname>\n");
    }
  } else if (strcmp(input, "help") == 0) {
    print_help();
  } else if (strcmp(input, "exit") == 0) {
    handlers->exit_cmd();
  } else {
    handlers->chat_message_cmd(input);
  }
}
