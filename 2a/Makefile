.SILENT:

build: lab2_add.c
	gcc -g -Wall -Werror -pthread lab2_add.c -o lab2_add

clean:	
	rm -rf *.dSYM *~ *tar* lab2_add
#Incomplete
dist: lab2_add.c Makefile README SortedList.c SortedList.h
	tar -czf lab2a-105032378.tar.gz lab2_add.c Makefile README SortedList.c SortedList.h 