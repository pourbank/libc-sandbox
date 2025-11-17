MODULE_NAME := monitor
SOURCE_FILE := Monitor.c

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(SOURCE_FILE:.c=.o)

KDIR := ../kernel/linux-6.17.7

OUTPUT_DIR := $(PWD)/output
POLICY_SCRIPT := scripts/CreatePolicy.py
POLICY_LOCAL_FILE := ../output/policy.bin
POLICY_DEST_DIR := ../kernel/rootfs/etc

MODULE_DEST_DIR := ../kernel/rootfs/lib/modules

.PHONY: all policy module install clean

all: policy install

policy:
	@pwd
	@echo "Generating Policy"
	@python3 $(POLICY_SCRIPT)
	@cp $(POLICY_LOCAL_FILE) $(POLICY_DEST_DIR)

module:
	@echo "Building Kernel Module"
	@$(MAKE) --no-print-directory -C $(KDIR) M=$(PWD) modules
	@mkdir -p $(OUTPUT_DIR)
	@mv -f $(MODULE_NAME).o $(MODULE_NAME).ko $(MODULE_NAME).mod.c $(MODULE_NAME).mod.o \
		modules.order Module.symvers $(SOURCE_FILE:.c=.o) $(OUTPUT_DIR)/
	@mv -f .*.cmd $(OUTPUT_DIR)/ 2>/dev/null || true
	@mv -f .*.o $(OUTPUT_DIR)/ 2>/dev/null || true
	@mv -f .*.mod $(OUTPUT_DIR)/ 2>/dev/null || true
	@mv -f .tmp_versions $(OUTPUT_DIR)/ 2>/dev/null || true

install: module
	@cp -f $(OUTPUT_DIR)/$(MODULE_NAME).ko $(MODULE_DEST_DIR)/

clean:
	@$(MAKE) --no-print-directory -C $(KDIR) M=$(PWD) clean
	@rm -rf $(OUTPUT_DIR)
	@rm -f $(POLICY_DEST_DIR)/$(POLICY_LOCAL_FILE)
