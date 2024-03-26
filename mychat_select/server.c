#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "errproc.h"

int accept_connection(int server_sockfd, struct sockaddr_in* client_addr, socklen_t* sin_size) 
{
  int client_sockfd = Accept(server_sockfd, (struct sockaddr_in* )client_addr, sin_size);
  printf("got connection from %s\n", inet_ntoa(client_addr->sin_addr));
  return client_sockfd;
}

int main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr, "launch format: ./server <PORT> <TIMEOUT>\n");
    return -1;
  }
  unsigned short int MYPORT = atoi(argv[1]);

  int server_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  /* removing 'adress already in use' error on our port */
  int yes = 1;
  Setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  struct sockaddr_in addr = {
    AF_INET,
    Htons(MYPORT),
    0
  };
  addr.sin_addr.s_addr = INADDR_ANY;
  printf("server IP: %s\nserver port: %d (host bytes order of %d)\n", inet_ntoa(addr.sin_addr), MYPORT, addr.sin_port);

  Bind(server_sockfd, (struct sockaddr* )&addr, sizeof(addr));

  Listen(server_sockfd, 10);

  struct sockaddr_in client_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);

  fd_set to_monitor;
  fd_set ready_to_read_fds;
  FD_ZERO(&to_monitor);
  FD_ZERO(&ready_to_read_fds);
  FD_SET(server_sockfd, &to_monitor);

  int fdmax = server_sockfd;
  int client_sockfd = -1;
  printf("server_sockfd = %d\n", server_sockfd);



  for ( ; ;) {
    struct timeval tv = {
      atoi(argv[2])
    };

    ready_to_read_fds = to_monitor;
    int res;
    if (atoi(argv[2]) == 0) {
      res = Select(fdmax + 1, &ready_to_read_fds, NULL, NULL, NULL);
    }
    else {
      res = Select(fdmax + 1, &ready_to_read_fds, NULL, NULL, &tv);
    }

    for (int i = 0; i <= fdmax; ++i) {
      if (FD_ISSET(i, &ready_to_read_fds)) {
        if (i == server_sockfd) {
          client_sockfd = accept_connection(server_sockfd, &client_addr, &sin_size);
          FD_SET(client_sockfd, &to_monitor);
          if (client_sockfd > fdmax) {
            fdmax = client_sockfd;
          }
          printf("we added client_sockfd(%d) to to_monitor!\n", client_sockfd);
        }
        else {
          printf("we found %d fd and its not server_fd!\n", i);
          /* starting recv and send */
          char buf[512];
          int nbytes = recv(i, &buf, sizeof(buf), 0);
          if (nbytes <= 0) {
            if (nbytes == 0) {
              printf("client socket %d (%s IP) hang up\n", i, inet_ntoa(client_addr.sin_addr));
            }
            else {
              perror("recv failed");
            }
            FD_CLR(i, &to_monitor);
            close(i);
            break;
          }
          printf("%s\n", buf);
          for (int j = 3; j < fdmax + 1; ++j) {
            if (getpeername(j, (struct sockaddr* ) &client_addr, &sin_size) != -1 && j != server_sockfd && j != i) {
              printf("starting send for %d socket\n", j);
              Send(j, &buf, sizeof(buf), 0);
            }
          }
        }
      }
      else {
        if (res == 0) {
          if (getpeername(i, (struct sockaddr* ) &client_addr, &sin_size) != -1 && i != server_sockfd) {
            printf("timed out for %d socket\n", i);
            FD_CLR(i, &to_monitor);
            close(i);
          }
        }
      }
    }
  }

  return 0;
}
