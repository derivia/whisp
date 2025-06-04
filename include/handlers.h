#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

typedef void (*RegisterLoginFunc)(const char *, const char *);
typedef void (*GroupNameFunc)(const char *);
typedef void (*SimpleFunc)(void);
typedef void (*MessageFunc)(const char *);
typedef void (*GroupCommandWithPassword)(const char *, const char *);
typedef void (*DirectMessageFunc)(const char *, const char *);

/* A struct abaixo serve para agrupar as funções acima (comportamentos do
 * strategy pattern). O `parse_command` serve como dispatcher que não se
 * importa com a implementação de cada comando.
 *
 * Ele vai receber uma struct de handlers com a real implementação, que pode
 * ser implementada de qualquer forma, desde que siga a assinatura das funções
 * aqui.
 *
 * É, basicamente, uma forma de passar uma espécie de "interface" para o
 * comando.
 *
 * Em outro arquivo, funções reais são associadas a cada membro de
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
  DirectMessageFunc direct_message_cmd;
  SimpleFunc list_groups_cmd;
  SimpleFunc list_members_cmd;
} CommandHandlers;

#endif
