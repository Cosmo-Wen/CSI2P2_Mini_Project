#!/usr/bin/env bash

TESTDIR="testcase/debug"
CESTR="Compile Error!"

gcc -Wall help.c -o optimized

for FILE in $TESTDIR/*; do
	echo "====== $FILE";
	X=$(($RANDOM % 200 - 100))
	Y=$(($RANDOM % 200 - 100))
	Z=$(($RANDOM % 200 - 100))
	# try ASMC
	if [ "$(cat $FILE | ./optimized)" != "$CESTR" ]; then
		ASMRES=$(cat $FILE | ./optimized | ./ASMC $X $Y $Z)
		if [ "$ASMRES" == "CE instruction found." ]; then
			ASMRES="$CESTR (CE instruction found.)"
		fi
	else
		ASMRES="Compile Error!"
	fi
	# try gcc compilation
	echo "#include <stdio.h>" > test.c
	echo "int main() { int x=$X, y=$Y, z=$Z; " >> test.c
	cat $FILE >> test.c
	echo "printf(\"x, y, z = %d, %d, %d\\n\", x, y, z); return 0; }" >> test.c
	gcc -Wall test.c -o test 2> /dev/null
	if [ $? -eq 0 ]; then
		GCCRES=$(./test)
	else
		GCCRES=$CESTR
	fi
	# compare results
	echo "GCC:  $GCCRES"
	echo "ASMC: $ASMRES"
	if [[ "$GCCRES" == "$CESTR" ]] && [[ "$ASMRES" == "$CESTR"* ]]; then # both failed
		continue
	elif [ "$(echo $ASMRES | cut -d ' ' -f 1-7)" != "$GCCRES" ]; then
		echo " PANIC!"
		exit 1
	fi
done
rm -f test.c test
