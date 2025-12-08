# Kernel Module

Monitor is a Loadable Kernel Module that enforces the library call policies at runtime.

## Enforcement Logic

The monitor (`Monitor.c`) operates using Linux Kprobes and our custom syscall hook.

##### **1.  Process Detection:**
- Hooks `bprm_execve` to detect when the target program specified by `programPath` is launched.
- Loads the corresponding policy file specified by `policyPath` from the disk into kernel memory.

##### **2.  State Tracking:**
- Maintains a *frontier* (a bitmap) representing the current active states of the NFA for the monitored process.

##### **3.  Transition Verification:**
- Intercepts `syscall 470` via `libcMonitorHook`.
- Calculates the next frontier based on the current state and the incoming library call ID.
- **Violation:** If the resulting frontier is empty (no valid transitions), the process is terminated with `SIGKILL`.

## Policy Conversion

The kernel module requires a binary representation of the policy.

* `scripts/CreatePolicy.py`: Parses the `.dot` output from the LLVM pass and serializes it into a binary format (`.bin`) containing the state count, start state, and transition table.