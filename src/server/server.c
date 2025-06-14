#include "../../include/chat.h"
#include "../../include/common.h"
#include "../../include/db.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/select.h>

ClientManager client_manager;
GroupManager group_manager;
Database database;

void *client_handler(void *arg);

typedef struct {
  int sockfd;
} ClientArgs;

/**
 * @brief Uma variável "booleana" para marcar se o servidor está rodando ou
 * não, atômica e volátil para ser alterada apenas por signals.
 */
volatile sig_atomic_t server_running = 1;

/**
 * @brief Handler de sinal para capturar SIGINT (Ctrl+C) e sinalizar o
 * encerramento do servidor.
 *
 * @param sig O número do sinal recebido.
 */
void handle_signal(int sig)
{
  (void)sig;
  server_running = 0;
  printf("\n[SERVER] Shutting down...\n");
}

/**
 * @brief Busca pelo IP da rede local e o retorna.
 * Iterates through network interfaces to find a non-loopback IPv4 address.
 *
 * @return Uma string com o IP local atual, ou NULL em caso de falha.
 */
char *get_local_ip()
{
  struct ifaddrs *ifaddr, *ifa;
  static char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return NULL;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL) continue;

    if (ifa->ifa_addr->sa_family == AF_INET &&
        strcmp(ifa->ifa_name, "lo") != 0) {
      getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                  NULL, 0, NI_NUMERICHOST);
      break;
    }
  }

  freeifaddrs(ifaddr);
  return host;
}

/**
 * @brief Configura um socket TCP para escuta em um IP e porta específicos.
 *
 * @param port A porta em que o servidor irá escutar.
 * @param ip_addr O endereço IP no qual o servidor irá se ligar.
 * @return O descritor de arquivo para o socket do servidor.
 */
int setup_server_with_ip(int port, const char *ip_addr)
{
  int server_fd;
  struct sockaddr_in address;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if (inet_pton(AF_INET, ip_addr, &address.sin_addr) <= 0) {
    perror("Invalid address/Address not supported");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

/**
 * @brief Função principal do servidor Whisp.
 * Inicializa o banco de dados, gerenciadores, configura o servidor e entra no
 * loop de aceitação de clientes.
 *
 * @param argc Número de argumentos da linha de comando.
 * @param argv Array de strings dos argumentos da linha de comando.
 * @return 0 em caso de sucesso, 1 em caso de falha.
 */
int main(int argc, char *argv[])
{
  int port = DEFAULT_PORT;

  if (argc > 1) port = atoi(argv[1]);

  signal(SIGPIPE, SIG_IGN);

  signal(SIGINT, handle_signal);

  if (!init_database(&database, "whisp.db")) {
    fprintf(stderr, "Failed to initialize database\n");
    return 1;
  }

  init_client_manager(&client_manager);
  init_group_manager(&group_manager);

  char *local_ip = get_local_ip();
  if (!local_ip || strlen(local_ip) == 0) {
    fprintf(stderr, "Failed to get local IP address\n");

    local_ip = "127.0.0.1";
    printf("[SERVER] Using fallback IP: %s\n", local_ip);
  }

  int server_fd = setup_server_with_ip(port, local_ip);
  printf("Whisp server started on %s:%d\n", local_ip, port);

  fd_set read_fds;
  struct timeval timeout;

  while (server_running) {
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int activity = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (activity < 0 && errno != EINTR) {
      perror("select error");
      break;
    }

    if (!server_running) break;

    if (FD_ISSET(server_fd, &read_fds)) {
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(client_addr);

      int client_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
      if (client_fd < 0) {
        perror("accept failed");
        continue;
      }

      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
      printf("New connection from %s\n", client_ip);

      ClientArgs *client_args = malloc(sizeof(ClientArgs));
      if (client_args == NULL) {
        perror("Failed to allocate memory for client_args");
        close(client_fd);
        continue;
      }
      client_args->sockfd = client_fd;

      pthread_t thread_id;

      if (pthread_create(&thread_id, NULL, client_handler, client_args) != 0) {
        perror("Failed to create thread for client");
        close(client_fd);
        free(client_args);
        continue;
      }

      pthread_detach(thread_id);
    }
  }

  close(server_fd);
  close_database(&database);
  printf("[SERVER] Shutdown complete.\n");
  return 0;
}
