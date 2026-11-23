#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define print_tokens(t)                                                        \
  for (size_t i = 0; i < t->ptr; i++) {                                        \
    printf("%zu - %s | ", i, t->arr[i]);				       \
  }                                                                            \
  printf("\n");                                                                \

#define reset_arr(t)  for (size_t i = 0; i < t->ptr; i++) { \
      t->arr[i] = NULL; \
    } \
    t->ptr=0;\

#define check_for(haystack, needle) strstr(haystack, needle) != NULL

#define starts_with(a, b) strncmp(a, b, strlen(b)) == 0

bool StartsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

// struct def
typedef struct {
  char** arr;
  size_t ptr;
  size_t size;
} tokens;

typedef struct {
  tokens** arr;
  size_t ptr;
  size_t size;
} commands;

typedef struct {
  char **key;
  char **value;
  size_t ptr;
  size_t size;
} alias;

// method def
int _exec(tokens *x);

char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

// Tokens



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


int run_piped_commands(commands *c) {
    int prev_read = -1;

    for (size_t i = 0; i < c->ptr; i++) {
        int next_pipe[2] = {-1, -1};

        if (i + 1 < c->ptr && pipe(next_pipe) == -1) {
            perror("pipe");
            if (prev_read != -1)
                close(prev_read);
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (prev_read != -1)
                close(prev_read);
            if (next_pipe[0] != -1)
                close(next_pipe[0]);
            if (next_pipe[1] != -1)
                close(next_pipe[1]);
            return 1;
        }

        if (pid == 0) {
            if (prev_read != -1 &&
                dup2(prev_read, STDIN_FILENO) == -1) {
                perror("dup2 stdin");
                _exit(1);
            }

            if (next_pipe[1] != -1 &&
                dup2(next_pipe[1], STDOUT_FILENO) == -1) {
                perror("dup2 stdout");
                _exit(1);
            }

            if (prev_read != -1)
                close(prev_read);
            if (next_pipe[0] != -1)
                close(next_pipe[0]);
            if (next_pipe[1] != -1)
                close(next_pipe[1]);

            _exec(c->arr[i]);
            perror("exec");
            _exit(127);
        }

        if (prev_read != -1)
            close(prev_read);
        if (next_pipe[1] != -1)
            close(next_pipe[1]);

        prev_read = next_pipe[0];
    }

    if (prev_read != -1)
        close(prev_read);

    for (size_t i = 0; i < c->ptr; i++) {
        while (wait(NULL) == -1) {
            if (errno == EINTR)
                continue;
            perror("wait");
            return 1;
        }
    }

    return 0;
}
// Alias

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
