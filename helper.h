#include <string.h>
#include <stdlib.h>

#define reset_tokens(t)  for (size_t i = 0; i < t->ptr; i++) { \
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

typedef struct {
  char** arr;
  size_t ptr;
  size_t size;
} tokens;

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


typedef struct {
  tokens** arr;
  size_t ptr;
  size_t size;
} commands;

commands *init_commands() {
  commands *t = malloc(sizeof(commands));
  if (t == NULL) {
    return NULL;
  }
  t->size = 3;
  t->arr = malloc(sizeof(char *)*t->size);
  return t;
}

void add_command(commands* c, tokens* ts) {
  if (c->ptr >= c->size) {
    c->size = c->size * 2;
    c->arr = realloc(c->arr, sizeof(c->arr) * (c->size));
  }
  c->arr[c->ptr] = ts;
  c->ptr = c->ptr+1;
}


