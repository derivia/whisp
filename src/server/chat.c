#include "../../include/chat.h"
#include "../../include/common.h"
#include "../../include/network.h"
#include <time.h>

/**
 * @brief Inicializa o gerenciador de grupos, configurando a contagem de grupos
 * como zero e inicializando o mutex do gerenciador. Também inicializa os
 * mutexes de cada grupo.
 *
 * @param gm Ponteiro para a estrutura GroupManager a ser inicializada.
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

/**
 * @brief Inicializa o gerenciador de clientes, configurando a contagem de
 * clientes como zero e inicializando o mutex do gerenciador.
 *
 * @param cm Ponteiro para a estrutura ClientManager a ser inicializada.
 */
void init_client_manager(ClientManager *cm)
{
  cm->client_count = 0;
  pthread_mutex_init(&cm->mutex, NULL);
}

/**
 * @brief Cria um novo grupo de chat. O nome do grupo deve ter no mínimo três
 * caracteres. Garante a atomicidade da operação usando o mutex do
 * GroupManager.
 *
 * @param gm Ponteiro para o GroupManager.
 * @param name O nome do novo grupo.
 * @param password A senha do novo grupo.
 * @param creator O nome de usuário do criador do grupo.
 * @return true se o grupo for criado com sucesso, false caso contrário (nome
 * muito curto, grupo já existe, limite de grupos atingido).
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
 * @brief Remove um grupo existente. A remoção só é permitida se o usuário que
 * solicita for o criador do grupo. Reorganiza o array de grupos após a
 * remoção.
 *
 * @param gm Ponteiro para o GroupManager.
 * @param name O nome do grupo a ser deletado.
 * @param username O nome de usuário que está solicitando a exclusão.
 * @return true se o grupo for removido com sucesso, false caso contrário
 * (grupo não existe, usuário não é o criador).
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
 * Garante a atomicidade da busca usando o mutex do GroupManager.
 *
 * @param gm Ponteiro para o GroupManager.
 * @param name O nome do grupo a ser encontrado.
 * @return Um ponteiro para a estrutura Group se encontrado, NULL caso
 * contrário.
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
 * @brief Compara a senha fornecida com a senha armazenada do grupo.
 * Garante a atomicidade da verificação usando o mutex do grupo.
 *
 * @param group Ponteiro para a estrutura Group.
 * @param password A senha a ser verificada.
 * @return true se a senha estiver correta, false caso contrário ou se o grupo
 * for NULL.
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
 * @brief Lida com a tentativa de um usuário entrar em um grupo.
 * Verifica se o grupo existe, se o usuário já está no grupo, e se há espaço.
 * Adiciona o usuário ao grupo e atualiza seu grupo atual.
 *
 * @param gm Ponteiro para o GroupManager (não utilizado diretamente nesta
 * função, mas comum na assinatura).
 * @param group Ponteiro para a estrutura Group em que o usuário deseja entrar.
 * @param user Ponteiro para a estrutura User que está tentando entrar.
 * @return true se o usuário entrar no grupo com sucesso, false caso contrário.
 */
bool join_group(GroupManager *gm, Group *group, User *user)
{
  (void)gm;

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
 * @brief Lida com a tentativa de um usuário sair de um grupo.
 * Remove o usuário da lista de membros do grupo e limpa seu grupo atual.
 *
 * @param gm Ponteiro para o GroupManager (não utilizado diretamente nesta
 * função, mas comum na assinatura).
 * @param group Ponteiro para a estrutura Group do qual o usuário deseja sair.
 * @param user Ponteiro para a estrutura User que está tentando sair.
 * @return true se o usuário sair do grupo com sucesso, false caso contrário
 * (grupo não existe, usuário não está no grupo).
 */
bool leave_group(GroupManager *gm, Group *group, User *user)
{
  (void)gm;

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
 * @brief Adiciona um timestamp (tempo atual) à mensagem.
 *
 * @param msg Ponteiro para a estrutura Message.
 */
void add_timestamp_to_message(Message *msg)
{
  msg->timestamp = time(NULL);
}

/**
 * @brief Formata a mensagem com o tempo atual e o nome de usuário do
 * remetente. Esta função é usada para exibir mensagens de chat no servidor
 * antes de enviá-las.
 *
 * @param buffer O buffer de saída para a string formatada.
 * @param size O tamanho do buffer de saída.
 * @param msg Ponteiro para a estrutura Message.
 */
void format_message_with_time(char *buffer, size_t size, const Message *msg)
{
  struct tm *time_info = localtime(&msg->timestamp);
  char time_str[9];
  strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

  snprintf(buffer, size, "[%s] %s: %s", time_str, msg->username, msg->message);
}

/**
 * @brief Envia uma mensagem para todos os membros de um grupo, excluindo um
 * determinado socket. A mensagem original é modificada para incluir o
 * timestamp antes de ser enviada.
 *
 * @param group Ponteiro para a estrutura Group.
 * @param original_msg Ponteiro para a mensagem original a ser transmitida.
 * @param exclude_sockfd O descritor de arquivo do socket a ser excluído do
 * broadcast (geralmente o remetente).
 */
void broadcast_to_group(Group *group, const Message *original_msg,
                        int exclude_sockfd)
{
  if (!group) return;

  pthread_mutex_lock(&group->mutex);

  Message msg_with_time = *original_msg;
  add_timestamp_to_message(&msg_with_time);

  for (int i = 0; i < group->member_count; i++) {
    if (group->members[i]->sockfd != exclude_sockfd) {
      send_message(group->members[i]->sockfd, &msg_with_time);
    }
  }

  pthread_mutex_unlock(&group->mutex);
}

/**
 * @brief Adiciona um novo cliente autenticado ao gerenciador de clientes.
 * Atribui nome de usuário, socket, e inicializa o grupo atual como vazio.
 *
 * @param cm Ponteiro para o ClientManager.
 * @param username O nome de usuário do cliente.
 * @param sockfd O descritor de arquivo do socket do cliente.
 * @return Um ponteiro para a estrutura User recém-adicionada, ou NULL se o
 * limite de clientes for atingido.
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
 * @brief Remove um cliente do gerenciador de clientes com base no seu socket.
 * Reorganiza o array de clientes após a remoção.
 *
 * @param cm Ponteiro para o ClientManager.
 * @param sockfd O descritor de arquivo do socket do cliente a ser removido.
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
 * @brief Busca um cliente pelo seu descritor de arquivo de socket.
 * Garante a atomicidade da busca usando o mutex do ClientManager.
 *
 * @param cm Ponteiro para o ClientManager.
 * @param sockfd O descritor de arquivo do socket a ser buscado.
 * @return Um ponteiro para a estrutura User se encontrado, NULL caso
 * contrário.
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
 * @brief Busca um cliente pelo seu nome de usuário.
 * Garante a atomicidade da busca usando o mutex do ClientManager.
 *
 * @param cm Ponteiro para o ClientManager.
 * @param username O nome de usuário a ser buscado.
 * @return Um ponteiro para a estrutura User se encontrado, NULL caso
 * contrário.
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
