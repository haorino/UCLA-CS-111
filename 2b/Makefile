#Siddharth Joshi

.SILENT:

default: build

build: lab2_list.c SortedList.c
	gcc -Wall -Wextra -Werror -pthread -g lab2_list.c SortedList.c -lrt -o lab2_list

clean:	
	rm -rf *tar* lab2_list  *.o *.dSYM DS_Store *~ 

graphs: tests
	set datafile separator ","
	gnuplot lab2_list.gp

dist: graphs lab2_list.c Makefile README SortedList.c SortedList.h
	tar -czf lab2b-105032378.tar.gz lab2_list.c Makefile README SortedList.c SortedList.h *.png *.csv *.gp 

tests: clean build listTests

listTests: singleThreadList unprotectedList protectedList

unprotectedList: regular yieldI yieldD yieldIL yieldDL

singleThreadList:
	./lab2_list --threads=1  --iterations=10	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=100	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=1000	      >> lab2_list.csv
	./lab2_list --threads=1  --iterations=10000	      >> lab2_list.csv

regular:
	-./lab2_list --threads=2  --iterations=1              >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=10             >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=100            >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=1000           >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1              >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=10             >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=100            >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1000           >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1              >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=10             >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=100            >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1000           >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1              >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=10             >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=100            >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1000           >> lab2_list.csv 2>/dev/null 

yieldI:
	-./lab2_list --threads=2  --iterations=1   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=2   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=4   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=8   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=16  --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=32  --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=2   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=4   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=8   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=16  --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=2   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=4   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=8   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=16  --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=2   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=4   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=8   --yield=i  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=16  --yield=i  >> lab2_list.csv 2>/dev/null 

yieldD:
	-./lab2_list --threads=2  --iterations=1   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=2   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=4   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=8   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=16  --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=32  --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=2   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=4   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=8   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=16  --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=2   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=4   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=8   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=16  --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=2   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=4   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=8   --yield=d  >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=16  --yield=d  >> lab2_list.csv 2>/dev/null 

yieldIL:
	-./lab2_list --threads=2  --iterations=1   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=2   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=4   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=8   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=16  --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=32  --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=2   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=4   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=8   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=16  --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=2   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=4   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=8   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=16  --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=2   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=4   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=8   --yield=il >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=16  --yield=il >> lab2_list.csv 2>/dev/null 

yieldDL:
	-./lab2_list --threads=2  --iterations=1   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=2   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=4   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=8   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=16  --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=2  --iterations=32  --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=1   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=2   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=4   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=8   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=4  --iterations=16  --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=1   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=2   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=4   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=8   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=8  --iterations=16  --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=1   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=2   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=4   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=8   --yield=dl >> lab2_list.csv 2>/dev/null 
	-./lab2_list --threads=12 --iterations=16  --yield=dl >> lab2_list.csv 2>/dev/null 

protectedList:
	./lab2_list --threads=12 --iterations=32 --yield=i  --sync=m >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=d  --sync=m >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=il --sync=m >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=dl --sync=m >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=i  --sync=s >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=d  --sync=s >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=il --sync=s >> lab2_list.csv
	./lab2_list --threads=12 --iterations=32 --yield=dl --sync=s >> lab2_list.csv
	./lab2_list --threads=1  --iterations=1000          >> lab2_list.csv
	./lab2_list --threads=1  --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=2  --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=4  --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=8  --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=m >> lab2_list.csv
	./lab2_list --threads=1  --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=2  --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=4  --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=8  --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=s >> lab2_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=s >> lab2_list.csv
