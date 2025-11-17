#!/bin/bash
set -e

cd kernel
make initramfs
cd ../llvm-passes
echo "Building Test"
make
cd ..