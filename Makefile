CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
TARGET = filemaster

SOURCES = src/main.c \
          src/menu.c \
          src/directory_operations.c \
          src/file_editor.c \
          src/file_operations.c \
          src/search.c \
          src/permission.c \
          src/tree_view.c

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
