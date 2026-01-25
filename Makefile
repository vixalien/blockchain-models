CC = gcc
CFLAGS = -Wall -Wextra -g -I src
LDFLAGS = -lssl -lcrypto

SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
TARGET = todo

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -rf data/

run: $(TARGET)
	./$(TARGET)
