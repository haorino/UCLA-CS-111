#Siddharth Joshi
#105032378
.SILENT:

build: lab4b.c 
	gcc -Wall -Wextra -g -lm -lmraa lab4b.c -o lab4b

clean: 
	rm -rf lab4b-105032378.tar.gz lab4b

check:	build
	chmod u=rwx tests.sh 
	./tests.sh

dist: Makefile README lab4b.c
	tar -czf lab4b-105032378.tar.gz lab4b.c README Makefile tests.sh
