nearthworm: nearthworm.o
	gcc -o $@ -lncurses nearthworm.o

nearthworm-dbg: nearthworm-dbg.o
	gcc -o $@ -lncurses nearthworm.o

nearthworm.o: nearthworm.c
	gcc -c nearthworm.c

nearthworm-dbg.o: nearthworm.c
	gcc -g -c nearthworm.c

all: nearthworm

clean:
	if [ -e nearthworm.o ] ; then rm nearthworm.o; fi
	if [ -e nearthworm ] ; then rm nearthworm; fi
