import os
import sys

SECTOR_SIZE = 512
IMAGE_SECTORS = 400
STAGE2_LBA = 1
STAGE2_SECTORS = 8
KERNEL_LBA = 129
KERNEL_SECTORS = 256

bd = sys.argv[1] if len(sys.argv) > 1 else 'build/x86_64'

def read_artifact(name):
    with open(os.path.join(bd, name), 'rb') as artifact:
        return artifact.read()


boot = read_artifact('bootloader/bios/boot.bin')
stage2 = read_artifact('bootloader/bios/stage2.bin')
kernel = read_artifact('kernel.bin')

if len(boot) != SECTOR_SIZE or boot[510:] != b'\x55\xaa':
    raise SystemExit('BIOS boot sector must be exactly 512 bytes and end in 0x55AA')
if not stage2 or len(stage2) > STAGE2_SECTORS * SECTOR_SIZE:
    raise SystemExit('BIOS stage2 must fit in its 128-sector load area')
if not kernel or len(kernel) > KERNEL_SECTORS * SECTOR_SIZE:
    raise SystemExit('Kernel must fit in its 256-sector load area (128 KiB)')

img = bytearray(SECTOR_SIZE * IMAGE_SECTORS)
img[:len(boot)] = boot
stage2_offset = STAGE2_LBA * SECTOR_SIZE
kernel_offset = KERNEL_LBA * SECTOR_SIZE
img[stage2_offset:stage2_offset + len(stage2)] = stage2
img[kernel_offset:kernel_offset + len(kernel)] = kernel

open(os.path.join(bd,'bootloader_bios.bin'),'wb').write(img)
print('Image created successfully')
