#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define MESSAGE_SIZE 256 // Tamanho máximo das mensagens
#define NUM_MOVIES 3 // Número de filmes disponíveis
#define NUM_PHRASES 5 // Número de frases por filme

struct clientInfo {
  int socket; // Socket do cliente
  int choice; // Opção escolhida pelo cliente
  int lastPhrase; // Última frase exibida
  struct sockaddr_storage sockaddr; // Endereço do cliente
};

// Variáveis globais
int numConnectedClients = 0;
pthread_mutex_t clientCountMutex = PTHREAD_MUTEX_INITIALIZER;
const char *phrases[NUM_MOVIES][NUM_PHRASES] = {
  { // Senhor dos Anéis
    "Um anel para a todos governar",
    "Na terra de Mordor onde as sombras se deitam",
    "Não é o que temos, mas o que fazemos com o que temos",
    "Não há mal que sempre dure",
    "O mundo está mudando, senhor Frodo"
  },
  { // O Poderoso Chefão
    "Vou fazer uma oferta que ele não pode recusar",
    "Mantenha seus amigos por perto e seus inimigos mais perto ainda",
    "É melhor ser temido que amado",
    "A vingança é um prato que se come frio",
    "Nunca deixe que ninguém saiba o que você está pensando"
  },
  { // Clube da Luta
    "Primeira regra do Clube da Luta: você não fala sobre o Clube da Luta",
    "Segunda regra do Clube da Luta: você não fala sobre o Clube da Luta",
    "O que você possui acabará possuindo você",
    "É apenas depois de perder tudo que somos livres para fazer qualquer coisa",
    "Escolha suas lutas com sabedoria"
  }
};

// Função para lidar com a comunicação com cada cliente
void* clientHandler(void *arg) {
  struct clientInfo *client = (struct clientInfo *)arg; // Cast the argument back to the struct type

  if (client->choice < 0 || client->choice > NUM_MOVIES-1) {
    printf("Opção inválida\n");
    return 0;
  }

  // Incrementa o contador de clientes conectados
  pthread_mutex_lock(&clientCountMutex); // Trava o mutex para proteção da variável compartilhada
  numConnectedClients++;
  pthread_mutex_unlock(&clientCountMutex); // Libera o mutex

  client->lastPhrase = 0;

  while (client->lastPhrase < NUM_PHRASES) {
    char buffer[MESSAGE_SIZE];
    strcpy(buffer, phrases[client->choice][client->lastPhrase]);

    /*
      Envia a frase para o cliente
      sendto() envia a mensagem para o cliente usando o socket associado
        client->socket: descritor de socket usado para enviar dados
        buffer: mensagem a ser enviada
        strlen(buffer): tamanho da mensagem
        0: flags (não utilizadas)
        (struct sockaddr*) &(client->sockaddr): endereço do cliente
        sizeof(client->sockaddr): tamanho do endereço do cliente
    */ 
    ssize_t numBytesSent = sendto(client->socket, buffer, strlen(buffer), 0, (struct sockaddr*) &(client->sockaddr), sizeof(client->sockaddr));
    if (numBytesSent < 0) {
      printErrorAndExit("sendto() failed");
    }

    client->lastPhrase++;

    sleep(3);
  }

 // Envia a mensagem de término ao cliente
  char endMessage[] = "END";
  ssize_t numBytesSent = sendto(client->socket, endMessage, strlen(endMessage), 0, (struct sockaddr*) &(client->sockaddr), sizeof(client->sockaddr));
  if (numBytesSent < 0) {
    printErrorAndExit("sendto() failed");
  }

  // Decrementa o contador de clientes conectados
  pthread_mutex_lock(&clientCountMutex); // Trava o mutex para proteção da variável compartilhada
  numConnectedClients--;
  pthread_mutex_unlock(&clientCountMutex); // Libera o mutex
  
  return 0;
}

// Função para imprimir uma mensagem de erro e encerrar o programa
void printErrorAndExit(char *message) {
  perror(message);
  exit(1);
}

// Função para imprimir o número de clientes conectados a cada 4 segundos
void* printNumConnectedClients() {
  while (1) {
    pthread_mutex_lock(&clientCountMutex);
    int count = numConnectedClients;
    pthread_mutex_unlock(&clientCountMutex);

    printf("Clientes: %d\n", count);
    sleep(4);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printErrorAndExit("Usage: ./server <ip-type> <port>");
  }

  char *ipType = argv[1];
  char *serverPort = argv[2];

  // Definição dos critérios para o endereço
  struct addrinfo addrCriteria; // Critérios para o endereço
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zera a estrutura
  addrCriteria.ai_family = (strcmp(ipType, "ipv6") == 0) ? AF_INET6 : AF_INET;
  addrCriteria.ai_flags = AI_PASSIVE; // Aceita conexões em qualquer endereço/porta
  addrCriteria.ai_socktype = SOCK_DGRAM; // Apenas sockets de datagrama (UDP)
  addrCriteria.ai_protocol = IPPROTO_UDP; // Apenas sockets UDP

  struct addrinfo *serverAddr; // Ponteiro para armazenar a lista de endereços retornados
  /*
    getaddrinfo() obtém uma lista de estruturas de endereços de acordo com os critérios fornecidos
      NULL: especifica que o endereço IP é preenchido pelo sistema (localhost)
      serverPort: porta do servidor
      &addrCriteria: critérios para seleção do endereço
      &serverAddr: ponteiro para armazenar a lista de endereços
  */
  int rtnVal = getaddrinfo(NULL, serverPort, &addrCriteria, &serverAddr);  
  if (rtnVal != 0) {
    printErrorAndExit("getaddrinfo() failed");
  }

  /*
    Cria um socket para o servidor
    socket() cria um socket e retorna um descritor de arquivo
      serverAddr->ai_family: família de protocolos (AF_INET ou AF_INET6)
      serverAddr->ai_socktype: tipo de socket (SOCK_DGRAM)
      serverAddr->ai_protocol: protocolo a ser usado (IPPROTO_UDP)
  */
  int serverSock = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);
  if (serverSock < 0) {
    printErrorAndExit("socket() failed");
  }

  /*
    Associa o socket a um endereço
    bind() associa o socket a um endereço local
      serverSock: descritor de socket
      serverAddr->ai_addr: ponteiro para a estrutura de endereço
      serverAddr->ai_addrlen: tamanho da estrutura de endereço
  */
  if (bind(serverSock, serverAddr->ai_addr, serverAddr->ai_addrlen) < 0) {
    printErrorAndExit("bind() failed");
  }

  // Libera a memória alocada para a lista de endereços
  freeaddrinfo(serverAddr);

  // Cria uma thread para imprimir o número de clientes conectados
  pthread_t printThread;
  int ret = pthread_create(&printThread, NULL, printNumConnectedClients, NULL);
  if (ret != 0) {
    printErrorAndExit("pthread_create() failed");
  }

  while (1) {
    struct clientInfo *client = malloc(sizeof(struct clientInfo));
    if (client == NULL) {
      printErrorAndExit("malloc() failed");
      continue;
    }

    // Recebe a escolha do filme do cliente
    socklen_t clntAddrLen = sizeof(client->sockaddr);

    char buffer[MESSAGE_SIZE]; // I/O buffer
    ssize_t numBytesRcvd = recvfrom(serverSock, buffer, MESSAGE_SIZE, 0, (struct sockaddr *) &(client->sockaddr), &clntAddrLen);
    if (numBytesRcvd < 0) {
      free(client);
      printErrorAndExit("recvfrom() failed");
    }
    
    client->socket = serverSock;
    client->choice = atoi(buffer) - 1;

    // Cria uma thread para lidar com a comunicação com o cliente
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, clientHandler, (void *)client);
    if (ret != 0) {
      printErrorAndExit("pthread_create() failed");
      free(client);
      continue;
    }
  }

  return 0;
}