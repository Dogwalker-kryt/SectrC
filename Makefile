# Compiler
CXX := g++

# Directories
SRC_DIR := src

BUILD_DIR := build
DEBUG_BUILD_DIR := build_debug

# Output binary name
TARGET := ox

# Flags
CXXFLAGS := -D_POSIX_C_SOURCE=200112L
LDFLAGS := -flto -s -O3 -lcrypto -ludev -lblkid

# Debug flags
DEBUG_CXXFLAGS := -D_POSIX_C_SOURCE=200112L -g -O0 -fno-omit-frame-pointer
DEBUG_LDFLAGS := -lcrypto -ludev -lblkid

# Source and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c') $(shell find $(SRC_DIR) -type f -name '*.cpp')

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SRCS)))

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile each source file into a matching object path
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(DEBUG_CXXFLAGS) -c $< -o $@

debug: $(patsubst $(SRC_DIR)/%.c,$(DEBUG_BUILD_DIR)/%.o,$(filter %.c,$(SRCS))) $(patsubst $(SRC_DIR)/%.cpp,$(DEBUG_BUILD_DIR)/%.o,$(filter %.cpp,$(SRCS)))
	$(CXX) $^ -o $(TARGET)_debug $(DEBUG_LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR) $(TARGET) $(TARGET)_debug

# Run the program
run: all
	./$(TARGET)

run_debug: debug
	./$(TARGET)_debug

