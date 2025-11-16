#!/bin/bash
set -e

echo "Cleaning"
cd monitor
make clean
cd ..

cd llvm-passes
make clean
cd ..

cd benchmark
make clean
cd ..

cd kernel
make clean
cd ..

echo "Done"