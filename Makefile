CC = gcc
CFLAGS = -fPIC -Wall -Werror -Wunused -Wextra -O2 -g -std=gnu11 -fprofile-arcs -ftest-coverage -lgcov
LDFLAGS = -shared -fprofile-arcs -lgcov
RM = rm -f
TARGET_LIB = libpslib.so
EXEC = driver

SRCS = common.c pslib_linux.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: $(EXEC)

.PHONY: shared
shared: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(EXEC): $(EXEC).c $(TARGET_LIB)
	$(CC) -o $@ $< -L. -lpslib -Wl,-rpath .

.PHONY: covclean
covclean:
	${RM} *.gcno *.gcda *.gcov

.PHONY: clean
clean:
	${RM} ${TARGET_LIB} ${OBJS} $(EXEC)

check-syntax:
	gcc -Wall -o /dev/null -S ${CHK_SOURCES}
