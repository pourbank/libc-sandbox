#!/bin/bash
set -e

echo "Building Kernel"
cd kernel
make
cd ..

echo "Building LLVM Pass"
cd llvm-passes
make
cd ..

echo "Building Monitor"
cd monitor
make
cd ..

echo "Building File System"
cd kernel
make initramfs
cd ..

echo "Build Completed"