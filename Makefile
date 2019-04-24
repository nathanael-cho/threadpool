CC=gcc -std=c99
CFLAGS = -ggdb3 -W -Wall -Wextra -Werror -pthread -O3
LDFLAGS =
LIBS =

default: main

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

main: threadpool.o main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f main *.o *~
