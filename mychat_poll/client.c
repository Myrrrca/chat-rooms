#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

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

  struct pollfd fds[2] = {
    {
      0,
      POLLIN,
      0
    },
    {
      client_sockfd,
      POLLIN,
      0
    }
  };
  char buf[512] = {0};

  for (; ;) {
    poll(fds, 2, -1);

    if (fds[0].revents & POLLIN) {
      fgets(buf, sizeof(buf), stdin);
      buf[strcspn(buf, "\n")] = '\0';
      Send(client_sockfd, &buf, sizeof(buf), 0);
    }
    else if (fds[1].revents & POLLIN) {
      int nbytes = recv(client_sockfd, &buf, sizeof(buf), 0);
      printf("%s\n", buf);
      if (nbytes == 0) {
        printf("server disconnected you\n");
        close(client_sockfd);
        break;
      }
    }
  }

  return 0;
}
