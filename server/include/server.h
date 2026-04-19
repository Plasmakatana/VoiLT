#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include "log.h"
#include "common.h"

typedef struct {
  int server_fd;
  struct sockaddr_in server_addr;
  int is_running;
  int is_worker;
  int epoll_fd;
  struct epoll_event events[MAX_EVENTS];
} server_t;

int server_init(server_t *server,int port);
int server_init_worker(server_t *server, int server_fd);
int server_start(server_t *server);
void server_stop(server_t *server);
void server_cleanup(server_t *server);
int server_run(server_t *server);
#endif
