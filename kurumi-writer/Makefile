PROG=kurumi
CFLAGS=-Wall

all: $(PROG)

$(PROG).o: $(PROG).c
	gcc -c $(PROG).c $(CFLAGS)

$(PROG): $(PROG).o
	gcc -o $(PROG) $(PROG).o $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o $(PROG)

