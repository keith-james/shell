CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -Iinclude

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET = shell

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: clean

run: $(TARGET)
	./$(TARGET)
