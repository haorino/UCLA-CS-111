#Siddharth Joshi

.SILENT:

default: build

build: lab2_list.c SortedList.c
	gcc -Wall -Wextra -Werror -pthread -lprofiler -g lab2_list.c SortedList.c -o lab2_list

clean:	
	rm -rf *tar* lab2_list  *.o *.dSYM DS_Store *~ 

graphs: tests
	set datafile separator ","
	gnuplot lab2_list.gp

dist: graphs profile lab2_list.c Makefile README SortedList.c SortedList.h
	tar -czf lab2b-105032378.tar.gz lab2_list.c Makefile README SortedList.c SortedList.h *.png *.csv *.gp profile.out

profile:
	rm -rf ./raw.gperf
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=timedLock ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

tests: clean build
	./lab2_list --iterations=1000 --sync=m --threads=1 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=1 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=m --threads=4 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=4 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=m --threads=8 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=8 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=m --threads=12 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=12 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=m --threads=16 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=16 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=m --threads=24 >> lab2b_list.csv
	./lab2_list --iterations=1000 --sync=s --threads=24 >> lab2b_list.csv 
	-./lab2_list --yield=id --lists=4 --threads=1 --iterations=1 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=1 --iterations=2 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=1 --iterations=4 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=1 --iterations=8 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=1 --iterations=16 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=4 --iterations=1 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=4 --iterations=2 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=4 --iterations=4 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=4 --iterations=8 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=4 --iterations=16 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=8 --iterations=1 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=8 --iterations=2 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=8 --iterations=4 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=8 --iterations=8 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=8 --iterations=16 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=12 --iterations=1 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=12 --iterations=2 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=12 --iterations=4 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=12 --iterations=8 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=12 --iterations=16 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=16 --iterations=1 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=16 --iterations=2 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=16 --iterations=4 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=16 --iterations=8 >> lab2b_list.csv
	-./lab2_list --yield=id --lists=4 --threads=16 --iterations=16 >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=10 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=10 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=20 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=20 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=40 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=40 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=80 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=1 --iterations=80 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=10 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=10 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=20 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=20 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=40 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=40 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=80 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=4 --iterations=80 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=10 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=10 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=20 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=20 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=40 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=40 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=80 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=8 --iterations=80 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=10 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=10 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=20 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=20 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=40 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=40 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=80 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=12 --iterations=80 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=10 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=10 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=20 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=20 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=40 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=40 --sync=s >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=80 --sync=m >> lab2b_list.csv
	./lab2_list --yield=id --lists=4 --threads=16 --iterations=80 --sync=s >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=1 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=1 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=1 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=1 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=1 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=1 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=2 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=2 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=2 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=2 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=2 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=2 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=4 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=4 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=4 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=4 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=4 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=4 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=8 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=8 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=8 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=8 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=8 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=8 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=12 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=12 --lists=4 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=12 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=12 --lists=8 >> lab2b_list.csv
	./lab2_list --sync=m --iterations=1000 --threads=12 --lists=16 >> lab2b_list.csv
	./lab2_list --sync=s --iterations=1000 --threads=12 --lists=16 >> lab2b_list.csv
