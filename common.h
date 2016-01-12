#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

float percentage(uint64_t, uint64_t);
int str_comp(const void *, const void *);
int int_comp(const void *, const void *);
char *grep_awk(FILE *, const char *, int, const char *);
char *squeeze(char *, const char *);

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...)                                                          \
  fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...)                                                        \
  fprintf(stderr, "[ERROR] (%s:%d:%s: errno: %d, %s) " M "\n", __FILE__,       \
          __LINE__, __FUNCTION__, errno, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...)                                                       \
  fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__,     \
          clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...)                                                       \
  fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...)                                                       \
  if (!(A)) {                                                                  \
    log_err(M, ##__VA_ARGS__);                                                 \
    errno = 0;                                                                 \
    goto error;                                                                \
  }

#define sentinel(M, ...)                                                       \
  {                                                                            \
    log_err(M, ##__VA_ARGS__);                                                 \
    errno = 0;                                                                 \
    goto error;                                                                \
  }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...)                                                 \
  if (!(A)) {                                                                  \
    debug(M, ##__VA_ARGS__);                                                   \
    errno = 0;                                                                 \
    goto error;                                                                \
  }
