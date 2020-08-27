#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define QLEN 5
#define BUFSIZE 4096
#define THREADS_MAX 4000

void *client_handler(void *arg) {
  int ssock = *(int *)arg;
  int cc;
  char buf[BUFSIZE];

  //   printf("A client has arrived for echoes.\n");
  //   fflush(stdout);
  /* start working for this guy */
  /* ECHO what the client says */
  for (;;) {
    if ((cc = read(ssock, buf, BUFSIZE)) <= 0) {
      //   printf("The client has gone.\n");
      close(ssock);
      pthread_exit(NULL);
    } else {
      buf[cc] = '\0';
      //   printf("The client says: %s\n", buf);
      if (write(ssock, buf, cc) < 0) {
        /* This guy is dead */
        close(ssock);
        pthread_exit(NULL);
      }
    }
  }
}

int passivesock(char *service, char *protocol, int qlen, int *rport);

/*
**	This poor server ... only serves one client at a time
*/
int main(int argc, char *argv[]) {
  char *service;
  int msock;
  int rport = 0;
  socklen_t alen;
  struct sockaddr_in fsin;
  int ssock;
  pthread_t thread;

  switch (argc) {
  case 1:
    // No args? let the OS choose a port and tell the user
    rport = 1;
    break;
  case 2:
    // User provides a port? then use it
    service = argv[1];
    break;
  default:
    fprintf(stderr, "usage: server [port]\n");
    exit(-1);
  }

  msock = passivesock(service, "tcp", QLEN, &rport);
  if (rport) {
    //	Tell the user the selected port
    printf("server: port %d\n", rport);
    fflush(stdout);
  }
  for (;;) {
    alen = sizeof(fsin);
    int *ssock = malloc(sizeof(int));
    *ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
    if (*ssock < 0) {
      fprintf(stderr, "accept: %s\n", strerror(errno));
      exit(-1);
    }
    pthread_create(&thread, NULL, client_handler, (void *)ssock);
  }
}
