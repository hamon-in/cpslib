#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"

float percentage(uint64_t n, uint64_t d) {
  /* TODO: Error check here */
  float percent = ((float)n / (float)d) * 100.0;
  return percent;
}

int str_comp(const void *key, const void *memb) {
  const char **a = (const char **)key;
  const char **b = (const char **)memb;
  return strcmp(*a, *b);
}

int int_comp(const void *key, const void *memb) {
  const int a = *(int *)key;
  const int b = *(int *)memb;
  if (a == b) {
    return 0;
  } else {
    return -1;
  }
}

char *grep_awk(FILE *fp, const char *fstr, int nfield, const char *delim) {
  char *ret = NULL;
  int i;
  char *line = (char *)calloc(500, sizeof(char));
  check_mem(line);
  while (fgets(line, 400, fp) != NULL) {
    if (strncasecmp(line, fstr, strlen(fstr)) == 0) {
      ret = strtok(line, delim);
      for (i = 0; i < nfield; i++) {
        ret = strtok(NULL, delim);
      }
      if (ret) {
        ret = strdup(ret);
        check_mem(ret);
        free(line);
        return ret;
      }
    }
  }
  free(line);
  return NULL;
error:
  free(line);
  return NULL;
}

char *squeeze(char *string, const char *chars) {
  char *src = string;
  char *target = string;
  char ch;
  for (; *chars; chars++) {
    ch = *chars;
    src = string;
    target = string;
    while (*src != '\0') {
      if (*src != ch) {
        *target = *src;
        target++;
      }
      src++;
    }
    *target = '\0';
  }
  return string;
}
