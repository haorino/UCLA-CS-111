#MakeFile for Project 1A
#Siddharth Joshi - 105032378

.SILENT:


default: lab1b-client.c lab1b-server.c SafeSysCalls.c
	gcc -lmcrypt -Wall -Wextra lab1b-client.c SafeSysCalls.c  -o lab1b-client
	gcc -lmcrypt -Wall -Wextra lab1b-server.c SafeSysCalls.c  -o lab1b-server
clean:
	rm -rf lab1b-client lab1b-server tar* *~ *.dSYM *.o *.so

dist:
	tar -czf lab1b-105032378.tar.gz lab1b* SafeSysCalls* UtilityFunctions* Constants.h  README Makefile 
