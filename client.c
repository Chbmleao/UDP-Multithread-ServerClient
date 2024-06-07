#include "utils.h"

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
  int serverPort = atoi(argv[3]); // Third arg: server port

  int choosedMovie = -1;

  while (choosedMovie != 0) {
    // Exibe o menu do cliente e aguarda a entrada do usuário
    printClientMenu();
    scanf("%d", &choosedMovie);

    if (choosedMovie == 0) break; // Se o usuário optar por sair, encerra o programa

    // Cria um socket usando UDP
    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
      exitWithError("ERROR opening socket");
    }

    // Constrói a estrutura de endereço do servidor
    union ServerAddress serverAddress = getServerAddressStructure(ipType, serverPort);

    // Envia uma mensagem para o servidor
    char message[MESSAGE_SIZE];
    sprintf(message, "%d", choosedMovie);
    memset(message, 0, MESSAGE_SIZE);
    int n = sendto(clientSocket, message, strlen(message), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (n < 0) {
      exitWithError("ERROR sending message to server");
    }

    int movieEnded = 0;
    while (!movieEnded) {
      // Recebe uma mensagem do servidor
      char buffer[MESSAGE_SIZE];
      memset(buffer, 0, MESSAGE_SIZE);
      n = recvfrom(clientSocket, buffer, MESSAGE_SIZE, 0, NULL, NULL);
      if (n < 0) {
        exitWithError("ERROR receiving message from server");
      }

      // Exibe a mensagem recebida
      printf("%s\n", buffer);

      // Verifica se a mensagem indica o fim do filme
      if (strcmp(buffer, "FIM") == 0) {
        movieEnded = 1;
      }
    }

    // Fecha o socket
    close(clientSocket);
  }

  return 0;
}