CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
TEST_DIR = .
SHADER_DIR = shaders

# Test executables
TESTS = test_vglsl test_file_parsing

.PHONY: all clean run-tests test

all: $(TESTS)

# Build basic functionality tests
test_vglsl: test_vglsl.c ../vglsl.h
	$(CC) $(CFLAGS) -o $@ $<

# Build file parsing tests  
test_file_parsing: test_file_parsing.c ../vglsl.h
	$(CC) $(CFLAGS) -o $@ $<

# Run all tests
test: run-tests

run-tests: $(TESTS)

# Clean up build artifacts
clean:
	rm -f $(TESTS)
	rm -f *.o
