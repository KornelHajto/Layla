# Makefile for Layla 2D Multiplayer Shooter (C/raylib)

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Detect OS for platform-specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lX11
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

TARGET = layla
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! Run with: ./$(TARGET)"

# Compile source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build with extra debugging info
debug: CFLAGS += -g -DDEBUG -O0
debug: $(TARGET)

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG -march=native
release: $(TARGET)

# Performance build with aggressive optimizations
performance: CFLAGS += -O3 -DNDEBUG -march=native -flto -ffast-math
performance: LDFLAGS += -flto
performance: $(TARGET)

# Install raylib if not present (Ubuntu/Debian)
install-deps:
	@echo "Installing raylib dependencies..."
	sudo apt update
	sudo apt install libraylib-dev libraylib4 raylib-examples

# Install raylib from source (if package not available)
install-raylib:
	@echo "Installing raylib from source..."
	git clone https://github.com/raysan5/raylib.git
	cd raylib/src && make PLATFORM=PLATFORM_DESKTOP
	sudo make install -C raylib/src PLATFORM=PLATFORM_DESKTOP
	rm -rf raylib

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJECTS) $(TARGET)

# Run the game
run: $(TARGET)
	./$(TARGET)

# Run with debug info
run-debug: debug
	gdb ./$(TARGET)

# Run with performance monitoring
run-perf: $(TARGET)
	perf record -g ./$(TARGET)

# Check for memory leaks
run-valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Static analysis
analyze:
	cppcheck --enable=all --std=c99 $(SOURCES)

# Format code
format:
	clang-format -i $(SOURCES) *.h

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the game (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  performance  - Build with aggressive optimizations"
	@echo "  clean        - Remove build files"
	@echo "  run          - Build and run the game"
	@echo "  run-debug    - Run with gdb debugger"
	@echo "  run-perf     - Run with performance profiling"
	@echo "  run-valgrind - Run with memory leak detection"
	@echo "  install-deps - Install raylib via package manager"
	@echo "  install-raylib - Install raylib from source"
	@echo "  analyze      - Run static code analysis"
	@echo "  format       - Format source code"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all debug release performance clean run run-debug run-perf run-valgrind install-deps install-raylib analyze format help

# Print build info
info:
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SOURCES)"
	@echo "OS: $(UNAME_S)"
