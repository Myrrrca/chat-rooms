#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
/* #include <poll.h> */

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

  /* struct pollfd fds[2] = { */
  /*   { */
  /*     0, */
  /*     POLLIN, */
  /*     0 */
  /*   }, */
  /*   { */
  /*     client_sockfd, */
  /*     POLLIN, */
  /*     0 */
  /*   } */
  /* }; */
  int epoll_fd = epoll_create1(0);

  struct epoll_event events = { .events = EPOLLIN, .data.fd = client_sockfd};
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &events);
  struct epoll_event events2 = { .events = EPOLLIN, .data.fd = 0};
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &events2);

  char buf[2048] = {0};

  for (; ;) {
    /* printf("aboba\n"); */
    int res = epoll_wait(epoll_fd, &events, 1, -1);
    int recieved_sockfd = events.data.fd;
    
    if (recieved_sockfd == 0) {
      fgets(buf, sizeof(buf), stdin);
      buf[strcspn(buf, "\n")] = '\0';
      Send(client_sockfd, buf, sizeof(buf), 0);
    }
    else if (recieved_sockfd == client_sockfd) {
      int nbytes = recv(client_sockfd, buf, sizeof(buf), 0);

    
      if (nbytes == 0) {
        printf("server disconnected you\n");
        close(client_sockfd);
        break;
      }
      printf("%s\n", buf);
    }















    /* poll(fds, 2, -1); */
    /**/
    /* if (fds[0].revents & POLLIN) { */
    /*   fgets(buf, sizeof(buf), stdin); */
    /*   buf[strcspn(buf, "\n")] = '\0'; */
    /*   Send(client_sockfd, buf, sizeof(buf), 0); */
    /* } */
    /* else if (fds[1].revents & POLLIN) { */
    /*   int nbytes = recv(client_sockfd, buf, sizeof(buf), 0); */
    /*   printf("nbytes = %d\n", nbytes); */
    /*   printf("%s\n", buf); */
    /*   if (nbytes == 0) { */
    /*     printf("server disconnected you\n"); */
    /*     close(client_sockfd); */
    /*     break; */
    /*   } */
    /* } */
  }

  return 0;
}
