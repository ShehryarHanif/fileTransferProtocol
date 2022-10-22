all: server client

clean:
	rm server client

server: server.c server_input.h server_users.h server_commands.h globals.h
	gcc server.c -o server/s

client: client.c server_input.h client_commands.h client_state.h globals.h
	gcc client.c -o client/c