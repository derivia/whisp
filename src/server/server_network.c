#include "../../include/auth.h"
#include "../../include/chat.h"
#include "../../include/common.h"
#include "../../include/db.h"
#include "../../include/network.h"

extern ClientManager client_manager;
extern GroupManager group_manager;
extern Database database;

typedef struct {
  int sockfd;
} ClientArgs;

/**
 * @brief Processa uma solicitação de registro, tentando cadastrar o usuário no
 * banco de dados. Responde ao cliente com sucesso ou erro, dependendo da
 * disponibilidade do nome de usuário e validade das credenciais.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de registro recebida.
 */
void handle_register(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  if (strlen(msg->username) < 3 || strlen(msg->username) >= MAX_USERNAME ||
      strlen(msg->password) < 4 || strlen(msg->password) >= MAX_PASSWORD) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid username or password length.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (register_user(&database, msg->username, msg->password)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Registration successful", MAX_BUFFER - 1);
  } else {
    response.type = CMD_ERROR;
    strncpy(
        response.message,
        "Registration failed: Username already exists or invalid credentials",
        MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

/**
 * @brief Autentica um usuário com base nas credenciais fornecidas.
 * Verifica se o usuário já está logado e o adiciona ao gerenciador de clientes
 * se válido.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de login recebida.
 */
void handle_login(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  if (strlen(msg->username) < 3 || strlen(msg->username) >= MAX_USERNAME ||
      strlen(msg->password) < 4 || strlen(msg->password) >= MAX_PASSWORD) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid username or password length.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (find_client_by_username(&client_manager, msg->username)) {
    response.type = CMD_ERROR;
    strncpy(response.message, "User already logged in", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (authenticate_user(&database, msg->username, msg->password)) {
    User *user = add_client(&client_manager, msg->username, sockfd);

    if (user) {
      response.type = CMD_SUCCESS;
      strncpy(response.message, "Login successful", MAX_BUFFER - 1);
    } else {
      response.type = CMD_ERROR;
      strncpy(response.message, "Server full, try again later",
              MAX_BUFFER - 1);
    }
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid username or password", MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

/**
 * @brief Cria um novo grupo com nome, senha e criador, se o usuário estiver
 * autenticado. Responde com sucesso ou falha dependendo da existência ou
 * limite de grupos, e validade das credenciais.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de criação de grupo recebida.
 */
void handle_create_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  if (strlen(msg->groupname) < 3 || strlen(msg->groupname) >= MAX_GROUPNAME ||
      strlen(msg->password) < 4 || strlen(msg->password) >= MAX_PASSWORD) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid groupname or password length.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (create_group(&group_manager, msg->groupname, msg->password,
                   user->username)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Group created successfully", MAX_BUFFER - 1);
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message,
            "Failed to create group (name exists or server full)",
            MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

/**
 * @brief Permite que um usuário autenticado entre em um grupo existente.
 * Verifica a senha, remove de grupo anterior se necessário, e envia
 * notificação aos membros do novo grupo.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de entrada em grupo recebida.
 */
void handle_enter_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  if (strlen(msg->groupname) < 3 || strlen(msg->groupname) >= MAX_GROUPNAME ||
      strlen(msg->password) < 4 || strlen(msg->password) >= MAX_PASSWORD) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid groupname or password length.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (user->current_group[0] != '\0') {
    Group *old_group = find_group(&group_manager, user->current_group);
    if (old_group) {
      Message old_notification;
      memset(&old_notification, 0, sizeof(Message));
      old_notification.type = CMD_NOTIFICATION;
      snprintf(old_notification.message, MAX_BUFFER,
               "%s has left the group %s", user->username, old_group->name);
      broadcast_to_group(old_group, &old_notification, sockfd);
      leave_group(&group_manager, old_group, user);
    }
  }

  Group *group = find_group(&group_manager, msg->groupname);
  if (!group) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Group does not exist", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (!verify_group_password(group, msg->password)) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Incorrect group password", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (join_group(&group_manager, group, user)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Joined group successfully", MAX_BUFFER - 1);
    send_message(sockfd, &response);

    Message notification;
    memset(&notification, 0, sizeof(Message));
    notification.type = CMD_NOTIFICATION;
    snprintf(notification.message, MAX_BUFFER, "%s has joined the group",
             user->username);
    broadcast_to_group(group, &notification, sockfd);
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message, "Failed to join group (group full)",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
  }
}

/**
 * @brief Lida com a solicitação de um usuário para sair do seu grupo atual.
 * Envia notificação aos membros do grupo se o usuário sair com sucesso.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de saída de grupo recebida (não
 * utilizado diretamente).
 */
void handle_leave_group(int sockfd, const Message *msg)
{
  (void)msg;

  Message response;
  memset(&response, 0, sizeof(Message));

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (user->current_group[0] == '\0') {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not in any group", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Group *group = find_group(&group_manager, user->current_group);
  if (!group) {
    response.type = CMD_ERROR;
    strncpy(response.message,
            "Current group not found (might have been deleted)",
            MAX_BUFFER - 1);
    user->current_group[0] = '\0';
    send_message(sockfd, &response);
    return;
  }

  Message notification;
  memset(&notification, 0, sizeof(Message));
  notification.type = CMD_NOTIFICATION;
  snprintf(notification.message, MAX_BUFFER, "%s has left the group",
           user->username);
  broadcast_to_group(group, &notification, sockfd);

  if (leave_group(&group_manager, group, user)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Left group successfully", MAX_BUFFER - 1);
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message, "Failed to leave group", MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

/**
 * @brief Lida com a solicitação para deletar um grupo.
 * Somente o criador do grupo pode deletá-lo. Notifica os membros antes de
 * deletar.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de exclusão de grupo recebida.
 */
void handle_delete_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  if (strlen(msg->groupname) < 3 || strlen(msg->groupname) >= MAX_GROUPNAME) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid groupname length.", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Group *group = find_group(&group_manager, msg->groupname);
  if (!group) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Group does not exist", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (strcmp(group->creator, user->username) != 0) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Failed to delete group: not owner",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Message notification;
  memset(&notification, 0, sizeof(Message));
  notification.type = CMD_NOTIFICATION;
  snprintf(
      notification.message, MAX_BUFFER,
      "Group '%s' is being deleted by owner. You have been removed from it.",
      group->name);
  broadcast_to_group(group, &notification, -1);

  pthread_mutex_lock(&group->mutex);
  for (int i = 0; i < group->member_count; i++) {
    group->members[i]->current_group[0] = '\0';
  }
  group->member_count = 0;
  pthread_mutex_unlock(&group->mutex);

  if (delete_group(&group_manager, msg->groupname, user->username)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Group deleted successfully", MAX_BUFFER - 1);
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message,
            "Failed to delete group: an internal error occurred",
            MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

/**
 * @brief Encaminha uma mensagem de chat de um usuário autenticado para todos
 * os membros do grupo em que ele está.
 *
 * @param sockfd O descritor de arquivo do socket do remetente.
 * @param msg Um ponteiro para a mensagem de chat recebida (original).
 */
void handle_message(int sockfd, const Message *msg)
{
  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    Message response;
    memset(&response, 0, sizeof(Message));
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (strlen(msg->message) == 0 || strlen(msg->message) >= MAX_MESSAGE) {
    Message response;
    memset(&response, 0, sizeof(Message));
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid message length.", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (user->current_group[0] == '\0') {
    Message response;
    memset(&response, 0, sizeof(Message));
    response.type = CMD_ERROR;
    strncpy(response.message, "Not in any group", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Group *group = find_group(&group_manager, user->current_group);
  if (!group) {
    Message response;
    memset(&response, 0, sizeof(Message));
    response.type = CMD_ERROR;
    strncpy(
        response.message,
        "Your current group no longer exists. Please leave and join another.",
        MAX_BUFFER - 1);
    user->current_group[0] = '\0';
    send_message(sockfd, &response);
    return;
  }

  Message chat_msg;
  memset(&chat_msg, 0, sizeof(Message));
  chat_msg.type = CMD_MESSAGE;
  strncpy(chat_msg.username, user->username, MAX_USERNAME - 1);
  chat_msg.username[MAX_USERNAME - 1] = '\0';

  strncpy(chat_msg.message, msg->message, MAX_BUFFER - 1);
  chat_msg.message[MAX_BUFFER - 1] = '\0';

  broadcast_to_group(group, &chat_msg, sockfd);
}

/**
 * @brief Lida com uma mensagem direta (DM) enviada por um usuário para outro.
 * Verifica a autenticação e existência do destinatário antes de encaminhar.
 *
 * @param sockfd O descritor de arquivo do socket do remetente.
 * @param msg Um ponteiro para a mensagem direta recebida (original).
 */
void handle_direct_message(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  User *sender = find_client_by_sockfd(&client_manager, sockfd);
  if (!sender || !sender->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (strlen(msg->username) == 0 || strlen(msg->username) >= MAX_USERNAME ||
      strlen(msg->message) == 0 || strlen(msg->message) >= MAX_MESSAGE) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Invalid recipient username or message length.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (strcmp(sender->username, msg->username) == 0) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Cannot send direct message to yourself.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  User *recipient = find_client_by_username(&client_manager, msg->username);
  if (!recipient) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Recipient not found or not online.",
            MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Message dm_msg;
  memset(&dm_msg, 0, sizeof(Message));
  dm_msg.type = CMD_DIRECT_MESSAGE;
  strncpy(dm_msg.username, sender->username, MAX_USERNAME - 1);
  dm_msg.username[MAX_USERNAME - 1] = '\0';

  strncpy(dm_msg.message, msg->message, MAX_BUFFER - 1);
  dm_msg.message[MAX_BUFFER - 1] = '\0';

  send_message(recipient->sockfd, &dm_msg);

  response.type = CMD_SUCCESS;
  snprintf(response.message, MAX_BUFFER, "Direct message sent to %s",
           recipient->username);
  send_message(sockfd, &response);
}

/**
 * @brief Lida com a solicitação para listar todos os grupos de chat
 * disponíveis no servidor.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de listagem de grupos (não utilizado
 * diretamente).
 */
void handle_list_groups(int sockfd, const Message *msg)
{
  (void)msg;

  Message response;
  memset(&response, 0, sizeof(Message));

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  pthread_mutex_lock(&group_manager.mutex);
  if (group_manager.group_count == 0) {
    strncpy(response.message, "No groups available.", MAX_BUFFER - 1);
  } else {
    char group_list[MAX_BUFFER];
    int current_len =
        snprintf(group_list, sizeof(group_list),
                 "Available groups (%d):", group_manager.group_count);
    for (int i = 0; i < group_manager.group_count; i++) {
      current_len += snprintf(
          group_list + current_len, sizeof(group_list) - current_len,
          "\n- %s (Creator: %s, Members: %d/%d)", group_manager.groups[i].name,
          group_manager.groups[i].creator,
          group_manager.groups[i].member_count, MAX_CLIENTS);
      if (current_len >= sizeof(group_list) - 1) break;
    }
    strncpy(response.message, group_list, MAX_BUFFER - 1);
  }
  pthread_mutex_unlock(&group_manager.mutex);

  response.type = CMD_NOTIFICATION;
  send_message(sockfd, &response);
}

/**
 * @brief Lida com a solicitação para listar os membros do grupo atual do
 * usuário.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem de listagem de membros (não utilizado
 * diretamente).
 */
void handle_list_members(int sockfd, const Message *msg)
{
  (void)msg;

  Message response;
  memset(&response, 0, sizeof(Message));

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (user->current_group[0] == '\0') {
    response.type = CMD_ERROR;
    strncpy(response.message, "You are not in any group.", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Group *group = find_group(&group_manager, user->current_group);
  if (!group) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Your current group no longer exists.",
            MAX_BUFFER - 1);
    user->current_group[0] = '\0';
    send_message(sockfd, &response);
    return;
  }

  pthread_mutex_lock(&group->mutex);
  char member_list[MAX_BUFFER];
  int current_len = snprintf(member_list, sizeof(member_list),
                             "Members in '%s' (%d/%d):", group->name,
                             group->member_count, MAX_CLIENTS);
  for (int i = 0; i < group->member_count; i++) {
    current_len +=
        snprintf(member_list + current_len, sizeof(member_list) - current_len,
                 "\n- %s %s", group->members[i]->username,
                 (strcmp(group->members[i]->username, group->creator) == 0)
                     ? "(Creator)"
                     : "");
    if (current_len >= sizeof(member_list) - 1) break;
  }
  pthread_mutex_unlock(&group->mutex);

  strncpy(response.message, member_list, MAX_BUFFER - 1);
  response.type = CMD_NOTIFICATION;
  send_message(sockfd, &response);
}

/**
 * @brief Identifica o tipo de comando recebido do cliente e redireciona para a
 * função handler correspondente.
 *
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @param msg Um ponteiro para a mensagem recebida.
 */
void handle_client_message(int sockfd, const Message *msg)
{
  switch (msg->type) {
  case CMD_REGISTER:
    handle_register(sockfd, msg);
    break;
  case CMD_LOGIN:
    handle_login(sockfd, msg);
    break;
  case CMD_LOGOUT:
    remove_client(&client_manager, sockfd);
    break;
  case CMD_CREATE:
    handle_create_group(sockfd, msg);
    break;
  case CMD_ENTER:
    handle_enter_group(sockfd, msg);
    break;
  case CMD_LEAVE:
    handle_leave_group(sockfd, msg);
    break;
  case CMD_DELETE:
    handle_delete_group(sockfd, msg);
    break;
  case CMD_MESSAGE:
    handle_message(sockfd, msg);
    break;
  case CMD_DIRECT_MESSAGE:
    handle_direct_message(sockfd, msg);
    break;
  case CMD_LIST_GROUPS:
    handle_list_groups(sockfd, msg);
    break;
  case CMD_LIST_MEMBERS:
    handle_list_members(sockfd, msg);
    break;
  default:

    break;
  }
}

/**
 * @brief Loop da thread do cliente no servidor.
 * Recebe mensagens continuamente, trata comandos e limpa recursos na
 * desconexão do cliente.
 *
 * @param arg Um ponteiro para a estrutura ClientArgs contendo o socket do
 * cliente.
 * @return NULL ao finalizar.
 */
void *client_handler(void *arg)
{
  ClientArgs *client_args = (ClientArgs *)arg;
  int sockfd = client_args->sockfd;
  free(client_args);

  set_nonblocking(sockfd);

  while (1) {
    Message msg;
    int received = receive_message(sockfd, &msg);

    if (received < 0) {
      break;
    } else if (received == 0) {
      usleep(10000);
      continue;
    }

    handle_client_message(sockfd, &msg);
  }

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (user) {
    if (user->current_group[0] != '\0') {
      Group *group = find_group(&group_manager, user->current_group);
      if (group) {
        Message notification;
        memset(&notification, 0, sizeof(Message));
        notification.type = CMD_NOTIFICATION;
        snprintf(notification.message, MAX_BUFFER, "%s has disconnected",
                 user->username);
        broadcast_to_group(group, &notification, sockfd);
        leave_group(&group_manager, group, user);
      }
    }
    remove_client(&client_manager, sockfd);
  } else {
    printf("Client with socket %d disconnected.\n", sockfd);
  }

  close(sockfd);

  return NULL;
}
