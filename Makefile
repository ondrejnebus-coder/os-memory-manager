CC ?= gcc
CFLAGS ?= -fPIC -Wall -Wextra -Werror -O2 -Isrc \
           -Dmalloc=__forbidden_malloc \
           -Dcalloc=__forbidden_calloc \
           -Drealloc=__forbidden_realloc \
           -Dfree=__forbidden_free
LDFLAGS ?= -shared

TARGET ?= build/libospager.so
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
