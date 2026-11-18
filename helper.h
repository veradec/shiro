#include <string.h>
#include <stdlib.h>

char *str_replace(char *str, char *username) {
  char* tmp = malloc(sizeof(str) + sizeof(username));
  if (str[0] == '~') {
    sprintf(tmp, "/home/%s%s", username, (str+1));
    return tmp;
  }

  return str;
}
