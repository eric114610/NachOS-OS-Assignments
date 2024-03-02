#!/bin/bash

./scripts/auto_gen_transformer --input ./tests/01_spec.json

make clean 
scl enable devtoolset-8 'make'

echo testing
./main 4000 ./tests/01.in ./tests/01.out 1
./main 4000 ./tests/01.in ./tests/01.out 2
./main 4000 ./tests/01.in ./tests/01.out 5
./main 4000 ./tests/01.in ./tests/01.out 10
./main 4000 ./tests/01.in ./tests/01.out 20
./main 4000 ./tests/01.in ./tests/01.out 50
./main 4000 ./tests/01.in ./tests/01.out 100
./main 4000 ./tests/01.in ./tests/01.out 200
./main 4000 ./tests/01.in ./tests/01.out 500
./main 4000 ./tests/01.in ./tests/01.out 1000
./main 4000 ./tests/01.in ./tests/01.out 2000
./main 4000 ./tests/01.in ./tests/01.out 4999
