#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <errno.h>
#include "helper.h"

#define MAX_LEN 1024

// Global Variables
alias *aliases;
int status;
char *username;
char* hostname;
tokens *PATH;
char *last_path;
char *shiro_history_path;
char *shiro_config_path;
char* home_dir;
int pid_counter = 0;
int pid_process_number = 0;

static volatile sig_atomic_t interrupted = 0;

static void on_sigint(int signo)
{
    (void)signo;
    interrupted = 1;   /* Keep signal handlers minimal */
}

void load_alias() {
  aliases = init_alias();
  shiro_config_path = malloc(sizeof("/.shirorc") + sizeof(home_dir));
  sprintf(shiro_config_path, "%s/.shirorc", home_dir);
  char buf[MAX_LEN] = "";
  char str[MAX_LEN] = "";
  char* tmp;
  FILE *rc = fopen(shiro_config_path, "r");
  if (!rc) {
    printf("%s", shiro_config_path);
    /* perror("couldnt read file"); */
    exit(1);
  }
  while (fgets(str, MAX_LEN, rc) != NULL) {
    if (starts_with(str, "alias")) {
      tmp = str + 6;
      tmp[strlen(tmp)-1] = '\0';

      char* key = strtok(tmp, "=");
      char *value = strtok(NULL, "=");
      if (value[0] == '"') {
        value = value + 1;
        value[strlen(value)-1] = '\0';
      }

      add_alias(aliases, strdup(key), strdup(value));
      continue;
    }

    if (starts_with(str, "export")) {
      tmp = tmp + 7;
      tmp[strlen(tmp)-1] = '\0';
      printf("TODO: Set Env - %s", str);
      continue;
    }
  }

}

void load_info() {


  PATH = init_tokens();
  add_token(PATH, "/usr/bin");
  add_token(PATH, "/usr/sbin");
  add_token(PATH, "/usr/local/bin");
  
  // allocating for username  
  username = malloc(sizeof(char) * 99);
  getlogin_r(username, sizeof(username));

  // allocating for home_dir
  home_dir = malloc(sizeof(username) + sizeof("/home/"));
  sprintf(home_dir, "/home/%s", username);
  // allocating for hostname  
  hostname = malloc(sizeof(char) * 99);
  gethostname(hostname, sizeof(hostname));

  // allocating for last_path
  last_path = malloc(sizeof(char) * 99);

  // allocating for shiro_history_path
  shiro_history_path =
      malloc(sizeof(username) + sizeof("/home/.shiro_history"));

  sprintf(shiro_history_path, "/home/%s/.shiro_history", username);
  read_history(shiro_history_path);

  rl_bind_key('\t', rl_complete);
  load_alias();
}

char *get_prompt() {
  // buf for cwd
  char path[MAX_LEN];
  getcwd(path, sizeof(path));
  ;
  // buf for prompt
  char *buf = malloc(sizeof(char) * 99);
  if (status != 0) {
    sprintf(buf, "[%s@\033[32m%s\033[0m](%s) \x1b[31m{%d}\x1b[0m ", username, hostname, str_replace(path, home_dir, "~"), status);
  } else {
    sprintf(buf, "[%s@\033[32m%s\033[0m](%s) \x1b[34m{%d}\x1b[0m ", username, hostname, str_replace(path, home_dir, "~"), status);
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

  dir = str_replace(dir, "~", home_dir);

  getcwd(last_path, 1024);

  
  int res = chdir(dir);
  if (res != 0) {
    printf("shiro: cd: %s doesn't exist\n", dir);
  }
  return (res);
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

//// EXEC
char *get_path_of_executable(char *application) {
  for (size_t i = 0; i < PATH->ptr; i++) {
    char *fname = malloc(sizeof(PATH->arr[i]) + sizeof(alias));
    sprintf(fname, "%s/%s", PATH->arr[i], application);
    if (access(fname, F_OK) == 0) {
      return fname;
    }
  }
  printf("shiro: command not found: %s\n", application);
  return NULL;
}

tokens* change_alias(tokens *t) {
  if (t == NULL || t->ptr == 0) {
    return t;
  }
  char *alias_val = NULL;
  for (size_t i = 0; i < aliases->ptr; i++) {
    if (strcmp(t->arr[0], aliases->key[i]) == 0) {
      alias_val = aliases->value[i];
      break;
    }
  }
  if (alias_val == NULL) {
    return t;
  }

  char *f = strdup(alias_val);
  if (f == NULL) {
    return t;
  }

  tokens* a = split_by_whitespace(f);
  if (a == NULL) {
    free(f);
    return t;
  }

  for (size_t i = 1; i < t->ptr; i++) {
    add_token(a, t->arr[i]);
  } 
  
  return a;
}

int _exec(tokens *t) {

  /// ALIAS TO TOKENS
  t = change_alias(t);
  ///////////////////

  char *app_path = get_path_of_executable(t->arr[0]);
  if (!app_path) {
    return -1;
  }

  t->arr[t->ptr] = (char*) NULL;
  return execvp(app_path, t->arr);
}

int exec(tokens *t) {
  pid_t pid = fork();
  if (pid == -1) {
      perror("fork failed");
      return EXIT_FAILURE;
  } else if (pid == 0) {
    
    // exec the command
    status = _exec(t);
    status = status % 256;
    //////////
  } else {
    waitpid(pid, &status, 0);
  }
}


/////////



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
  
  rl_getc_function = getc;

  struct sigaction sa = {0};
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  while (1) {
    interrupted = 0;
    prompt = get_prompt();
    input = read_input(prompt);
    if (interrupted) {
      continue;
    }
    if (!input) {
      exit(-1);
    }

    if (strlen(input) == 0) {
      continue;
    }

    input = str_replace(input, "~", home_dir);
    
    if (strcmp(input, "exit") == 0) {
      return 0;
    }

    if (check_for(input, "|")) {
      commands *c = split_by(input, "|");
      run_piped_commands(c);
      continue;
    }

    if (check_for(input, "&&")) {
      commands *c = split_by(input, "&&");
      for (size_t i = 0; i < c->ptr; i++) {
	exec(c->arr[i]);
      }
      continue;
    }

    
    ////////////////////////////////

    
    t = split_by_whitespace(input);
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
    exec(t);
    reset_arr(t);
  }
}
