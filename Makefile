CFLAGS  = -fPIC -Wall -Werror -Wunused -Wextra -O0 -g -std=gnu11
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS = -dynamiclib -fprofile-arcs
RM = rm -rf
TARGET_LIB = libpslib.dylib
EXEC = driver

SRCS = common.c pslib_osx.c
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
clean: covclean
	${RM} ${TARGET_LIB} ${OBJS} $(EXEC) *dSYM

check-syntax:
	gcc -Wall -o /dev/null -S ${CHK_SOURCES}
