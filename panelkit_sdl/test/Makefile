# Test programs Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -I../src/core -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lzlog -lpthread

# Test programs
TESTS = test_logger

.PHONY: all clean test_logger

all: $(TESTS)

test_logger: test_logger.c ../src/core/logger.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TESTS)

run: test_logger
	./test_logger ../config/zlog.conf