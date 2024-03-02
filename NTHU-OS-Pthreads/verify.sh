#!/bin/bash
echo verify00
./scripts/verify --output ./tests/00.out --answer ./tests/00.ans
echo 
echo 
echo verify01
./scripts/verify --output ./tests/01.out --answer ./tests/01.ans
