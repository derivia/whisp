#include "../../include/chat.h"
#include "../../include/common.h"
#include "../../include/network.h"
#include <time.h>

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

bool verify_group_password(Group *group, const char *password)
{
  if (!group) return false;

  pthread_mutex_lock(&group->mutex);
  bool result = (strcmp(group->password, password) == 0);
  pthread_mutex_unlock(&group->mutex);

  return result;
}

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

void add_timestamp_to_message(Message *msg)
{
  msg->timestamp = time(NULL);
}

void format_message_with_time(char *buffer, size_t size, const Message *msg)
{
  struct tm *time_info = localtime(&msg->timestamp);
  char time_str[9];
  strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

  snprintf(buffer, size, "[%s]: %s", time_str, msg->message);
}

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
