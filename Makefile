CC = gcc
CFLAGS = -Wall -std=c99
TARGET = part1_server
SOURCES = game_server_core.c main_server.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

game_server_core.o: game_server_core.c game_server_core.h
	$(CC) $(CFLAGS) -c game_server_core.c

main_server.o: main_server.c game_server_core.h
	$(CC) $(CFLAGS) -c main_server.c

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run