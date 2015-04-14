proj = program2
flags = -g -std=gnu99
comp = gcc

all: Driver.o
	$(comp) $(flagS) -o $(proj) Driver.o -lpthread -lrt

Driver.o:
	$(comp) $(flags) -c Driver.c

clean:
	rm *.o $(proj)
