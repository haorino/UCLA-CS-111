CC=gcc
CFLAGS=-Wall -Wextra -g

.SILENT:

build: lab3a.c
	$(CC) $(CFLAGS) -o lab3a lab3a.c -lm

clean: 
	rm -rf lab3a *.dSYM lab3a-504969404.tar.gz *~

dist:
	tar -czf lab3a-504969404.tar.gz README Makefile lab3a.c ext2_fs.h
