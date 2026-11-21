#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define reset_arr(t)  for (size_t i = 0; i < t->ptr; i++) { \
      t->arr[i] = NULL; \
    } \
    t->ptr=0;\

#define check_for(haystack, needle) strstr(haystack, needle) != NULL

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

typedef struct {
  tokens** arr;
  size_t ptr;
  size_t size;
} commands;

commands *init_commands() {
  commands *c = malloc(sizeof(commands));
  if (c == NULL) {
    return NULL;
  }
  c->ptr=0;
  c->size = 3;
  c->arr = malloc(sizeof(char *)*c->size);
  return c;
}

int add_command(commands *c, tokens *ts) {
  if (c == NULL) return 0;
  if (c->ptr >= c->size) {
    c->size = c->size * 2;
    c->arr = realloc(c->arr, sizeof(c->arr) * (c->size));
  }
  c->arr[c->ptr] = ts;
  c->ptr = c->ptr + 1;
  return 1;
}


commands *split_by(char *input, char* delim)
{
    if (input == NULL)
        return NULL;

    commands *c = init_commands();
    if (c == NULL)
        return NULL;

    char *saveptr = NULL;
    char *command = strtok_r(input, delim, &saveptr);

    while (command != NULL) {
        tokens *args = split_by_whitespace(command);
        if (args == NULL || !add_command(c, args)) {
            /* Free args and c here if your API requires it. */
            return NULL;
        }

        command = strtok_r(NULL, delim, &saveptr);
    }

    return c;
}


int run_piped_commands(commands *c, size_t ptr);

// Alias

typedef struct {
  char **key;
  char **value;
  size_t ptr;
  size_t size;
} alias;

int add_alias(alias *a, char *k, char *v) {
  if (a == NULL) return 0;
  if (a->ptr >= a->size) {
    a->size = a->size * 2;
    a->key = realloc(a->key, sizeof(a->key) * (a->size));
    a->value = realloc(a->value, sizeof(a->value) * (a->size));
  }
  a->key[a->ptr] = k;
  a->value[a->ptr] = v;
  a->ptr = a->ptr + 1;
  return 1;
}

alias *init_alias(void) {
    alias *a = malloc(sizeof *a);
    if (a == NULL) return NULL;

    a->size = 3;
    a->ptr = 0;
    a->key = malloc(a->size * sizeof *a->key);
    a->value = malloc(a->size * sizeof *a->value);
    if (a->key == NULL || a->value == NULL) {
        free(a);
        return NULL;
    }

    return a;
}
