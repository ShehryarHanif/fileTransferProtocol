all: server client

clean:
	rm server client

server: server.c server_input.h server_users.h server_commands.h
	gcc server.c -o server/s

client: client.c
	gcc client.c -o client/c