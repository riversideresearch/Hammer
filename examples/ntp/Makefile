CC = gcc
CFLAGS = -g $(shell pkg-config --cflags libhammer)
LDFLAGS = $(shell pkg-config --libs libhammer)
TARGET = ntp_parser
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

# Real NTP packet captured
testp: $(TARGET)
	cat packet/ntp_stream.bin | ./$(TARGET)
