CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = lexer
OBJECTS = main.o lexer.o frequency.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

main.o: main.c lexer.h frequency.h
	$(CC) $(CFLAGS) -c main.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

frequency.o: frequency.c frequency.h lexer.h
	$(CC) $(CFLAGS) -c frequency.c

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
