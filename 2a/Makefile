#Siddharth Joshi

.SILENT:

default: build

build: lab2_add.c lab2_list.c
	gcc -g -Wall -Werror -pthread lab2_add.c -o lab2_add
	gcc -pthread -g lab2_list.c SortedList.c -o lab2_list

clean:	
	rm -rf *.dSYM *~ *tar* lab2_add lab2_list *.o *.csv *.png

graph: tests
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist: graph lab2_add.c lab2_list.c Makefile README SortedList.c SortedList.h
	tar -czf lab2a-105032378.tar.gz lab2_add.c lab2_list.c Makefile README SortedList.c SortedList.h *.png *.csv *.gp 

tests: clean build
	./lab2_list --threads=1  --iterations=10	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=100	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=1000	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=10000	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=20000	      >> lab2_list.csv
	