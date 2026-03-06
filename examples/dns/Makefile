CC = gcc
CFLAGS = -g -Wall -I. $(shell pkg-config --cflags libhammer)
LDFLAGS = $(shell pkg-config --libs libhammer)

TARGET = dns_parser
SRCS = main.c dns.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c dns.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)