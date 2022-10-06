all: server client

clean:
	rm server client demo

server: server.c
	gcc server.c -o server

client: client.c
	gcc client.c -o client