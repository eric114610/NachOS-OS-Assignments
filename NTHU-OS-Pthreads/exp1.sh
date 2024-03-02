#!/bin/bash

./scripts/auto_gen_transformer --input ./tests/01_spec.json

make clean 
scl enable devtoolset-8 'make'

echo testing
./main 4000 ./experiment/01.in ./experiment/01.out

echo verify
./scripts/verify --output ./experiment/01.out --answer ./tests/00.ans
