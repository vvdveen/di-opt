CC     = gcc
CFLAGS = -g -Wall

all: target

target: target.o
	$(CC) $(CLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o target

