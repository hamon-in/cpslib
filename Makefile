UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  include linux.mk
endif
ifeq ($(UNAME_S),Darwin)
  include darwin.mk
endif
