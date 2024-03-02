#!/bin/bash

./scripts/auto_gen_transformer --input ./tests/01_spec.json

make clean 
scl enable devtoolset-8 'make'

echo testing
./main 4000 ./tests/01.in ./tests/01.out 10 80
./main 4000 ./tests/01.in ./tests/01.out 20 80
./main 4000 ./tests/01.in ./tests/01.out 30 80
./main 4000 ./tests/01.in ./tests/01.out 40 80
./main 4000 ./tests/01.in ./tests/01.out 50 80
./main 4000 ./tests/01.in ./tests/01.out 60 80
./main 4000 ./tests/01.in ./tests/01.out 70 80
./main 4000 ./tests/01.in ./tests/01.out 80 80

./main 4000 ./tests/01.in ./tests/01.out 20 20
./main 4000 ./tests/01.in ./tests/01.out 20 30
./main 4000 ./tests/01.in ./tests/01.out 20 40
./main 4000 ./tests/01.in ./tests/01.out 20 50
./main 4000 ./tests/01.in ./tests/01.out 20 60
./main 4000 ./tests/01.in ./tests/01.out 20 70 
./main 4000 ./tests/01.in ./tests/01.out 20 80
./main 4000 ./tests/01.in ./tests/01.out 20 90 
