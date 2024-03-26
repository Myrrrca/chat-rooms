#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

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
    fprintf(stderr, "launch format: ./server <PORT> <MAXLIMIT>\n");
    return -1;
  }
  assert(atoi(argv[2]) >= -1);
  unsigned short int MYPORT = atoi(argv[1]);
  /* int TIMEOUT = -1; */
  /* if (atoi(argv[2]) >= 0) { */
  /*   TIMEOUT = atoi(argv[2]) * 1000; */
  /* } */

  int listener_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  /* removing 'adress already in use' error on our port */
  int yes = 1;
  Setsockopt(listener_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  struct sockaddr_in addr = {
    AF_INET,
    Htons(MYPORT),
    0
  };
  /* letting kernel fill our ip address by itself */
  addr.sin_addr.s_addr = INADDR_ANY;
  printf("server IP: %s\nserver port: %d (host bytes order of %d)\n", inet_ntoa(addr.sin_addr), MYPORT, addr.sin_port);

  Bind(listener_sockfd, (struct sockaddr* )&addr, sizeof(addr));

  Listen(listener_sockfd, 10);

  struct sockaddr_in client_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);

  int client_sockfd = -1;
  /* printf("server_sockfd = %d\n", listener_sockfd); */

  int epoll_fd = epoll_create1(0);

  struct epoll_event events = { .events = EPOLLIN, .data.fd = listener_sockfd }; 
  
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_sockfd, &events); 
  int fdcount = 0;
  int clients_sockfds[1024] = {0};  // do dynamic arrays you lazy ass

  for ( ; ;) {
    int res = epoll_wait(epoll_fd, &events, 1, -1);   
    /* printf("res = %d\n", res); */
    int recieved_sockfd = events.data.fd;
  
    if (recieved_sockfd == listener_sockfd) {
      client_sockfd = accept_connection(listener_sockfd, &client_addr, &sin_size);
      struct epoll_event client_events = { .events = EPOLLIN, .data.fd = client_sockfd };
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &client_events);
      ++fdcount;
      clients_sockfds[fdcount - 1] = client_sockfd; 
      /* printf("we added client socket %d\n", clients_sockfds[fdcount - 1]); */
    }
    else {
      char buff[2048] = {0};
      int nbytes = recv(recieved_sockfd, &buff, 2048, 0); 
      if (nbytes == 0) {
        close(events.data.fd);
        --fdcount;
        printf("client socket %d hang up\n", recieved_sockfd);
        continue;
      }
      printf("%s\n", buff);

      for (int j = 0; j < fdcount; ++j) {
        int fd = clients_sockfds[j];
        if (fd != recieved_sockfd) {
          /* printf("doing send for %d socket\n", fd); */
          Send(fd, buff, sizeof(buff), 0);
        }
      }
    } 
  }

  return 0;
}
