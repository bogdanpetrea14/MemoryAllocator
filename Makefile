CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

objects := $(patsubst %.c,%.o,$(wildcard *.c))

build: vma

vma: $(objects)
		$(CC) $(CFLAGS) $^ -o $@ -lm

%.o: %.c
		$(CC) $(CFLAGS) -c $^ -lm

clean:
		rm -rf *.o vma

.PHONY: clean pack
