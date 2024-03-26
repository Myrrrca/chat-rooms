#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include "errproc.h"

int accept_connection(int server_sockfd, struct sockaddr_in* client_addr, socklen_t* sin_size) 
{
  int client_sockfd = Accept(server_sockfd, (struct sockaddr_in* )client_addr, sin_size);
  printf("got connection from %s\n", inet_ntoa(client_addr->sin_addr));
  return client_sockfd;
}

int main(int argc, char** argv)
{
  if (argc != 4) {
    fprintf(stderr, "launch format: ./server <PORT> <TIMEOUT> <MAX_CLIENTS>\n");
    return -1;
  }
  assert(atoi(argv[2]) >= 0);
  assert(atoi(argv[3]) >= 2);
  unsigned short int MYPORT = atoi(argv[1]);
  int TIMEOUT = atoi(argv[2]) * 1000;
  int MAX_CLIENTS = atoi(argv[3]);

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

  int client_sockfd = -1;
  printf("server_sockfd = %d\n", server_sockfd);

  struct pollfd fds[MAX_CLIENTS];
  memset(fds, 0, sizeof(fds));
  /* struct pollfd fds[1]; */
  fds[0].fd = server_sockfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  int fdcount = 1;
  int res;

  for ( ; ;) {
    if (atoi(argv[2]) == 0) {
      res = poll(fds, MAX_CLIENTS, -1);
    }
    else {
      res = poll(fds, MAX_CLIENTS, TIMEOUT);
    }

    /* printf("res = %d\n", res); */


    for (int i = 0; i < fdcount; ++i) {
      if (fds[i].revents & POLLIN) {
        printf("something changed!\n");
        if (i == 0) {
          client_sockfd = accept_connection(server_sockfd, &client_addr, &sin_size);
          fds[fdcount].fd = client_sockfd;
          fds[fdcount].events = POLLIN;
          fds[fdcount].revents = 0;
          ++fdcount;
          printf("we added client_sockfd(%d) to fds!\n", client_sockfd);
        }
        else {
          printf("we found modified %d fd and its not server_fd!\n", i);
          /* starting recv and send */
          char buf[512];
          int nbytes = recv(fds[i].fd, &buf, sizeof(buf), 0);
          if (nbytes <= 0) {
            if (nbytes == 0) {
              printf("client socket %d (%s IP) hang up\n", fds[i].fd, inet_ntoa(client_addr.sin_addr));
            }
            else {
              perror("recv failed");
            }
            close(fds[i].fd);
            memset(&fds[i], 0, sizeof(fds[0]));
            --fdcount;
            break;
          }
          printf("%s\n", buf);
          for (int j = 1; j < fdcount + 1; ++j) {
            if (getpeername(fds[j].fd, (struct sockaddr* ) &client_addr, &sin_size) != -1 && fds[j].fd != fds[i].fd) {
              printf("starting send for %d socket\n", fds[j].fd);
              Send(fds[j].fd, &buf, sizeof(buf), 0);
            }
          }
        }
      }
      else {
        if (res == 0 && fds[i].fd != server_sockfd) {
          if (getpeername(fds[i].fd, (struct sockaddr* ) &client_addr, &sin_size) != -1 && fds[i].fd != server_sockfd) {
            printf("timed out for %d socket\n", fds[i].fd);
            close(fds[i].fd);
            memset(&fds[i], 0, sizeof(fds[0]));
            --fdcount;
          }
        }
      }
    }

  }

  return 0;
}
