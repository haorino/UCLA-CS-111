.SILENT:

build:
	make clean
	touch lab3b
	chmod +x lab3b
	echo '#!/bin/bash' > lab3b
	echo 'python main.py "$$@"' >> lab3b
	echo 'EXIT_CODE=$$?' >> lab3b
	echo 'exit $$EXIT_CODE' >> lab3b
	echo "Success"
dist:
	tar -czvf lab3b-504969404.tar.gz README Makefile main.py
clean:
	rm -f lab3b-504969404.tar.gz lab3b
