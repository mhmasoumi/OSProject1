all: server.o client.o


server.o: server.c
	gcc server.c -o server.o

client.o: client.c
	gcc client.c -o client.o

clean:
	-rm -f *.o