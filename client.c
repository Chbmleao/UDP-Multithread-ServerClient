#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

void printClientMenu() {
  printf("0 - Sair\n");
  printf("1 - Senhor dos anéis\n");
  printf("2 - O Poderoso Chefão\n");
  printf("3 - Clube da Luta\n");
}

int main(int argc, char *argv[]) {
  if (argc != 4) { // Test for correct number of arguments
    exitWithError("Usage: ./client <ip-type> <server-ip> <server-port>");
  }  

  int ipType = (strcmp(argv[1], "ipv6") == 0) ? IPV6 : IPV4;; // First arg: ip type
  char *serverIP = argv[2]; // Second arg: server IP address
  char *serverPort = argv[3]; // Third arg: server port

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria; // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC; // Any address family
  addrCriteria.ai_socktype = SOCK_DGRAM; // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP; // Only UDP socket

  // Get address(es)
  struct addrinfo *servAddr; // Holder for returned list of server addrs
  int rtnVal = getaddrinfo(serverIP, serverPort, &addrCriteria, &servAddr);
  if (rtnVal != 0) {
    exitWithError("getaddrinfo() failed");
  }

  int choosedMovie = -1;

  while (choosedMovie != 0) {
    // Exibe o menu do cliente e aguarda a entrada do usuário
    printClientMenu();
    scanf("%d", &choosedMovie);

    if (choosedMovie == 0) break; // Se o usuário optar por sair, encerra o programa

    // Cria um socket usando UDP
    int clientSocket = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol); // Socket descriptor for client
    if (clientSocket < 0) {
      exitWithError("ERROR opening socket");
    }

    // Envia a escolha do filme para o servidor
    char echoString[MESSAGE_SIZE];
    sprintf(echoString, "%d", choosedMovie);
    ssize_t numBytesSent = sendto(clientSocket, echoString, strlen(echoString), 0, servAddr->ai_addr, servAddr->ai_addrlen);
    if (numBytesSent < 0) {
      exitWithError("ERROR sending message to server");
    } else if ((size_t)numBytesSent != strlen(echoString)) {
      exitWithError("ERROR sending unexpected number of bytes");
    }

    printf("numBytesSent: %ld\n", numBytesSent);
    printf("message: %s\n", echoString);

    int movieEnded = 0;
    while (!movieEnded) {
      ssize_t numBytesRcvd = 0; // Bytes received in single recv()
      // Receive responses
      struct sockaddr_storage fromAddr; // Source address of server
      // Set length of from address structure (in-out parameter)
      socklen_t fromAddrLen = sizeof(fromAddr);

      // Recebe uma mensagem do servidor
      char buffer[MESSAGE_SIZE]; // I/O buffer
      memset(buffer, 0, MESSAGE_SIZE);
      numBytesRcvd = recvfrom(clientSocket, buffer, MESSAGE_SIZE, 0, (struct sockaddr *) &fromAddr, &fromAddrLen);
      if (numBytesRcvd < 0) {
        exitWithError("ERROR receiving message from server");
      }

      // Verifica se a mensagem indica o fim do filme
      if (strcmp(buffer, "FIM") == 0) {
        movieEnded = 1;
        break;
      }

      // Exibe a mensagem recebida
      printf("%s\n", buffer);
    }

    // Fecha o socket
    close(clientSocket);
  }

  return 0;
}