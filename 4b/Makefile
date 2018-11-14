#Siddharth Joshi
#105032378
.SILENT:

build: lab4b.c 
	gcc -Wall -Wextra -g -lmraa lab4b.c -o lab4b

clean: 
	rm -rf lab4b-105032378.tar.gz lab4b

check:	
	echo Checks initiated

dist: Makefile README lab4b.c 
	tar -czf lab4b-105032378.tar.gz lab4b.c README lab4b.c