#include "../../include/chat.h"
#include "../../include/common.h"
#include "../../include/network.h"
#include <time.h>

/**
 * @brief Inicializa o gerenciamento dos grupos, cada grupo iniciando com zero
 ombros e tendo seus mutexes inicializados.
 *
 * @param gm GroupManager
 */
void init_group_manager(GroupManager *gm)
{
  gm->group_count = 0;
  pthread_mutex_init(&gm->mutex, NULL);

  for (int i = 0; i < MAX_GROUPS; i++) {
    gm->groups[i].member_count = 0;
    pthread_mutex_init(&gm->groups[i].mutex, NULL);
  }
}

void init_client_manager(ClientManager *cm)
{
  cm->client_count = 0;
  pthread_mutex_init(&cm->mutex, NULL);
}

/**
 * @brief Cria um grupo, com no mínimo três caracteres no nome, uma senha e um
 * criador.
 *
 * Basicamente cuida do estado do mutex do group manager.
 *
 * @param gm
 * @param name
 * @param password
 * @param creator
 * @return
 */
bool create_group(GroupManager *gm, const char *name, const char *password,
                  const char *creator)
{
  if (strlen(name) < 3) return false;

  pthread_mutex_lock(&gm->mutex);

  for (int i = 0; i < gm->group_count; i++) {
    if (strcmp(gm->groups[i].name, name) == 0) {
      pthread_mutex_unlock(&gm->mutex);
      return false;
    }
  }

  if (gm->group_count >= MAX_GROUPS) {
    pthread_mutex_unlock(&gm->mutex);
    return false;
  }

  Group *new_group = &gm->groups[gm->group_count++];
  strncpy(new_group->name, name, MAX_GROUPNAME - 1);
  new_group->name[MAX_GROUPNAME - 1] = '\0';

  strncpy(new_group->password, password, MAX_PASSWORD - 1);
  new_group->password[MAX_PASSWORD - 1] = '\0';

  strncpy(new_group->creator, creator, MAX_USERNAME - 1);
  new_group->creator[MAX_USERNAME - 1] = '\0';

  new_group->member_count = 0;

  pthread_mutex_unlock(&gm->mutex);
  return true;
}

/**
 * @brief Remove um grupo caso o usuário que solicitou seja o criador.
 * Faz verificação do nome e da autoria. Se o grupo for deletado, realoca
 * os grupos seguintes no array.
 *
 * @param gm Gerenciador de grupos
 * @param name Nome do grupo
 * @param username Nome do usuário solicitante
 * @return true se removido com sucesso, false caso contrário
 */
bool delete_group(GroupManager *gm, const char *name, const char *username)
{
  pthread_mutex_lock(&gm->mutex);

  for (int i = 0; i < gm->group_count; i++) {
    if (strcmp(gm->groups[i].name, name) == 0) {
      if (strcmp(gm->groups[i].creator, username) != 0) {
        pthread_mutex_unlock(&gm->mutex);
        return false;
      }

      for (int j = i; j < gm->group_count - 1; j++) {
        memcpy(&gm->groups[j], &gm->groups[j + 1], sizeof(Group));
      }

      gm->group_count--;
      pthread_mutex_unlock(&gm->mutex);
      return true;
    }
  }

  pthread_mutex_unlock(&gm->mutex);
  return false;
}

/**
 * @brief Busca linearmente por um grupo na lista de grupos existentes.
 *
 * @param gm
 * @param name
 * @return Group se encontrado, NULL se não.
 */
Group *find_group(GroupManager *gm, const char *name)
{
  pthread_mutex_lock(&gm->mutex);

  for (int i = 0; i < gm->group_count; i++) {
    if (strcmp(gm->groups[i].name, name) == 0) {
      pthread_mutex_unlock(&gm->mutex);
      return &gm->groups[i];
    }
  }

  pthread_mutex_unlock(&gm->mutex);
  return NULL;
}

/**
 * @brief Compara a senha do grupo com a senha que o usuário tentou para
 * entrar.
 *
 * @param group
 * @param password
 */
bool verify_group_password(Group *group, const char *password)
{
  if (!group) return false;

  pthread_mutex_lock(&group->mutex);
  bool result = (strcmp(group->password, password) == 0);
  pthread_mutex_unlock(&group->mutex);

  return result;
}

/**
 * @brief Lida com a tentativa de entrar num grupo.
 * Se o grupo não existir, retorna false.
 * Se o usuário já está no grupo, retorna true.
 * Se o grupo já estiver com o máximo de usuários, retorna false.
 * Altera o grupo atual do usuário que tentou entrar caso ele entre.
 * @param gm
 * @param group
 * @param user
 * @return
 */
bool join_group(GroupManager *gm, Group *group, User *user)
{
  if (!group) return false;

  pthread_mutex_lock(&group->mutex);

  for (int i = 0; i < group->member_count; i++) {
    if (group->members[i] == user) {
      pthread_mutex_unlock(&group->mutex);
      return true;
    }
  }

  if (group->member_count >= MAX_CLIENTS) {
    pthread_mutex_unlock(&group->mutex);
    return false;
  }

  group->members[group->member_count++] = user;
  strncpy(user->current_group, group->name, MAX_GROUPNAME - 1);
  user->current_group[MAX_GROUPNAME - 1] = '\0';

  pthread_mutex_unlock(&group->mutex);
  return true;
}

/**
 * @brief Lida com a tentativa de sair de um grupo.
 * Verifica se o grupo existe, se o usuário está no grupo, move os usuários
 * que não são o usuário atual para trás, mantendo o array inteiro.
 *
 * @param gm
 * @param group
 * @param user
 * @return
 */
bool leave_group(GroupManager *gm, Group *group, User *user)
{
  if (!group) return false;

  pthread_mutex_lock(&group->mutex);

  int user_index = -1;
  for (int i = 0; i < group->member_count; i++) {
    if (group->members[i] == user) {
      user_index = i;
      break;
    }
  }

  if (user_index == -1) {
    pthread_mutex_unlock(&group->mutex);
    return false;
  }

  for (int i = user_index; i < group->member_count - 1; i++) {
    group->members[i] = group->members[i + 1];
  }

  group->member_count--;
  user->current_group[0] = '\0';

  pthread_mutex_unlock(&group->mutex);
  return true;
}

/**
 * @brief Adiciona um tipo `time` (tempo atual) à mensagem.
 *
 * @param msg
 */
void add_timestamp_to_message(Message *msg)
{
  msg->timestamp = time(NULL);
}

/**
 * @brief Formata a mensagem com o termo atual.
 *
 * @param buffer
 * @param size
 * @param msg
 */
void format_message_with_time(char *buffer, size_t size, const Message *msg)
{
  struct tm *time_info = localtime(&msg->timestamp);
  char time_str[9];
  strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

  snprintf(buffer, size, "[%s]: %s", time_str, msg->message);
}

/**
 * @brief Repete a mensagem recebida por um usuário para todos os usuários que
 * não são o usuário que enviou a mensagem em um grupo.
 *
 * Utiliza `send_message` para cada cliente, ignorando o `exclude_sockfd`.
 * @param group
 * @param original_msg
 * @param exclude_sockfd
 */
void broadcast_to_group(Group *group, const Message *original_msg,
                        int exclude_sockfd)
{
  if (!group) return;

  pthread_mutex_lock(&group->mutex);

  Message msg_with_time = *original_msg;
  add_timestamp_to_message(&msg_with_time);

  char formatted_msg[MAX_MESSAGE];
  format_message_with_time(formatted_msg, sizeof(formatted_msg),
                           &msg_with_time);

  strncpy(msg_with_time.message, formatted_msg, MAX_CONTENT_SIZE - 1);
  msg_with_time.message[MAX_CONTENT_SIZE - 1] = '\0';

  for (int i = 0; i < group->member_count; i++) {
    if (group->members[i]->sockfd != exclude_sockfd) {
      send_message(group->members[i]->sockfd, &msg_with_time);
    }
  }

  pthread_mutex_unlock(&group->mutex);
}

/**
 * @brief Adiciona um novo cliente autenticado ao gerenciador.
 *
 * Atribui nome, socket e zera `current_group`.
 *
 * @param cm Gerenciador de clientes
 * @param username Nome do cliente
 * @param sockfd Socket do cliente
 * @return Ponteiro para o novo usuário, ou NULL se cheio
 */
User *add_client(ClientManager *cm, const char *username, int sockfd)
{
  pthread_mutex_lock(&cm->mutex);

  if (cm->client_count >= MAX_CLIENTS) {
    pthread_mutex_unlock(&cm->mutex);
    return NULL;
  }

  User *user = &cm->clients[cm->client_count++];
  strncpy(user->username, username, MAX_USERNAME - 1);
  user->username[MAX_USERNAME - 1] = '\0';
  user->sockfd = sockfd;
  user->authenticated = true;
  user->current_group[0] = '\0';

  pthread_mutex_unlock(&cm->mutex);
  return user;
}

/**
 * @brief Remove um cliente com base no socket.
 * Realoca os clientes seguintes no array.
 *
 * @param cm Gerenciador de clientes
 * @param sockfd Socket do cliente a ser removido
 */
void remove_client(ClientManager *cm, int sockfd)
{
  pthread_mutex_lock(&cm->mutex);

  int client_index = -1;
  for (int i = 0; i < cm->client_count; i++) {
    if (cm->clients[i].sockfd == sockfd) {
      client_index = i;
      break;
    }
  }

  if (client_index != -1) {
    for (int i = client_index; i < cm->client_count - 1; i++) {
      memcpy(&cm->clients[i], &cm->clients[i + 1], sizeof(User));
    }
    cm->client_count--;
  }

  pthread_mutex_unlock(&cm->mutex);
}

/**
 * @brief Busca um cliente pelo socket.
 *
 * @param cm Gerenciador de clientes
 * @param sockfd Socket a ser buscado
 * @return Ponteiro para o usuário, ou NULL se não encontrado
 */
User *find_client_by_sockfd(ClientManager *cm, int sockfd)
{
  pthread_mutex_lock(&cm->mutex);

  for (int i = 0; i < cm->client_count; i++) {
    if (cm->clients[i].sockfd == sockfd) {
      pthread_mutex_unlock(&cm->mutex);
      return &cm->clients[i];
    }
  }

  pthread_mutex_unlock(&cm->mutex);
  return NULL;
}

/**
 * @brief Busca um cliente pelo nome de usuário.
 *
 * @param cm Gerenciador de clientes
 * @param username Nome de usuário a ser buscado
 * @return Ponteiro para o usuário, ou NULL se não encontrado
 */
User *find_client_by_username(ClientManager *cm, const char *username)
{
  pthread_mutex_lock(&cm->mutex);

  for (int i = 0; i < cm->client_count; i++) {
    if (strcmp(cm->clients[i].username, username) == 0) {
      pthread_mutex_unlock(&cm->mutex);
      return &cm->clients[i];
    }
  }

  pthread_mutex_unlock(&cm->mutex);
  return NULL;
}
