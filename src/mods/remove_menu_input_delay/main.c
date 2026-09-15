/*
 * Copyright (C) 2024,2025, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

/*
 * Removes the menu "input accept delay" introduced by Elden Ring patch 1.12
 * ("Adjusted the input speed in some menus, such as conversation menus, to
 * prevent accidental skips").
 *
 * Reverse-engineered background (verified on 1.10.1 vs 1.12-1.17):
 *
 *  - Every yes/no dialog (CS::MessageBoxDialog) and conversation menu has an
 *    input-accept gate: its Scaleform movie is only told that confirm input
 *    is ready once the time elapsed since the dialog opened
 *    (dialog+0x2300) reaches a per-dialog threshold stored at
 *    dialog+0x1278 (~0.32s on 1.12+). The gate itself exists pre-1.12, but
 *    with a threshold of 0 it accepts confirm on the first frame.
 *
 *  - The dialog copies the threshold from its window descriptor
 *    (desc+0x18 -> dialog+0x1278) when it is created. The descriptor field
 *    is written by a tiny setter called from the message box template init
 *    and from the conversation-menu (menu id 0xB) open path.
 *
 *  - Before 1.12 that setter was an empty stub:
 *        mov rax, rcx ; ret
 *    (1.10.1: eldenring.exe+77D560). Patch 1.12 filled the stub in:
 *        mov rbx, rcx
 *        call <MenuMan.MenuOpenPadBlockTime property getter> ; -> xmm0 (~0.32)
 *        movss dword ptr [rbx+18h], xmm0
 *        mov rax, rbx ; ret
 *    (1.12: eldenring.exe+78DDE0). "MenuMan.MenuOpenPadBlockTime" (UTF-16
 *    debug property name) is new in 1.12, which is what feeds the delay.
 *
 * Fix: replace the 5-byte `call` to the getter inside the setter with
 * `xorps xmm0, xmm0; nop; nop` so the descriptor threshold always stays 0,
 * restoring the pre-1.12 behavior (confirm accepted instantly).
 *
 * The patch point below matches exactly once on 1.12-1.17 and does not
 * match anything before 1.12 (checked against 1.02-1.17 dumps), so the mod
 * is a no-op on builds without the delay.
 */

#include "process/image.h"
#include "process/scanner.h"

#include <windows.h>
#include <stddef.h>

/* setter core: mov rbx,rcx; call <getter>; movss [rbx+18h],xmm0; mov rax,rbx; add rsp,20h; pop rbx; ret */
#define SETTER_SIG "48 8B D9 E8 ?? ?? ?? ?? F3 0F 11 43 18 48 8B C3 48 83 C4 20 5B C3"

/* offset of the 5-byte `call` inside the signature above */
#define CALL_OFFSET 3

static const uint8_t patch_bytes[] = { 0x0F, 0x57, 0xC0, 0x90, 0x90 }; /* xorps xmm0,xmm0; nop; nop */

static void init(void) {
    size_t size;
    void *base = get_module_image_base(NULL, &size);
    if (base == NULL) {
        return;
    }
    uint8_t *addr = sig_scan(base, size, SETTER_SIG);
    if (addr == NULL) {
        /* pre-1.12 build, or the setter has already been patched by another mod */
        return;
    }
    /* make sure the match is unique before patching */
    size_t consumed = (size_t)(addr - (uint8_t *)base) + 1;
    if (consumed < size && sig_scan(addr + 1, size - consumed, SETTER_SIG) != NULL) {
        return;
    }
    uint8_t *call_addr = addr + CALL_OFFSET;
    DWORD old_protect;
    if (!VirtualProtect(call_addr, sizeof(patch_bytes), PAGE_EXECUTE_READWRITE, &old_protect)) {
        return;
    }
    memcpy(call_addr, patch_bytes, sizeof(patch_bytes));
    VirtualProtect(call_addr, sizeof(patch_bytes), old_protect, &old_protect);
    FlushInstructionCache(GetCurrentProcess(), call_addr, sizeof(patch_bytes));
}

BOOL APIENTRY DllMain(HMODULE module, DWORD ul_reason_for_call, LPVOID reserved) {
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
