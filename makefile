all: program
# all: old
# all: example

debug: clean program.c obj/InputThread.o obj/List.o obj/Invader.o
	cc -g -o bin/program.bin obj/InputThread.o obj/List.o obj/Invader.o program.c -lncurses -lpthread

program: clean program.c obj/InputThread.o obj/List.o obj/Invader.o
	cc -o bin/program.bin obj/InputThread.o obj/List.o obj/Invader.o program.c -lncurses -lpthread -O3

old: clean obj/invader.o  obj/List.o obj/InputThread.o	
	cc -o bin/prog.bin obj/invader.o obj/List.o obj/InputThread.o -lpthread

obj/Invader.o: Invader.c
	cc -c Invader.c -o obj/Invader.o

obj/List.o: List.c
	cc -c List.c -o obj/List.o

obj/Position.o: Position.c
	cc -c Position.c -o obj/Position.o

obj/InputThread.o: InputThread.c
	cc -c InputThread.c -o obj/InputThread.o

example: old/example_Invaders.c
	cc -o bin/example.bin old/example_Invaders.c -lncurses

clean:
	rm -rf ./obj
	rm -rf ./bin
	mkdir -p obj
	mkdir -p bin

