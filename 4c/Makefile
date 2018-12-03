#Siddharth Joshi
#105032378
.SILENT:

build: tcp tls 

tcp: 
	gcc -Wall -Wextra -g -lm -lmraa lab4c_tcp.c -o lab4c_tcp

tls:
	gcc -Wall -Wextra -g -lm -lmraa -lssl -lcrypto lab4c_tls.c -o lab4c_tls

clean: 
	rm -rf lab4c-105032378.tar.gz lab4c_tls lab4c_tcp *~ *#

dist: Makefile README lab4c_tls.c lab4c_tcp.c
	tar -czf lab4c-105032378.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile 
