#!/bin/bash
cd "$(dirname "$0")"

echo "Building test_model..."
gcc -c ../../dataset/model/model.c -o model.o -lm
gcc -c test_model.c -o test_model.o -lm
gcc model.o test_model.o -o test.model -lm

echo "Running test_model..."
./test.model
