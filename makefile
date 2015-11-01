LIE=-lpthread
CC=gcc

maze: maze.c
	$(CC) maze.c -o maze $(LIE) 
