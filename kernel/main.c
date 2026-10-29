#include <kernel.h>
#include <serial.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <ipc/port.h>
#include <ui.h>

void kmain(void) {
    serial_write_str("AntigramicOS: Kernel starting...\n");
    
    // Initialize memory management
    pmm_init();
    serial_write_str("AntigramicOS: PMM initialized\n");
    
    vmm_init();
    serial_write_str("AntigramicOS: VMM initialized\n");
    
    // Initialize IPC
    ipc_init();
    serial_write_str("AntigramicOS: IPC initialized\n");
    
    ui_run();
}
