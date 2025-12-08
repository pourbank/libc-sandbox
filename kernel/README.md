# Kernel Modifications & Runtime Environment

This directory contains the necessary modifications to the Linux Kernel to support the Liberator sandbox, as well as the root filesystem configuration for the QEMU environment.

## Kernel Patch

The core of the communication channel between the user-space application and the kernel monitor is established via a custom system call. We apply `patch.diff` to a standard Linux Kernel source tree.

### What the patch does:
##### **1.  Registers System Call 470:** Adds `dummy` to the syscall table (`syscall_64.tbl`).
##### **2.  Implements** `sys_dummy`**:** Defines the system call in `kernel/sys.c`. This function acts as a trampoline. It checks if a security hook is registered (`hook_ptr`) and delegates execution to it.
##### **3.  Exports Symbols:** Exports `hook_ptr` so the Loadable Kernel Module can register itself dynamically.

### Manual Patching
If not using the root `build.sh`, apply the patch as follows:
```bash
cd linux-6.17.7
patch -p1 < ../patch.diff
```

### Root Filesystem (`rootfs/`)

We use a minimal `initramfs` to boot the kernel in QEMU quickly.

- `init`: The initialization script that mounts virtual filesystems (`/proc`, `/sys`), sets up the environment, and drops the user into a shell.
- `/bin`: Contains the instrumented binaries.
- `/etc`: Contains the binary policy file `policy.bin`.
