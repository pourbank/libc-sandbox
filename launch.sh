#!/bin/bash
set -e

qemu-system-x86_64 -kernel kernel/linux-6.17.7/arch/x86/boot/bzImage -nographic -append "console=ttyS0 quiet" -initrd kernel/rootfs.cpio.gz
