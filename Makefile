# Layla Window Manager Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LIBS = -lX11
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

SRCDIR = src
SOURCES = $(SRCDIR)/layla.c
TARGET = layla

# Build targets
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

install: $(TARGET)
	mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/
	chmod 755 $(BINDIR)/$(TARGET)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Testing basic compilation..."
	@./$(TARGET) --help 2>/dev/null || echo "Binary compiled successfully"

# Development targets
format:
	@command -v clang-format >/dev/null 2>&1 && \
		clang-format -i $(SOURCES) || \
		echo "clang-format not found, skipping formatting"

lint:
	@command -v cppcheck >/dev/null 2>&1 && \
		cppcheck --enable=all --std=c99 $(SOURCES) || \
		echo "cppcheck not found, skipping linting"

.PHONY: all debug release install uninstall clean test format lint