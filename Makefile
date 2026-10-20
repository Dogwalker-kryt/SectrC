# Compiler
CXX := gcc

# Directories
SRC_DIR := src

BUILD_DIR := build
DEBUG_BUILD_DIR := build_debug

# Output binary name
TARGET := ox

# Flags
CXXFLAGS := -std=c17
LDFLAGS := -flto -s -fopt-info-inline -O3 -lcrypto

# Debug flags
DEBUG_CXXFLAGS := -std=c17 -g -O0 -fno-omit-frame-pointer
DEBUG_LDFLAGS := -lcrypto

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# i dont know waht this is
VPATH := $(SRC_DIR)

# Compile each .cpp into .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
$(DEBUG_BUILD_DIR)/%.o: %.c
	@mkdir -p $(DEBUG_BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

debug: $(patsubst %.c,$(DEBUG_BUILD_DIR)/%.o,$(notdir $(SRCS)))
	$(CXX) $^ -o $(TARGET)_debug $(DEBUG_LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*.o $(DEBUG_BUILD_DIR)/*.o $(TARGET) $(TARGET)_debug

# Run the program
run: all
	./$(TARGET)

run_debug: debug
	./$(TARGET)

