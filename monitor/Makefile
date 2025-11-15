MODULE_NAME := monitor
SOURCE_FILE := Monitor.c

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(SOURCE_FILE:.c=.o)

KDIR := ../kernel/linux-6.17.7

OUTPUT_DIR := $(PWD)/output
POLICY_SCRIPT := scripts/CreatePolicy.py
POLICY_LOCAL_FILE := policy.bin
POLICY_DEST_DIR := ../kernel/rootfs/etc

MODULE_DEST_DIR := ../kernel/rootfs/lib/modules

.PHONY: all policy module install clean

all: policy install

policy:
	@echo "Generating Policy"
	@python3 $(POLICY_SCRIPT)
	@mkdir -p $(POLICY_DEST_DIR)
	@cp -f $(POLICY_LOCAL_FILE) $(POLICY_DEST_DIR)/
	@rm -f $(POLICY_LOCAL_FILE)

module:
	@echo "Building Kernel Module"
	@$(MAKE) --no-print-directory -C $(KDIR) M=$(PWD) modules
	@mkdir -p $(OUTPUT_DIR)
	@mv -f $(MODULE_NAME).o $(MODULE_NAME).ko $(MODULE_NAME).mod.c $(MODULE_NAME).mod.o \
		modules.order Module.symvers $(SOURCE_FILE:.c=.o) $(OUTPUT_DIR)/
	@mv -f .*.cmd $(OUTPUT_DIR)/ 2>/dev/null || true
	@mv -f .tmp_versions $(OUTPUT_DIR)/ 2>/dev/null || true

install: module
	@mkdir -p $(MODULE_DEST_DIR)
	@cp -f $(OUTPUT_DIR)/$(MODULE_NAME).ko $(MODULE_DEST_DIR)/

clean:
	@echo "Cleaning Up"
	@$(MAKE) --no-print-directory -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUTPUT_DIR)
	@rm -f $(POLICY_LOCAL_FILE)
	@rm -f $(MODULE_DEST_DIR)/$(MODULE_NAME).ko
	@rm -f $(POLICY_DEST_DIR)/$(POLICY_LOCAL_FILE)