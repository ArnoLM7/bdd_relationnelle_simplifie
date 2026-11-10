CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude
SRC    = $(wildcard src/*.c)

mydb: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o mydb

clean:
	rm -f mydb