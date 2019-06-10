#!/usr/bin/env bash

filters=(3 6 9 12 22)
threads=(1 2 4 8)
modes=(block interleaved)

date > times.txt

for f in ${filters[*]}; do
    for t in ${threads[*]}; do
        for m in ${modes[*]}; do
            ./filterman ${t} ${m} resources/lena.ascii.pgm filters_dir/filter-${f}.txt output/lena_${m}_${f}.ascii.pgm
        done
    done
done