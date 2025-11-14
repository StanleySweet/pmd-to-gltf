# Makefile for PMD to glTF Converter

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

# Source files
SOURCES = main.c pmd_parser.c psa_parser.c gltf_exporter.c skeleton.c filesystem.c
HEADERS = pmd_psa_types.h skeleton.h filesystem.h
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = converter
ifeq ($(OS),Windows_NT)
    TARGET = converter.exe
endif

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Static build (useful for distribution)
static: LDFLAGS += -static
static: $(TARGET)

# MinGW cross-compilation targets
mingw-x86_64:
	x86_64-w64-mingw32-gcc $(SOURCES) -o converter-x86_64.exe $(LDFLAGS) -static $(CFLAGS)

mingw-i686:
	i686-w64-mingw32-gcc $(SOURCES) -o converter-i686.exe $(LDFLAGS) -static $(CFLAGS)

mingw: mingw-x86_64 mingw-i686

# Test target
test: $(TARGET)
	./$(TARGET) input/horse output/test_output.gltf Horse
	@echo "Test completed successfully!"

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET) converter.exe converter-*.exe
	rm -f output/test_output.gltf

# Linting
lint:
	@command -v cppcheck >/dev/null 2>&1 || { echo "cppcheck not found. Install with: sudo apt-get install cppcheck"; exit 1; }
	cppcheck --enable=all --suppress=missingIncludeSystem --std=c99 $(SOURCES) $(HEADERS)

# Format code
format:
	@command -v clang-format >/dev/null 2>&1 || { echo "clang-format not found. Install with: sudo apt-get install clang-format"; exit 1; }
	clang-format -i $(SOURCES) $(HEADERS)

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build the converter (default)"
	@echo "  static       - Build with static linking"
	@echo "  mingw-x86_64 - Cross-compile for Windows 64-bit using MinGW"
	@echo "  mingw-i686   - Cross-compile for Windows 32-bit using MinGW"
	@echo "  mingw        - Cross-compile for both Windows architectures"
	@echo "  test         - Build and run a test conversion"
	@echo "  clean        - Remove build artifacts"
	@echo "  lint         - Run static analysis with cppcheck"
	@echo "  format       - Format code with clang-format"
	@echo "  help         - Show this help message"

.PHONY: all static mingw-x86_64 mingw-i686 mingw test clean lint format help
