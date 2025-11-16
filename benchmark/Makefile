LLVM_DIR      := ../llvm-passes
BUILD_DIR     := $(LLVM_DIR)/build
INCLUDE_DIR   := $(LLVM_DIR)/include
OUTPUT_DIR    := $(LLVM_DIR)/output
SRC_DIR       := $(LLVM_DIR)/src
SCRIPTS_DIR   := $(LLVM_DIR)/scripts

CXX           := clang++
PYTHON        := python3

LLVM_CXXFLAGS := $(shell llvm-config --cxxflags)
LLVM_LDFLAGS  := $(shell llvm-config --ldflags --libs --system-libs)

CXXFLAGS      := -std=c++17 -fPIC $(LLVM_CXXFLAGS)
LDFLAGS       := -shared $(LLVM_LDFLAGS)

CALLNAMES_H       := $(INCLUDE_DIR)/CallNames.h
DUMMYSYSCALLS_H   := $(INCLUDE_DIR)/DummySyscalls.h
GENERATED_HEADERS := $(CALLNAMES_H) $(DUMMYSYSCALLS_H)

LIBC_PASS_SO       := $(BUILD_DIR)/LibcPass.so
INSTRUMENT_PASS_SO := $(BUILD_DIR)/InstrumentPass.so
SYSCALL_PASS_SO    := $(BUILD_DIR)/SyscallPass.so
PASSES             := $(LIBC_PASS_SO) $(INSTRUMENT_PASS_SO) $(SYSCALL_PASS_SO)

MBEDTLS_DIR        := $(abspath $(CURDIR)/mbedtls)
PATCH_FILE         := patch.diff
WRAPPER_SCRIPT     := wrapper.sh
EXTRACT_SCRIPT     := extract.sh

.DEFAULT_GOAL := all
.PHONY: all clean mbedtls passes copy-passes build-mbedtls extract

# ---------------------------------------------------
# Default target
# ---------------------------------------------------
all: mbedtls passes build-mbedtls extract

# ---------------------------------------------------
# Clean build artifacts
# ---------------------------------------------------
clean:
	@rm -rf $(BUILD_DIR) $(OUTPUT_DIR) $(GENERATED_HEADERS)

# ---------------------------------------------------
# Clone mbedTLS, apply patch, update submodules, move wrapper
# ---------------------------------------------------
mbedtls:
	@echo "Checking if $(MBEDTLS_DIR) exists..."
	@if [ ! -d "$(MBEDTLS_DIR)" ]; then \
		echo "Cloning mbedTLS into $(MBEDTLS_DIR)..."; \
		git clone --branch mbedtls-3.6.0 --depth 1 https://github.com/Mbed-TLS/mbedtls.git "$(MBEDTLS_DIR)" || { echo "Git clone failed"; exit 1; }; \
	else \
		echo "mbedTLS already exists at $(MBEDTLS_DIR)"; \
	fi
	@echo "Applying patch to mbedTLS..."
	@patch -p1 -d $(MBEDTLS_DIR) --dry-run < $(PATCH_FILE) >/dev/null 2>&1 || true
	@patch -p1 -d $(MBEDTLS_DIR) < $(PATCH_FILE) || echo "Patch already applied."
	@echo "Updating submodules..."
	@cd $(MBEDTLS_DIR) && git submodule update --init --recursive
	@if [ -f "$(WRAPPER_SCRIPT)" ]; then \
		echo "Moving $(WRAPPER_SCRIPT) into $(MBEDTLS_DIR)/"; \
		cp $(WRAPPER_SCRIPT) $(MBEDTLS_DIR)/; \
	else \
		echo "$(WRAPPER_SCRIPT) not found, skipping."; \
	fi

# ---------------------------------------------------
# Build LLVM passes if missing, then copy all passes to mbedTLS
# ---------------------------------------------------
passes: $(PASSES) copy-passes

# Compile pass only if it does not exist
$(BUILD_DIR)/%.so: $(SRC_DIR)/%.cpp $(GENERATED_HEADERS) | $(BUILD_DIR)
	@if [ ! -f $@ ]; then \
		echo "Compiling LLVM Pass $@"; \
		$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS); \
	else \
		echo "$@ already exists, skipping compilation."; \
	fi

# Always copy passes to mbedTLS
copy-passes:
	@echo "Copying all passes into $(MBEDTLS_DIR)..."
	@for pass in $(PASSES); do \
		if [ -f $$pass ]; then \
			echo "Copying $$pass to $(MBEDTLS_DIR)/"; \
			cp $$pass $(MBEDTLS_DIR)/; \
		else \
			echo "Warning: $$pass not found, skipping copy."; \
		fi \
	done

# ---------------------------------------------------
# Build mbedTLS programs
# ---------------------------------------------------
build-mbedtls: passes mbedtls
	@echo "Configuring and building mbedTLS..."
	@cd $(MBEDTLS_DIR) && cmake -S . -B build
	@cd $(MBEDTLS_DIR) && cmake --build build -j$$(nproc)

# ---------------------------------------------------
# Run extract.sh at the very end
# ---------------------------------------------------
extract:
	@if [ -f "$(EXTRACT_SCRIPT)" ]; then \
		echo "Running $(EXTRACT_SCRIPT)..."; \
		bash $(EXTRACT_SCRIPT); \
	else \
		echo "$(EXTRACT_SCRIPT) not found, skipping."; \
	fi

# ---------------------------------------------------
# Ensure directories exist
# ---------------------------------------------------
$(BUILD_DIR) $(OUTPUT_DIR):
	@mkdir -p $@

# ---------------------------------------------------
# Generate headers
# ---------------------------------------------------
$(CALLNAMES_H): $(SCRIPTS_DIR)/LibcCallNames.py | $(INCLUDE_DIR)
	@echo "Generating CallNames.h"
	@$(PYTHON) $<

$(DUMMYSYSCALLS_H): $(SCRIPTS_DIR)/DummySyscalls.py | $(INCLUDE_DIR)
	@echo "Generating DummySyscalls.h"
	@$(PYTHON) $<
