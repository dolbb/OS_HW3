CC=gcc
CFLAGS=-std=c99
LIBS=-lpthread
#EXEC=main
#OBJS=somemain.o hashtable.o

main: hashtable.c hashtable.h test.c
	$(CC) $(CFLAGS) $(LIBS) test.c hashtable.c -o test

#$(EXEC): $(OBJS)
#	$(CC) $(CFLAGS) $(LIBS) $(OBJS) -o $(EXEC)

clean:
#	rm -f $(OBJS)
