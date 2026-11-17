#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_LEN 1024

typedef struct {
  char** arr;
  size_t ptr;
  size_t size;
} tokens;

tokens *init_tokens() {
  tokens *t = malloc(sizeof(tokens));
  t->size = 3;
  t->arr = malloc(sizeof(char *)*t->size);
  return t;
}

void add_token(tokens* t, char* token) {
  if (t->ptr >= t->size) {
    t->size = t->size * 2;
    realloc(t, t->size);
  }

  t->arr[t->ptr] = token;
  t->ptr = t->ptr+1;
}

char *get_prompt() {
  // allocating for username  
  char* username = malloc(sizeof(char) * 99);
  getlogin_r(username, sizeof(username));
  
  // allocating for hostname  
  char* hostname = malloc(sizeof(char) * 99);
  gethostname(hostname, sizeof(hostname));

  // buf for cwd
  char path[MAX_LEN];
  getcwd(path, sizeof(path));

  // buf for prompt
  char *buf = malloc(sizeof(char) * 99);
  sprintf(buf, "\n[%s@%s](%s)", username, hostname, path);
  return buf;
}

int cd(char *dir) {
  int res = chdir(dir);
  return (res);
}

int main() {
  char input[MAX_LEN];
  tokens *t = init_tokens();
  char* prompt = get_prompt();

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

    if (strcmp(t->arr[0], "cd") == 0) {
      cd(t->arr[1]);
      continue;
    }

    t->ptr = 0;
  }
}
