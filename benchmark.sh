#!/bin/bash
set -e

echo "Building Mbed-TLS"
cd benchmark
make
cd ..