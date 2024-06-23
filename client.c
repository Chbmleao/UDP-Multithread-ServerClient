#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

#define MESSAGE_SIZE 256

void printClientMenu() {
  printf("\n");
  printf("0 - Sair\n");
  printf("1 - Senhor dos anéis\n");
  printf("2 - O Poderoso Chefão\n");
  printf("3 - Clube da Luta\n");
}

void printErrorAndExit(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 4) { // Teste para verificar se o número de argumentos é válido
    printErrorAndExit("Usage: ./client <ip-type> <server-ip> <server-port>");
  }  

  char *ipType = argv[1]; // Primeiro arg: tipo de IP
  char *serverIP = argv[2]; // Segundo arg: server IP
  char *serverPort = argv[3]; // Terceiro arg: server port

  // Definição dos critérios para o endereço
  struct addrinfo addrCriteria; // Critérios para o endereço
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zera a estrutura
  addrCriteria.ai_family = (strcmp(ipType, "ipv6") == 0) ? AF_INET6 : AF_INET;
  addrCriteria.ai_socktype = SOCK_DGRAM; // Apenas sockets de datagrama (UDP)
  addrCriteria.ai_protocol = IPPROTO_UDP; // Apenas sockets UDP

  // Obtem a lista de endereços do servidor
  struct addrinfo *servAddr; // Ponteiro para armazenar a lista de endereços retornados
  /*
    getaddrinfo() obtém uma lista de estruturas de endereços de acordo com os critérios fornecidos
      serverIP: endereço IP do servidor
      serverPort: porta do servidor
      &addrCriteria: critérios para seleção do endereço
      &servAddr: ponteiro para armazenar a lista de endereços
  */
  int rtnVal = getaddrinfo(serverIP, serverPort, &addrCriteria, &servAddr);
  if (rtnVal != 0) {
    printErrorAndExit("getaddrinfo() failed");
  }

  int choosedMovie = -1;

  while (choosedMovie != 0) {
    // Exibe o menu do cliente e aguarda a entrada do usuário
    printClientMenu();
    scanf("%d", &choosedMovie);

    if (choosedMovie == 0) break; // Se o usuário optar por sair, encerra o programa

    /*
      socket() cria um socket e retorna um descritor de arquivo que se refere ao socket
        servAddr->ai_family: família de protocolos (AF_INET para IPv4 ou AF_INET6 para IPv6)
        servAddr->ai_socktype: tipo de socket (SOCK_DGRAM para datagrama)
        servAddr->ai_protocol: protocolo a ser usado (IPPROTO_UDP para UDP)
    */
    int clientSocket = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol); // Socket descriptor for client
    if (clientSocket < 0) {
      printErrorAndExit("ERROR opening socket");
    }

    // Envia a escolha do filme para o servidor
    char echoString[MESSAGE_SIZE];
    sprintf(echoString, "%d", choosedMovie);

    /*
      sendto() envia uma mensagem para o servidor
        clientSocket: descritor do socket
        echoString: mensagem a ser enviada
        strlen(echoString): tamanho da mensagem
        0: flags
        servAddr->ai_addr: endereço do servidor
        servAddr->ai_addrlen: tamanho do endereço
    */
    ssize_t numBytesSent = sendto(clientSocket, echoString, strlen(echoString), 0, servAddr->ai_addr, servAddr->ai_addrlen);
    if (numBytesSent < 0) {
      printErrorAndExit("ERROR sending message to server");
    } else if ((size_t)numBytesSent != strlen(echoString)) {
      printErrorAndExit("ERROR sending unexpected number of bytes");
    }

    int movieEnded = 0;
    while (!movieEnded) {
      ssize_t numBytesRcvd = 0; // Bytes recebidos do servidor
      struct sockaddr_storage fromAddr; // Endereço origem do servidor
      // Tamanho da estrutura de endereço 
      socklen_t fromAddrLen = sizeof(fromAddr);

      // Recebe uma mensagem do servidor
      char buffer[MESSAGE_SIZE]; // I/O buffer
      memset(buffer, 0, MESSAGE_SIZE);
      /*
        recvfrom() recebe uma mensagem do servidor
          clientSocket: descritor do socket
          buffer: buffer para armazenar a mensagem
          MESSAGE_SIZE: tamanho do buffer
          0: flags
          (struct sockaddr *) &fromAddr: endereço do servidor
          &fromAddrLen: tamanho do endereço
      */
      numBytesRcvd = recvfrom(clientSocket, buffer, MESSAGE_SIZE, 0, (struct sockaddr *) &fromAddr, &fromAddrLen);
      if (numBytesRcvd < 0) {
        printErrorAndExit("ERROR receiving message from server");
      }

      // Verifica se a mensagem indica o fim do filme
      if (strcmp(buffer, "END") == 0) {
        movieEnded = 1;
        break;
      }

      // Exibe a mensagem recebida
      printf("%s\n", buffer);
    }

    /*
      close() fecha o socket
        clientSocket: descritor do socket
    */
    close(clientSocket);
  }

  return 0;
}