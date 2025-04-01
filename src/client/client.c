#include "../../include/common.h"
#include "../../include/network.h"

volatile bool running = true;
int client_fd = -1;

void *receive_handler(void *arg);

void send_register_command(const char *username, const char *password);
void send_login_command(const char *username, const char *password);
void send_logout_command();
void send_create_group_command(const char *groupname);
void send_enter_group_command(const char *groupname);
void send_leave_group_command();
void send_delete_group_command(const char *groupname);
void send_chat_message(const char *message);

void print_help();

typedef struct {
  void (*register_cmd)(const char *, const char *);
  void (*login_cmd)(const char *, const char *);
  void (*create_group_cmd)(const char *);
  void (*enter_group_cmd)(const char *);
  void (*leave_group_cmd)(void);
  void (*delete_group_cmd)(const char *);
  void (*exit_cmd)(void);
  void (*chat_message_cmd)(const char *);
} CommandHandlers;

void parse_command(char *input, CommandHandlers *handlers);

void handle_exit()
{
  printf("Exiting...\n");
  send_logout_command();
  running = false;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <server_ip> [port]\n", argv[0]);
    return 1;
  }

  const char *server_ip = argv[1];
  int port = DEFAULT_PORT;

  if (argc > 2) {
    port = atoi(argv[2]);
  }

  signal(SIGPIPE, SIG_IGN);

  printf("Connecting to %s:%d...\n", server_ip, port);

  client_fd = connect_to_server(server_ip, port);
  printf("Connected! Type 'help' for available commands.\n");

  CommandHandlers handlers = {.register_cmd = send_register_command,
                              .login_cmd = send_login_command,
                              .create_group_cmd = send_create_group_command,
                              .enter_group_cmd = send_enter_group_command,
                              .leave_group_cmd = send_leave_group_command,
                              .delete_group_cmd = send_delete_group_command,
                              .exit_cmd = handle_exit,
                              .chat_message_cmd = send_chat_message};

  pthread_t receive_thread;
  pthread_create(&receive_thread, NULL, receive_handler, &client_fd);

  char input[MAX_BUFFER];

  while (running) {
    printf("> ");
    fflush(stdout);

    if (fgets(input, MAX_BUFFER, stdin) == NULL) {
      break;
    }

    input[strcspn(input, "\n")] = '\0';

    if (strlen(input) == 0) {
      continue;
    }

    parse_command(input, &handlers);
  }

  pthread_join(receive_thread, NULL);
  close(client_fd);

  return 0;
}
