CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: news_broadcasting.out

news_broadcasting.out: news_broadcasting.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f news_broadcasting.out
