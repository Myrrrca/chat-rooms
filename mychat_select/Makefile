
all: server client

server: server.c
	gcc -Wformat -Wformat-signedness -std=gnu17 -Wall -pedantic -g server.c errproc.c -lm -o server

client: client.c
	gcc -Wformat -Wformat-signedness -std=gnu17 -Wall -pedantic -g client.c errproc.c -lm -o client

clean:
	rm -f server client
