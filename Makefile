ARCH ?= x86_64
BUILD_DIR := build/$(ARCH)

CC := clang
LD := ld.lld
AS := nasm
OBJCOPY := objcopy

CFLAGS := -target $(ARCH)-pc-none-elf -ffreestanding -fno-pic -Wall -Wextra -O2 -g \
          -I kernel/include -I kernel/arch/$(ARCH)/include \
          -I kernel/vm -I kernel/ipc -I kernel/sched \
          -I userspace/libc/include

LDFLAGS := -nostdlib -static -z max-page-size=0x1000

USER_CFLAGS := -target $(ARCH)-pc-none-elf -ffreestanding -fno-pic -nostdlib -nostdinc \
               -I userspace/libc/include -Wall -Wextra -O2 -g

ifeq ($(ARCH),x86_64)
    CFLAGS += -march=x86-64 -mno-sse -mno-avx -mno-red-zone -mcmodel=kernel
    ASFLAGS := -f elf64
    USER_CFLAGS += -march=x86-64
    USER_LDFLAGS := -nostdlib -static -z max-page-size=0x1000
else ifeq ($(ARCH),i386)
    CFLAGS += -march=i686 -m32
    ASFLAGS := -f elf32
    LDFLAGS += -m elf_i386
    USER_CFLAGS += -march=i686 -m32
    USER_LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -m elf_i386
endif

KERNEL_OBJS := \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/entry.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/gdt.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/idt.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/paging.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/syscall.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/apic.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/isr_stubs.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/isr.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/syscall_entry.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/context_switch.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/serial.o \
    $(BUILD_DIR)/kernel/arch/$(ARCH)/mouse.o \
    $(BUILD_DIR)/kernel/ipc/port.o \
    $(BUILD_DIR)/kernel/mm/pmm.o \
    $(BUILD_DIR)/kernel/mm/vmm.o \
    $(BUILD_DIR)/kernel/drivers/rtc.o \
    $(BUILD_DIR)/kernel/sched/sched.o \
    $(BUILD_DIR)/kernel/ui/ui.o \
    $(BUILD_DIR)/kernel/ui/png.o \
    $(BUILD_DIR)/kernel/ui/assets.o \
    $(BUILD_DIR)/kernel/lib/string.o \
    $(BUILD_DIR)/kernel/main.o

BOOTLOADER_BIOS_OBJS := \
    $(BUILD_DIR)/bootloader/bios/boot.bin \
    $(BUILD_DIR)/bootloader/bios/stage2.bin

BOOTLOADER_UEFI_OBJS := \
    $(BUILD_DIR)/bootloader/uefi/main.o

USER_OBJS := \
    $(BUILD_DIR)/userspace/libc/string.o \
    $(BUILD_DIR)/userspace/libc/stdlib.o \
    $(BUILD_DIR)/userspace/init/crt0.o \
    $(BUILD_DIR)/userspace/init/init.o

all: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/bootloader_bios.bin $(BUILD_DIR)/init.elf

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) kernel/arch/$(ARCH)/linker.ld
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -T kernel/arch/$(ARCH)/linker.ld -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	@$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/bootloader_bios.bin: $(BOOTLOADER_BIOS_OBJS) $(BUILD_DIR)/kernel.bin build_image.py
	@mkdir -p $(@D)
	python3 build_image.py $(BUILD_DIR)


$(BUILD_DIR)/init.elf: $(USER_OBJS) userspace/init/linker.ld
	@mkdir -p $(@D)
	$(LD) $(USER_LDFLAGS) -T userspace/init/linker.ld -o $@ $(USER_OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/ui/ui.o: kernel/ui/font_data.h
$(BUILD_DIR)/kernel/ui/ui.o: CFLAGS += -Os
$(BUILD_DIR)/kernel/ui/assets.o: assets/images/home.png assets/images/app-icons.png

$(BUILD_DIR)/userspace/%.o: userspace/%.c
	@mkdir -p $(@D)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/bootloader/bios/boot.bin: bootloader/bios/boot.asm
	@mkdir -p $(@D)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/bootloader/bios/stage2.bin: bootloader/bios/stage2.asm
	@mkdir -p $(@D)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

font-data:
	python3 kernel/ui/gen_font.py

.PHONY: all clean font-data
