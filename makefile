cc = gcc

all: program
# all: old
# all: example

debug: clean program.c obj/InputThread.o 
	$(cc) -g -o bin/program.bin obj/InputThread.o  program.c -lncurses -lpthread

program: clean program.c obj/InputThread.o
	$(cc) -o bin/program.bin obj/InputThread.o program.c -lncurses -lpthread -O3

old: clean obj/InputThread.o	
	$(cc) -o bin/prog.bin obj/InputThread.o -lpthread

obj/Position.o: Position.c
	$(cc) -c Position.c -o obj/Position.o

obj/InputThread.o: InputThread.c
	$(cc) -c InputThread.c -o obj/InputThread.o

example: old/example_Invaders.c
	$(cc) -o bin/example.bin old/example_Invaders.c -lncurses

clean:
	rm -rf ./obj
	rm -rf ./bin
	mkdir -p obj
	mkdir -p bin

