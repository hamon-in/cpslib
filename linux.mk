CC = gcc
CFLAGS  = -O0 -g -std=gnu11 -fPIC -march=native -Wall -Wextra -Wunused
CFLAGS += -Werror -Wshadow
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS = -shared --coverage
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
	$(CC) ${CFLAGS} -o $@ $< -L. -lpslib -Wl,-rpath .

test: clean shared
	cd bindings/python && make clean && make && cd - && export LD_LIBRARY_PATH=`pwd` && py.test -v -ra

.PHONY: covclean
covclean:
	${RM} *.gcno *.gcda *.gcov *.gch

.PHONY: clean
clean: covclean
	${RM} ${TARGET_LIB} ${OBJS} $(EXEC) driver.o

check-syntax:
	gcc -Wall -o /dev/null -S ${CHK_SOURCES}
