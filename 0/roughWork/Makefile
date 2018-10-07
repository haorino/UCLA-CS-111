test = Hello World

.SILENT:

default: 
	gcc -Wall -Wextra -g -o lab0 lab0.c

check: | default ioTest succesfulExitTest segFaultTest faultyInputTest faultyOutputTest unknownArgTest
	if [ -s eval.txt ]; then \
	echo "Error Report"; \
	cat eval.txt; \
	rm -f eval.txt input.txt output.txt; \
	else \
	echo "All tests passed"; \
	fi
dist:
	tar -czvf lab0-105032378.tar.gz lab0.c README Makefile backtrace.png breakpoint.png

clean:
	rm -R -f lab0 lab0-105032378.tar.gz *.txt *.o *.d*

ioTest: 
	echo $test > input.txt; \
	./lab0 --input=input.txt --output=output.txt; \
	if cmp -s input.txt output.txt; then \
	echo "Passed basic input output comparison test"; \
	else \
	echo "Failed input output comparison test" >> eval.txt; \
	fi	

succesfulExitTest:
	if ./lab0 --input=input.txt --output=output.txt; then \
	echo "Passed successful exit test"; \
        else \
        echo "Returned incorrect exit code on successful execution" >> eval.txt; \
        fi

segFaultTest:
	./lab0 --input=input.txt --output=output.txt --segfault --catch > /dev/null 2>&1; \
	if [ $$?==4 ]; then \
	echo "Passed segmentation fault creation and handling test"; \
	else \
	echo "Failed segmentation fault and catch test: exited with incorrect code" >> eval.txt; \
	fi 

faultyInputTest:
	touch faultyInput.txt; \
	chmod u-r faultyInput.txt; \
	./lab0 --input=faultyInput.txt> /dev/null 2>&1; \
	if [ $$?==2 ]; then \
	echo "Passed unreadable input file test"; \
	else \
	echo "Failed unreadable input file test: exited with incorrect code" >> eval.txt; \
	fi

faultyOutputTest:
	touch faultyOutput.txt; \
	chmod u-w faultyOutput.txt; \
	./lab0 --input=input.txt --output=faultyOutput.txt > /dev/null 2>&1; \
	if [ $$?==3 ]; then \
	echo "Passed unwritiable output file test"; \
	else \
	echo "Failed unwritable output file test: exited with incorrect code" >> eval.txt; \
	fi

unknownArgTest:
	./lab0 --unknown> /dev/null 2>&1; \
	if [ $$?==1 ]; then \
	echo "Passed unknown argument test"; \
	else \
	echo "Failed unknown argument test: did not successfully recognize as unknown" >> eval.txt; \
	fi 