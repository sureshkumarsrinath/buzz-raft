# Compiler and flags
INCLUDE_PATH ?= /opt/homebrew/include  # For Apple Silicon, use /usr/local/include for Intel
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -I$(INCLUDE_PATH) -pthread
LDLIBS :=

# Source files
SRC := messages.cpp
OBJ := $(SRC:.cpp=.o)
EXE := messages

# Test files
TEST_SRC := test_messages.cpp
TEST_OBJ := $(TEST_SRC:.cpp=.o)
TEST_EXE := test_messages

# Default target
all: $(EXE)

# Build main executable
$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# Build and run test executable
test: $(TEST_EXE)

$(TEST_EXE): $(TEST_OBJ) $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Test JSON installation
test_json: test_json.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
	./$@

# Format code (requires clang-format)
format:
	clang-format -i *.cpp *.hpp

# Clean build artifacts
clean:
	rm -f $(OBJ) $(EXE) $(TEST_OBJ) $(TEST_EXE) test_json

.PHONY: all clean format test test_json