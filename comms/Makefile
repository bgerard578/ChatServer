PORT?=8888
SERVER?=localhost

.PHONY: all clean test

all: server client threads

server: server.o saferq.c saferq.h
	gcc -o server server.o saferq.c -lpthread

pingser.o: server.c
	gcc -c server.c

client: client.o
	gcc -o client client.o

pingcli.o: client.c
	gcc -c client.c

threads: ping_sq.c saferq.c saferq.h
	gcc -o threads ping_sq.c saferq.c -lpthread

test: server client threads
	@echo ""
	@echo "Testing ping-pong client-server..."
	@echo ""
	./server $(PORT) &
	./client $(SERVER) $(PORT)
	@echo ""
	@echo "Testing ping-pong threaded..."
	@echo ""
	./threads

clean:
	rm *.o server client threads
