#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "errproc.h"

int main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr, "launch format: ./client <SERVER_IP> <PORT>\n");
    return -1;
  }
  unsigned short int SERVER_PORT = atoi(argv[2]);

  int client_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {
    AF_INET,
    Htons(SERVER_PORT),
    0
  };
  Inet_aton(argv[1], &(addr.sin_addr));
  struct sockaddr_in server_addr;

  Connect(client_sockfd, (struct sockaddr* )&addr, sizeof(addr));
  printf("connected to: %s on port %d (host bytes order of %d)\n", inet_ntoa(addr.sin_addr), SERVER_PORT, addr.sin_port);

  fd_set to_monitor;
  fd_set ready_to_read_fds;
  FD_ZERO(&to_monitor);
  FD_ZERO(&ready_to_read_fds);
  int fdmax = client_sockfd;
  FD_SET(0, &to_monitor);
  FD_SET(client_sockfd, &to_monitor);

  for (; ;) {
    char buf[512] = {0};
    ready_to_read_fds = to_monitor; 
    Select(fdmax + 1, &ready_to_read_fds, NULL, NULL, NULL);

    if (FD_ISSET(0, &ready_to_read_fds)) {
      fgets(buf, sizeof(buf), stdin);
      buf[strcspn(buf, "\n")] = '\0';
      Send(client_sockfd, &buf, sizeof(buf), 0);
    }
    else if (FD_ISSET(client_sockfd, &ready_to_read_fds)) {
      recv(client_sockfd, &buf, sizeof(buf), 0);
      printf("%s\n", buf);
    }

  }

  return 0;
}
