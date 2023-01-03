all: file_manager file_client

file_manager: file_manager.c
	gcc -o file_manager file_manager.c

file_client: file_client.c
	gcc -o file_client file_client.c


