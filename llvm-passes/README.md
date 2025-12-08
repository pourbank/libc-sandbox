# LLVM Static Analysis & Instrumentation Toolchain

This directory contains the compiler passes responsible for analyzing the source code and injecting the security mechanism.

## Components

### 1. Instrumentation Pass (`src/InstrumentPass.cpp`)
This pass modifies the program's Intermediate Representation (IR).
* **Target:** Identifies `call` instructions to standard libc functions.
* **Action:** Injects a `syscall(470, ID)` instruction immediately before the library call.
* **Mapping:** Uses `include/DummySyscalls.h` to map function names to unique integer IDs.

### 2. Policy Extraction Pass (`src/SyscallPass.cpp`)
This pass analyzes the instrumented IR to generate the enforcement policy.
* **CFG Construction:** Builds a Control Flow Graph where edges are transitions triggered by the dummy syscalls.
* **NFA Optimization:** Uses `src/FSM.cpp` to perform epsilon-closure removal and state merging, reducing the graph to a minimized NFA/DFA.
* **Output:** Generates a `.dot` file representing the valid state transitions.

### 3. Helpers
* `scripts/LibcCallNames.py`: Scans the host's `libc.so` to generate a list of all available library functions.
* `scripts/DummySyscalls.py`: Generates the unique ID mapping used by both the compiler and the kernel.
