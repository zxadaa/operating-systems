CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: program1 program2 program3

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

program1.o: program1.c common.h
	$(CC) $(CFLAGS) -c program1.c

program1: common.o program1.o
	$(CC) $(CFLAGS) common.o program1.o -o program1 -lm

program2.o: program2.c common.h
	$(CC) $(CFLAGS) -c program2.c

program2: common.o program2.o
	$(CC) $(CFLAGS) common.o program2.o -o program2 -lm

program3.o: program3.c common.h
	$(CC) $(CFLAGS) -c program3.c

program3: common.o program3.o
	$(CC) $(CFLAGS) common.o program3.o -o program3 -lm

clean:
	rm -f *.o program1 program2 program3 robot_log.txt report.txt