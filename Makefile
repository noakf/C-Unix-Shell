all: myshell looper mypipeline

myshell: myshell.o LineParser.o
	gcc -Wall -g -m32 -o myshell myshell.o LineParser.o

myshell.o: myshell.c LineParser.h
	gcc -Wall -g -m32 -c myshell.c

LineParser.o: LineParser.c LineParser.h
	gcc -Wall -g -m32 -c LineParser.c

looper: Looper.c
	gcc -Wall -g -m32 -o looper Looper.c

mypipeline: mypipeline.c
	gcc -Wall -g -m32 -o mypipeline mypipeline.c

clean:
	rm -f *.o myshell looper mypipeline
