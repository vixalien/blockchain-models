CC = gcc
CFLAGS = -Wall -Wextra -g -I src
LDFLAGS =

SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
TARGET = blockchain

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
