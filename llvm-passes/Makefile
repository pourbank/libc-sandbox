BUILD_DIR   := build
INCLUDE_DIR := include
OUTPUT_DIR  := output
SCRIPTS_DIR := scripts
SRC_DIR     := src
TEST_DIR    := test
ROOTFS_BIN  := ../kernel/rootfs/bin

CXX         := clang++
CC          := clang
PYTHON      := python3
OPT         := opt
DOT         := dot

LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
LLVM_LDFLAGS  := $(shell llvm-config --ldflags --libs --system-libs)

CXXFLAGS    := -std=c++17 -fPIC $(LLVM_CXXFLAGS)
LDFLAGS     := -shared $(LLVM_LDFLAGS)

CALLNAMES_H       := $(INCLUDE_DIR)/CallNames.h
DUMMYSYSCALLS_H   := $(INCLUDE_DIR)/DummySyscalls.h
GENERATED_HEADERS := $(CALLNAMES_H) $(DUMMYSYSCALLS_H)

LIBC_PASS_SO      := $(BUILD_DIR)/LibcPass.so
INSTRUMENT_PASS_SO := $(BUILD_DIR)/InstrumentPass.so
SYSCALL_PASS_SO   := $(BUILD_DIR)/SyscallPass.so
PASSES            := $(LIBC_PASS_SO) $(INSTRUMENT_PASS_SO) $(SYSCALL_PASS_SO)

TEST_SRC          := $(TEST_DIR)/test.c
TEST_BC           := $(TEST_DIR)/test.bc
TEST_INSTRUMENTED_BC := $(TEST_DIR)/test.instrumented.bc
TEST_EXE          := $(TEST_DIR)/test

LIBC_CFG_DOT      := $(OUTPUT_DIR)/test_cfg.dot
SYSCALL_CFG_DOT   := $(OUTPUT_DIR)/Syscall.dot
LIBC_CFG_PNG      := $(OUTPUT_DIR)/LibcCFG.png
SYSCALL_CFG_PNG   := $(OUTPUT_DIR)/SyscallCFG.png

.DEFAULT_GOAL := all

.PHONY: all clean run

all: $(LIBC_CFG_PNG) $(SYSCALL_CFG_PNG) $(TEST_EXE)
	@echo "Build complete."
	@echo "Graphs in: $(OUTPUT_DIR)"
	@echo "Executable at: $(TEST_EXE)"


clean:
	@echo "Cleaning Up"
	@rm -f $(TEST_BC) $(TEST_INSTRUMENTED_BC) $(TEST_EXE)
	@rm -f $(GENERATED_HEADERS)
	@rm -f test_cfg.dot
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OUTPUT_DIR)

$(BUILD_DIR) $(OUTPUT_DIR):
	@mkdir -p $@

$(CALLNAMES_H): $(SCRIPTS_DIR)/LibcCallNames.py | $(INCLUDE_DIR)
	@echo "Generating CallNames.h"
	@$(PYTHON) $<

$(DUMMYSYSCALLS_H): $(SCRIPTS_DIR)/DummySyscalls.py | $(INCLUDE_DIR)
	@echo "Generating DummySyscalls.h"
	@$(PYTHON) $<

$(TEST_BC): $(TEST_SRC)
	@echo "Compiling $< to bitcode"
	@$(CC) -emit-llvm -c $< -o $@

$(BUILD_DIR)/%.so: $(SRC_DIR)/%.cpp $(GENERATED_HEADERS) | $(BUILD_DIR)
	@echo "Compiling LLVM Pass $@"
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(LIBC_CFG_DOT): $(TEST_BC) $(LIBC_PASS_SO) | $(OUTPUT_DIR)
	@echo "Running Libc Call Graph Pass"
	@$(OPT) -load-pass-plugin=$(LIBC_PASS_SO) -passes=libc-cfg-pass $< -o /dev/null
	@mv test_cfg.dot $@

$(SYSCALL_CFG_DOT): $(TEST_BC) $(INSTRUMENT_PASS_SO) $(SYSCALL_PASS_SO) | $(OUTPUT_DIR)
	@echo "Running Instrumentation + Syscall Graph Pass"
	@$(OPT) -load-pass-plugin=$(INSTRUMENT_PASS_SO) -load-pass-plugin=$(SYSCALL_PASS_SO) -passes="instrument-pass,syscall-cfg-pass" $< -o /dev/null
	@mv test_cfg.dot $@

$(LIBC_CFG_PNG): $(LIBC_CFG_DOT)
	@echo "Generating $@"
	@$(DOT) -Tpng $< -o $@

$(SYSCALL_CFG_PNG): $(SYSCALL_CFG_DOT)
	@echo "Generating $@"
	@$(DOT) -Tpng $< -o $@

$(TEST_INSTRUMENTED_BC): $(TEST_BC) $(INSTRUMENT_PASS_SO)
	@echo "Instrumenting bitcode"
	@$(OPT) -load-pass-plugin=$(INSTRUMENT_PASS_SO) -passes=instrument-pass $< -o $@

$(TEST_EXE): $(TEST_INSTRUMENTED_BC)
	@echo "Compiling final executable $@"
	@$(CC) $< -static -o $@
	@cp $@ $(ROOTFS_BIN)