#!/bin/bash

i=7
while [ $((i++)) -lt 30 ]; do
    mkdir -p $i

    j=0
    while [ $((j++)) -lt 15 ]; do
        ./test $i > /dev/null 2> $i/$j.time
    done
done
