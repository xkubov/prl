#!/bin/bash

i=0
while [ $((i++)) -lt 30 ]; do
    printf "$i:"
    echo $(echo "scale=2;($(cat $i/*.time | sed 's/$/+/g' | tr -d '\n' | sed 's/+$/\n/'))/$(ls $i/* | wc -l)") | bc
done
