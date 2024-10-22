CXX = g++

COMMON_FLAGS = -Wall -Wextra -g

# Filesystem-specific flags (C++11)
FS_CXXFLAGS = $(COMMON_FLAGS) -std=c++11

# Test-specific flags (C++14)
TEST_CXXFLAGS = $(COMMON_FLAGS) -std=c++14

# Include paths
INCLUDES = -I. -I/usr/local/include

# Filesystem source file
FS_SRC = passthrough_hp.cc

# Test source file
TEST_SRC = TokenBucket_ts.cc

# Output executables
FS_OUT = passthrough_hp
TEST_OUT = token_bucket_test

# FUSE flags and libraries
FUSE_FLAGS = $(shell pkg-config fuse3 --cflags --libs)

# GTest libraries
GTEST_LIBS = -lgtest -lgtest_main -pthread

# Default target
all: $(FS_OUT) $(TEST_OUT)

# Build the filesystem
$(FS_OUT): $(FS_SRC) cxxopts.hpp TokenBucket.hpp
	$(CXX) $(FS_CXXFLAGS) $(INCLUDES) $< -o $@ $(FUSE_FLAGS)

# Build the test
$(TEST_OUT): $(TEST_SRC) TokenBucket.hpp
	$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) $< -o $@ $(GTEST_LIBS) -lrt

# Run the test
test: $(TEST_OUT)
	./$(TEST_OUT)

# Clean up built files
clean:
	rm -f $(FS_OUT) $(TEST_OUT)

# Ensure cxxopts.hpp is available
cxxopts.hpp:
	@if [ ! -f cxxopts.hpp ]; then \
		echo "Downloading cxxopts.hpp..."; \
		curl -O https://github.com/libfuse/libfuse/blob/master/example/cxxopts.hpp; \
	fi

# Rule for TokenBucket.hpp (adjust as needed)
TokenBucket.hpp:
	@echo "Please ensure TokenBucket.hpp is present in the current directory."

.PHONY: all test clean