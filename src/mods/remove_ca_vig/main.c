#include "process/image.h"
#include "process/scanner.h"

#include <windows.h>
#include <stddef.h>

static void init(void) {
    size_t size;
    static const uint8_t new_bytes[] = { 0x66, 0x0f, 0xef, 0xc9 };
    static const uint8_t new_bytes2[] = { 0xf3, 0x0f, 0x5c, 0xc0, 0x90 };
    DWORD old_protect;

    void *base = get_module_image_base(NULL, &size);
    if (base == NULL) {
        return;
    }

    uint8_t *addr = sig_scan(base, size, "0F 11 ?? 60 ?? 8D ?? 80 00 00 00 0F 10 ?? A0 00 00 00 0F 11 ?? F0 ?? 8D ?? B0 00 00 00 0F 10 ?? 0F 11 ?? 0F 10 ?? 10");
    if (addr && (size_t)(addr - (uint8_t *)base) + 0x2F + sizeof(new_bytes) <= size &&
        VirtualProtect(addr + 0x2F, sizeof(new_bytes), PAGE_EXECUTE_READWRITE, &old_protect)) {
        memcpy(addr + 0x2F, new_bytes, sizeof(new_bytes));
        VirtualProtect(addr + 0x2F, sizeof(new_bytes), old_protect, &old_protect);
        FlushInstructionCache(GetCurrentProcess(), addr + 0x2F, sizeof(new_bytes));
    }

    addr = sig_scan(base, size, "F3 0F 10 ?? 50 F3 0F 59 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 ?? 0F 5C ?? F3 ?? 0F 59 ?? ?? 8D ?? ?? A0 00 00 00");
    if (addr) {
        if (!VirtualProtect(addr + 0x17, sizeof(new_bytes2), PAGE_EXECUTE_READWRITE, &old_protect)) {
            return;
        }
        memcpy(addr + 0x17, new_bytes2, sizeof(new_bytes2));
        VirtualProtect(addr + 0x17, sizeof(new_bytes2), old_protect, &old_protect);
        FlushInstructionCache(GetCurrentProcess(), addr + 0x17, sizeof(new_bytes2));
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD ul_reason_for_call, LPVOID reserved) {
    (void)module;
    (void)reserved;
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            init();
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
