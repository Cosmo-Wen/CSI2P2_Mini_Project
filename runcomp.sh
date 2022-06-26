#!/usr/bin/env bash

TESTDIR="testcase"
CESTR="Compile Error!"

gcc -Wall optimized_v2.c -o optimized_v2
gcc -Wall optimized.c -o optimized
gcc -Wall mini.c -o mini

for FILE in $TESTDIR/*; do
	echo "====== $FILE";
	X=$(($RANDOM % 200 - 100))
	Y=$(($RANDOM % 200 - 100))
	Z=$(($RANDOM % 200 - 100))
	# try optimized_v2 ASMC
	if [ "$(cat $FILE | ./optimized_v2)" != "$CESTR" ]; then
		VASMRES=$(cat $FILE | ./optimized_v2 | ./ASMC $X $Y $Z)
		if [ "$VASMRES" == "CE instruction found." ]; then
			VASMRES="$CESTR (CE instruction found.)"
		fi
	else
		VASMRES="Compile Error!"
	fi
	# try optimized ASMC
	if [ "$(cat $FILE | ./optimized)" != "$CESTR" ]; then
		OASMRES=$(cat $FILE | ./optimized | ./ASMC $X $Y $Z)
		if [ "$OASMRES" == "CE instruction found." ]; then
			OASMRES="$CESTR (CE instruction found.)"
		fi
	else
		OASMRES="Compile Error!"
	fi
	# try old ASMC
	if [ "$(cat $FILE | ./mini)" != "$CESTR" ]; then
		MASMRES=$(cat $FILE | ./mini | ./ASMC $X $Y $Z)
		if [ "$MASMRES" == "CE instruction found." ]; then
			MASMRES="$CESTR (CE instruction found.)"
		fi
	else
		MASMRES="Compile Error!"
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
	echo "GCC: $GCCRES"
	echo "Optimized_v2: ASMC: $VASMRES" 
	echo "Optimized ASMC: $OASMRES"
	echo "Original ASMC: $MASMRES"
	if [[ "$GCCRES" == "$CESTR" ]] && [[ "$OASMRES" == "$CESTR"* ]] && [[ "$MASMRES" == "$CESTR"* ]] && [[ "$VASMRES" == "$CESTR"* ]]; then # both failed
		continue
	elif [ "$(echo $MASMRES | cut -d ' ' -f 1-7)" != "$GCCRES" ]; then
		echo "Original Error!"
		exit 1
	elif [ "$(echo $OASMRES | cut -d ' ' -f 1-7)" != "$GCCRES" ]; then
		echo "Optimized Error!"
		exit 1
	elif [ "$(echo $VASMRES | cut -d ' ' -f 1-7)" != "$GCCRES" ]; then
		echo "Optimized_v2 Error!"
		exit 1
	fi
done
rm -f test.c test
