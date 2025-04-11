# Whisp 🌐

*Um servidor e cliente de chat em tempo real, minimalista e thread-safe, escrito em C99.*

## Descrição

Whisp é um sistema de chat em grupo para redes locais onde os usuários podem:

- Registrar/login.
- Criar, entrar, sair ou deletar grupos de chat.
- Enviar mensagens em tempo real.

Construído com C99, pthreads e SQLite3 para simplicidade e desempenho.

## Funcionalidades

- Sistema de Autenticação
    - Armazenamento de senhas.
    - Garantia de unicidade de nomes de usuário.

- Chats em Grupo
    - Criar/deletar grupos (privilégios do criador).
    - Entrada/saída dinâmica com notificações por broadcast.

- Concorrência
    - Modelo de uma thread por cliente.
    - Dados compartilhados protegidos por mutex (grupos, lista de usuários).

- Sem Dependências
    - Puro C99 e SQLite3 (embutido).

## Uso

### 1. Compilar

```sh
# Clonar e compilar (requer SQLite3)
git clone https://github.com/derivia/whisp
cd whisp
make           # Compila servidor + cliente
```

### 2. Servidor

```sh
./whisp_server  # Escuta na porta 6969 por padrão
```

### 3. Clientes

```sh
./whisp_client <ip_servidor> [porta] # Conecta ao servidor
```

Comandos do cliente:
- `register <usuario> <senha>`
- `login <usuario> <senha>`
- `create <grupo> <senha>`
- `enter <grupo> <senha>`
- `leave <grupo>`
- `delete <grupo>`

## Projeto Técnico

### Servidor

- Pool de Threads: Gerencia clientes simultaneamente.
- Mutexes: Protegem `groups_db` e `active_users`.
- SQLite: Armazena pares `(username, password)`.

### Cliente

- I/O não bloqueante (threads).
- Reconexão automática em caso de falha.

## Limitações

- Sem histórico de mensagens (chats efêmeros).
- Sem criptografia (por enquanto; apenas rede local).

## Licença

[MIT](./LICENSE)
