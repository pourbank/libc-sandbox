# Liberator: In-Kernel Per-Process Sandbox

**Liberator** is a Linux kernel-based security sandbox designed to enforce strict library call policies on user-space processes. Unlike traditional system call sandboxes like seccomp, Liberator enforces granular control over `libc` function calls by combining static analysis with kernel-level monitoring.


## Project Overview

The goal of this project is to prevent code execution exploits such as ROP chains or code injection by enforcing a statically determined Control Flow Graph of library calls.

The system operates in two distinct phases:
##### **1.  Static Analysis & Instrumentation (Compile-Time):** An LLVM-based toolchain analyzes the C source code, extracts a valid policy, and instruments the binary with "dummy" system calls before every library function.

##### **2.  Policy Enforcement (Runtime):** A custom Linux Kernel Module loads the extracted policy and monitors the process. It verifies that every *dummy* syscall matches a valid transition in the policy graph. If a violation is detected, the process is terminated.

## Architecture

The system is composed of three main modules:

##### **1. LLVM Toolchain (**`llvm-passes/`**):**
- **InstrumentPass:** Injects a custom syscall `sys_dummy(id)` before specific library calls.
- **SyscallPass:** Extracts the permitted NFA of these calls.
##### **2. Kernel Patch (**`kernel/`**):**
- Modifies the Linux Kernel to introduce system call `sys_dummy`.
- Provides a hook mechanism for the security monitor to intercept this call.
##### **3. Security Monitor (**`monitor/`**):**
- A Loadable Kernel Module that loads the per-process policy from the filesystem.
- Uses `kprobes` to detect process start and exit.
- Enforces the NFA state transitions in real-time.

## Prerequisites

To build and run this project, you need a Linux environment with the following dependencies:

- **QEMU:** `qemu-system-x86_64` (for running the custom kernel)
- **LLVM/Clang:** Version 14+ (for building passes)
- **Build Essentials:** `make`, `gcc`, `bison`, `flex`, `libssl-dev`
- **Python 3:** For helper scripts.
- **Graphviz:** For visualizing the extracted DOT graphs.

## Building the Project

We provide a master build script that compiles the kernel, the LLVM passes, the monitor, and prepares the initramfs.

***Note:*** *Building the Linux kernel for the first time can take significant time.*

```bash
# Build everything (Kernel, LLVM Passes, Monitor, RootFS)
./build.sh
```

If you only need to rebuild the test case and repackage the filesystem after changing `test/main.c`:

```bash
./test.sh
```

## Running the Sandbox

The project runs inside a QEMU virtual machine using a lightweight `initramfs`.

##### **1. Launch QEMU:**

```bash
./launch.sh
```

##### **2. Inside the QEMU Shell:** The system will boot into a shell. The instrumented binary and policy are located in `/bin` and `/etc` respectively.

```bash
# 1. Insert the Security Monitor Module
# programPath: The binary to protect
# policyPath: The binary policy file
insmod /monitor.ko programPath="/bin/benchmark" policyPath="/etc/policy.bin"

# 2. Run the target application
/bin/benchmark
```
##### **3. Observing Results:** 
- **Success:** If the program runs correctly, check kernel logs:
```bash
dmesg | tail
# Output: Monitor: Policy Loaded... Monitor: Monitored PID exiting.
```
- **Violation:** If the control flow deviates:
```bash
dmesg | tail
# Output: Monitor: POLICY VIOLATION! PID [x]: No valid transition...
```

## Benchmark

The system is evaluated against *mbed-tls*. The `benchmark/` directory contains the logic to compile *mbed-tls* with our instrumentation passes.

To build the benchmark suite:

```bash
./benchmark.sh
```

## Cleaning Up

To clean up all build artifacts, including the kernel, LLVM passes, and the monitor module, run the clean script:

```bash
./clean.sh
```

***Warning:*** *This command performs a make clean on the kernel directory. Rebuilding the kernel after running this will take a significant amount of time.*