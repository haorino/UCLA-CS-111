#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (insert + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#   8. per operation locking time (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists
#   lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

# Throughput vs Threads
set title "ScalableParallelism-1: Throughput of lists synchronized concurrency (using custom spin locks and mutexes) v/s Number of Threads"
set xlabel "Number of Threads"
set ylabel "Throughput (operations carried out per second)"
set xrange [0.75:]
set logscale x 2
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
        title 'list insert/lookup/delete w/mutex' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'list insert/lookup/delete w/spin' with linespoints lc rgb 'red'

# Mutex List Per Operation Time
set title "ScalableParallelism-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "Number of Threads"
set ylabel "Mean Time Per Operation (nanoseconds)"
set xrange [0.75:]
set logscale x 2
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):($7) \
        title 'completion time' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):($8) \
        title 'wait for lock' with linespoints lc rgb 'red'

set title "ScalableParallelism-3: Successful Iterations using SubLists (Unprotected and Protected List Operations)"
set xrange [0.75:17]
set yrange [0.75:100]
set xlabel "Number of Threads"
set ylabel "Number of Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3)\
	with points lc rgb "blue" title "Unprotected", \
    "< grep 'list-id-m,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "red" title "Mutex", \
    "< grep 'list-id-s,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "Spin-Lock"

set title "ScalableParallelism-4: Throughput using SubLists and Mutex-Protected List Operations"
set xlabel "Number of Threads"
set ylabel "Throughput (operations carried out per second)"
set logscale x 2
set logscale y
unset xrange
set xrange [0.75:]
unset yrange
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9][2]\\?,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,4,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,8,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,16,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=16' with linespoints lc rgb 'orange'

set title "ScalableParallelism-5: Throughput using SubLists and Spin Lock List Operations"
set xlabel "Number of Threads"
set ylabel "Throughput (operations carried out per second)"
set logscale x 2
set logscale y
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,[0-9][2]\\?,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,4,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,8,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,16,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	    title 'lists=16' with linespoints lc rgb 'orange'