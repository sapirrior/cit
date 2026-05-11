CC = gcc
CFLAGS = -Wall -g \
         -Isrc \
         -Isrc/core/object \
         -Isrc/core/index \
         -Isrc/core/hash \
         -Isrc/core/config \
         -Isrc/core/refs \
         -Isrc/commands \
         -Isrc/core/compress \
         -Isrc/utils \
         -Isrc/ui
LDFLAGS = -lcurl

SRC = $(shell find src -name "*.c")
BUILD_DIR = build
OBJDIR = $(BUILD_DIR)/obj
OBJ = $(SRC:src/%.c=$(OBJDIR)/%.o)
TARGET = $(BUILD_DIR)/cit

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
