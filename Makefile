NAME        := simulate8086
CC          := gcc

SRC_DIR     := src_new
BUILD_DIR   := build
BIN_DIR     := bin

TARGET      := $(BIN_DIR)/$(NAME)

INC_DIRS    := $(shell find $(SRC_DIR) -type d)
CFLAGS      := -Wall -Wextra -Werror -MMD -MP -g -fsanitize=address $(addprefix -I,$(INC_DIRS))

SRCS        := $(shell find $(SRC_DIR) -name "*.c")
OBJS        := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

re: clean all

-include $(OBJS:.o=.d)

.PHONY: all clean re