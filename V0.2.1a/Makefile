# Nirvana Lang v0.2.1 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -lm
DEBUG_FLAGS = -Wall -Wextra -std=c99 -g -O0 -DDEBUG -lm

TARGET = nirvana
SRCS = main.c lexer.c parser.c vm.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug: $(SRCS)
	$(CC) $(DEBUG_FLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o

# Test examples
test: $(TARGET)
	@echo "Running tests..."
	@echo 'cetak(1 + 2 * 3)' | ./$(TARGET) -r

# Install to /usr/local/bin (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)
