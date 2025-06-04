#include "../../include/common.h"
#include "../../include/handlers.h"
#include "../../include/network.h"

volatile bool running = true;
int client_fd = -1;
pthread_t receive_thread;
void *receive_handler(void *arg);

// Declaração das funções para enviar comandos ao servidor
void send_register_command(const char *username, const char *password);
void send_login_command(const char *username, const char *password);
void send_logout_command();
void send_create_group_command(const char *groupname, const char *password);
void send_enter_group_command(const char *groupname, const char *password);
void send_leave_group_command();
void send_delete_group_command(const char *groupname);
void send_chat_message(const char *message);
void send_direct_message(const char *recipient, const char *message);
void send_list_groups_command();
void send_list_members_command();
void print_help();
void parse_command(char *input, CommandHandlers *handlers);

/**
 * @brief Handler de encerramento do cliente. Envia logout, fecha o socket e
 * encerra o loop principal.
 */
void handle_exit()
{
  if (!running) return;
  running = false;
  send_logout_command();
  shutdown(client_fd, SHUT_RDWR);
  close(client_fd);
  printf("\nDisconnected.\n");
}

/**
 * @brief Captura o sinal SIGINT (Ctrl+C) e delega a finalização para
 * handle_exit.
 *
 * @param sig O número do sinal recebido (não utilizado, mas necessário para a
 * assinatura).
 */
void sigint_handler(int sig)
{
  (void)sig;
  handle_exit();
}

/**
 * @brief Estabelece conexão com o servidor, configura sinais, inicia a thread
 * de recepção de mensagens e processa comandos da entrada padrão (stdin).
 *
 * @param argc Número de argumentos da linha de comando.
 * @param argv Array de strings dos argumentos da linha de comando.
 * @return 0 em caso de sucesso, 1 em caso de erro.
 */
int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <server_ip> [port]\n", argv[0]);
    return 1;
  }

  const char *server_ip = argv[1];
  int port = DEFAULT_PORT;
  if (argc > 2) port = atoi(argv[2]);

  signal(SIGPIPE, SIG_IGN);

  signal(SIGINT, sigint_handler);

  printf("Connecting to %s:%d...\n", server_ip, port);

  client_fd = connect_to_server(server_ip, port);
  printf("Connected! Enter '/help' for available commands.\n");

  CommandHandlers handlers = {.register_cmd = send_register_command,
                              .login_cmd = send_login_command,
                              .create_group_cmd = send_create_group_command,
                              .enter_group_cmd = send_enter_group_command,
                              .leave_group_cmd = send_leave_group_command,
                              .delete_group_cmd = send_delete_group_command,
                              .exit_cmd = handle_exit,
                              .chat_message_cmd = send_chat_message,
                              .direct_message_cmd = send_direct_message,
                              .list_groups_cmd = send_list_groups_command,
                              .list_members_cmd = send_list_members_command};

  pthread_create(&receive_thread, NULL, receive_handler, &client_fd);

  char input[MAX_BUFFER];
  printf("> ");
  fflush(stdout);

  while (running) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};

    int activity = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
    if (activity < 0 && errno != EINTR) {
      perror("select error");
      break;
    }

    if (!running) break;

    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      if (fgets(input, MAX_BUFFER, stdin) == NULL) break;
      input[strcspn(input, "\n")] = '\0';
      if (strlen(input) > 0) {
        parse_command(input, &handlers);
      }
      printf("> ");
      fflush(stdout);
    }
  }

  pthread_join(receive_thread, NULL);
  return 0;
}
