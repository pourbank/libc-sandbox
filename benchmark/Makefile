MBEDTLS_DIR   		:= mbedtls
MBEDTLS_BUILD   	:= $(MBEDTLS_DIR)/build

OUTPUT_DIR   		:= ../output
LLVMPASS_DIR   		:= ../llvm-passes/build
MONITOR_DIR  	  	:= ../monitor
KERNEL_DIR   		:= ../kernel
KERNEL_ROOTFS_DIR 	:= $(KERNEL_DIR)/rootfs

CC    := clang
CFLAGS    := -static
PASSFLAGS := -fpass-plugin=$(LLVMPASS_DIR)/InstrumentPass.so \
       -fpass-plugin=$(LLVMPASS_DIR)/SyscallPass.so

BENCH_SRC   := mbedtls/programs/test/benchmark.c
BENCH_EXE   := $(OUTPUT_DIR)/benchmark
SYSCALL_DOT := $(OUTPUT_DIR)/syscall.dot

POLICY_BIN_SRC := $(OUTPUT_DIR)/policy.bin

.DEFAULT_GOAL := all

.PHONY: all clean clean-passes build-mbedtls benchmarksm organize-output build-policy deploy initramfs

all: initramfs

$(MBEDTLS_DIR):
	@echo "Cloning Mbed-TLS repository"
	@git clone --branch mbedtls-2.16.0 --depth 1 https://github.com/Mbed-TLS/mbedtls.git

build-mbedtls: $(MBEDTLS_DIR)
	@echo "Building Mbed-TLS"
	@mkdir -p $(MBEDTLS_BUILD)
	@cd $(MBEDTLS_BUILD) && cmake .. && make

benchmarksm: build-mbedtls
	@echo "Building Benchmark" 
	@$(CC) $(CFLAGS) $(PASSFLAGS) \
		-I$(MBEDTLS_BUILD)/include \
		-L$(MBEDTLS_BUILD)/library \
		$(BENCH_SRC) \
		-lmbedtls -lmbedx509 -lmbedcrypto \
		-o benchmarksm

organize-output: clean-passes
	@mkdir -p $(OUTPUT_DIR)
	@mv -f syscall_cfg.dot $(SYSCALL_DOT)
	@mv -f benchmarksm $(BENCH_EXE)

build-policy: organize-output
	@$(MAKE) -C $(MONITOR_DIR) policy

deploy: build-policy
	@cp $(BENCH_EXE) $(KERNEL_ROOTFS_DIR)/bin/
	@cp $(POLICY_BIN_SRC) $(KERNEL_ROOTFS_DIR)/etc/

initramfs: deploy
	@$(MAKE) -C $(KERNEL_DIR) initramfs

clean-passes: benchmarksm
	@$(MAKE) -C ../llvm-passes clean

clean:
	@rm -rf $(OUTPUT_DIR) $(MBEDTLS_DIR)
	@rm -rf $(KERNEL_ROOTFS_DIR)/bin/benchmark
	@rm -rf $(KERNEL_ROOTFS_DIR)/etc/policy.bin