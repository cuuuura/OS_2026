CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wfloat-equal -pedantic
LDLIBS = -lm

all: parent child

parent: ./src/parent.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

child: ./src/child.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: ./src/test.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f parent child

delete_txt:
	rm -f *.txt