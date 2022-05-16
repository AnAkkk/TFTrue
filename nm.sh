#!/bin/bash
find . -name '*.a' > /tmp/nmnm

while read -r line
do
	echo ""
	echo $line
	nm $line
done < <(tr -d "\r" < /tmp/nmnm)
