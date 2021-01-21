all: 
	gcc -g -Wall -no-pie -o bomberman bomberman.o player.c -lm
