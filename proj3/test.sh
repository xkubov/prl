#!/bin/bash

#
# This is testing script created for project 3 of PRL class
# on FIT VUT in Brno.
#
# Author: Peter Kubov
# Date: 16. 4. 2020
#

SRC=vid.cpp
OUT=vid
CC=mpic++
RUNNER=mpirun
MPI=/usr/local/share/OpenMPI

function usage()
{
	echo "usage: $0 STRING"
	echo "where:"
	echo "    STRING is string in format num_1,num_2,num_3,...,num_n"
}

function exitWith()
{
	test ! -z "$1" && echo "error:" "$1."
	usage
	exit 1
}

function check_input()
{
	test ! -z "$2" && exitWith "just one argument is expected"
	test -z "$1" && exitWith "expected argument"
	test -z "$(echo "$1" | sed -E 's/^([0-9]+,)*[0-9]+$//')" || exitWith "invalid format of input"
}

function parse_input()
{
	IFS=','
	for i in $1; do
		echo "$i"
	done
}

function compute_cpus()
{
	n=$1
	python <<- END
		from math import log

		p = 2**int(log($n, 2))
		while $n/p < log(p, 2):
		    p = p >> 1

		print(p)
	END
}

function main()
{
	# compilation
	"$CC" "$SRC" --prefix "$MPI" -o "$OUT" || exitWith "unable to compile file $SRC"
	count="$(compute_cpus "$(parse_input "$@" | wc -l)")"
	check_input "$@"

	parse_input "$@" | "$RUNNER" --prefix "$MPI" -np "$count" "$OUT"

	# clean
	rm -f $OUT
}

main "$@"
exit 0
