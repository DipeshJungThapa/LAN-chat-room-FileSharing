# Makefile for LAN Chat Client (Cross-platform)
# Supports Linux, macOS, and Windows (MinGW)

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

# Target executable
TARGET = client

# Source files
SOURCES = client.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM = Windows
    LDFLAGS = -lws2_32
    RM = del /Q
    TARGET := $(TARGET).exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = Linux
        LDFLAGS = -pthread
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM = macOS
        LDFLAGS = -pthread
    endif
    RM = rm -f
endif

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJECTS)
	@echo "Building $(TARGET) for $(PLATFORM)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@echo "✓ Build successful!"

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	$(RM) $(OBJECTS) $(TARGET)
	@echo "✓ Clean complete"

# Run the client (requires server IP as argument)
run: $(TARGET)
	@echo "Usage: ./$(TARGET) <server_ip>"
	@echo "Example: ./$(TARGET) 127.0.0.1"

# Rebuild
rebuild: clean all

.PHONY: all clean run rebuild
