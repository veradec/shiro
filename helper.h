#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define reset_arr(t)  for (size_t i = 0; i < t->ptr; i++) { \
      t->arr[i] = NULL; \
    } \
    t->ptr=0;\


char *str_replace(char *str, char *username) {
  char* tmp = malloc(sizeof(str) + sizeof(username));
  if (str[0] == '~') {
    sprintf(tmp, "/home/%s%s", username, (str+1));
    return tmp;
  }

  return str;
}

// Tokens

typedef struct {
  char** arr;
  size_t ptr;
  size_t size;
} tokens;


tokens *init_tokens(void) {
    tokens *t = malloc(sizeof *t);
    if (t == NULL) return NULL;

    t->size = 3;
    t->ptr = 0;
    t->arr = malloc(t->size * sizeof *t->arr);
    if (t->arr == NULL) {
        free(t);
        return NULL;
    }

    return t;
}

int add_token(tokens *t, char *token) {
    if (t == NULL) return 0;

    if (t->ptr >= t->size) {
        size_t new_size = t->size * 2;
        char **new_arr = realloc(t->arr, new_size * sizeof *t->arr);
        if (new_arr == NULL) return 0;

        t->arr = new_arr;
        t->size = new_size;
    }

    t->arr[t->ptr++] = token;
    return 1;
}

tokens *split_by_whitespace(char *input) {
    tokens *t = init_tokens();
    if (t == NULL) return NULL;

    char *token = strtok(input, " \t\n\r");
    while (token != NULL) {
        if (!add_token(t, token)) {
            return NULL;
        }
        token = strtok(NULL, " \t\n\r");
    }

    return t;
}




// Commands 

/* typedef struct { */
/*   tokens** arr; */
/*   size_t ptr; */
/*   size_t size; */
/* } commands; */

/* commands *init_commands() { */
/*   commands *c = malloc(sizeof(commands)); */
/*   if (c == NULL) { */
/*     return NULL; */
/*   } */
/*   c->size = 3; */
/*   c->arr = malloc(sizeof(char *)*c->size); */
/*   return c; */
/* } */

/* void add_command(commands* c, tokens* ts) { */
/*   if (c->ptr >= c->size) { */
/*     c->size = c->size * 2; */
/*     c->arr = realloc(c->arr, sizeof(c->arr) * (c->size)); */
/*   } */
/*   c->arr[c->ptr] = ts; */
/*   c->ptr = c->ptr+1; */
/* } */

/* bool check_pipe(char *input) { */
/*   for (size_t i = 0; i < strlen(input); i++) { */
/*     if (input[i] == '|') { */
/*       return true; */
/*     } */
/*   } */
/*   return false; */
/* } */

/* commands *split_line_by_pipe(char *input) { */
/*   commands *c = init_commands(); */
/*   /\* add_command(c, ); *\/ */
/*   return c; */
/* } */
/* int run_piped_commands(commands* c, size_t ptr); */
