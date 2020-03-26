#!/bin/bash

i=0
while [ $((i++)) -lt 30 ]; do
    test -d $i || continue
    printf "avg $i : "
    echo $(echo "scale=2;($(cat $i/*.time | sed 's/$/+/g' | tr -d '\n' | sed 's/+$/\n/'))/$(ls $i/* | wc -l)") | bc
    printf "harmavg $i : "
    python -c "print(round(($(cat $i/*.time | sed 's/$/*/g' | tr -d '\n' | sed 's/\*$/\n/'))**(1.0/$(ls $i/* | wc -l)),2))"
done
