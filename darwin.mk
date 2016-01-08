CFLAGS  = -O0 -g -std=c11 -fPIC -march=native -Wall -Wextra -Wunused
CFLAGS += -Werror -Wshadow
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS  = -dynamiclib --coverage
LDFLAGS += -framework CoreFoundation -framework IOKit
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
	$(CC) ${CFLAGS} -o $@ $< -L. -lpslib -Wl,-rpath .

test: clean shared
	cd bindings/python && make clean && make && cd - && export DYLD_LIBRARY_PATH=`pwd` && py.test -v -ra

.PHONY: covclean
covclean:
	${RM} *.gcno *.gcda *.gcov

.PHONY: clean
clean: covclean
	${RM} ${TARGET_LIB} ${OBJS} $(EXEC) *dSYM

check-syntax:
	gcc -Wall -o /dev/null -S ${CHK_SOURCES}
