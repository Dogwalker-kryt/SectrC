# Compiler
CXX := gcc

# Directories
SRC_DIR := src

BUILD_DIR := build
DEBUG_BUILD_DIR := build_debug

# Output binary name
TARGET := ox

# Flags
CXXFLAGS := -D_POSIX_C_SOURCE=200112L
LDFLAGS := -flto -s -fopt-info-inline -O3 -lcrypto

# Debug flags
DEBUG_CXXFLAGS := -D_POSIX_C_SOURCE=200112L -g -O0 -fno-omit-frame-pointer
DEBUG_LDFLAGS := -lcrypto

# Source and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile each source file into a matching object path
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

debug: $(patsubst $(SRC_DIR)/%.c,$(DEBUG_BUILD_DIR)/%.o,$(SRCS))
	$(CXX) $^ -o $(TARGET)_debug $(DEBUG_LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*.o $(DEBUG_BUILD_DIR)/*.o $(TARGET) $(TARGET)_debug

# Run the program
run: all
	./$(TARGET)

run_debug: debug
	./$(TARGET)_debug

