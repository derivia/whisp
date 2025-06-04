#include "../../include/common.h"
#include "../../include/handlers.h"
#include "ctype.h"

/**
 * @brief Imprime uma mensagem de ajuda com todos os comandos disponíveis para
 * o usuário.
 */
void print_help()
{
  printf("\n");
  printf("Available commands:\n");
  printf("  /register <username> <password> - Create a new account\n");
  printf("  /login <username> <password> - Log in to your account\n");
  printf("  /create <groupname> <password> - Create a new chat group\n");
  printf("  /enter <groupname> <password> - Join a chat group\n");
  printf("  /leave - Leave current chat group\n");
  printf("  /delete <groupname> - Delete a chat group (must be owner)\n");
  printf("  /dm <recipient_username> <message> - Send a direct message to a "
         "user\n");
  printf("  /listgroups - List all available chat groups\n");

  printf("  /who - List members in your current chat group\n");

  printf("  /help - Show this help\n");
  printf("  /exit - Quit the application\n");
  printf("\n");
  printf("When in a group, any other text will be sent as a message.\n");
  printf("\n");
}

/**
 * @brief Verifica se uma string consiste apenas de caracteres alfanuméricos.
 *
 * @param s A string a ser verificada.
 * @return true se a string for alfanumérica, false caso contrário.
 */
static bool is_alphanumeric(const char *s)
{
  if (!s || *s == '\0') return false;
  for (int i = 0; s[i] != '\0'; i++) {
    if (!isalnum((unsigned char)s[i])) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Analisa a entrada do usuário e executa o comando apropriado através
 * dos handlers. Inclui validações básicas para os argumentos dos comandos.
 *
 * @param input A string de entrada lida do usuário.
 * @param handlers Um ponteiro para a estrutura CommandHandlers que contém as
 * funções de callback.
 */
void parse_command(char *input, CommandHandlers *handlers)
{
  // /register <username> <password>
  if (strncmp(input, "/register ", 10) == 0) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    if (sscanf(input + 10, "%s %s", username, password) == 2) {
      if (strlen(username) >= 3 && strlen(username) < MAX_USERNAME &&
          strlen(password) >= 4 && strlen(password) < MAX_PASSWORD &&
          is_alphanumeric(username) && is_alphanumeric(password)) {
        handlers->register_cmd(username, password);
      } else {
        printf("Error: Username must be 3-%d alphanumeric characters, "
               "password 4-%d alphanumeric characters.\n",
               MAX_USERNAME - 1, MAX_PASSWORD - 1);
      }
    } else {
      printf("Usage: /register <username> <password>\n");
    }
  }
  // /login <username> <password>
  else if (strncmp(input, "/login ", 7) == 0) {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    if (sscanf(input + 7, "%s %s", username, password) == 2) {
      if (strlen(username) >= 3 && strlen(username) < MAX_USERNAME &&
          strlen(password) >= 4 && strlen(password) < MAX_PASSWORD &&
          is_alphanumeric(username) && is_alphanumeric(password)) {
        handlers->login_cmd(username, password);
      } else {
        printf("Error: Username must be 3-%d alphanumeric characters, "
               "password 4-%d alphanumeric characters.\n",
               MAX_USERNAME - 1, MAX_PASSWORD - 1);
      }
    } else {
      printf("Usage: /login <username> <password>\n");
    }
  }
  // /create <groupname> <password>
  else if (strncmp(input, "/create ", 8) == 0) {
    char groupname[MAX_GROUPNAME], password[MAX_PASSWORD];
    if (sscanf(input + 8, "%s %s", groupname, password) == 2) {
      if (strlen(groupname) >= 3 && strlen(groupname) < MAX_GROUPNAME &&
          strlen(password) >= 4 && strlen(password) < MAX_PASSWORD &&
          is_alphanumeric(groupname) && is_alphanumeric(password)) {
        handlers->create_group_cmd(groupname, password);
      } else {
        printf("Error: Groupname must be 3-%d alphanumeric characters, "
               "password 4-%d alphanumeric characters.\n",
               MAX_GROUPNAME - 1, MAX_PASSWORD - 1);
      }
    } else {
      printf("Usage: /create <groupname> <password>\n");
    }
  }
  // /enter <groupname> <password> ou /join <groupname> <password>
  else if (strncmp(input, "/enter ", 7) == 0 ||
           strncmp(input, "/join ", 6) == 0) {
    char groupname[MAX_GROUPNAME], password[MAX_PASSWORD];
    const char *ptr = input + (input[6] == ' ' ? 6 : 7);
    if (sscanf(ptr, "%s %s", groupname, password) == 2) {
      if (strlen(groupname) >= 3 && strlen(groupname) < MAX_GROUPNAME &&
          strlen(password) >= 4 && strlen(password) < MAX_PASSWORD &&
          is_alphanumeric(groupname) && is_alphanumeric(password)) {
        handlers->enter_group_cmd(groupname, password);
      } else {
        printf("Error: Groupname must be 3-%d alphanumeric characters, "
               "password 4-%d alphanumeric characters.\n",
               MAX_GROUPNAME - 1, MAX_PASSWORD - 1);
      }
    } else {
      printf("Usage: /enter <groupname> <password>\n");
    }
  }
  // /leave
  else if (strcmp(input, "/leave") == 0) {
    handlers->leave_group_cmd();
  }
  // /delete <groupname>
  else if (strncmp(input, "/delete ", 8) == 0) {
    char groupname[MAX_GROUPNAME];

    if (sscanf(input + 8, "%s", groupname) == 1) {
      if (strlen(groupname) >= 3 && strlen(groupname) < MAX_GROUPNAME &&
          is_alphanumeric(groupname)) {
        handlers->delete_group_cmd(groupname);
      } else {
        printf("Error: Groupname must be 3-%d alphanumeric characters.\n",
               MAX_GROUPNAME - 1);
      }
    } else {
      printf("Usage: /delete <groupname>\n");
    }
  }
  // /dm <recipient_username> <message>
  else if (strncmp(input, "/dm ", 4) == 0) {
    char recipient_username[MAX_USERNAME];
    char *message_start = NULL;
    int offset = 0;

    if (sscanf(input + 4, "%s%n", recipient_username, &offset) == 1) {
      message_start = input + 4 + offset;

      if (strlen(recipient_username) >= 3 &&
          strlen(recipient_username) < MAX_USERNAME &&
          is_alphanumeric(recipient_username)) {
        if (strlen(message_start) > 0 && strlen(message_start) < MAX_MESSAGE) {
          handlers->direct_message_cmd(recipient_username, message_start);
        } else {
          printf("Error: Direct message cannot be empty and must be less than "
                 "%d characters.\n",
                 MAX_MESSAGE);
        }
      } else {
        printf("Error: Recipient username must be 3-%d alphanumeric "
               "characters.\n",
               MAX_USERNAME - 1);
      }
    } else {
      printf("Usage: /dm <recipient_username> <message>\n");
    }
  }
  // /listgroups
  else if (strcmp(input, "/listgroups") == 0) {
    handlers->list_groups_cmd();
  }
  // /who
  else if (strcmp(input, "/who") == 0) {
    handlers->list_members_cmd();
  }
  // /help
  else if (strcmp(input, "/help") == 0) {
    print_help();
  }
  // /exit
  else if (strcmp(input, "/exit") == 0) {
    handlers->exit_cmd();
  }

  else {
    if (strlen(input) > 0 && strlen(input) < MAX_MESSAGE) {
      handlers->chat_message_cmd(input);
    } else {
      printf("Error: Chat message cannot be empty and must be less than %d "
             "characters.\n",
             MAX_MESSAGE);
    }
  }
}
