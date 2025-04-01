#ifndef WHISP_NETWORK_H
#define WHISP_NETWORK_H

#include "common.h"

int create_socket(void);
int connect_to_server(const char *address, int port);
int setup_server(int port);
void send_message(int sockfd, const Message *msg);
int receive_message(int sockfd, Message *msg);

#endif
