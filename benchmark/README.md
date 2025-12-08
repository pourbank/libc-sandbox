# mbed-tls Benchmark

This directory handles the compilation of *mbed-tls*, a real-world cryptographic library, to evaluate the effectiveness and performance of the Liberator sandbox.

## Setup

The `Makefile` automates the following steps:

##### **1.  Download:** Clones specific version of `mbed-tls`.
##### **2.  Instrumentation:** Compiles the library and the benchmark application using `clang` with our custom LLVM plugins loaded:
    -fpass-plugin=../llvm-passes/build/InstrumentPass.so
    -fpass-plugin=../llvm-passes/build/SyscallPass.so
##### **3.  Deployment:** Copies the resulting binary (`benchmark`) and the generated policy (`policy.bin`) to the kernel root filesystem staging area.

## Running the Benchmark

To compile the benchmark and update the initramfs:

```bash
./benchmark.sh
```