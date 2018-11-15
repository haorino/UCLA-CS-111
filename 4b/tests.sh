#!/bin/sh                                                                                                                                                                                                   

echo "Smoke Tests started:"

#Check Bogus Argument                                                                                                                                                                                       
echo | ./lab4b --bogus &> /dev/null; \
if [[ $? -ne 1 ]]; then \
echo "Failed: --bogus argument returns incorrect exit code"; \
else \
echo "Passed: --bogus  returns correct exit code"; \
fi

#Check LogFile Creation                                                                                                                                                                                     
./lab4b --period=2 --scale="C" --log="LOGFILE" <<-EOF                                                                                                                                                       
SCALE=C                                                                                                                                                                                                     
STOP                                                                                                                                                                                                        
START                                                                                                                                                                                                       
SCALE=F                                                                                                                                                                                                     
PERIOD=2                                                                                                                                                                                                    
OFF                                                                                                                                                                                                         
EOF                                                                                                                                                                                                         
returnValue=$?

ret=$?
if [ $ret -ne 0 ]
then
        echo "Failed: Incorrect return code"
fi

if [ ! -s LOGFILE ]
then
        echo "Failed: Logfile not created"
else
        echo "Passed: Logfile created"
        for c in SCALE=C STOP START SCALE=F PERIOD=2 OFF
        do
                grep "$c" LOGFILE > /dev/null
                if [ $? -ne 0 ]
                then
                        echo "Failed: Logfile doesn't contain $c command"
                else
                        echo "Passed: Logfile contains $c command"
                fi
        done
fi                                                                                                                                                                    

