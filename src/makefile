CC=gcc
CFLAGS= -g -Wall -I.



main: shellex.c csapp.c
	gcc -Wall -o shellex -pthread shellex.c csapp.c

run: fork_exec_pt2 fork_exec giveme

fork_exec_pt2: fork_exec_pt2.c
	$(CC) -o  fork_exec_pt2 fork_exec_pt2.c

fork_exec: fork_exec.c
	$(CC) -o  fork_exec fork_exec.c

giveme: giveme.c
	$(CC) -o  giveme giveme.c

cleanexec: 
	rm fork_exec fork_exec_pt2 giveme


cleanlists:
	rm cookies.list pizza.list
