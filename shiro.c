#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "helper.h"

#define MAX_LEN 1024

int status;
char *username;
char* hostname;
tokens *PATH;
char *last_path;
char *shiro_history_path;
alias *aliases;

void load_alias() {
  aliases = init_alias();
  add_alias(aliases, "ls", "eza");
  /* add_alias(aliases, "cat", "bat"); */
}


void load_info() {
  load_alias();
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

  last_path = malloc(sizeof(char) * 99);

  shiro_history_path =
      malloc(sizeof(username) + sizeof("/home/.shiro_history"));

  sprintf(shiro_history_path, "/home/%s/.shiro_history", username);
  read_history(shiro_history_path);
}

char *get_prompt() {
  // buf for cwd
  char path[MAX_LEN];
  getcwd(path, sizeof(path));
  
  // buf for prompt
  char *buf = malloc(sizeof(char) * 99);
  if (status != 0) {
    sprintf(buf, "[%s@\033[32m%s\033[0m](%s) \x1b[31m{%d}\x1b[0m ", username, hostname, path, status);
  } else {
    sprintf(buf, "[%s@\033[32m%s\033[0m](%s) \x1b[34m{%d}\x1b[0m ", username, hostname, path, status);
  }
  return buf;
}

int cd(char *dir) {
  if (strcmp("-", dir) == 0) {
    if (last_path == NULL || last_path[0] == '\0') {
        return -1;
    }
    
    char target[1024];
    char current[1024];
    
    snprintf(target, sizeof target, "%s", last_path);
    
    if (getcwd(current, sizeof current) == NULL) {
      perror("getcwd");
        return -1;
    }
    
    if (chdir(target) == -1) {
      perror("cd");
        return -1;
    }
    
    snprintf(last_path, 1024, "%s", current);
    return 0;
  }
  dir = str_replace(dir, username);

  getcwd(last_path, 1024);

  
  int res = chdir(dir);
  if (res != 0) {
    printf("shiro: cd: %s doesn't exist\n", dir);
  }
  return (res);
}

char *check_alias(alias* a, char *application) {
  for (size_t i = 0; i < a->ptr; i++) {
    if (strcmp(application, a->key[i]) == 0) {
      return a->value[i];
    }
  }

  return NULL;
}

char *get_path_of_executable(char *application) {
  if (check_alias(aliases, application) != NULL){
    return (get_path_of_executable(check_alias(aliases, application)));
  }
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
    printf("shiro: command not found: %s\n", x->arr[0]);
    return -1;
  }

  x->arr[x->ptr] = (char*) NULL;
  return execvp(app_path, x->arr);
}

char *read_input(const char *prompt)
{
    char *input = readline(prompt);
    if (input == NULL)
        return NULL;

    size_t len = strlen(input);

    while (len > 0 && input[len - 1] == '\\') {
        input[--len] = '\0';

        char *part = readline("> ");
        if (part == NULL)
            break;

        size_t part_len = strlen(part);
        char *grown = realloc(input, len + part_len + 1);
        if (grown == NULL) {
            free(input);
            free(part);
            return NULL;
        }

        input = grown;
        memcpy(input + len, part, part_len + 1);
        len += part_len;
        free(part);
    }

    if (len > 0) {
        add_history(input);
        append_history(1, shiro_history_path);
    }

    return input;
}

int main() {
  char* input;
  tokens *t;
  char* prompt = get_prompt();
  load_info();
  
  
  while (1) {
    prompt = get_prompt();
    input = read_input(prompt);
    if (strcmp(input, "exit") == 0) {
      return 0;
    }

    if (check_for(input, "|")) {
      commands *c = split_by_pipe(input);
      for (size_t _ = 0; _ < c->ptr; _++) {
        tokens *t = c->arr[_];
        printf("%zu ", _);
        for (size_t i = 0; i < t->ptr; i++) {
	  printf("%s", t->arr[i]);
        }
        printf("\n");
      }
      /* run_piped_commands(c, 0); */
      continue;
    }

    if (check_for(input), "&&") {
      commands *c = split_by_pipe(input);
      for (size_t _ = 0; _ < c->ptr; _++) {
        tokens *t = c->arr[_];
        printf("%zu ", _);
        for (size_t i = 0; i < t->ptr; i++) {
	  printf("%s", t->arr[i]);
        }
        printf("\n");
      }
      /* run_piped_commands(c, 0); */
      continue;
    }

    
    ////////////////////////////////

    
    t = split_by_whitespace(input);
    /* replace_alias(); */

    if (strcmp(t->arr[0], "cd") == 0) {
      if (t->ptr == 1) {
        char *tmp = malloc(sizeof(username) + sizeof("/home/"));
        sprintf(tmp, "/home/%s", username);
        cd(tmp);
      } else {
	cd(t->arr[1]);
      }
      continue;
    }


    if (strcmp(t->arr[0], "where") == 0) {
      if (strcmp(t->arr[1], "where") == 0) {
	printf("shiro: where built-in command\n");
      } else {
	where(t->arr[1]);
      }

      continue;
    }

    
    if (strcmp(t->arr[0], "exit") == 0) {
      if (strcmp(t->arr[1], "exit") == 0) {
	printf("shiro: exit built-in command\n");
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
      status = status % 256;
      //////////
      
    } else {
      waitpid(pid, &status, 0);
    }
    reset_arr(t);
  }
}
