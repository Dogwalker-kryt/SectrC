# Compiler
CC := gcc

# Directories
SRC_DIR := src

BUILD_DIR := build
DEBUG_BUILD_DIR := build_debug

# Output binary name
TARGET := ox

# Flags
CFLAGS := -Wall -Wextra -O3 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -pthread
LDFLAGS := -flto -s -O3 -lcrypto -ludev -lblkid -pthread -latomic

# Debug flags
DEBUG_CFLAGS := -Wall -Wextra -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -g -O0 -fno-omit-frame-pointer -pthread
DEBUG_LDFLAGS := -lcrypto -ludev -lblkid -pthread

# Version: taken from the latest git tag (e.g. v0.3.1, or v0.3.1-4-gabc1234
# for commits after it). Never edit this by hand: tags are created by CI.
# Override with: make VERSION=v1.2.3
VERSION ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo development)
VERSION_STAMP := $(BUILD_DIR)/.version

# Source and object files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default rule
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile each source file into a matching object path
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Only main.c needs the version. The stamp file is rewritten only when the
# version string changes, so main.o is rebuilt only then.
$(VERSION_STAMP): FORCE
	@mkdir -p $(@D)
	@echo '$(VERSION)' | cmp -s - $@ || echo '$(VERSION)' > $@

$(BUILD_DIR)/main.o $(DEBUG_BUILD_DIR)/main.o: $(VERSION_STAMP)
$(BUILD_DIR)/main.o: CFLAGS += -DSECTRC_VERSION='"$(VERSION)"'
$(DEBUG_BUILD_DIR)/main.o: DEBUG_CFLAGS += -DSECTRC_VERSION='"$(VERSION)"'

version:
	@echo $(VERSION)

FORCE:

.PHONY: all debug clean run run_debug version FORCE

# Debug build
$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

debug: $(patsubst $(SRC_DIR)/%.c,$(DEBUG_BUILD_DIR)/%.o,$(SRCS))
	$(CC) $^ -o $(TARGET)_debug $(DEBUG_LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR) $(TARGET) $(TARGET)_debug

# Run the program
run: all
	./$(TARGET)

run_debug: debug
	./$(TARGET)_debug

