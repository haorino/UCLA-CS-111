#MakeFile for Project 1A
#Siddharth Joshi - 105032378

.SILENT:


default: lab1a.c
	gcc -Wall -Wextra -g -o lab1a lab1a.c
clean:
	rm -rf lab1a *.tar* *~ *.dSYM *.o *.so

dist:
	tar -czf lab1a-105032378.tar.gz lab1a.c README Makefile 
