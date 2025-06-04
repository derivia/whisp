# Whisp 🌐

*Um servidor e cliente de chat em tempo real, minimalista e thread-safe, escrito em C99.*

## Descrição

Whisp é um sistema de chat em grupo e mensagens diretas para redes locais onde os usuários podem:

- Registrar/login.
- Criar, entrar, sair ou deletar grupos de chat.
- Enviar mensagens em tempo real para grupos.
- Enviar mensagens diretas para outros usuários.
- Listar grupos disponíveis e membros de grupos.

## Funcionalidades

- Sistema de Autenticação
    - Armazenamento de senhas com hash SHA256.
    - Garantia de unicidade de nomes de usuário.

- Chats em Grupo
    - Criar/deletar grupos.
    - Entrada/saída dinâmica com notificações por broadcast.
    - Listagem de grupos disponíveis.
    - Listagem de membros do grupo atual.

- Mensagens Diretas
    - Envie mensagens privadas para usuários online.

- Concorrência
    - Modelo de uma thread por cliente.
    - Dados compartilhados protegidos por mutex (grupos, lista de usuários).

- Segurança e Validações
    - Proteção contra SQL Injection para operações no banco de dados.
    - Validações de entrada de strings no cliente e no servidor.

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
./whisp_server # Escuta na porta 6969 por padrão
```

### 3. Clientes

```sh
./whisp_client <ip_servidor> [porta] # Conecta ao servidor
```

#### Comandos do cliente:

- `register <usuario> <senha>`
- `login <usuario> <senha>`
- `create <grupo> <senha>`
- `enter <grupo> <senha>`
- `leave`
- `delete <grupo>`
- `dm <destinatario> <mensagem>`
- `listgroups`
- `who`
- `help`
- `exit`

## Projeto Técnico

### Servidor

- Pool de Threads: Gerencia clientes simultaneamente.
- Mutexes: Protegem dados compartilhados como o gerenciamento de grupos e a lista de usuários ativos.
- SQLite: Armazena pares `(username, password)` de forma segura com hash.
- Tratamento de Desconexão: Detecta automaticamente a desconexão de clientes e a queda do servidor.

### Cliente

- I/O Não Bloqueante: Utiliza threads para receber mensagens em segundo plano.
- Detecção de Desconexão: O cliente agora detecta quando o servidor fecha a conexão e encerra graciosamente.

## Limitações

- Sem histórico de mensagens persistente (chats efêmeros).
- Sem criptografia de ponta a ponta (as mensagens são visíveis no servidor).
- Sem funcionalidades administrativas avançadas (ex: banir usuários).

## Licença

[MIT](./LICENSE)
