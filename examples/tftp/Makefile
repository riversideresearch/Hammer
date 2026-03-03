CC = gcc
CFLAGS = -g $(shell pkg-config --cflags libhammer)
LDFLAGS = $(shell pkg-config --libs libhammer)
TARGET = tftp
SRC = main.c tftp.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
