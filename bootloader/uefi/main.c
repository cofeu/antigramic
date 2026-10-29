#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    Print(L"AntigramicOS UEFI Bootloader\n\r");

    EFI_FILE_IO_INTERFACE *fs;
    EFI_FILE_HANDLE root, kernel;
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &FileSystemProtocol, (void**)&fs);
    if (EFI_ERROR(status)) {
        Print(L"FileSystemProtocol bulunamadi: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root);
    if (EFI_ERROR(status)) {
        Print(L"Root volume acilamadi: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(root->Open, 5, root, &kernel, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"kernel.elf bulunamadi: %r\n\r", status);
        return status;
    }

    UINTN size = 0;
    status = uefi_call_wrapper(kernel->GetInfo, 4, kernel, &FileInfoGuid, &size, NULL);
    EFI_FILE_INFO *info = AllocatePool(size);
    status = uefi_call_wrapper(kernel->GetInfo, 4, kernel, &FileInfoGuid, &size, info);

    VOID *buffer = AllocatePages(info->FileSize / 4096 + 1);
    status = uefi_call_wrapper(kernel->Read, 3, kernel, &info->FileSize, buffer);
    if (EFI_ERROR(status)) {
        Print(L"Kernel okunamadi: %r\n\r", status);
        return status;
    }

    Print(L"Kernel yuklendi: %d bytes, %p adresine\n\r", info->FileSize, buffer);

    uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, 0);

    typedef void (*kernel_entry)(void);
    kernel_entry entry = (kernel_entry)((uint64_t)buffer + 0x1000);
    entry();

    return EFI_SUCCESS;
}