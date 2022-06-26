#!/usr/bin/env bash

TESTDIR="testcase"
CESTR="Compile Error!"

for FILE in $TESTDIR/*; do
	echo "====== $FILE";
	X=$(($RANDOM % 200 - 100))
	Y=$(($RANDOM % 200 - 100))
	Z=$(($RANDOM % 200 - 100))
	# try gcc compilation
	echo "#include <stdio.h>" > test.c
	echo "int main() { int x=$X, y=$Y, z=$Z; " >> test.c
	cat $FILE >> test.c
	echo "printf(\"x, y, z = %d, %d, %d\\n\", x, y, z); return 0; }" >> test.c
	gcc -Wall test.c -o test > /dev/null
	if [ $? -eq 0 ]; then
		GCCRES=$(./test)
	else
		GCCRES=$CESTR
	fi
	# compare results
	echo "GCC:  $GCCRES"
done
rm -f test.c test
