all: shell

shell: myshell.o utility.o
	cc myshell.o utility.o -o myshell

myshell.o: myshell.c
	cc -c myshell.c 

utility.o: utility.c
	cc -c utility.c

remove:
	rm -rf *o myshell