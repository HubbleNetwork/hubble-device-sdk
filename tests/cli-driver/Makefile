CC      = gcc
CFLAGS  = -Wall -Wextra -I. -I../../samples/freertos/ti/sat-dtm/src \
          $(shell pkg-config --cflags check)
LDFLAGS = $(shell pkg-config --libs check)
SRCS    = ../test_invariant_cli_driver.c \
          ../../samples/freertos/ti/sat-dtm/src/cli_driver.c \
          stubs.c
TARGET  = test_invariant_cli_driver

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
