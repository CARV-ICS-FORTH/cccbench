#!/bin/bash

XSTART=0
XSTOP=10
YSTART=0
YSTOP=10
ITERATIONS=10

OUTFILE=example.csv

#touch ${OUTFILE}

echo "xcore,ycore,iteration,cost,cost_rev,j,j_rev" > ${OUTFILE}

for (( I=0; I<=${ITERATIONS}; I++ ))
do
echo "$I"
sleep 10
for (( X=${XSTART}; X<=${XSTOP}; X++ ))
do
    for (( Y=${YSTART}; Y<=${YSTOP}; Y++ ))
    do
        if [[ $Y != $X ]]
        then
                RESULT=$(numactl --cpunodebind=0 --localalloc ./tc2c $X $Y)
                echo "$X,$Y,$I,${RESULT}" >> ${OUTFILE}
        fi
    done
done
done


