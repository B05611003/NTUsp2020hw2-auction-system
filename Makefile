all: host player

host: host.c
	gcc -o host -Wall host.c 
player: player.c
	gcc -o player -Wall player.c 