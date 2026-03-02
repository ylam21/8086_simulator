NAME        := simulate8086
CC          := gcc

SRC_DIR     := src
TARGET      := $(NAME)

CFLAGS      := -Wall -Wextra -MMD -MP -g -fsanitize=address
OFLAGS      := -O3 -mpopcnt

OBJ         := src/main.o

all: $(TARGET)

$(TARGET): src/main.c
	$(CC) $(CFLAGS) $(OFLAGS) $^ -o $@

-include $(OBJ:.o=.d)

clean:
	rm -rf $(TARGET)
	rm -rf src/*d src/*.o

re: clean all

.PHONY: all clean re