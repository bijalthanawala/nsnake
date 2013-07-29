nearthworm: nearthworm.o
	gcc -o $@ -lncurses nearthworm.o

nearthworm.o: nearthworm.c
	gcc -c nearthworm.c

all: nearthworm

clean:
	if [ -e nearthworm.o ] ; then rm nearthworm.o; fi
	if [ -e nearthworm ] ; then rm nearthworm; fi
