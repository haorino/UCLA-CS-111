for n in 1 4 8 12 16 24 ; do \
    echo ./lab2_list --iterations=1000 --sync=m --threads=$n  ; \
    echo ./lab2_list --iterations=1000 --sync=s --threads=$n  ; \
    done

for n in 1 4 8 12 16 ; do \
    for i in 1 2 4 8 16 ; do \
    echo ./lab2_list --yield=id --lists=4 --threads=$n --iterations=$i  || true ; \
    done ; \
    done
for n in 1 4 8 12 16 ; do \
    for i in 10 20 40 80 ; do \
    echo ./lab2_list --yield=id --lists=4 --threads=$n --iterations=$i --sync=m  ; \
    echo ./lab2_list --yield=id --lists=4 --threads=$n --iterations=$i --sync=s  ; \
    done ; \
    done

for n in 1 2 4 8 12 ; do \
    for l in 4 8 16 ; do \
    echo ./lab2_list --sync=m --iterations=1000 --threads=$n --lists=$l  ; \
    echo ./lab2_list --sync=s --iterations=1000 --threads=$n --lists=$l  ; \
    done ; \
    done