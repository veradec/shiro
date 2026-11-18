#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "helper.h"

#define MAX_LEN 1024

typedef struct {
  char** arr;
  size_t ptr;
  size_t size;
} tokens;

int status;
char *username;
char* hostname;

tokens *PATH;

tokens *init_tokens() {
  tokens *t = malloc(sizeof(tokens));
  if (t == NULL) {
    return NULL;
  }
  t->size = 3;
  t->arr = malloc(sizeof(char *)*t->size);
  return t;
}

void add_token(tokens* t, char* token) {
  if (t->ptr >= t->size) {
    t->size = t->size * 2;
    t->arr = realloc(t->arr, sizeof(t->arr) * (t->size));
  }

  t->arr[t->ptr] = token;
  t->ptr = t->ptr+1;
}

void load_info() {

  PATH = init_tokens();
  add_token(PATH, "/usr/bin");
  add_token(PATH, "/usr/sbin");
  add_token(PATH, "/usr/local/bin");

  // allocating for username  
  username = malloc(sizeof(char) * 99);
  getlogin_r(username, sizeof(username));
  
  // allocating for hostname  
  hostname = malloc(sizeof(char) * 99);
  gethostname(hostname, sizeof(hostname));
}

char *get_prompt() {
  // buf for cwd
  char path[MAX_LEN];
  getcwd(path, sizeof(path));

  // buf for prompt
  char *buf = malloc(sizeof(char) * 99);
  sprintf(buf, "\n[%s@%s](%s) {%d}", username, hostname, path, status);
  return buf;
}

int cd(char *dir) {
  dir = str_replace(dir, username);
  int res = chdir(dir);
  if (res != 0) {
    printf("shiro: cd: %s doesn't exist", dir);
  }
  return (res);
}

char *get_path_of_executable(char *application) {
  for (size_t i = 0; i < PATH->ptr; i++) {
    char *fname = malloc(sizeof(PATH->arr[i]) + sizeof(application));
    sprintf(fname, "%s/%s", PATH->arr[i], application);
    if (access(fname, F_OK) == 0) {
      return fname;
    }
  }
  return NULL;
}

void where(char *application) {
  for (size_t i = 0; i < PATH->ptr; i++) {
    char *fname = malloc(sizeof(PATH->arr[i])  + sizeof(application));
    sprintf(fname, "%s/%s", PATH->arr[i], application);
    if (access(fname, F_OK) == 0) {
      printf("%s\n", fname);
    }
  }
}

int exec(tokens *x) {
  char *app_path = get_path_of_executable(x->arr[0]);
  if (!app_path) {
    printf("shiro: command not found: %s", x->arr[0]);
    return -1;
  }
  return execv(app_path, x->arr);
}

int main() {
  char input[MAX_LEN];
  tokens *t = init_tokens();
  char* prompt = get_prompt();
  load_info();

  while (1) {
    prompt = get_prompt();
    printf("%s ", prompt);
    fgets(input, MAX_LEN, stdin);
    input[strlen(input)-1] = '\0';
    if (strcmp(input, "exit") == 0) {
      return 0;
    }

    char *token = strtok(input, " ");
    add_token(t, token);

    while (token != NULL) {
      token = strtok(NULL, " ");
      add_token(t, token);
    }

    add_token(t, (char*) NULL);

    if (strcmp(t->arr[0], "cd") == 0) {
      cd(t->arr[1]);
      continue;
    }


    if (strcmp(t->arr[0], "where") == 0) {
      if (strcmp(t->arr[1], "where") == 0) {
	printf("shiro: where built-in command");
      } else {
	where(t->arr[1]);
      }

      continue;
    }

    

    // START EXEC
    pid_t pid = fork();
    
    if (pid == -1) {
      perror("fork failed");
      return EXIT_FAILURE;
    } else if (pid == 0) {
      
      // exec the command
      status = exec(t);
      //////////
      
    } else {
      waitpid(pid, &status, 0);
    }
    /////////////
    t->ptr=0;
  }
}
