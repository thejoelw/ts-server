#!/bin/sh

# sudo apt-get install tup
# sudo apt-get install libfftw3-dev

tup init 2> /dev/null
for file in configs/*.config; do
	tup variant "$file" 2> /dev/null
done

nice tup
