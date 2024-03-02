#!/bin/bash

./scripts/auto_gen_transformer --input ./tests/01_spec.json

make clean 
scl enable devtoolset-8 'make'

echo testing
./main 4000 ./tests/01.in ./tests/01.out 10000
./main 4000 ./tests/01.in ./tests/01.out 100000
./main 4000 ./tests/01.in ./tests/01.out 1000000
./main 4000 ./tests/01.in ./tests/01.out 2000000
./main 4000 ./tests/01.in ./tests/01.out 4000000
./main 4000 ./tests/01.in ./tests/01.out 5000000
./main 4000 ./tests/01.in ./tests/01.out 10000000
