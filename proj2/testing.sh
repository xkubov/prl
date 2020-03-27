#!/bin/bash

i=0
while [ $((i++)) -lt 25 ]; do
    mkdir -p $i

    j=0
    while [ $((j++)) -lt 32 ]; do
        printf "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r"
        printf "                                "
        printf "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r"
        printf "($i/20) $j/32"
        ./test $i > /dev/null 2> $i/$j.time
    done
done
