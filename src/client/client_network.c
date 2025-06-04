#include "../../include/common.h"
#include "../../include/network.h"

extern volatile bool running;

extern int client_fd;

extern void handle_exit();

/**
 * @brief Thread responsável por receber mensagens do servidor e exibi-las na
 * tela. Filtra mensagens por tipo (sucesso, erro, notificação, chat) e imprime
 * com cores. Detecta o fechamento da conexão pelo servidor e aciona a
 * desconexão do cliente.
 *
 * @param arg Um ponteiro para o descritor de arquivo do socket do cliente.
 */
void *receive_handler(void *arg)
{
  int sockfd = *(int *)arg;

  while (running) {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    int received = receive_message(sockfd, &msg);

    if (received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        usleep(10000);
        continue;
      }

      printf("\n[ERROR] Server connection lost.\n");

      handle_exit();
      break;
    } else if (received == 0) {
      printf("\n[NOTIFICATION] Server disconnected.\n");
      handle_exit();
      break;
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
    case CMD_DIRECT_MESSAGE:
      if (msg.type == CMD_MESSAGE) {
        printf("\033[34m[%s] %s\033[0m\n", msg.username, msg.message);
      } else {
        printf("\033[35m[DM de %s] %s\033[0m\n", msg.username, msg.message);
      }
      break;
    default:
      break;
    }
  }

  return NULL;
}

/**
 * @brief Envia um comando de registro ao servidor.
 *
 * @param username O nome de usuário para registro.
 * @param password A senha para registro.
 */
void send_register_command(const char *username, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_REGISTER;
  strncpy(msg.username, username, MAX_USERNAME - 1);
  msg.username[MAX_USERNAME - 1] = '\0';
  strncpy(msg.password, password, MAX_PASSWORD - 1);
  msg.password[MAX_PASSWORD - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando de login ao servidor.
 *
 * @param username O nome de usuário para login.
 * @param password A senha para login.
 */
void send_login_command(const char *username, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_LOGIN;
  strncpy(msg.username, username, MAX_USERNAME - 1);
  msg.username[MAX_USERNAME - 1] = '\0';
  strncpy(msg.password, password, MAX_PASSWORD - 1);
  msg.password[MAX_PASSWORD - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando de logout ao servidor.
 */
void send_logout_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LOGOUT;

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para criar um grupo ao servidor.
 *
 * @param groupname O nome do grupo a ser criado.
 * @param password A senha do grupo.
 */
void send_create_group_command(const char *groupname, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_CREATE;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);
  msg.groupname[MAX_GROUPNAME - 1] = '\0';
  strncpy(msg.password, password, MAX_PASSWORD - 1);
  msg.password[MAX_PASSWORD - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para entrar em um grupo ao servidor.
 *
 * @param groupname O nome do grupo a ser entrado.
 * @param password A senha do grupo.
 */
void send_enter_group_command(const char *groupname, const char *password)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_ENTER;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);
  msg.groupname[MAX_GROUPNAME - 1] = '\0';
  strncpy(msg.password, password, MAX_PASSWORD - 1);
  msg.password[MAX_PASSWORD - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para sair do grupo atual ao servidor.
 */
void send_leave_group_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LEAVE;

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para deletar um grupo ao servidor.
 *
 * @param groupname O nome do grupo a ser deletado.
 */
void send_delete_group_command(const char *groupname)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_DELETE;
  strncpy(msg.groupname, groupname, MAX_GROUPNAME - 1);
  msg.groupname[MAX_GROUPNAME - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia uma mensagem de chat para o grupo atual ao servidor.
 *
 * @param message O conteúdo da mensagem de chat.
 */
void send_chat_message(const char *message)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_MESSAGE;
  strncpy(msg.message, message, MAX_BUFFER - 1);
  msg.message[MAX_BUFFER - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia uma mensagem direta a um usuário específico.
 *
 * @param recipient O nome de usuário do destinatário.
 * @param message O conteúdo da mensagem.
 */
void send_direct_message(const char *recipient, const char *message)
{
  Message msg;
  memset(&msg, 0, sizeof(Message));

  msg.type = CMD_DIRECT_MESSAGE;
  strncpy(msg.username, recipient, MAX_USERNAME - 1);
  msg.username[MAX_USERNAME - 1] = '\0';
  strncpy(msg.message, message, MAX_BUFFER - 1);
  msg.message[MAX_BUFFER - 1] = '\0';

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para listar todos os grupos disponíveis no servidor.
 */
void send_list_groups_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LIST_GROUPS;

  send_message(client_fd, &msg);
}

/**
 * @brief Envia um comando para listar os membros do grupo atual em que o
 * usuário está.
 */
void send_list_members_command()
{
  Message msg;
  memset(&msg, 0, sizeof(Message));
  msg.type = CMD_LIST_MEMBERS;

  send_message(client_fd, &msg);
}
