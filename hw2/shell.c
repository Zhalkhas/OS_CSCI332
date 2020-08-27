//#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// #define DEBUG

#define CMDLEN 254
#define GRN "\x1B[32m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

int main() {
  int pid;
  int status;
  char command[CMDLEN];
  char *args[100];
  unsigned int back, argsP, redirect;
  printf("Program begins.\n");

  for (;;) {
    back = 0;
    redirect = 0;
    // printing username
    /*char *username = getlogin();
printf(GRN "%s%% " RESET, username);
    */
    printf("smsh%% ");
    fgets(command, CMDLEN, stdin);
    command[strlen(command) - 1] = '\0';
    if (strcmp(command, "quit") == 0 || *command == 0)
      break;

    if (command[strlen(command) - 1] == '&') {
      back = 1;
      command[strlen(command) - 1] = '\0';
    }

    argsP = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
      args[argsP] = token;
      token = strtok(NULL, " ");
      argsP++;
    }
    //(char* ) NULL for execvp arguments array at the end
    args[argsP] = (char *)NULL;

#ifdef DEBUG
    for (unsigned int i = 0; i < argsP; i++) {
      printf(RED "%s " RESET, args[i]);
      printf(RED "%i\n" RESET, i);
    }
#endif
    // redirect if 0 = no redirect, 1 = truncate, 2 = append
    if (argsP > 2) {
      if (strcmp(args[argsP - 2], ">") == 0)
        redirect = 1;
      if (strcmp(args[argsP - 2], ">>") == 0)
        redirect = 2;
    }
#ifdef DEBUG
    printf(RED "Start forking\n" RESET);
#endif

    pid = fork();

    if (pid < 0) {
      printf("Error in fork.\n");
      exit(-1);
    }
    if (pid != 0) {
#ifdef DEBUG
      printf(RED "PARENT. pid = %d, mypid = %d.\n" RESET, pid, getpid());
#endif
      if (back == 0) {
        waitpid(pid, &status, 0);
#ifdef DEBUG
        printf(RED "Program exited with code %d\n" RESET, status);
#endif
      }
    } else {
#ifdef DEBUG
      printf(RED "CHILD. pid = %d, mypid = %d.\n" RESET, pid, getpid());
#endif
      if (redirect > 0) {
        int fd = 0;
        switch (redirect) {
        case 1:
          fd = open(args[argsP - 1], O_TRUNC | O_CREAT | O_WRONLY, 0640);
          break;
        case 2:
          fd = open(args[argsP - 1], O_APPEND | O_CREAT | O_WRONLY, 0640);
          break;
        }
        if (fd > 0) {
          dup2(fd, 1);
        }
        // removing last two arguments from arguments list
        args[argsP - 2] = (char *)NULL;
      }
      int return_status = execvp(args[0], args);
#ifdef DEBUG
      printf(RED "CHILD return:%d\n" RESET, return_status);
#endif
      if (return_status < 0) {
        printf("Error: no such file or directory: %s\n", args[0]);
		//error printing
		// printf("%s\n", strerror(errno));
      }

      exit(return_status);
    }
  }
}
