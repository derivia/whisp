#ifndef WHISP_CHAT_H
#define WHISP_CHAT_H

#include "auth.h"
#include "common.h"

typedef struct {
  char name[MAX_GROUPNAME];
  char creator[MAX_USERNAME];
  char password[MAX_PASSWORD];
  User *members[MAX_CLIENTS];
  int member_count;
  pthread_mutex_t mutex;
} Group;

typedef struct {
  Group groups[MAX_GROUPS];
  int group_count;
  pthread_mutex_t mutex;
} GroupManager;

typedef struct {
  User clients[MAX_CLIENTS];
  int client_count;
  pthread_mutex_t mutex;
} ClientManager;

void init_group_manager(GroupManager *gm);
void init_client_manager(ClientManager *cm);
bool create_group(GroupManager *gm, const char *name, const char *password,
                  const char *creator);
bool delete_group(GroupManager *gm, const char *name, const char *username);
Group *find_group(GroupManager *gm, const char *name);
bool join_group(GroupManager *gm, Group *group, User *user);
bool leave_group(GroupManager *gm, Group *group, User *user);
void broadcast_to_group(Group *group, const Message *msg, int exclude_sockfd);

bool verify_group_password(Group *group, const char *password);
User *add_client(ClientManager *cm, const char *username, int sockfd);
void remove_client(ClientManager *cm, int sockfd);
User *find_client_by_sockfd(ClientManager *cm, int sockfd);
User *find_client_by_username(ClientManager *cm, const char *username);

#endif
