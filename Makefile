nsnake: nsnake.o
	gcc nsnake.o -lncurses -o $@

nsnake-dbg: nsnake-dbg.o
	gcc nsnake.o -DDEBUG -lncurses -o $@

nsnake.o: nsnake.c
	gcc -c nsnake.c

nsnake-dbg.o: nsnake.c
	gcc -g -c nsnake.c

all: nsnake

clean:
	if [ -e nsnake.o ] ; then rm nsnake.o; fi
	if [ -e nsnake ] ; then rm nsnake; fi
	if [ -e nsnake-dbg ] ; then rm nsnake-dbg; fi
