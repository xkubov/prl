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
FLAGS="--prefix /usr/local/share/OpenMPI"

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

function parse_input()
{
	test ! -z "$2" && exitWith "just one argument is expected"
	test -z "$1" && exitWith "expected argument"
	test -z "$(echo "$1" | sed -E 's/^([0-9]+,)*[0-9]+$//')" || exitWith "invalid format of input"

	IFS=','
	for i in $1; do
		echo "$i"
	done
}

function compute_cpus()
{
	echo 1
}

function main()
{
	# compilation
	"$CC" "$SRC" "$FLAGS" -o "$OUT" || exitWith "unable to compile file $SRC"
	count="$(compute_cpus "$(parse_input "$@" | wc -l)")"
	parse_input "$@" | "$RUNNER" "$FLAGS" -np "$count" "$OUT"

	# clean
	rm -f $OUT
}

main "$@"
exit 0
