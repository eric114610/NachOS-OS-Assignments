#!/bin/bash

./scripts/auto_gen_transformer --input ./tests/00_spec.json

make clean 
scl enable devtoolset-8 'make'

echo testing
./main 200 ./tests/00.in ./tests/00.out

echo verify
./scripts/verify --output ./tests/00.out --answer ./tests/00.ans
