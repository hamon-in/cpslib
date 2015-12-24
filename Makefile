CFLAGS = -fPIC -Wall -Werror -Wunused -Wextra -O2 -g -std=gnu11
LDFLAGS = -dynamiclib
RM = rm -f
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

.PHONY: clean
clean:
	${RM} ${TARGET_LIB} ${OBJS} $(EXEC)
