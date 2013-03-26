CC = gcc
CFLAGS = -c -g -lgmp -Wall -Wextra
LFLAGS = -lgmp -Wall -Wextra

.PHONY: all clean

all: pi pi-sched test-sched junkdata

pi: pi.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

pi-sched: pi-sched.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

test-sched: test-sched.o
	$(CC) $(LFLAGS) $^ -o $@ -lm
junkdata: Makefile
	dd if=/dev/urandom of=junkdata bs=1M count=100

pi.o: pi.c
	$(CC) $(CFLAGS) $<

pi-sched.o: pi-sched.c
	$(CC) $(CFLAGS) $<

test-sched.o: test-sched.c 
	$(CC) $(CFLAGS) $<

clean:
	rm -f pi pi-sched test-sched
	rm -f *.o
	rm -f *~
	rm -f junkdata
	rm -f handout/*~
	rm -f handout/*.log
	rm -f handout/*.aux
