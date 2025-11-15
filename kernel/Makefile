KERNEL_VERSION := 6.17.7
KERNEL_MAJOR   := 6.x
KERNEL_TARBALL := linux-$(KERNEL_VERSION).tar.xz
KERNEL_SRC_DIR := linux-$(KERNEL_VERSION)
KERNEL_URL     := https://cdn.kernel.org/pub/linux/kernel/v$(KERNEL_MAJOR)/$(KERNEL_TARBALL)

PATCH_FILE     := patch.diff
MAKE           := make

ROOTFS_DIR     := rootfs
ROOTFS_SUBDIRS := $(ROOTFS_DIR)/bin $(ROOTFS_DIR)/dev $(ROOTFS_DIR)/etc $(ROOTFS_DIR)/lib/modules $(ROOTFS_DIR)/proc $(ROOTFS_DIR)/sbin $(ROOTFS_DIR)/sys

BUSYBOX_BIN   := $(ROOTFS_DIR)/bin/busybox
BUSYBOX_LINKS := sh ls dmesg insmod rmmod mount clear mknod setsid cttyhack cat
BUSYBOX_LINK_TARGETS := $(foreach link,$(BUSYBOX_LINKS),$(ROOTFS_DIR)/bin/$(link))

ROOTFS_CPIO_FILE := rootfs.cpio.gz

KERNEL_IMAGE_PATH := $(KERNEL_SRC_DIR)/arch/x86_64/boot/bzImage

.DEFAULT_GOAL := all

.PHONY: all kernel build rootfs initramfs clean

all: build initramfs

kernel: $(KERNEL_SRC_DIR) $(PATCH_FILE)
	@echo "Patching Linux Kernel Source"
	@patch -p1 -d $(KERNEL_SRC_DIR) < $(PATCH_FILE)

$(KERNEL_SRC_DIR): $(KERNEL_TARBALL)
	@echo "Extracting Linux Kernel Source"
	@tar -xf $(KERNEL_TARBALL)

$(KERNEL_TARBALL):
	@echo "Downloading Linux Kernel Source"
	@wget -c $(KERNEL_URL)

build: $(KERNEL_IMAGE_PATH)

$(KERNEL_IMAGE_PATH): kernel
	@echo "Building Linux Kernel"
	@$(MAKE) -C $(KERNEL_SRC_DIR) defconfig
	@$(MAKE) -C $(KERNEL_SRC_DIR) -j$(shell nproc)

initramfs: $(ROOTFS_CPIO_FILE)

rootfs: $(ROOTFS_SUBDIRS) $(BUSYBOX_LINK_TARGETS)

$(ROOTFS_CPIO_FILE): rootfs
	@echo "Creating initramfs CPIO archive"
	@(cd $(ROOTFS_DIR) && find . | cpio -o -H newc | gzip > ../$(ROOTFS_CPIO_FILE))

$(BUSYBOX_LINK_TARGETS): $(BUSYBOX_BIN)
	@(cd $(ROOTFS_DIR)/bin && ln -sf busybox $(@F))

$(BUSYBOX_BIN):
	@cp $(shell which busybox) $@

$(ROOTFS_SUBDIRS):
	@mkdir -p $@

clean:
	@echo "Cleaning Up"
	@rm -f $(ROOTFS_CPIO_FILE)
	@rm -f $(KERNEL_TARBALL)
	@rm -rf $(KERNEL_SRC_DIR)
	@rm -rf $(ROOTFS_DIR)/bin $(ROOTFS_DIR)/dev $(ROOTFS_DIR)/proc $(ROOTFS_DIR)/sbin $(ROOTFS_DIR)/sys $(ROOTFS_DIR)/lib
