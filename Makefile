all: server client

clean:
	rm server client demo

server: server.c server_input.h server_users.h server_commands.h
	gcc server.c -o server

client: client.c
	gcc client.c -o client