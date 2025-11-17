BUILD_DIR   := build
INCLUDE_DIR := include
OUTPUT_DIR  := ../output
SCRIPTS_DIR := scripts
SRC_DIR     := src
TEST_DIR    := test
BIN_DIR     := ../kernel/rootfs/bin

CXX     := clang++
CC      := clang
PYTHON  := python3
OPT     := opt
DOT     := dot

LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
LLVM_LDFLAGS  := $(shell llvm-config --ldflags --libs --system-libs)

CXXFLAGS := -std=c++17 -fPIC $(LLVM_CXXFLAGS)
LDFLAGS  := -shared $(LLVM_LDFLAGS)

CALLNAMES_H       := $(INCLUDE_DIR)/CallNames.h
DUMMYSYSCALLS_H   := $(INCLUDE_DIR)/DummySyscalls.h
GENERATED_HEADERS := $(CALLNAMES_H) $(DUMMYSYSCALLS_H)

LIBC_PASS_SO       := $(BUILD_DIR)/LibcPass.so
INSTRUMENT_PASS_SO := $(BUILD_DIR)/InstrumentPass.so
SYSCALL_PASS_SO    := $(BUILD_DIR)/SyscallPass.so

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_BCS  := $(patsubst $(TEST_DIR)/%.c,$(OUTPUT_DIR)/%.bc,$(TEST_SRCS))

COMBINED_BC := $(OUTPUT_DIR)/combined.bc
TEST_EXE    := $(OUTPUT_DIR)/combined

.DEFAULT_GOAL := all

.PHONY: all clean libc_pass instrument_syscall overwrite_instrument

all: $(TEST_EXE)
	@echo "Build complete."

clean:
	@rm -f $(OUTPUT_DIR)/*.bc
	@rm -f $(OUTPUT_DIR)/*.dot
	@rm -f $(OUTPUT_DIR)/*.png
	@rm -rf $(OUTPUT_DIR)
	@rm -f $(GENERATED_HEADERS)
	@rm -f $(TEST_EXE)
	@rm -f $(BIN_DIR)/combined

$(BUILD_DIR) $(OUTPUT_DIR):
	@mkdir -p $@

$(CALLNAMES_H): $(SCRIPTS_DIR)/LibcCallNames.py | $(INCLUDE_DIR)
	@echo "Generating CallNames.h"
	@$(PYTHON) $<

$(DUMMYSYSCALLS_H): $(SCRIPTS_DIR)/DummySyscalls.py | $(INCLUDE_DIR)
	@echo "Generating DummySyscalls.h"
	@$(PYTHON) $<


$(BUILD_DIR)/%.so: $(SRC_DIR)/%.cpp $(GENERATED_HEADERS) | $(BUILD_DIR)
	@echo "Building LLVM Pass $@"
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)


$(OUTPUT_DIR)/%.bc: $(TEST_DIR)/%.c | $(OUTPUT_DIR)
	@echo "Compiling $< $@"
	@$(CC) -emit-llvm -c $< -o $@


$(COMBINED_BC): $(TEST_BCS)
	@echo "Linking $(COMBINED_BC)"
	@llvm-link $(TEST_BCS) -o $(COMBINED_BC)


libc_pass: $(COMBINED_BC) $(LIBC_PASS_SO)
	@echo "Running libccall pass"
	@$(OPT) -load-pass-plugin=$(LIBC_PASS_SO) -passes=libc-cfg-pass \
		$(COMBINED_BC) -o /dev/null
	@echo "Moving libccall.dot"
	@mv -f llvm-link_cfg.dot $(OUTPUT_DIR)/libccall.dot
	@$(DOT) -Tpng $(OUTPUT_DIR)/libccall.dot -o $(OUTPUT_DIR)/libccall.png


instrument_syscall: $(COMBINED_BC) $(INSTRUMENT_PASS_SO) $(SYSCALL_PASS_SO)
	@echo "Running instrument + syscall pass"
	@$(OPT) -load-pass-plugin=$(INSTRUMENT_PASS_SO) -load-pass-plugin=$(SYSCALL_PASS_SO) \
		-passes="instrument-pass,syscall-cfg-pass" \
		$(COMBINED_BC) -o /dev/null
	@echo "Moving syscall.dot"
	@mv -f syscall_cfg.dot $(OUTPUT_DIR)/syscall.dot
	@$(DOT) -Tpng $(OUTPUT_DIR)/syscall.dot -o $(OUTPUT_DIR)/syscall.png


overwrite_instrument: $(COMBINED_BC) $(INSTRUMENT_PASS_SO)
	@echo "Overwriting combined.bc with instrument pass"
	@$(OPT) -load-pass-plugin=$(INSTRUMENT_PASS_SO) -passes=instrument-pass \
		$(COMBINED_BC) -o $(COMBINED_BC)


$(TEST_EXE): $(COMBINED_BC) libc_pass instrument_syscall overwrite_instrument
	@echo "Compiling final executable $(TEST_EXE)"
	@$(CC) $(COMBINED_BC) -static -o $(TEST_EXE)
	@cp $(TEST_EXE) $(BIN_DIR)
