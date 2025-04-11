#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

typedef void (*RegisterLoginFunc)(const char *, const char *);
typedef void (*GroupNameFunc)(const char *);
typedef void (*SimpleFunc)(void);
typedef void (*MessageFunc)(const char *);
typedef void (*GroupCommandWithPassword)(const char *, const char *);

/* A struct abaixo serve para agrupar as funções acima (comportamentos do
 * strategy pattern). O `parse_command` serve como dispatcher que não se
 * importa com a implementação de cada comando.
 *
 * Ele vai receber uma struct de handlers com a real implementação, que eu
 * posso implementar de qualquer forma, basta ter a assinatura das funções
 * daqui.
 *
 * É, basicamente, uma forma de passar uma espécie de "interface" para o
 * comando.
 *
 * Em outro arquivo, funções reais são associadas a cada uma de
 * CommandHandlers.
 */
typedef struct {
  RegisterLoginFunc register_cmd;
  RegisterLoginFunc login_cmd;
  SimpleFunc leave_group_cmd;
  GroupNameFunc delete_group_cmd;
  SimpleFunc exit_cmd;
  MessageFunc chat_message_cmd;
  GroupCommandWithPassword create_group_cmd;
  GroupCommandWithPassword enter_group_cmd;
} CommandHandlers;

#endif
