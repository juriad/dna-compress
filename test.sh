#!/bin/bash

rm -rf sandbox

cd Debug
make all
cd ..

name="`basename "$1"`"
mkdir -p sandbox
cp "$1" "sandbox/$name-in"
cd sandbox

time ../Debug/bio-project ./$name-in ./$name-c | tee log1
time ../Debug/bio-project ./$name-c ./$name-out | tee log2
python -c "print(`du -b "$name-c" | sed 's/\s.*//'` / `du -b "$name-in" | sed 's/\s.*//'` * 8)"

cat "$name-in" | tr -d '\n\r' | tr -t 'acgt' 'ACGT' > "$name-in-lin"
cat "$name-out" | tr -d '\n\r' | tr -t 'acgt' 'ACGT' > "$name-out-lin"

diff -q "./$name-in-lin" "./$name-out-lin"
