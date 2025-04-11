#include "../../include/auth.h"
#include "../../include/chat.h"
#include "../../include/db.h"
#include "../../include/network.h"

extern ClientManager client_manager;
extern GroupManager group_manager;
extern Database database;

typedef struct {
  int sockfd;
} ClientArgs;

void handle_register(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

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

void handle_login(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

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

void handle_create_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

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
    strncpy(response.message, "Failed to create group", MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

void handle_enter_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

  User *user = find_client_by_sockfd(&client_manager, sockfd);
  if (!user || !user->authenticated) {
    response.type = CMD_ERROR;
    strncpy(response.message, "Not authenticated", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  if (user->current_group[0] != '\0') {
    Group *old_group = find_group(&group_manager, user->current_group);
    leave_group(&group_manager, old_group, user);
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
    strncpy(response.message, "Failed to join group", MAX_BUFFER - 1);
    send_message(sockfd, &response);
  }
}

void handle_leave_group(int sockfd, const Message *msg)
{
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

void handle_delete_group(int sockfd, const Message *msg)
{
  Message response;
  memset(&response, 0, sizeof(Message));

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

  Message notification;
  memset(&notification, 0, sizeof(Message));
  notification.type = CMD_NOTIFICATION;
  snprintf(notification.message, MAX_BUFFER,
           "Group '%s' is being deleted by owner", group->name);
  broadcast_to_group(group, &notification, -1);

  if (delete_group(&group_manager, msg->groupname, user->username)) {
    response.type = CMD_SUCCESS;
    strncpy(response.message, "Group deleted successfully", MAX_BUFFER - 1);
  } else {
    response.type = CMD_ERROR;
    strncpy(response.message, "Failed to delete group: not owner",
            MAX_BUFFER - 1);
  }

  send_message(sockfd, &response);
}

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

  if (user->current_group[0] == '\0') {
    Message response;
    memset(&response, 0, sizeof(Message));
    response.type = CMD_ERROR;
    strncpy(response.message, "Not in any group", MAX_BUFFER - 1);
    send_message(sockfd, &response);
    return;
  }

  Group *group = find_group(&group_manager, user->current_group);
  Message chat_msg;
  memset(&chat_msg, 0, sizeof(Message));
  chat_msg.type = CMD_MESSAGE;
  strncpy(chat_msg.username, user->username, MAX_USERNAME - 1);
  strncpy(chat_msg.message, msg->message, MAX_BUFFER - 1);

  broadcast_to_group(group, &chat_msg, sockfd);
}

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
  default:
    break;
  }
}

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
  if (user && user->current_group[0] != '\0') {
    Group *group = find_group(&group_manager, user->current_group);

    Message notification;
    memset(&notification, 0, sizeof(Message));
    notification.type = CMD_NOTIFICATION;
    snprintf(notification.message, MAX_BUFFER, "%s has disconnected",
             user->username);
    broadcast_to_group(group, &notification, sockfd);

    leave_group(&group_manager, group, user);
  }

  remove_client(&client_manager, sockfd);
  close(sockfd);

  return NULL;
}
