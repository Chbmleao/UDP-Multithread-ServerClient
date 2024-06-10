#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define MESSAGE_SIZE 256
#define NUM_MOVIES 3
#define NUM_PHRASES 5

struct clientInfo {
  int socket;
  int choice; // Opção escolhida pelo cliente
  int lastPhrase; // Última frase exibida
  struct sockaddr_storage sockaddr;
};

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

void* clientHandler(void *arg) {
  struct clientInfo *client = (struct clientInfo *)arg; // Cast the argument back to the struct type

  if (client->choice < 0 || client->choice > NUM_MOVIES-1) {
    printf("Opção inválida\n");
    return 0;
  }

  client->lastPhrase = 0;

  while (client->lastPhrase < NUM_PHRASES) {
    char buffer[MESSAGE_SIZE];
    strcpy(buffer, phrases[client->choice][client->lastPhrase]);

    ssize_t numBytesSent = sendto(client->socket, buffer, strlen(buffer), 0, (struct sockaddr*) &(client->sockaddr), sizeof(client->sockaddr));
    if (numBytesSent < 0) {
      perror("sendto() failed");
    }

    client->lastPhrase++;

    sleep(3);
  }

  char endMessage[] = "END";

  ssize_t numBytesSent = sendto(client->socket, endMessage, strlen(endMessage), 0, (struct sockaddr*) &(client->sockaddr), sizeof(client->sockaddr));
  if (numBytesSent < 0) {
    perror("sendto() failed");
  }
  
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    perror("Usage: ./server <ip-type> <port>");
  }

  char *ipType = argv[1];
  char *serverPort = argv[2];

  // Construct the server address structure
  struct addrinfo addrCriteria; // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = (strcmp(ipType, "ipv6") == 0) ? AF_INET6 : AF_INET;
  addrCriteria.ai_flags = AI_PASSIVE; // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM; // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP; // Only UDP socket

  struct addrinfo *serverAddr; // List of server addresses
  int rtnVal = getaddrinfo(NULL, serverPort, &addrCriteria, &serverAddr);  
  if (rtnVal != 0) {
    perror("getaddrinfo() failed");
  }

  // Create socket for incoming connections
  int serverSock = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);
  if (serverSock < 0) {
    perror("socket() failed");
  }

  // Bind to the local address
  if (bind(serverSock, serverAddr->ai_addr, serverAddr->ai_addrlen) < 0) {
    perror("bind() failed");
  }

  // Free address list allocated by getaddrinfo()
  freeaddrinfo(serverAddr);

  while (1) {
    printf("Aguardando conexão...\n");
    struct clientInfo *client = malloc(sizeof(struct clientInfo));
    if (client == NULL) {
      perror("malloc() failed");
      continue;
    }

    // Set length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(client->sockaddr);

    char buffer[MESSAGE_SIZE]; // I/O buffer
    ssize_t numBytesRcvd = recvfrom(serverSock, buffer, MESSAGE_SIZE, 0, (struct sockaddr *) &(client->sockaddr), &clntAddrLen);
    if (numBytesRcvd < 0) {
      free(client);
      perror("recvfrom() failed");
    }
    
    client->socket = serverSock;
    client->choice = atoi(buffer) - 1;

    pthread_t thread;
    int ret = pthread_create(&thread, NULL, clientHandler, (void *)client);
    if (ret != 0) {
      perror("pthread_create() failed");
      free(client);
      continue;
    }
  }

  return 0;
}